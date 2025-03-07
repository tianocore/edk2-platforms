/** @file
PCH SPI Common Driver implements the SPI Host Controller Compatibility Interface.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifdef _MSC_VER
  #pragma optimize( "", off )
#endif

#ifdef __GNUC__
  #ifndef __clang__
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
  #else
    #pragma clang optimize off
  #endif
#endif

#include "FchSpiProtect.h"
#include "SpiInfo.h"
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SpiFlashDeviceLib.h>
#include <Protocol/SpiCommon.h>
#include <Protocol/SmmBase2.h>

#define SPI_WREN_INDEX  0                     // Prefix Opcode 0: SPI_COMMAND_WRITE_ENABLE
#define SPI_EWSR_INDEX  1                     // Prefix Opcode 1: SPI_COMMAND_WRITE_S_EN

#define FCH_SPI_MMIO_REG48_TXBYTECOUNT  0x48
#define FCH_SPI_MMIO_REG4B_RXBYTECOUNT  0x4B
#define FCH_SPI_MMIO_REG80_FIFO         0x80
#define FCH_SPI_MMIO_REG50_ADDR32CTRL0  0x50
#define FCH_SPI_MMIO_REG5C_ADDR32CTRL3  0x5C

volatile UINTN        mFchSpiProtect_LOCKED_ReadyToBoot = FALSE;
volatile UINTN        mSpiMmioBase;
extern CONST BOOLEAN  gInSmm;
volatile BOOLEAN      mSupport4ByteAddrFlag = FALSE;

STATIC
EFI_STATUS
WaitForSpiDeviceWriteEnabled (
  IN  EFI_SPI_PROTOCOL  *This
  );

/**

  Initialize an SPI protocol instance.
  The function will assert in debug if FCH SPI has not been initialized

  @param SpiInstance   - Pointer to SpiInstance to initialize

  @retval EFI_SUCCESS     The protocol instance was properly initialized
  @retval EFI_UNSUPPORTED The FCH is not supported by this module

**/
EFI_STATUS
SpiProtocolConstructor (
  SPI_INSTANCE  *SpiInstance
  )
{
  DEBUG ((DEBUG_INFO, "SpiProtocolConstructor enter!\n"));

  SpiInstance->InitDone = FALSE;  // Indicate NOT READY.
  //
  // Initialize the SPI protocol instance
  //
  SpiInstance->Signature           = FCH_SPI_PRIVATE_DATA_SIGNATURE;
  SpiInstance->Handle              = NULL;
  SpiInstance->SpiProtocol.Init    = SpiProtocolInit;
  SpiInstance->SpiProtocol.Lock    = SpiProtocolLock;
  SpiInstance->SpiProtocol.Execute = SpiProtocolExecute;

  //
  // Sanity check to ensure FCH SPI initialization has occurred previously.
  //
  SpiInstance->SpiBar = (EFI_PHYSICAL_ADDRESS)PciRead32 (
                                                PCI_LIB_ADDRESS (
                                                  0,
                                                  20,
                                                  3,
                                                  0xA0
                                                  )
                                                )&0x00000000FFFFFFE0;
  ASSERT (SpiInstance->SpiBar != 0);
  mSpiMmioBase = SpiInstance->SpiBar;

  DEBUG ((DEBUG_VERBOSE, "SpiInstance->SpiBar = 0x%x\n", SpiInstance->SpiBar));
  return EFI_SUCCESS;
}

