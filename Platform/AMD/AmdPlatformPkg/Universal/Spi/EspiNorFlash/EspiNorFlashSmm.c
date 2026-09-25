/** @file

  eSPI NOR flash SMM implementation.

  Copyright (C) 2018 - 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <FchRegistersCommon.h>
#include "EspiNorFlash.h"
#include "EspiNorFlashInstance.h"

/**
  Check if SAFS mode is enabled. Valid eSPI SAFS RomType configs:
  2'b01: Boot from eSPI SAFS Channel with address high bits = 0x3F.
  2'b10: Boot from eSPI SAFS Channel (with address high bits = 0x00).

  @retval TRUE                   SAFS mode is enabled.
  @retval FALSE                  MAFS mode is enabled

**/
BOOLEAN
EFIAPI
IsEspiSafsMode (
  )
{
  UINT32  Misc80;
  UINT32  RomType;

  Misc80 = MmioRead32 (ACPI_MMIO_BASE + MISC_BASE + FCH_MISC_REG80);
  // romtype_1 is BIT3, Romtype_0 is BIT1.
  RomType = ((Misc80 & BIT3) >> 2) | ((Misc80 & BIT1) >> 1);
  if ((RomType == 0x1) || (RomType == 0x2)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Entry point of the eSPI Nor Flash Driver

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

  if (!IsEspiSafsMode ()) {
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
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to locate SPI IO Protocol\n", __FUNCTION__));
    FreePool (Instance);
    return Status;
  }

  Protocol                  = &Instance->Protocol;
  Protocol->GetFlashid      = GetFlashId;
  Protocol->ReadData        = ReadData;
  Protocol->ReadStatus      = ReadStatus;
  Protocol->WriteStatus     = WriteStatus;
  Protocol->WriteData       = WriteData;
  Protocol->Erase           = Erase;
  Protocol->EraseBlockBytes = SIZE_4KB;

  // ESPI SAFS
  Instance->EspiSafsMode    = TRUE;
  Instance->EspiFlashOffset = (UINT32)(PcdGet64 (PcdRom3FlashAreaSize) - 0x1000000);
  Protocol->FlashSize       = PcdGet32 (PcdRemoteFlashRomSize);
  Instance->EspiBaseAddress =  ((
                                 PciRead32 (PCI_LIB_ADDRESS (FCH_LPC_BUS, FCH_LPC_DEV, FCH_LPC_FUNC, FCH_LPC_REGA0))
                                 ) & 0xFFFFFF00) + PcdGet32 (PcdAmdEspiOffset);

  Instance->EspiEraseBlockMap = FchEspiCmd_GetConfiguration (Instance->EspiBaseAddress, SLAVE_FA_CAPCFG2);
  FaCapCfg.Value              = FchEspiCmd_GetConfiguration (Instance->EspiBaseAddress, SLAVE_FA_CAPCFG);

  if (FaCapCfg.Field.ChMaxReadReqSize != 0) {
    Instance->EspiMaxReadReqSize = 64u << (FaCapCfg.Field.ChMaxReadReqSize - 1);
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

  Status = gSmst->SmmInstallProtocolInterface (
                    &Instance->Handle,
                    &gEfiSpiSmmNorFlashProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &Instance->Protocol
                    );
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
  }

  DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __FUNCTION__, Status));

  return Status;
}
