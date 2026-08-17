/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include "NorFlashDxe.h"

#include <Library/IoLib.h>
#include <Library/TimerLib.h>

#define SPIFMC_CTRL                0x00
#define SPIFMC_CTRL_WP_OL          BIT15

STATIC NOR_FLASH_INSTANCE         *mNorFlashInstance;
STATIC SOPHGO_SPI_MASTER_PROTOCOL *SpiMasterProtocol;
STATIC EFI_EVENT                  mNorFlashVirtualAddrChangeEvent;

EFI_STATUS
EFIAPI
SpiNorGetFlashId (
  IN SPI_NOR     *Nor,
  IN BOOLEAN     UseInRuntime
  )
{
  UINT8      Id[NOR_FLASH_MAX_ID_LEN];
  EFI_STATUS Status;

  Status = SpiMasterProtocol->ReadRegister (Nor, SPINOR_OP_RDID, SPI_NOR_MAX_ID_LEN, Id);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ReadId: Spi error while reading id\n"
      ));
    return Status;
  }

  Status = NorFlashGetInfo (Id, &Nor->Info, UseInRuntime);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: Unrecognized JEDEC Id bytes: 0x%02x%02x%02x\n",
      __func__,
      Id[0],
      Id[1],
      Id[2]));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorReadStatus (
  IN SPI_NOR     *Nor,
  IN UINT8       *Sr
  )
{
  EFI_STATUS Status;

  Status = SpiMasterProtocol->ReadRegister (Nor, SPINOR_OP_RDSR, 1, Sr);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Read the Status Register - %r\n",
      __func__,
      Status
      ));
  }

  return Status;
}

/**
  Wait for a predefined amount of time for the flash to be ready,
  or timeout occurs.
**/
EFI_STATUS
SpiNorWaitTillReady (
  IN SPI_NOR *Nor
  )
{
  UINT32 WaitTime;
  /* Unit is us */
  CONST UINT32 CHECK_INTERVAL = 100;
  /*
   * Maximum 4K sector erase time of GD25LB512ME is 700ms, in -40 ~ 125 celsius.
   * Set 2 seconds for safe and compatibility.
   */
  CONST UINT32 MAX_WAIT_TIME = 2000000;

  for (WaitTime = 0; WaitTime <= MAX_WAIT_TIME / CHECK_INTERVAL; ++WaitTime) {
    MicroSecondDelay (CHECK_INTERVAL);

    //
    // Query the Status Register to see if the flash is ready for new commands.
    //
    SpiNorReadStatus (Nor, Nor->BounceBuf);

    if (!(Nor->BounceBuf[0] & SR_WIP)) {
      return EFI_SUCCESS;
    }
  }

  return EFI_TIMEOUT;
}

STATIC
EFI_STATUS
SpiNorWriteEnable (
  IN SPI_NOR  *Nor
  )
{
  EFI_STATUS Status;

  Status = SpiMasterProtocol->WriteRegister (Nor, SPINOR_OP_WREN, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor error while write enable\n",
      __func__
      ));
    return Status;
  }

  Status = SpiNorReadStatus (Nor, Nor->BounceBuf);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor read status error while write enable\n",
      __func__
      ));
    return Status;
  }

  if (!(Nor->BounceBuf[0] & SR_WEL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable failed, get status: 0x%x\n",
      __func__,
      Nor->BounceBuf[0]
      ));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

STATIC
EFI_STATUS
SpiNorWriteDisable (
  IN SPI_NOR  *Nor
  )
{
  EFI_STATUS Status;

  Status = SpiMasterProtocol->WriteRegister (Nor, SPINOR_OP_WRDI, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor error while write disable\n",
      __func__
      ));

    return Status;
  }

  Status = SpiNorReadStatus (Nor, Nor->BounceBuf);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor read status error while write disable\n",
      __func__
      ));
    return Status;
  }

  if ((Nor->BounceBuf[0] & SR_WEL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable failed, get status: 0x%x\n",
      __func__,
      Nor->BounceBuf[0]
      ));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

