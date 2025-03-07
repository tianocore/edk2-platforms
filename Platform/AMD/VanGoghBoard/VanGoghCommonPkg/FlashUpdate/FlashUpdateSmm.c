/** @file
  Implements AMD FlashUpdateSmm.c

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FlashUpdateSmm.h"

#define PM1_EN_HIGH_BYTE  0x03
#define RTC_EVENT_ENABLE  0x04
#define ACPIMMIO16(x)  (*(volatile UINT16*)(UINTN)(x))
#define ACPI_MMIO_BASE    0xFED80000ul
#define ACPI_PM1_EVT_BLK  0x60
#define ACPI_PM1_CNT_BLK  0x62
#define PMIO_BASE         0x300                     // DWORD
#define SUS_S3            0x0C00U                   // S3
#define SUS_S5            0x1400U                   // S5
#define SLP_TYPE          0x1C00U                   // MASK
#define SLP_EN            0x2000U                   // BIT13

EFI_SPI_PROTOCOL  *mSmmSpiProtocol = NULL;
UINTN             mFlashAreaBaseAddress;
UINTN             mFlashSize;
UINTN             mBlockSize;

EFI_SMM_SPI_FLASH_UPDATE_PROTOCOL  mEfiSmmSpiFlashUpdateProtocol = {
  FlashUpdateServiceFlashFdRead,
  FlashUpdateServiceFlashFdErase,
  FlashUpdateServiceFlashFdWrite,
  FlashUpdateServiceGetFlashSizeBlockSize,
};

/**
  Read data from flash device.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte.
  @param[out] Buffer                      Buffer contain the read data.

  @retval EFI_SUCCESS                     Read successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceFlashFdRead (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes,
  OUT VOID   *Buffer
  )
{
  if ((FlashAddress >= mFlashSize) || (NumBytes == 0) || (NumBytes > mFlashSize) ||
      (Buffer == NULL) || (FlashAddress + NumBytes > mFlashSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Buffer, (UINT8 *)(FlashAddress + mFlashAreaBaseAddress), NumBytes);

  return EFI_SUCCESS;
}

/**
  Erase flash region according to input in a block size.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte, a block size in flash device.

  @retval EFI_SUCCESS                     Erase successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceFlashFdErase (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes
  )
{
  EFI_STATUS  Status;

  if ((FlashAddress >= mFlashSize) || (NumBytes == 0) || (NumBytes > mFlashSize) ||
      (FlashAddress + NumBytes > mFlashSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (mSmmSpiProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "mSmmSpiProtocol = NULL\n"));
    return EFI_NOT_FOUND;
  }

  Status = mSmmSpiProtocol->Execute (
                              mSmmSpiProtocol,
                              SPI_OPCODE_ERASE_INDEX,  // OpcodeIndex
                              0,                       // PrefixOpcodeIndex
                              FALSE,                   // DataCycle
                              TRUE,                    // Atomic
                              TRUE,                    // ShiftOut
                              FlashAddress,            // Address
                              0,                       // Data Number
                              NULL,                    // Buffer
                              EnumSpiRegionBios        // SpiRegionType
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "!!!ERROR: Erase flash %r\n", Status));
    return Status;
  }

  AsmWbinvd ();

  return EFI_SUCCESS;
}

/**
  Write data to flash device.

  Write Buffer(FlashAddress|NumBytes) to flash device.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte.
  @param[in]  Buffer                      Buffer contain the write data.

  @retval EFI_SUCCESS                     Write successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceFlashFdWrite (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes,
  IN  UINT8  *Buffer
  )
{
  EFI_STATUS  Status;

  if ((FlashAddress >= mFlashSize) || (NumBytes == 0) || (NumBytes > mFlashSize) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (FlashAddress + NumBytes > mFlashSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (mSmmSpiProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "mSmmSpiProtocol = NULL\n"));
    return EFI_NOT_FOUND;
  }

  Status = mSmmSpiProtocol->Execute (
                              mSmmSpiProtocol,
                              SPI_OPCODE_WRITE_INDEX, // OpcodeIndex
                              0,                      // PrefixOpcodeIndex
                              TRUE,                   // DataCycle
                              TRUE,                   // Atomic
                              TRUE,                   // ShiftOut
                              FlashAddress,           // Address
                              (UINT32)NumBytes,       // Data Number
                              Buffer,                 // Buffer
                              EnumSpiRegionBios       // Spi Region Type
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "!!!ERROR: Write flash %r\n", Status));
    return Status;
  }

  AsmWbinvd ();

  return EFI_SUCCESS;
}

/**
  Get flash device size and flash block size.

  @param[out] FlashSize                   Pointer to the size of flash device.
  @param[out] BlockSize                   Pointer to the size of block in flash device.

  @retval EFI_SUCCESS                     Get successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceGetFlashSizeBlockSize (
  OUT  UINTN  *FlashSize,
  OUT  UINTN  *BlockSize
  )
{
  if ((FlashSize == 0) || (BlockSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *FlashSize = mFlashSize;
  *BlockSize = mBlockSize;

  return EFI_SUCCESS;
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for the update flash routines.

  @param[in]      DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]      RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *RegisterContext,
  IN OUT VOID        *CommBuffer,
  IN OUT UINTN       *CommBufferSize
  )
{
  EFI_STATUS                          Status;
  FLASH_UPDATE_SMM_COMMUNICATION_CMN  *Buffer;
  SMM_COMM_RWE_FLASH                  *RweBuffer;
  SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE  *GetFlashSizeBlockSizeBuffer;

  //
  // If input is invalid, stop processing this SMI
  //
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    DEBUG ((DEBUG_ERROR, "!!!ERROR: FlashUpdateServiceHandler: Invalid parameter!\n"));
    return EFI_SUCCESS;
  }

  if (!SmmIsBufferOutsideSmmValid ((UINTN)CommBuffer, *CommBufferSize)) {
    DEBUG ((DEBUG_ERROR, "!!!ERROR: FlashUpdateServiceHandler: SMM communication buffer in SMRAM or overflow!\n"));
    return EFI_SUCCESS;
  }

  GetFlashSizeBlockSizeBuffer = NULL;
  Buffer                      = (FLASH_UPDATE_SMM_COMMUNICATION_CMN *)CommBuffer;
  RweBuffer                   = (SMM_COMM_RWE_FLASH *)CommBuffer;

  switch (Buffer->id) {
    case SPI_SMM_COMM_ID_GET_FLASH_SIZE_BLOCK_SIZE:
      GetFlashSizeBlockSizeBuffer               = (SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE *)CommBuffer;
      GetFlashSizeBlockSizeBuffer->ReturnStatus = EFI_SUCCESS;
      GetFlashSizeBlockSizeBuffer->FlashSize    = mFlashSize;
      GetFlashSizeBlockSizeBuffer->BlockSize    = mBlockSize;
      break;

    case SPI_SMM_COMM_ID_READ_FLASH:
      Status = mEfiSmmSpiFlashUpdateProtocol.Read (
                                               RweBuffer->FlashAddress,
                                               RweBuffer->NumBytes,
                                               RweBuffer->Buffer
                                               );
      RweBuffer->ReturnStatus = Status;
      break;

    case SPI_SMM_COMM_ID_WRITE_FALSH:
      Status = mEfiSmmSpiFlashUpdateProtocol.Write (
                                               RweBuffer->FlashAddress,
                                               RweBuffer->NumBytes,
                                               RweBuffer->Buffer
                                               );
      RweBuffer->ReturnStatus = Status;
      break;

    case SPI_SMM_COMM_ID_ERASE_FALSH:
      Status = mEfiSmmSpiFlashUpdateProtocol.Erase (
                                               RweBuffer->FlashAddress,
                                               RweBuffer->NumBytes
                                               );
      RweBuffer->ReturnStatus = Status;
      break;

    default:
      ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}

/**
  Read RTC through its registers  using IO access.

  @param  Address   Address offset of RTC. It is recommended to use
                    macros such as RTC_ADDRESS_SECONDS.
  @param  Data      The content you want to write into RTC.

**/
STATIC
UINT8
IoRtcRead (
  IN  UINT8  Address
  )
{
  IoWrite8 (0x70, Address);
  return IoRead8 (0x71);
}