/**

  Initialize the host controller to execute SPI command.

  @param This                    Pointer to the EFI_SPI_PROTOCOL instance.

  @retval EFI_SUCCESS             Initialization completed.
  @retval EFI_ACCESS_DENIED       The SPI static configuration interface has been locked-down.
  @retval EFI_INVALID_PARAMETER   Bad input parameters.
  @retval EFI_UNSUPPORTED         Can't get Descriptor mode VSCC values

**/
EFI_STATUS
EFIAPI
SpiProtocolInit (
  IN EFI_SPI_PROTOCOL  *This
  )
{
  EFI_STATUS    Status;
  SPI_INSTANCE  *SpiInstance;
  UINT8         FlashPartId[3];
  UINT8         FlashIndex;

  DEBUG ((DEBUG_INFO, "SpiProtocolInit enter!\n"));

  SpiInstance = SPI_INSTANCE_FROM_SPIPROTOCOL (This);
  CopyMem (&SpiInstance->SpiInitTable, &mSpiInitTable[0], sizeof (SPI_INIT_TABLE));

  Status = SpiProtocolExecute (
             This,
             SPI_OPCODE_JEDEC_ID_INDEX,
             0,
             TRUE,
             TRUE,
             FALSE,
             (UINTN)0,
             3,
             FlashPartId,
             EnumSpiRegionDescriptor
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (FlashIndex = 0; FlashIndex < mNumSpiFlashMax; FlashIndex++) {
    CopyMem (&SpiInstance->SpiInitTable, &mSpiInitTable[FlashIndex], sizeof (SPI_INIT_TABLE));

    if ((FlashPartId[0] != SpiInstance->SpiInitTable.VendorId) ||
        (FlashPartId[1] != SpiInstance->SpiInitTable.DeviceId0) ||
        (FlashPartId[2] != SpiInstance->SpiInitTable.DeviceId1))
    {
      DEBUG ((
        DEBUG_INFO,
        "SpiProtocolInit()   Target SPI Flash Device [VendorID 0x%02x, DeviceID 0x%02x%02x]  ",
        FlashPartId[0],
        FlashPartId[1],
        FlashPartId[2]
        ));
      DEBUG ((
        DEBUG_INFO,
        "but Current SPI Flash device [VendorId 0x%02x, DeviceID 0x%02x%02x]!\n",
        SpiInstance->SpiInitTable.VendorId,
        SpiInstance->SpiInitTable.DeviceId0,
        SpiInstance->SpiInitTable.DeviceId1
        )
        );
    } else {
      DEBUG ((
        DEBUG_INFO,
        "Smm Mode: Supported SPI Flash device found, Vendor Id: 0x%02x, Device ID: 0x%02x%02x!\n",
        FlashPartId[0],
        FlashPartId[1],
        FlashPartId[2]
        ));
      break;
    }
  }

  if (FlashIndex >= mNumSpiFlashMax) {
    Status = EFI_UNSUPPORTED;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR - Unknown SPI Flash Device, Vendor Id: 0x%02x, Device ID: 0x%02x%02x!\n",
      FlashPartId[0],
      FlashPartId[1],
      FlashPartId[2]
      ));
    ASSERT_EFI_ERROR (Status);
  }

  SpiInstance->InitDone = TRUE;

  return EFI_SUCCESS;
}

/**

  Lock the SPI Static Configuration Interface.
  Once locked, the interface can not be changed and can only be clear by system reset.

  @param This      Pointer to the EFI_SPI_PROTOCOL instance.

  @retval EFI_SUCCESS             Lock operation succeed.
  @retval EFI_DEVICE_ERROR        Device error, operation failed.
  @retval EFI_ACCESS_DENIED       The interface has already been locked.

**/
EFI_STATUS
EFIAPI
SpiProtocolLock (
  IN EFI_SPI_PROTOCOL  *This
  )
{
  if (gInSmm) {
    // Trigger FCH SPI Protect/Lock
    if (EFI_SUCCESS == FchSpiProtect_Lock (mSpiMmioBase)) {
      mFchSpiProtect_LOCKED_ReadyToBoot = TRUE;
      DEBUG ((DEBUG_INFO, "Set FchSpiProtect to LOCK SUCCESS! \n"));
    } else {
      DEBUG ((DEBUG_INFO, "Set FchSpiProtect to LOCK FAILED!!! \n"));
    }
  }

  return EFI_SUCCESS;
}

