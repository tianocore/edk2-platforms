/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PciSegmentLib.h>
#include <Protocol/DxeMmReadyToLock.h>
#include <Protocol/SpiHc.h>
#include <FchRegistersCommon.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include "AmdSpiHc.h"
#include "AmdSpiHcNull.h"
#include "AmdSpiHcInstance.h"
#include "AmdSpiHcInternal.h"

/**
  Entry point of the AMD SPI Host Controller driver.

  @param ImageHandle  Image handle of this driver.
  @param SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS       Succeed.
  @retval EFI_OUT_OF_RESOURCES  Fail to install EFI_SPI_SMM_HC_PROTOCOL protocol.
  @retval EFI_DEVICE_ERROR  Fail to install EFI_SPI_SMM_HC_PROTOCOL protocol.
**/
EFI_STATUS
EFIAPI
AmdSpiHcProtocolEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                          Status;
  SPI_HOST_CONTROLLER_INSTANCE        *Instance;

  DEBUG((DEBUG_INFO, "%a - ENTRY\n", __FUNCTION__));

  // Allocate the SPI Host Controller Instance
  Instance = AllocateZeroPool (sizeof (SPI_HOST_CONTROLLER_INSTANCE));
  ASSERT (Instance != NULL);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Instance->Signature = SPI_HOST_CONTROLLER_SIGNATURE;

  // Fill in the SPI Host Controller Protocol
  Instance->HcAddress = (
      PciSegmentRead32 (
        PCI_SEGMENT_LIB_ADDRESS (0x00, FCH_LPC_BUS, FCH_LPC_DEV, FCH_LPC_FUNC, FCH_LPC_REGA0)
        )
      ) & 0xFFFFFF00;
  Instance->Protocol.Attributes = HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS |
                               HC_SUPPORTS_READ_ONLY_OPERATIONS |
                               HC_SUPPORTS_WRITE_ONLY_OPERATIONS;

  Instance->Protocol.FrameSizeSupportMask = FCH_SPI_FRAME_SIZE_SUPPORT_MASK;
  Instance->Protocol.MaximumTransferBytes = SPI_HC_MAXIMUM_TRANSFER_BYTES;

  if (FeaturePcdGet (PcdRomArmorEnable)) {
    Instance->PspMailboxSpiMode = TRUE;
    Instance->Protocol.ChipSelect = ChipSelectNull;
    Instance->Protocol.Clock = ClockNull;
    Instance->Protocol.Transaction = TransactionNull;
  } else {
    Instance->PspMailboxSpiMode = FALSE;
    Instance->Protocol.ChipSelect = ChipSelect;
    Instance->Protocol.Clock = Clock;
    Instance->Protocol.Transaction = Transaction;
  }
  // Install Host Controller protocol
  Status = gBS->InstallProtocolInterface (
                    &Instance->Handle,
                    &gEfiSpiHcProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &Instance->Protocol
                    );

  DEBUG((DEBUG_INFO, "%a - EXIT Status=%r\n", __FUNCTION__, Status));

  return Status;
}