/**
  Write RTC through its registers  using IO access.

  @param  Address   Address offset of RTC. It is recommended to use
                    macros such as RTC_ADDRESS_SECONDS.
  @param  Data      The content you want to write into RTC.

**/
STATIC
VOID
IoRtcWrite (
  IN  UINT8  Address,
  IN  UINT8  Data
  )
{
  IoWrite8 (0x70, Address);
  IoWrite8 (0x71, Data);
}

/**
  Write RTC through its registers  using IO access.

  @param  Address   Address offset of RTC. It is recommended to use
                    macros such as RTC_ADDRESS_SECONDS.
  @param  Data      The content you want to write into RTC.

**/
VOID
EnableRtcWakeup (
  IN UINT16  AcpiBaseAddr,
  IN UINT8   WakeAfter
  )
{
  volatile RTC_REGISTER_B  RtcRegisterB;

  RtcRegisterB.Data = IoRtcRead (RTC_ADDRESS_REGISTER_B);
  UINT8  CurrentSecond = IoRtcRead (RTC_ADDRESS_SECONDS);
  UINT8  CurrentMinute = IoRtcRead (RTC_ADDRESS_MINUTES);
  UINT8  CurrentHour   = IoRtcRead (RTC_ADDRESS_HOURS);

  if (!(RtcRegisterB.Bits.Dm)) {
    CurrentSecond = BcdToDecimal8 (CurrentSecond);
    CurrentMinute = BcdToDecimal8 (CurrentMinute);
    CurrentHour   = BcdToDecimal8 (CurrentHour);
  }

  CurrentSecond += WakeAfter;
  CurrentMinute += CurrentSecond/60;
  CurrentHour   += CurrentMinute/60;

  CurrentSecond %= 60;
  CurrentMinute %= 60;
  CurrentHour   %= 24;

  if (!(RtcRegisterB.Bits.Dm)) {
    CurrentSecond = DecimalToBcd8 (CurrentSecond);
    CurrentMinute = DecimalToBcd8 (CurrentMinute);
    CurrentHour   = DecimalToBcd8 (CurrentHour);
  }

  IoRtcWrite (RTC_ADDRESS_SECONDS_ALARM, CurrentSecond);
  IoRtcWrite (RTC_ADDRESS_MINUTES_ALARM, CurrentMinute);
  IoRtcWrite (RTC_ADDRESS_HOURS_ALARM, CurrentHour);
  IoRtcRead (RTC_ADDRESS_REGISTER_C);

  RtcRegisterB.Data     = IoRtcRead (RTC_ADDRESS_REGISTER_B);
  RtcRegisterB.Bits.Aie = 1;
  IoRtcWrite (RTC_ADDRESS_REGISTER_B, RtcRegisterB.Data);

  UINT8  RtcSts = IoRead8 (AcpiBaseAddr);

  RtcSts |= 0x400;
  IoWrite8 (AcpiBaseAddr, RtcSts);

  UINT8  RtcEn = IoRead8 (AcpiBaseAddr + PM1_EN_HIGH_BYTE);

  RtcEn |= RTC_EVENT_ENABLE;
  IoWrite8 (AcpiBaseAddr + PM1_EN_HIGH_BYTE, RtcEn);
  return;
}