/**

  Execute SPI commands from the host controller.
  This function would be called by runtime driver, please do not use any MMIO marco here

  @param This              Pointer to the EFI_SPI_PROTOCOL instance.
  @param OpcodeIndex       Index of the command in the OpCode Menu.
  @param PrefixOpcodeIndex Index of the first command to run when in an atomic cycle sequence.
  @param DataCycle         TRUE if the SPI cycle contains data
  @param Atomic            TRUE if the SPI cycle is atomic and interleave cycles are not allowed.
  @param ShiftOut          If DataByteCount is not zero, TRUE to shift data out and FALSE to shift data in.
  @param Address           In Descriptor Mode, for Descriptor Region, GbE Region, ME Region and Platform
                    Region, this value specifies the offset from the Region Base; for BIOS Region,
                    this value specifies the offset from the start of the BIOS Image. In Non
                    Descriptor Mode, this value specifies the offset from the start of the BIOS Image.
                    Please note BIOS Image size may be smaller than BIOS Region size (in Descriptor
                    Mode) or the flash size (in Non Descriptor Mode), and in this case, BIOS Image is
                    supposed to be placed at the top end of the BIOS Region (in Descriptor Mode) or
                    the flash (in Non Descriptor Mode)
  @param DataByteCount     Number of bytes in the data portion of the SPI cycle. This function may break the
                    data transfer into multiple operations. This function ensures each operation does
                    not cross 256 byte flash address boundary.
                    *NOTE: if there is some SPI chip that has a stricter address boundary requirement
                    (e.g., its write page size is < 256 byte), then the caller cannot rely on this
                    function to cut the data transfer at proper address boundaries, and it's the
                    caller's reponsibility to pass in a properly cut DataByteCount parameter.
  @param Buffer            Pointer to caller-allocated buffer containing the dada received or sent during the
                    SPI cycle.
  @param SpiRegionType     SPI Region type. Values EnumSpiRegionBios, EnumSpiRegionGbE, EnumSpiRegionMe,
                    EnumSpiRegionDescriptor, and EnumSpiRegionPlatformData are only applicable in
                    Descriptor mode. Value EnumSpiRegionAll is applicable to both Descriptor Mode
                    and Non Descriptor Mode, which indicates "SpiRegionOffset" is actually relative
                    to base of the 1st flash device (i.e., it is a Flash Linear Address).

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_UNSUPPORTED         Command not supported.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.

**/
EFI_STATUS
EFIAPI
SpiProtocolExecute (
  IN     EFI_SPI_PROTOCOL  *This,
  IN     UINT8             OpcodeIndex,
  IN     UINT8             PrefixOpcodeIndex,
  IN     BOOLEAN           DataCycle,
  IN     BOOLEAN           Atomic,
  IN     BOOLEAN           ShiftOut,
  IN     UINTN             Address,
  IN     UINT32            DataByteCount,
  IN OUT UINT8             *Buffer,
  IN     SPI_REGION_TYPE   SpiRegionType
  )
{
  EFI_STATUS  Status;
  UINT8       SpiStatus;

  //
  // Check if the parameters are valid.
  //
  if ((OpcodeIndex >= SPI_NUM_OPCODE) || (PrefixOpcodeIndex >= SPI_NUM_PREFIX_OPCODE)) {
    return EFI_INVALID_PARAMETER;
  }

  if (gInSmm) {
    if (mFchSpiProtect_LOCKED_ReadyToBoot) {
      FchSpiProtect_UnLock (mSpiMmioBase);
      DEBUG ((DEBUG_INFO, "FchSpiProtect UnLock!\n"));
    }
  }

  SendSpiCmd (
    This,
    SPI_OPCODE_READ_S_INDEX,
    0,
    TRUE,
    FALSE,
    FALSE,
    0,
    1,
    &SpiStatus,
    EnumSpiRegionAll
    );
  if ((SpiStatus & 1) != 0) {
    if ((OpcodeIndex == SPI_OPCODE_ERASE_INDEX) && (ShiftOut == FALSE)) {
      return EFI_ALREADY_STARTED;
    }

    DEBUG ((DEBUG_INFO, "SPI Busy, WaitForSpiCycleComplete\n"));
    WaitForSpiCycleComplete (This);
  }

  //
  // Enter 4 bytes address
  //
  if (MmioRead8 (mSpiMmioBase+FCH_SPI_MMIO_REG50_ADDR32CTRL0) & BIT0) {
    DEBUG ((DEBUG_INFO, "Enter 4-Byte address mode\n"));
    mSupport4ByteAddrFlag = TRUE;
    Status                = SendSpiCmd (
                              This,
                              SPI_COMMAND_Enter_4Byte_Addr_INDEX,
                              SPI_WREN_INDEX,
                              FALSE,
                              TRUE,
                              FALSE,
                              0,
                              0,
                              NULL,
                              EnumSpiRegionDescriptor
                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Enter 4-Byte address mode fail\n"));
      goto Exit;
    }
  }

  //
  // Sends the command to the SPI interface to execute.
  //
  Status = SendSpiCmd (
             This,
             OpcodeIndex,
             PrefixOpcodeIndex,
             DataCycle,
             Atomic,
             ShiftOut,
             Address,
             DataByteCount,
             Buffer,
             SpiRegionType
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Operate SPI Flash fail\n"));
    goto Exit;
  }

  //
  // Exit 32-bit address
  //
  if (mSupport4ByteAddrFlag) {
    mSupport4ByteAddrFlag = FALSE;
    Status                = SendSpiCmd (
                              This,
                              SPI_COMMAND_Exit_4Byte_Addr_INDEX,
                              SPI_WREN_INDEX,
                              FALSE,
                              TRUE,
                              FALSE,
                              0,
                              0,
                              NULL,
                              EnumSpiRegionDescriptor
                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Exit 4-Byte address mode fail\n"));
      goto Exit;
    }
  }

Exit:
  if (gInSmm) {
    if (mFchSpiProtect_LOCKED_ReadyToBoot) {
      FchSpiProtect_Lock (mSpiMmioBase);
      DEBUG ((DEBUG_INFO, "FchSpiProtect Lock again!\n"));
    }
  }

  return Status;
}

