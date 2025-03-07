/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SpiSmmConfiguration.h>
#include <Protocol/SpiSmmHc.h>
#include <Protocol/SpiIo.h>
#include "BoardSpiBus.h"
#include "BoardSpiBusInstance.h"

/**
  Entry point of the Board SPI Configuration driver.

  @param ImageHandle  Image handle of this driver.
  @param SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS       Succeed.
  @retval EFI_DEVICE_ERROR  Fail to install EFI_SPI_SMM_HC_PROTOCOL protocol.
**/
EFI_STATUS
EFIAPI
BoardSpiBusEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;
  SPI_IO_INSTANCE *Instance;
  EFI_SPI_HC_PROTOCOL *SpiHc;
  EFI_SPI_CONFIGURATION_PROTOCOL *SpiConfiguration;
  EFI_SPI_PERIPHERAL *SpiPeripheral;
  EFI_SPI_BUS *Bus;

  DEBUG((DEBUG_INFO, "%a - ENTRY\n", __FUNCTION__));

  // Locate the SPI HC Protocol.  Only one supported in SMM anyway
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSpiSmmHcProtocolGuid,
                    NULL,
                    (VOID **)&SpiHc
                    );

  // Locate the SPI Configuration Protocol.
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSpiSmmConfigurationProtocolGuid,
                    NULL,
                    (VOID **)&SpiConfiguration
                    );

  // SMM SPI Bus driver only supports one bus.
  if (SpiConfiguration->BusCount != 1) {
    DEBUG ((DEBUG_ERROR, "%a: Smm SPI Configuration Bus Count > 1.\n",
          __FUNCTION__));
    ASSERT (FALSE);
  }
  Bus = (EFI_SPI_BUS *)SpiConfiguration->Buslist[0];
  DEBUG ((DEBUG_INFO, "%a: Enumerating SPI BUS: %s\n", __FUNCTION__,
        Bus->FriendlyName));
  SpiPeripheral = (EFI_SPI_PERIPHERAL *)Bus->Peripherallist;
  do {
    DEBUG ((DEBUG_INFO, "%a: Installing SPI IO protocol for %s, by %s, PN=%s\n",
          __FUNCTION__, SpiPeripheral->FriendlyName,
          SpiPeripheral->SpiPart->Vendor, SpiPeripheral->SpiPart->PartNumber));
    // Allocate the SPI IO Instance
    Instance = AllocateZeroPool (sizeof (SPI_IO_INSTANCE));
    ASSERT (Instance != NULL);
    if (Instance !=NULL) {
      // fill in the instance
      Instance->Signature = SPI_IO_SIGNATURE;
      Instance->SpiConfig = SpiConfiguration;
      Instance->SpiHc = SpiHc;
      Instance->Protocol.SpiPeripheral = SpiPeripheral;
      Instance->Protocol.OriginalSpiPeripheral = SpiPeripheral;
      Instance->Protocol.FrameSizeSupportMask = SpiHc->FrameSizeSupportMask;
      Instance->Protocol.MaximumTransferBytes = SpiHc->MaximumTransferBytes;
      if ((SpiHc->Attributes & HC_TRANSFER_SIZE_INCLUDES_ADDRESS) != 0) {
        Instance->Protocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS;
      }
      if ((SpiHc->Attributes & HC_TRANSFER_SIZE_INCLUDES_OPCODE) != 0) {
        Instance->Protocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE;
      }
      if ((SpiHc->Attributes & HC_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != 0) {
        Instance->Protocol.Attributes |= SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH;
      }
      if ((SpiHc->Attributes & HC_SUPPORTS_2_BIT_DATA_BUS_WIDTH) != 0) {
        Instance->Protocol.Attributes |= SPI_IO_SUPPORTS_2_BIT_DATA_BUS_WIDTH;
      }
      Instance->Protocol.Transaction = Transaction;
      Instance->Protocol.UpdateSpiPeripheral = UpdateSpiPeripheral;
      // Install the SPI IO Protocol
      Status = gSmst->SmmInstallProtocolInterface(
                        &Instance->Handle,
                        (GUID *)SpiPeripheral->SpiPeripheralDriverGuid,
                        EFI_NATIVE_INTERFACE,
                        &Instance->Protocol
                        );
    } else {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((DEBUG_ERROR, "%a: Out of Memory resources\n",
            __FUNCTION__));
      break;
    }
    SpiPeripheral = (EFI_SPI_PERIPHERAL *)SpiPeripheral->NextSpiPeripheral;
  } while (SpiPeripheral != NULL);

  DEBUG((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __FUNCTION__, Status));

  return Status;
}