EFI_STATUS
EFIAPI
SpiNorWriteStatus (
  IN SPI_NOR     *Nor,
  IN UINT8       *Sr,
  IN UINTN       Length
  )
{
  EFI_STATUS Status;

  Status = SpiNorWriteEnable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SpiMasterProtocol->WriteRegister (Nor, SPINOR_OP_WRSR, Sr, Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Register - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SpiNorWaitTillReady (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Flash is not ready for new commands - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorReadData (
  IN  SPI_NOR   *Nor,
  IN  UINTN      FlashOffset,
  IN  UINTN      Length,
  OUT UINT8      *Buffer
  )
{
  UINTN       Index;
  UINTN       Address;
  UINTN       PageOffset;
  UINTN       PageRemain;
  EFI_STATUS  Status;

  if (Length == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Length is Zero!\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is NULL!\n",
      __func__
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // read data from flash memory by PAGE
  //
  for (Index = 0; Index < Length; Index += PageRemain) {
    Address = FlashOffset + Index;
    PageOffset = IS_POW2 (Nor->Info->PageSize) ?
	         (Address & (Nor->Info->PageSize - 1)) :
		 (Address % Nor->Info->PageSize);
    PageRemain = MIN (Nor->Info->PageSize - PageOffset, Length - Index);

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Length=0x%lx\tIndex=0x%lx\tAddress=0x%lx\tPageRemain=0x%lx\tPageOffset=0x%lx\n",
      __func__,
      Length,
      Index,
      Address,
      PageRemain,
      PageOffset
      ));

    Status = SpiMasterProtocol->Read (Nor, Address, PageRemain, Buffer + Index);
    if (EFI_ERROR(Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Read Data from flash memory - %r!\n",
        __func__,
        Status
        ));
        return Status;
      }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorWriteData (
  IN SPI_NOR     *Nor,
  IN UINTN       FlashOffset,
  IN UINTN       Length,
  IN UINT8       *Buffer
  )
{
  UINTN       Index;
  UINTN       Address;
  UINTN       PageOffset;
  UINTN       PageRemain;
  EFI_STATUS  Status;

  if (Length == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Length is Zero!\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is NULL!\n",
      __func__
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Write data by PAGE
  //
  for (Index = 0; Index < Length; Index += PageRemain) {
    Address = FlashOffset + Index;
    PageOffset = IS_POW2 (Nor->Info->PageSize) ?
	         (Address & (Nor->Info->PageSize - 1)) :
		 (Address % Nor->Info->PageSize);
    PageRemain = MIN (Nor->Info->PageSize - PageOffset, Length - Index);

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Length=0x%lx\tIndex=0x%lx\tAddress=0x%lx\tPageRemain=0x%lx\n",
      __func__,
      Length,
      Index,
      Address,
      PageRemain
      ));

    Status = SpiNorWriteEnable (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Write Enable - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = SpiMasterProtocol->Write (Nor, Address, PageRemain, Buffer + Index);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Write Data - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = SpiNorWaitTillReady (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
          DEBUG_ERROR,
          "%a: Flash is not ready for new commands - %r\n",
          __func__,
          Status
          ));
      return Status;
    }
  }

  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorErase (
  IN SPI_NOR    *Nor,
  IN UINTN      FlashOffset,
  IN UINTN      Length
  )
{
  UINT32     ErasedSectors;
  INT32      Index;
  UINTN      Address;
  UINTN      EraseSize;
  EFI_STATUS Status;

  if (Length == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Length is Zero!\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Nor->Info->Flags & NOR_FLASH_ERASE_4K) {
    EraseSize = SIZE_4KB;
  } else {
    EraseSize = Nor->Info->SectorSize;
  }

  if ((FlashOffset % EraseSize) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: <flash offset addr> is not aligned erase sector size (0x%x)!\n",
      __func__,
      EraseSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Erase Sector
  //
  ErasedSectors = (Length + EraseSize - 1) / EraseSize;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Start erasing %d sectors, each %d bytes\n",
    __func__,
    ErasedSectors,
    EraseSize
    ));
  for (Index = 0; Index < ErasedSectors; Index++) {
    Address = FlashOffset + Index * EraseSize;
    //
    // Write enable
    //
    Status = SpiNorWriteEnable (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Write Enable - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Length=0x%lx\tIndex=0x%lx\tAddress=0x%lx\n",
      __func__,
      Length,
      Index,
      Address
      ));

    Status = SpiMasterProtocol->Erase (Nor, Address);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Erase Sector - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = SpiNorWaitTillReady (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Flash is not ready for new commands - %r\n",
        __func__,
        Status
        ));
      return Status;
    }
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorEraseChip (
  IN SPI_NOR   *Nor
  )
{
  EFI_STATUS Status;

  Nor->EraseOpcode = SPINOR_OP_CHIP_ERASE;

  Status = SpiNorWriteEnable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SpiMasterProtocol->Erase (Nor, 0x0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Erase Sector - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SpiNorWaitTillReady (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Flash is not ready for new commands - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorGetFlashVariableOffset (
  IN SPI_NOR              *Nor
  )
{
  EFI_STATUS           Status;
  FLASH_PARTITION_INFO *Info;
  UINTN                NameLength;
  UINTN                SuffixLength;
  UINTN                Address;

  CONST CHAR8 *Suffix = ".fd";
  SuffixLength = AsciiStrLen (Suffix);
  Address = PcdGet64 (PcdFlashPartitionTableAddress);

  Info = AllocateRuntimeZeroPool (sizeof(FLASH_PARTITION_INFO));
  if (Info == NULL) {
    DEBUG((
      DEBUG_ERROR,
      "SpiNor: Cannot allocate memory\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  do {
    Status = SpiNorReadData (Nor, Address, sizeof (FLASH_PARTITION_INFO), (UINT8 *)Info);
    if (EFI_ERROR(Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Read partition table - %r!\n",
        __func__,
        Status
        ));
      goto Error;
    }

    if (Info->Magic != DPT_MAGIC) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Bad partition table magic, set default variable offset!\n",
        __func__,
        Status
        ));

      PcdSet64S (PcdFlashVariableOffset, PcdGet64 (PcdFdOffset) + PcdGet32 (PcdRiscVDxeFvSize));

      goto Error;
    }

    Address += sizeof (FLASH_PARTITION_INFO);
    NameLength = AsciiStrLen (Info->Name);

  } while (AsciiStrCmp (Info->Name + NameLength - SuffixLength, Suffix));

  PcdSet64S (PcdFlashVariableOffset, Info->Offset + PcdGet32 (PcdRiscVDxeFvSize));

  DEBUG ((
    DEBUG_INFO,
    "%a: Load %a from nor flash 0x%x to memory 0x%lx size %d\n",
    __func__,
    Info->Name,
    Info->Offset,
    Info->Lma,
    Info->Size
    ));
  Status = EFI_SUCCESS;
Error:
  FreePool (Info);

  return Status;
}

EFI_STATUS
EFIAPI
SpiNorSoftReset (
  IN SPI_NOR     *Nor
  )
{
  EFI_STATUS Status;

  Status = SpiMasterProtocol->WriteRegister (Nor, SPINOR_SRSTEN_OP, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Enable Soft Reset - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SpiMasterProtocol->WriteRegister (Nor, SPINOR_SRST_OP, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Soft Reset - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Software Reset is not instant, and the delay varies from flash to
  // flash. Looking at a few flashes, most range somewhere below 100
  // microseconds.
  //
  MicroSecondDelay (200);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorSetProtectAll (
  IN SPI_NOR     *Nor,
  IN BOOLEAN     IsProtectAll
  )
{
  EFI_STATUS Status;
  UINTN      SpiBase;
  UINT32     Register;
  UINT8      NorStatusReg;
  UINT8      Tmp;

  //
  // Set Wp pin level to 1 to unlock Status Register.
  //
  SpiBase = Nor->SpiBase;
  Register = MmioRead32 ((UINTN)(SpiBase + SPIFMC_CTRL));
  Register |= SPIFMC_CTRL_WP_OL;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_CTRL), Register);

  NorStatusReg = 0;
  Status = SpiNorReadStatus (Nor, &NorStatusReg);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
           __func__, Status));
    return Status;
  }

  if (IsProtectAll) {
    if ((NorStatusReg & (SR_BP2 | SR_BP3 | SR_SRP0)) != (SR_BP2 | SR_BP3 | SR_SRP0)) {
      //
      // Set BP2 and BP3 to 1 to protect all blocks.
      // Set SRP0 to 1 to enable hardware protection for status registers.
      //
      Tmp = NorStatusReg | SR_BP2 | SR_BP3 | SR_SRP0;
      Status = SpiNorWriteStatus (Nor, &Tmp, 1);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Write status register - %r\n",
              __func__, Status));
        return Status;
      }

      NorStatusReg = 0;
      Status = SpiNorReadStatus (Nor, &NorStatusReg);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
              __func__, Status));
        return Status;
      }
      if (NorStatusReg != Tmp) {
        DEBUG ((DEBUG_ERROR, "Write status register fail!\n"));
        return EFI_DEVICE_ERROR;
      }
    }

    //
    // Set Wp pin level to 0.
    // The Status Register locked and cannot be written to
    // when SRP0 and Wp are set to 1 and 0, respectively.
    //
    Register = MmioRead32 ((UINTN)(SpiBase + SPIFMC_CTRL));
    Register &= ~SPIFMC_CTRL_WP_OL;
    MmioWrite32 ((UINTN)(SpiBase + SPIFMC_CTRL), Register);

    //
    // Test whether the status register is locked
    //
    Tmp = NorStatusReg & (~(SR_BP2 | SR_BP3 | SR_SRP0));
    Status = SpiNorWriteStatus (Nor, &Tmp, 1);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a: Write status register - %r\n",
             __func__, Status));
      return Status;
    }
    Tmp = 0;
    Status = SpiNorReadStatus (Nor, &Tmp);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
             __func__, Status));
      return Status;
    }
    if (Tmp != NorStatusReg) {
      DEBUG ((DEBUG_ERROR, "Test status register lock fail!\n"));
      DEBUG ((DEBUG_ERROR, "Status register can be change from 0x%x to 0x%x!\n",
              NorStatusReg, Tmp));
      return EFI_DEVICE_ERROR;
    }
  } else {
    if ((NorStatusReg & (SR_BP2 | SR_BP3 | SR_SRP0)) != 0) {
      //
      // Clear BP2, BP3 and SRP0
      //
      Tmp = NorStatusReg & (~(SR_BP2 | SR_BP3 | SR_SRP0));
      Status = SpiNorWriteStatus (Nor, &Tmp, 1);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Write status register - %r\n",
              __func__, Status));
        return Status;
      }
      Status = SpiNorReadStatus (Nor, &NorStatusReg);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
              __func__, Status));
        return Status;
      }
      if (Tmp != NorStatusReg) {
        DEBUG ((DEBUG_ERROR, "Status register clear fail!\n"));
        return EFI_DEVICE_ERROR;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorInit (
  IN SOPHGO_NOR_FLASH_PROTOCOL *This,
  IN SPI_NOR                   *Nor
  )
{
  EFI_STATUS Status;

  Nor->AddrNbytes = (Nor->Info->Flags & NOR_FLASH_4B_ADDR) ? 4 : 3;

  Status = SpiNorWriteEnable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Nor->ReadOpcode    = SPINOR_OP_READ; // Low Frequency
  Nor->ProgramOpcode = SPINOR_OP_PP;
  Nor->EraseOpcode   = (Nor->Info->Flags & NOR_FLASH_ERASE_4K) ?
	                SPINOR_OP_BE_4K : SPINOR_OP_SE;

  if (Nor->AddrNbytes == 4) {
    //
    // Enter 4-byte mode
    //
    Status = SpiMasterProtocol->WriteRegister (Nor, SPINOR_OP_EN4B, NULL, 0);
    if (EFI_ERROR (Status)) {
      DEBUG((
        DEBUG_ERROR,
        "%a: Enter 4-byte mode - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Nor->ReadOpcode    = SPINOR_OP_READ_4B;
    Nor->ProgramOpcode = SPINOR_OP_PP_4B;
    Nor->EraseOpcode   = (Nor->Info->Flags & NOR_FLASH_ERASE_4K) ?
	                  SPINOR_OP_BE_4K_4B : SPINOR_OP_SE_4B;
  }

  //
  // Initialize flash status register
  //
  Status = SpiNorWriteStatus (Nor, Nor->BounceBuf, 1);
  if (EFI_ERROR (Status)) {
    DEBUG((
      DEBUG_ERROR,
      "%a: Initialize status register - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
SpiNorVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EfiConvertPointer (0x0, (VOID**)&SpiMasterProtocol);

  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.GetFlashid);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.ReadData);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.ReadStatus);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.WriteData);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.WriteStatus);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.Erase);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.EraseChip);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.Init);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.GetFlashVariableOffset);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.SoftReset);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance->NorFlashProtocol.SetProtectAll);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance);
}