/**

  Waits for SPI device not busy

  @param SpiBar           The SPI Bar Address

  @retval EFI_SUCCESS     Function successfully returned
  @retval EFI_TIMEOUT     timeout, SPI device busy more than 6s.

**/
EFI_STATUS
FchSpiControllerNotBusy (
  UINTN  SpiBar
  )
{
  volatile  UINT32  SpiStatus;
  UINT64            WaitTicks;
  UINT64            WaitCount;

  //
  // Convert the wait period allowed into to tick count
  //
  WaitCount = WAIT_TIME / WAIT_PERIOD;
  //
  // Wait until SPI Conroller Not Busy
  //
  for (WaitTicks = 0; WaitTicks < WaitCount; WaitTicks++) {
    SpiStatus = MmioRead32 (SpiBar + FCH_SPI_MMIO_REG4C);
    if (SpiStatus & FCH_SPI_BUSY) {
      MicroSecondDelay (WAIT_PERIOD);
    } else {
      return EFI_SUCCESS;
    }
  }

  return EFI_TIMEOUT;
}

STATIC
EFI_STATUS
ResetFifoIndex (
  SPI_INSTANCE  *SpiInstance
  )
{
  return EFI_SUCCESS;
}

/**

  This function sends the programmed SPI command to the slave device.

  @param OpcodeIndex       Index of the command in the OpCode Menu.
  @param PrefixOpcodeIndex Index of the first command to run when in an atomic cycle sequence.
  @param DataCycle         TRUE if the SPI cycle contains data
  @param Atomic            TRUE if the SPI cycle is atomic and interleave cycles are not allowed.
  @param ShiftOut          If DataByteCount is not zero, TRUE to shift data out and FALSE to shift data in.
  @param Address           In Descriptor Mode, for Descriptor Region, GbE Region, ME Region and Platform
                           Region, this value specifies the offset from the Region Base; for BIOS Region,
                           this value specifies the offset from the start of the BIOS Image. In Non
                           Descriptor Mode, this value specifies the offset from the start of the BIOS Image.
                           Please note BIOS Image size may be smaller than BIOS Region size (in Descriptor
                           Mode) or the flash size (in Non Descriptor Mode), and in this case, BIOS Image is
                           supposed to be placed at the top end of the BIOS Region (in Descriptor Mode) or
                           the flash (in Non Descriptor Mode)
  @param DataByteCount     Number of bytes in the data portion of the SPI cycle. This function may break the
                           data transfer into multiple operations. This function ensures each operation does
                           not cross 256 byte flash address boundary.
                           *NOTE: if there is some SPI chip that has a stricter address boundary requirement
                           (e.g., its write page size is < 256 byte), then the caller cannot rely on this
                           function to cut the data transfer at proper address boundaries, and it's the
                           caller's reponsibility to pass in a properly cut DataByteCount parameter.
  @param Buffer            Data received or sent during the SPI cycle.
  @param SpiRegionType     SPI Region type. Values EnumSpiRegionBios, EnumSpiRegionGbE, EnumSpiRegionMe,
                           EnumSpiRegionDescriptor, and EnumSpiRegionPlatformData are only applicable in
                           Descriptor mode. Value EnumSpiRegionAll is applicable to both Descriptor Mode
                           and Non Descriptor Mode, which indicates "SpiRegionOffset" is actually relative
                           to base of the 1st flash device (i.e., it is a Flash Linear Address).

  @retval EFI_SUCCESS             SPI command completes successfully.
  @retval EFI_DEVICE_ERROR        Device error, the command aborts abnormally.
  @retval EFI_ACCESS_DENIED       Some unrecognized command encountered in hardware sequencing mode
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.

**/
EFI_STATUS
SendSpiCmd (
  IN     EFI_SPI_PROTOCOL  *This,
  IN     UINT8             OpcodeIndex,
  IN     UINT8             PrefixOpcodeIndex,
  IN     BOOLEAN           DataCycle,
  IN     BOOLEAN           Atomic,
  IN     BOOLEAN           ShiftOut,
  IN     UINTN             Address,
  IN     UINT32            DataByteCount,
  IN OUT UINT8             *Buffer,
  IN     SPI_REGION_TYPE   SpiRegionType
  )
{
  SPI_INSTANCE           *SpiInstance;
  UINTN                  SpiBiosSize;
  UINT32                 SpiDataCount;
  UINT32                 TxByteCount;
  UINT32                 RxByteCount;
  UINTN                  Addr, Retry;
  INTN                   Index, CountIndex;
  UINTN                  SpiBar;
  BOOLEAN                WriteFlag;
  BOOLEAN                AddressFlag;
  UINT8                  PrefixOpcode;
  SPI_OPCODE_MENU_ENTRY  OPCodeMenu;
  UINT8                  Dummy;
  UINT8                  AddressByteNum;

  SpiInstance    = SPI_INSTANCE_FROM_SPIPROTOCOL (This);
  SpiBiosSize    = SpiInstance->SpiInitTable.BiosSize;
  OPCodeMenu     = SpiInstance->SpiInitTable.OpcodeMenu[OpcodeIndex];
  PrefixOpcode   = SpiInstance->SpiInitTable.PrefixOpcode[PrefixOpcodeIndex];
  SpiBar         = SpiInstance->SpiBar;
  Dummy          = 0;
  AddressByteNum = 3;

  AddressFlag  = (OPCodeMenu.Type == EnumSpiOpcodeWrite);
  WriteFlag    = AddressFlag;
  AddressFlag |= (OPCodeMenu.Type == EnumSpiOpcodeRead);
  WriteFlag   |= (OPCodeMenu.Type == EnumSpiOpcodeWriteNoAddr);

  //
  // Check if the value of opcode register is 0 or the BIOS Size of SpiInitTable is 0
  //
  if (SpiBiosSize == 0) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if ((DataCycle == FALSE) && (DataByteCount > 0)) {
    DataByteCount = 0;
  }

  do {
    SpiDataCount = DataByteCount;
    TxByteCount  = 0;
    RxByteCount  = 0;

    //
    // Calculate the number of bytes to shift in/out during the SPI data cycle.
    // Valid settings for the number of bytes duing each data portion of the
    // FCH SPI cycles are: 0, 1, 2, 3, 4, 5, 6, 7, 8, 16, 24, 32, 40, 48, 56, 64
    //
    if ((Address & 0xFF) == 0) {
      SpiDataCount = (DataByteCount > 0x100) ? 0x100 : DataByteCount;
    } else {
      SpiDataCount = (DataByteCount > ((~Address + 1) & 0xFF)) ? ((~Address + 1) & 0xFF) : DataByteCount;
    }

    SpiDataCount = (SpiDataCount > 64) ? 64 : SpiDataCount;

    if (Atomic) {
      Retry = 0;
      do {
        MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG45, PrefixOpcode);
        MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG48_TXBYTECOUNT, 0);
        MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG4B_RXBYTECOUNT, 0);
        MmioOr8 (SpiBar + FCH_SPI_MMIO_REG47, FCH_SPI_EXEC_OPCODE);
        if (EFI_ERROR (FchSpiControllerNotBusy (SpiBar))) {
          return EFI_DEVICE_ERROR;
        }

        if (PrefixOpcodeIndex == SPI_WREN_INDEX) {
          if (WaitForSpiDeviceWriteEnabled (This) == EFI_SUCCESS) {
            Retry = 0;
          } else {
            Retry++;
            if (Retry >= 3) {
              return EFI_DEVICE_ERROR;
            }
          }
        }
      } while (Retry);
    }

    Index = 0;
    //
    // Check Address Mode
    //
    if (AddressFlag) {
      ResetFifoIndex (SpiInstance);
      Addr = (UINTN)Address;
      // if not SPI_COMMAND_READ_SFDP and 32bit address
      if ((OPCodeMenu.Code != SPI_COMMAND_READ_SFDP) && mSupport4ByteAddrFlag) {
        AddressByteNum = 4;
        Addr           = Addr | (UINT32)((MmioRead8 (SpiBar+FCH_SPI_MMIO_REG5C_ADDR32CTRL3) & BIT0) << 24);
      }

      for (CountIndex = 0; CountIndex < AddressByteNum; CountIndex++) {
        MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG80_FIFO + Index, (UINT8)((Addr >> (AddressByteNum - CountIndex - 1) * 8) & 0xff));
        Index++;
      }

      TxByteCount += AddressByteNum;
    }

    if ((OPCodeMenu.Code == SPI_COMMAND_READ_SFDP) || (OPCodeMenu.Code == SPI_COMMAND_RPMC_OP2)) {
      MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG80_FIFO + Index, Dummy);
      Index++;
      TxByteCount += 1;
    }

    if (DataCycle) {
      //
      // Check Read/Write Mode
      //
      if (WriteFlag) {
        TxByteCount += SpiDataCount;
        for (CountIndex = 0; CountIndex < (INTN)(SpiDataCount); CountIndex++) {
          MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG80_FIFO + Index, Buffer[CountIndex]);
          Index++;
        }
      } else {
        RxByteCount = SpiDataCount;
      }
    }

    // Set SPI Command
    MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG45, OPCodeMenu.Code);
    MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG48_TXBYTECOUNT, (UINT8)TxByteCount);
    MmioWrite8 (SpiBar + FCH_SPI_MMIO_REG4B_RXBYTECOUNT, (UINT8)RxByteCount);
    MmioOr8 (SpiBar + FCH_SPI_MMIO_REG47, FCH_SPI_EXEC_OPCODE);
    if (EFI_ERROR (FchSpiControllerNotBusy (SpiBar))) {
      return EFI_DEVICE_ERROR;
    }

    if (ShiftOut) {
      Retry = 0;
      do {
        if (WaitForSpiCycleComplete (This)) {
          Retry = 0;
        } else {
          Retry++;
          if (Retry >= FCH_SPI_RETRY_TIMES) {
            return EFI_DEVICE_ERROR;
          }
        }
      } while (Retry);
    }

    if (DataCycle && RxByteCount) {
      //
      // Reset Fifo Ptr
      //
      ResetFifoIndex (SpiInstance);

      for (CountIndex = 0; CountIndex < (INTN)(SpiDataCount); CountIndex++) {
        Buffer[CountIndex] = MmioRead8 (SpiBar + FCH_SPI_MMIO_REG80_FIFO + Index);
        Index++;
      }
    }

    //
    // If shifts data in, get data from the SPI data buffer.
    //
    Address       += SpiDataCount;
    Buffer        += SpiDataCount;
    DataByteCount -= SpiDataCount;
  } while (DataByteCount > 0);

  return EFI_SUCCESS;
}