/**
  Set Capsule S3 Flag SMI Handler.

  This SMI handler provides services for marking capsule update.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.

**/
EFI_STATUS
EFIAPI
SetCapsuleS3FlagHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *RegisterContext,
  IN OUT VOID        *CommBuffer,
  IN OUT UINTN       *CommBufferSize
  )
{
  EFI_STATUS                     Status;
  AMD_CAPSULE_SMM_HOOK_PROTOCOL  *AmdCapsuleSmmHookProtocol;

  AmdCapsuleSmmHookProtocol = NULL;
  Status                    = gSmst->SmmLocateProtocol (
                                       &gAmdCapsuleSmmHookProtocolGuid,
                                       NULL,
                                       (VOID **)&AmdCapsuleSmmHookProtocol
                                       );
  if (!EFI_ERROR (Status)) {
    AmdCapsuleSmmHookProtocol->Hook (0);
  }

  DEBUG ((DEBUG_INFO, "Entering S3 sleep.\n"));
  // Transform system into S3 sleep state
  EnableRtcWakeup (ACPIMMIO16 (ACPI_MMIO_BASE + PMIO_BASE + ACPI_PM1_EVT_BLK), 2);
  UINTN   AcpiPm1CntBase = ACPIMMIO16 (ACPI_MMIO_BASE + PMIO_BASE + ACPI_PM1_CNT_BLK);
  UINT16  PmCntl         = IoRead16 (AcpiPm1CntBase);

  PmCntl = (PmCntl & ~SLP_TYPE) | SUS_S3 | SLP_EN;
  IoWrite16 (AcpiPm1CntBase, PmCntl);
  return Status;
}

