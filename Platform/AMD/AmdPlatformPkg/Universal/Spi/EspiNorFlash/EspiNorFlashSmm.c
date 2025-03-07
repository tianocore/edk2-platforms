/*****************************************************************************
 * Copyright (C) 2018-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *****************************************************************************/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SpiSmmConfiguration.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include "EspiNorFlash.h"
#include "EspiNorFlashInstance.h"

/**
  Entry point of the Macronix SPI NOR Flash driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS       Succeed.
  @retval EFI_DEVICE_ERROR  Fail to install EFI_ESPI_SMM_NOR_FLASH_PROTOCOL.
**/
EFI_STATUS
EFIAPI
EspiNorFlashEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  ESPI_NOR_FLASH_INSTANCE     *Instance;
  EFI_SPI_NOR_FLASH_PROTOCOL  *Protocol;
  ESPI_SL40_SLAVE_FA_CAPCFG   FaCapCfg;
  ESPIx68_SLAVE0_CONFIG       EspiReg68;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __FUNCTION__));

  if (PcdGet8 (PcdAmdPspRomArmorSelection) >= 2) {
    // If RomArmor2 or 3 is enabled, skip
    DEBUG ((DEBUG_INFO, "PcdAmdPspRomArmorSelection >= 2"));
    return EFI_UNSUPPORTED;
  }

  // Allocate the Board SPI Configuration Instance
  Instance = AllocateZeroPool (sizeof (ESPI_NOR_FLASH_INSTANCE));
  ASSERT (Instance != NULL);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->Signature = ESPI_NOR_FLASH_SIGNATURE;

  // Locate the SPI IO Protocol
  Status = gSmst->SmmLocateProtocol (
                    &gEdk2EspiSmmDriverProtocolGuid,
                    NULL,
                    (VOID **)&Instance->SpiIo
                    );

  if (EFI_ERROR (Status) ||
      ((Instance->SpiIo->Attributes & (SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS |
                                       SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE)) != 0))
  {
    FreePool (Instance);
    Status = EFI_UNSUPPORTED;
  } else {
    // Allocate write buffer for SPI IO transactions with extra room for Opcode
    // and Address
    Instance->SpiTransactionWriteBuffer = AllocatePool (
                                            Instance->SpiIo->MaximumTransferBytes + 10 // Add extra room
                                            );
    Protocol                  = &Instance->Protocol;
    Protocol->SpiPeripheral   = Instance->SpiIo->SpiPeripheral;
    Protocol->GetFlashid      = GetFlashId;
    Protocol->ReadData        = ReadData;
    Protocol->LfReadData      = LfReadData;
    Protocol->ReadStatus      = ReadStatus;
    Protocol->WriteStatus     = WriteStatus;
    Protocol->WriteData       = WriteData;
    Protocol->Erase           = Erase;
    Protocol->EraseBlockBytes = SIZE_4KB;

    if (IsEspiSafsMode (&(Instance->EspiBaseAddress))) {
      // ESPI SAFS
      Instance->EspiSafsMode = TRUE;
      Protocol->FlashSize    = PcdGet32 (PcdFlashAreaSize);
      Instance->EspiEraseBlockMap = FchEspiCmd_GetConfiguration (Instance->EspiBaseAddress, SLAVE_FA_CAPCFG2);
      FaCapCfg.Value              = FchEspiCmd_GetConfiguration (Instance->EspiBaseAddress, SLAVE_FA_CAPCFG);

      if (FaCapCfg.Field.ChMaxReadReqSize != 0) {
        Instance->EspiMaxReadReqSize = 64 << (FaCapCfg.Field.ChMaxReadReqSize - 1);
      } else {
        Instance->EspiMaxReadReqSize = 64;  // Set 64 bytes as default
      }

      EspiReg68.Value = FchEspiCmd_GetConfiguration (Instance->EspiBaseAddress, ESPI_SLAVE0_CONFIG);
      if (EspiReg68.Field.FlashMaxPayloadSize == 0x01) {
        Instance->EspiMaxPayloadSize = 64;
      } else if (EspiReg68.Field.FlashMaxPayloadSize == 0x02) {
        Instance->EspiMaxPayloadSize = 128;
      } else if (EspiReg68.Field.FlashMaxPayloadSize == 0x03) {
        Instance->EspiMaxPayloadSize = 256;
      } else {
        Instance->EspiMaxPayloadSize = 64;   // Set 64 bytes as default
      }

      DEBUG ((DEBUG_INFO, "ESPI SAFS mode, EspiBaseAddress = 0x%x\n", Instance->EspiBaseAddress));
      DEBUG ((DEBUG_INFO, "  EspiEraseBlockMap  = 0x%x\n", Instance->EspiEraseBlockMap));
      DEBUG ((DEBUG_INFO, "  EspiMaxReadReqSize = 0x%x\n", Instance->EspiMaxReadReqSize));
      DEBUG ((DEBUG_INFO, "  EspiMaxPayloadSize = 0x%x\n", Instance->EspiMaxPayloadSize));
    } else {
      // SPI MAFS
      Instance->EspiSafsMode = FALSE;
      Status                 = Protocol->GetFlashid (
                                           Protocol,
                                           (UINT8 *)&Protocol->Deviceid
                                           );
      ASSERT_EFI_ERROR (Status);
      DEBUG ((
        DEBUG_INFO,
        "%a: Flash ID: Manufacturer=0x%02X, Device=0x%02X%02X\n",
        __FUNCTION__,
        Protocol->Deviceid[0],
        Protocol->Deviceid[1],
        Protocol->Deviceid[2]
        ));

      Status = ReadSfdpBasicParameterTable (Instance);
      ASSERT_EFI_ERROR (Status);

      // SFDP DWORD 2
      Protocol->FlashSize = (Instance->SfdpBasicFlash->Density + 1) / 8;
      DEBUG ((DEBUG_INFO, "%a: Flash Size=0x%X\n", __FUNCTION__, Protocol->FlashSize));

      if (Protocol->FlashSize > SIZE_16MB) {
        // If flash size is more than 16MB, enable 4byte mode
        Instance->SpiTransactionWriteBuffer[0] = 0xb7;// SPI_FLASH_4BYTEMODE; // 4byte mode opcode
        Status                                 = Instance->SpiIo->Transaction (
                                                                    Instance->SpiIo,
                                                                    SPI_TRANSACTION_WRITE_ONLY,
                                                                    FALSE,
                                                                    0,
                                                                    1,
                                                                    8,
                                                                    1,
                                                                    Instance->SpiTransactionWriteBuffer,
                                                                    0,
                                                                    NULL
                                                                    );
        DEBUG ((DEBUG_INFO, "%a: enable 4-Byte mode (OpCode 0xB7) %r\n", __FUNCTION__, Status));
        ASSERT_EFI_ERROR (Status);
      }
    }

    Status = gSmst->SmmInstallProtocolInterface (
                      &Instance->Handle,
                      &gAmdEspiSmmNorFlashProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &Instance->Protocol
                      );
  }

  DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __FUNCTION__, Status));

  return Status;
}