/**

  Wait execution cycle to complete on the SPI interface. Check both Hardware
  and Software Sequencing status registers

  @param This   The SPI protocol instance

  @retval TRUE  SPI cycle completed on the interface.
  @retval FALSE Time out while waiting the SPI cycle to complete.
             It's not safe to program the next command on the SPI interface.

**/
BOOLEAN
WaitForSpiCycleComplete (
  IN     EFI_SPI_PROTOCOL  *This
  )
{
  UINT8   SpiStatus;
  UINT64  WaitTicks;
  UINT64  WaitCount;

  //
  // Convert the wait period allowed into to tick count
  //
  WaitCount = WAIT_TIME / WAIT_PERIOD;

  //
  // Wait for the SPI cycle to complete.
  //
  for (WaitTicks = 0; WaitTicks < WaitCount; WaitTicks++) {
    //
    // Execute Read Status Command
    //
    SendSpiCmd (
      This,
      SPI_OPCODE_READ_S_INDEX,
      0,
      TRUE,
      FALSE,
      FALSE,
      0,
      1,
      &SpiStatus,
      EnumSpiRegionAll
      );

    if ((SpiStatus & 1) != 0) {
      MicroSecondDelay (WAIT_PERIOD);
    } else {
      return TRUE;
    }
  }

  DEBUG ((DEBUG_VERBOSE, "WaitForSpiCycleComplete() Timeout\n"));
  return FALSE;
}

