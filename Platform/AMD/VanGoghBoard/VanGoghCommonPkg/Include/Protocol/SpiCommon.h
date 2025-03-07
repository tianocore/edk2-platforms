/** @file
  Implements AMD SpiCommon
  Header file for the PCH SPI Common Driver.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_COMMON_H_
#define SPI_COMMON_H_

#include "Protocol/Spi.h"

#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
// #include <Library/SpiFlashDeviceLib.h>

#include <Uefi/UefiBaseType.h>

//
// Maximum time allowed while waiting the SPI cycle to complete
//  Wait Time = 6 seconds = 6000000 microseconds
//  Wait Period = 10 microseconds
//
#define WAIT_TIME    6000000
#define WAIT_PERIOD  10

//
// Private data structure definitions for the driver
//
#define FCH_SPI_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'I')

//
// SPI default opcode slots
//
#define SPI_OPCODE_JEDEC_ID_INDEX           0
#define SPI_OPCODE_WRITE_S_INDEX            1
#define SPI_OPCODE_WRITE_INDEX              2
#define SPI_OPCODE_READ_INDEX               3
#define SPI_OPCODE_ERASE_INDEX              4
#define SPI_OPCODE_READ_S_INDEX             5
#define SPI_OPCODE_CHIP_ERASE_INDEX         6
#define SPI_OPCODE_READ_SFDP_INDEX          7
#define SPI_COMMAND_RPMC_OP1_INDEX          8
#define SPI_COMMAND_RPMC_OP2_INDEX          9
#define SPI_COMMAND_Enter_4Byte_Addr_INDEX  10
#define SPI_COMMAND_Exit_4Byte_Addr_INDEX   11

typedef struct {
  UINTN               Signature;
  EFI_HANDLE          Handle;
  EFI_SPI_PROTOCOL    SpiProtocol;
  SPI_INIT_TABLE      SpiInitTable;
  UINTN               SpiBar;
  BOOLEAN             InitDone; // Set to TRUE on SpiProtocolInit SUCCESS.
  SPI_INIT_INFO       InitInfo;
} SPI_INSTANCE;

#define SPI_INSTANCE_FROM_SPIPROTOCOL(a)  CR (a, SPI_INSTANCE, SpiProtocol, FCH_SPI_PRIVATE_DATA_SIGNATURE)

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
;

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
;

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
;

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
;

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
;

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
;

#endif