EFI_STATUS
EFIAPI
SpiNorEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS            Status;
  UINT32                Index;

  Index = 0;

  //
  // Locate SPI Master protocol
  //
  Status = gBS->LocateProtocol (
                  &gSophgoSpiMasterProtocolGuid,
                  NULL,
                  (VOID **)&SpiMasterProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
        DEBUG_ERROR,
        "%a: Cannot locate SPI Master protocol\n",
        __func__
        ));
    return Status;
  }

  //
  // Initialize Nor Flash Instance
  //
  mNorFlashInstance = AllocateRuntimeZeroPool (sizeof (NOR_FLASH_INSTANCE));
  if (mNorFlashInstance == NULL) {
    DEBUG((
      DEBUG_ERROR,
      "SpiNor: Cannot allocate memory\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create DevicePath for SPI Nor Flash
  //
  // for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    mNorFlashInstance->NorFlashProtocol.Init                    = SpiNorInit;
    mNorFlashInstance->NorFlashProtocol.GetFlashid              = SpiNorGetFlashId;
    mNorFlashInstance->NorFlashProtocol.ReadData                = SpiNorReadData;
    mNorFlashInstance->NorFlashProtocol.WriteData               = SpiNorWriteData;
    mNorFlashInstance->NorFlashProtocol.ReadStatus              = SpiNorReadStatus;
    mNorFlashInstance->NorFlashProtocol.WriteStatus             = SpiNorWriteStatus;
    mNorFlashInstance->NorFlashProtocol.Erase                   = SpiNorErase;
    mNorFlashInstance->NorFlashProtocol.EraseChip               = SpiNorEraseChip;
    mNorFlashInstance->NorFlashProtocol.GetFlashVariableOffset  = SpiNorGetFlashVariableOffset;
    mNorFlashInstance->NorFlashProtocol.SoftReset               = SpiNorSoftReset;
    mNorFlashInstance->NorFlashProtocol.SetProtectAll           = SpiNorSetProtectAll;
    mNorFlashInstance->Signature = NOR_FLASH_SIGNATURE;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &(mNorFlashInstance->Handle),
                    &gSophgoNorFlashProtocolGuid,
                    &(mNorFlashInstance->NorFlashProtocol),
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG((
        DEBUG_ERROR,
        "SpiNor: Cannot install SPI flash protocol\n"
        ));
      goto ErrorInstallProto;
    }
  // }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SpiNorVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mNorFlashVirtualAddrChangeEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to register VA change event\n",
      __func__
      ));
    goto ErrorCreateEvent;
  }

  return EFI_SUCCESS;

ErrorCreateEvent:
  gBS->UninstallMultipleProtocolInterfaces (
        &mNorFlashInstance->Handle,
        &gSophgoNorFlashProtocolGuid,
        NULL);

ErrorInstallProto:
  FreePool (mNorFlashInstance);

  return Status;
}