/**

  Wait execution cycle to complete on the SPI interface. Check both Hardware
  and Software Sequencing status registers

  @param This   The SPI protocol instance

  @retval EFI_SUCCESS  SPI cycle completed on the interface.
  @retval EFI_TIMEOUT  Time out while waiting the SPI cycle to complete.
             It's not safe to program the next command on the SPI interface.

**/
STATIC
EFI_STATUS
WaitForSpiDeviceWriteEnabled (
  IN  EFI_SPI_PROTOCOL  *This
  )
{
  UINT8   SpiStatus;
  UINT64  WaitTicks;
  UINT64  WaitCount;

  DEBUG ((DEBUG_VERBOSE, "WaitForSpiDeviceWriteEnabled() Enter!\n"));

  //
  // Convert the wait period allowed into to tick count
  //
  WaitCount = WAIT_TIME / WAIT_PERIOD;

  //
  // Wait for the SPI cycle to complete.
  //
  for (WaitTicks = 0; WaitTicks < WaitCount; WaitTicks++) {
    //
    // Execute Read Status Command
    //
    SendSpiCmd (
      This,
      SPI_OPCODE_READ_S_INDEX,
      0,
      TRUE,
      FALSE,
      FALSE,
      0,
      1,
      &SpiStatus,
      EnumSpiRegionAll
      );

    if ((SpiStatus & 2) == 0) {
      MicroSecondDelay (WAIT_PERIOD);
    } else {
      DEBUG ((DEBUG_VERBOSE, "WaitForSpiCycleComplete() Exit\n"));
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_VERBOSE, "WaitForSpiDeviceWriteEnabled() Exit Timeout !\n"));
  return EFI_TIMEOUT;
}

#ifdef _MSC_VER
  #pragma optimize( "", on )
#endif
#ifdef __GNUC__
  #ifndef __clang__
    #pragma GCC pop_options
  #endif
#endif