/**
  Smm Flash Update Driver main entry point.

  Install the Smm Flash Update Protocol on a new handle. Register SMM flash update
  SMI handler. Locate SmmSpiProtocol and init the flash device size and block size.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Variable service successfully initialized.
  @retval others            Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    FlashUpdateRegisterHandle;
  EFI_HANDLE    FlashUpdateInstallHandle;
  SPI_INSTANCE  *SpiInstance;
  EFI_HANDLE    SetCapsuleS3FlagHandle;

  SpiInstance = NULL;

  //
  // Install the Smm Flash Update Protocol on a new handle
  //
  FlashUpdateInstallHandle = NULL;
  Status                   = gSmst->SmmInstallProtocolInterface (
                                      &FlashUpdateInstallHandle,
                                      &gEfiSmmSpiFlashUpdateProtocolGuid,
                                      EFI_NATIVE_INTERFACE,
                                      &mEfiSmmSpiFlashUpdateProtocol
                                      );
  ASSERT_EFI_ERROR (Status);

  //
  // Register SMM flash update SMI handler
  //
  FlashUpdateRegisterHandle = NULL;
  Status                    = gSmst->SmiHandlerRegister (
                                       FlashUpdateServiceHandler,
                                       &gEfiSmmSpiFlashUpdateProtocolGuid,
                                       &FlashUpdateRegisterHandle
                                       );
  ASSERT_EFI_ERROR (Status);

  SetCapsuleS3FlagHandle = NULL;
  Status                 = gSmst->SmiHandlerRegister (
                                    SetCapsuleS3FlagHandler,
                                    &gAmdSetCapsuleS3FlagGuid,
                                    &SetCapsuleS3FlagHandle
                                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSpiProtocolGuid,
                    NULL,
                    (VOID **)&mSmmSpiProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  SpiInstance           = SPI_INSTANCE_FROM_SPIPROTOCOL (mSmmSpiProtocol);
  mFlashSize            = SpiInstance->SpiInitTable.BiosSize;
  mFlashAreaBaseAddress = FixedPcdGet32 (PcdFlashAreaBaseAddress);
  mBlockSize            = SpiInstance->SpiInitTable.OpcodeMenu[SPI_OPCODE_ERASE_INDEX].Operation;

  return Status;
}
