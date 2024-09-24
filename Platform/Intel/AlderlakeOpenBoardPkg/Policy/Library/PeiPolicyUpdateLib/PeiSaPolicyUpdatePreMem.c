/** @file
Do Platform Stage System Agent initialization.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiSaPolicyUpdate.h"
#include "MemoryConfig.h"
#include <Guid/MemoryOverwriteControl.h>
#include <Guid/MemoryTypeInformation.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CpuPcieInfoFruLib.h>
#include <Library/CpuPlatformLib.h>
#include <Library/GpioLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiSiPolicyUpdateLib.h>
#include <Library/SiPolicyLib.h>
#include <Register/CommonMsr.h>
#include <Register/Cpuid.h>
#include <Register/Msr.h>
#include <CpuRegs.h>
#include <HostBridgeConfig.h>
#include <PlatformBoardConfig.h>
#include <PolicyUpdateMacro.h>
#include <SaDataHob.h>
#include <Ppi/FspmArchConfigPpi.h>

///
/// Memory Reserved should be between 125% to 150% of the Current required memory
/// otherwise BdsMisc.c would do a reset to make it 125% to avoid s4 resume issues.
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   FixedPcdGet32 (PcdPlatformEfiAcpiReclaimMemorySize) },  // ASL
  { EfiACPIMemoryNVS,       FixedPcdGet32 (PcdPlatformEfiAcpiNvsMemorySize) },      // ACPI NVS (including S3 related)
  { EfiReservedMemoryType,  FixedPcdGet32 (PcdPlatformEfiReservedMemorySize) },     // BIOS Reserved (including S3 related)
  { EfiRuntimeServicesData, FixedPcdGet32 (PcdPlatformEfiRtDataMemorySize) },       // Runtime Service Data
  { EfiRuntimeServicesCode, FixedPcdGet32 (PcdPlatformEfiRtCodeMemorySize) },       // Runtime Service Code
  { EfiMaxMemoryType, 0 }
};

#define  PEI_MIN_MEMORY_SIZE               (10 * 0x800000)   // 80MB

/**
  UpdatePeiSaPolicyPreMem performs SA PEI Policy initialization

  @retval EFI_SUCCESS              The policy is installed and initialized.
**/
EFI_STATUS
EFIAPI
UpdatePeiSaPolicyPreMem (
  VOID
  )
{
  EFI_STATUS                                      Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI                 *VariableServices;
  UINTN                                           VariableSize;
  SA_MEMORY_RCOMP                                 *RcompData;
  WDT_PPI                                         *gWdtPei;
  UINT8                                           WdtTimeout;

  UINT8                                           Index;
  UINTN                                           DataSize;
  EFI_MEMORY_TYPE_INFORMATION                     MemoryData[EfiMaxMemoryType + 1];
  EFI_BOOT_MODE                                   BootMode;
  UINT8                                           MorControl;
  UINT64                                          PlatformMemorySize;
  VOID                                            *MemorySavedData;
  VOID                                            *NullSpdPtr;
  SI_PREMEM_POLICY_PPI                            *SiPreMemPolicyPpi;
  MEMORY_CONFIGURATION                            *MemConfig;
  SA_MISC_PEI_PREMEM_CONFIG                       *MiscPeiPreMemConfig;
  MEMORY_CONFIG_NO_CRC                            *MemConfigNoCrc;
  EFI_PEI_PPI_DESCRIPTOR                          *FspmArchConfigPpiDesc;
  FSPM_ARCH_CONFIG_PPI                            *FspmArchConfigPpi;
  HOST_BRIDGE_PREMEM_CONFIG                       *HostBridgePreMemConfig;
  UINT8                                           SaDisplayConfigTable[16];
  UINT32                                          ProcessorTraceTotalMemSize;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX     Ebx;
  UINT32                                          CapsuleSupportMemSize;

  DEBUG ((DEBUG_INFO, "Update PeiSaPolicyUpdate Pre-Mem Start\n"));
  ZeroMem ((VOID*) SaDisplayConfigTable, sizeof (SaDisplayConfigTable));
  WdtTimeout           = 0;
  RcompData            = NULL;
  PlatformMemorySize   = 0;
  SiPreMemPolicyPpi    = NULL;
  MemConfig            = NULL;
  MemConfigNoCrc       = NULL;


  MiscPeiPreMemConfig  = NULL;
  HostBridgePreMemConfig = NULL;
  FspmArchConfigPpi    = NULL;

  ProcessorTraceTotalMemSize = 0;
  CapsuleSupportMemSize = 0;

  Status = PeiServicesLocatePpi (&gSiPreMemPolicyPpiGuid, 0, NULL, (VOID **) &SiPreMemPolicyPpi);
  ASSERT_EFI_ERROR (Status);

  Status = GetConfigBlock ((VOID *) SiPreMemPolicyPpi, &gHostBridgePeiPreMemConfigGuid, (VOID *) &HostBridgePreMemConfig);
  ASSERT_EFI_ERROR (Status);

  Status = GetConfigBlock((VOID *) SiPreMemPolicyPpi, &gSaMiscPeiPreMemConfigGuid, (VOID *) &MiscPeiPreMemConfig);
  ASSERT_EFI_ERROR(Status);

  Status = GetConfigBlock((VOID *) SiPreMemPolicyPpi, &gMemoryConfigGuid, (VOID *) &MemConfig);
  ASSERT_EFI_ERROR(Status);

  Status = GetConfigBlock((VOID *) SiPreMemPolicyPpi, &gMemoryConfigNoCrcGuid, (VOID *) &MemConfigNoCrc);
  ASSERT_EFI_ERROR(Status);

  RcompData = MemConfigNoCrc->RcompData;

  //
  // Locate system configuration variable
  //
  Status = PeiServicesLocatePpi(
             &gEfiPeiReadOnlyVariable2PpiGuid, // GUID
             0,                                // INSTANCE
             NULL,                             // EFI_PEI_PPI_DESCRIPTOR
             (VOID **) &VariableServices       // PPI
             );
  ASSERT_EFI_ERROR(Status);

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize S3 Data variable (S3DataPtr)
  //
  VariableSize = 0;
  MemorySavedData = NULL;
  Status = VariableServices->GetVariable (
                               VariableServices,
                               L"MemoryConfig",
                               &gMemoryConfigVariableGuid,
                               NULL,
                               &VariableSize,
                               MemorySavedData
                               );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    MemorySavedData = AllocateZeroPool (VariableSize);
    ASSERT (MemorySavedData != NULL);

    Status = VariableServices->GetVariable (
                                 VariableServices,
                                 L"MemoryConfig",
                                 &gMemoryConfigVariableGuid,
                                 NULL,
                                 &VariableSize,
                                 MemorySavedData
                                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Fail to retrieve Variable: MemoryConfig, Status = %r\n", Status));
      ASSERT_EFI_ERROR (Status);
    }
  }
  FspmArchConfigPpi = (FSPM_ARCH_CONFIG_PPI *) AllocateZeroPool (sizeof (FSPM_ARCH_CONFIG_PPI));
  if (FspmArchConfigPpi == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }
  FspmArchConfigPpi->Revision     = 1;
  FspmArchConfigPpi->NvsBufferPtr = MemorySavedData;
  MiscPeiPreMemConfig->S3DataPtr  = MemorySavedData;

  FspmArchConfigPpiDesc = (EFI_PEI_PPI_DESCRIPTOR *) AllocateZeroPool (sizeof (EFI_PEI_PPI_DESCRIPTOR));
  if (FspmArchConfigPpiDesc == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }
  FspmArchConfigPpiDesc->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  FspmArchConfigPpiDesc->Guid  = &gFspmArchConfigPpiGuid;
  FspmArchConfigPpiDesc->Ppi   = FspmArchConfigPpi;

  //
  // Install FSP-M Arch Config PPI
  //
  Status = PeiServicesInstallPpi (FspmArchConfigPpiDesc);
  ASSERT_EFI_ERROR (Status);

  VariableSize = sizeof (MorControl);
  Status = VariableServices->GetVariable(
                               VariableServices,
                               MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                               &gEfiMemoryOverwriteControlDataGuid,
                               NULL,
                               &VariableSize,
                               &MorControl
                               );
  if (EFI_ERROR (Status)) {
    MorControl = 0;
  }

  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.UserBd,     MiscPeiPreMemConfig->UserBd,      0); // It's a CRB mobile board by default (btCRBMB)

  MiscPeiPreMemConfig->TxtImplemented = 0;

  if (PcdGet32 (PcdMrcRcompTarget)) {
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.RcompTarget, (VOID *)RcompData->RcompTarget, (VOID *)(UINTN)PcdGet32 (PcdMrcRcompTarget), sizeof (UINT16) * MRC_MAX_RCOMP_TARGETS);
  }

  if (PcdGetBool (PcdMrcDqPinsInterleavedControl)) {
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.DqPinsInterleaved, MemConfig->DqPinsInterleaved, PcdGetBool (PcdMrcDqPinsInterleaved));
  }

  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[0], MiscPeiPreMemConfig->SpdAddressTable[0], PcdGet8 (PcdMrcSpdAddressTable0));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[1], MiscPeiPreMemConfig->SpdAddressTable[1], PcdGet8 (PcdMrcSpdAddressTable1));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[2], MiscPeiPreMemConfig->SpdAddressTable[2], PcdGet8 (PcdMrcSpdAddressTable2));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[3], MiscPeiPreMemConfig->SpdAddressTable[3], PcdGet8 (PcdMrcSpdAddressTable3));
  if (PcdGet8 (PcdMrcLp5CccConfig)) {
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.Lp5CccConfig, MemConfig->Lp5CccConfig, PcdGet8 (PcdMrcLp5CccConfig));
  }


  NullSpdPtr = AllocateZeroPool (SPD_DATA_SIZE);
  ASSERT (NullSpdPtr != NULL);

  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[4], MiscPeiPreMemConfig->SpdAddressTable[4], PcdGet8 (PcdMrcSpdAddressTable4));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[5], MiscPeiPreMemConfig->SpdAddressTable[5], PcdGet8 (PcdMrcSpdAddressTable5));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[6], MiscPeiPreMemConfig->SpdAddressTable[6], PcdGet8 (PcdMrcSpdAddressTable6));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[7], MiscPeiPreMemConfig->SpdAddressTable[7], PcdGet8 (PcdMrcSpdAddressTable7));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[8], MiscPeiPreMemConfig->SpdAddressTable[8], PcdGet8 (PcdMrcSpdAddressTable8));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[9], MiscPeiPreMemConfig->SpdAddressTable[9], PcdGet8 (PcdMrcSpdAddressTable9));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[10], MiscPeiPreMemConfig->SpdAddressTable[10], PcdGet8 (PcdMrcSpdAddressTable10));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[11], MiscPeiPreMemConfig->SpdAddressTable[11], PcdGet8 (PcdMrcSpdAddressTable11));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[12], MiscPeiPreMemConfig->SpdAddressTable[12], PcdGet8 (PcdMrcSpdAddressTable12));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[13], MiscPeiPreMemConfig->SpdAddressTable[13], PcdGet8 (PcdMrcSpdAddressTable13));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[14], MiscPeiPreMemConfig->SpdAddressTable[14], PcdGet8 (PcdMrcSpdAddressTable14));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SpdAddressTable[15], MiscPeiPreMemConfig->SpdAddressTable[15], PcdGet8 (PcdMrcSpdAddressTable15));
  if (PcdGet32 (PcdMrcRcompResistor)) {
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.RcompResistor, RcompData->RcompResistor, (UINT8) PcdGet32 (PcdMrcRcompResistor));
  }
  if (PcdGet32 (PcdMrcDqsMapCpu2Dram)) {
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.DqsMapCpu2DramMc0Ch0, (VOID *)MemConfigNoCrc->DqDqsMap->DqsMapCpu2Dram, (VOID *)(UINTN)PcdGet32 (PcdMrcDqsMapCpu2Dram), sizeof (UINT8) * MEM_CFG_MAX_CONTROLLERS * MEM_CFG_MAX_CHANNELS * MEM_CFG_NUM_BYTES_MAPPED);
  }
  if (PcdGet32 (PcdMrcDqMapCpu2Dram)) {
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.DqMapCpu2DramMc0Ch0, (VOID *)MemConfigNoCrc->DqDqsMap->DqMapCpu2Dram, (VOID *)(UINTN)PcdGet32 (PcdMrcDqMapCpu2Dram), sizeof (UINT8) * MEM_CFG_MAX_CONTROLLERS * MEM_CFG_MAX_CHANNELS * MEM_CFG_NUM_BYTES_MAPPED * 8);
  }
  if (PcdGetBool (PcdSpdPresent)) {
    // Clear SPD data so it can be filled in by the MRC init code
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr000, (VOID *) MemConfigNoCrc->SpdData->SpdData[0][0][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr010, (VOID *) MemConfigNoCrc->SpdData->SpdData[0][1][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr020, (VOID *) MemConfigNoCrc->SpdData->SpdData[0][2][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr030, (VOID *) MemConfigNoCrc->SpdData->SpdData[0][3][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr100, (VOID *) MemConfigNoCrc->SpdData->SpdData[1][0][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr110, (VOID *) MemConfigNoCrc->SpdData->SpdData[1][1][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr120, (VOID *) MemConfigNoCrc->SpdData->SpdData[1][2][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
    COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr130, (VOID *) MemConfigNoCrc->SpdData->SpdData[1][3][0], (VOID *)(UINT32) NullSpdPtr, SPD_DATA_SIZE);
  } else {
    if (PcdGet32 (PcdMrcSpdData)) {
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr000, (VOID *)MemConfigNoCrc->SpdData->SpdData[0][0][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr010, (VOID *)MemConfigNoCrc->SpdData->SpdData[0][1][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr020, (VOID *)MemConfigNoCrc->SpdData->SpdData[0][2][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr030, (VOID *)MemConfigNoCrc->SpdData->SpdData[0][3][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr100, (VOID *)MemConfigNoCrc->SpdData->SpdData[1][0][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr110, (VOID *)MemConfigNoCrc->SpdData->SpdData[1][1][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr120, (VOID *)MemConfigNoCrc->SpdData->SpdData[1][2][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
      COPY_POLICY ((VOID *)((FSPM_UPD *) FspmUpd)->FspmConfig.MemorySpdPtr130, (VOID *)MemConfigNoCrc->SpdData->SpdData[1][3][0], (VOID *)(UINTN)PcdGet32 (PcdMrcSpdData), SPD_DATA_SIZE);
    }
  }

  HostBridgePreMemConfig->MchBar   = (UINTN) PcdGet64 (PcdMchBaseAddress);
  HostBridgePreMemConfig->DmiBar   = (UINTN) PcdGet64 (PcdDmiBaseAddress);
  HostBridgePreMemConfig->EpBar    = (UINTN) PcdGet64 (PcdEpBaseAddress);
  HostBridgePreMemConfig->EdramBar = (UINTN) PcdGet64 (PcdEdramBaseAddress);
  MiscPeiPreMemConfig->SmbusBar = (UINTN) PcdGet16 (PcdSmbusBaseAddress);
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.TsegSize,           MiscPeiPreMemConfig->TsegSize,           PcdGet32 (PcdTsegSize));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.UserBd,             MiscPeiPreMemConfig->UserBd,             PcdGet8 (PcdSaMiscUserBd));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.DisableMrcRetrainingOnRtcPowerLoss,MiscPeiPreMemConfig->DisableMrcRetrainingOnRtcPowerLoss,   PcdGet8(PcdSaMiscDisableMrcRetrainingOnRtcPowerLoss));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.MmioSizeAdjustment, HostBridgePreMemConfig->MmioSizeAdjustment, PcdGet16 (PcdSaMiscMmioSizeAdjustment));
  //
  // Display DDI Initialization ( default Native GPIO as per board during AUTO case)
  //
  CopyMem (SaDisplayConfigTable, (VOID *) (UINTN) PcdGet32 (PcdSaDisplayConfigTable), (UINTN)PcdGet16 (PcdSaDisplayConfigTableSize));

  gWdtPei = NULL;
  Status = PeiServicesLocatePpi(
             &gWdtPpiGuid,
             0,
             NULL,
             (VOID **) &gWdtPei
             );
  if (gWdtPei != NULL) {
    WdtTimeout = gWdtPei->CheckStatus();
  } else {
    WdtTimeout = FALSE;
  }

  if ((WdtTimeout == FALSE)) {
    //
    // If USER custom profile is selected, we will start the WDT.
    //
    if (gWdtPei != NULL) {
      Status = gWdtPei->ReloadAndStart(WDT_TIMEOUT);
    }
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.VddVoltage,   MemConfig->VddVoltage,          0); // Use platform default as the safe value.
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.VddqVoltage,  MemConfig->VddqVoltage,         0); // Use platform default as the safe value.
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.VppVoltage,   MemConfig->VppVoltage,          0); // Use platform default as the safe value.
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.Ratio,        MemConfig->Ratio,               0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tCL,          MemConfig->tCL,                 0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tCWL,         MemConfig->tCWL,                0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tFAW,         MemConfig->tFAW,                0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tRAS,         MemConfig->tRAS,                0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tRCDtRP,      MemConfig->tRCDtRP,             0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tREFI,        MemConfig->tREFI,               0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tRFC,         MemConfig->tRFC,                0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tRRD,         MemConfig->tRRD,                0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tRTP,         MemConfig->tRTP,                0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tWR,          MemConfig->tWR,                 0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.tWTR,         MemConfig->tWTR,                0);
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.NModeSupport, MemConfig->NModeSupport,        0);
  }


  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.CmdMirror,                            MemConfig->CmdMirror,             PcdGet8 (PcdMrcCmdMirror)); // BitMask where bits [3:0] are controller 0 Channel [3:0] and [7:4] are Controller 1 Channel [3:0].  0 = No Command Mirror and 1 = Command Mirror.

  // FirstDimmBitMask defines which DIMM should be populated first on a 2DPC board

  COMPARE_AND_UPDATE_POLICY(((FSPM_UPD *)FspmUpd)->FspmConfig.FirstDimmBitMask, MemConfig->FirstDimmBitMask, PcdGet8(PcdSaMiscFirstDimmBitMask));
  COMPARE_AND_UPDATE_POLICY(((FSPM_UPD *)FspmUpd)->FspmConfig.FirstDimmBitMaskEcc, MemConfig->FirstDimmBitMaskEcc, PcdGet8(PcdSaMiscFirstDimmBitMaskEcc));

  //
  // Update CleanMemory variable from Memory overwrite request value. Ignore if we are performing capsule update.
  //
  if ((BootMode != BOOT_ON_FLASH_UPDATE) && (BootMode != BOOT_ON_S3_RESUME)) {
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.CleanMemory, MemConfigNoCrc->CleanMemory, (BOOLEAN)(MorControl & MOR_CLEAR_MEMORY_BIT_MASK));
  }

  DataSize = sizeof (MemoryData);
  Status = VariableServices->GetVariable (
                               VariableServices,
                               EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                               &gEfiMemoryTypeInformationGuid,
                               NULL,
                               &DataSize,
                               &MemoryData
                               );
  ///
  /// Accumulate maximum amount of memory needed
  ///
  PlatformMemorySize = MemConfigNoCrc->PlatformMemorySize;
  AsmCpuidEx (CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, 0, NULL, &Ebx.Uint32, NULL, NULL);

  if (EFI_ERROR (Status)) {
    ///
    /// Use default value to avoid memory fragment.
    /// OS boot/installation fails if there is not enough continuous memory available
    ///
    PlatformMemorySize = PEI_MIN_MEMORY_SIZE + ProcessorTraceTotalMemSize + CapsuleSupportMemSize;
    DataSize = sizeof (mDefaultMemoryTypeInformation);
    CopyMem (MemoryData, mDefaultMemoryTypeInformation, DataSize);
  } else {
    ///
    /// Start with at least PEI_MIN_MEMORY_SIZE of memory for the DXE Core and the DXE Stack
    ///
    PlatformMemorySize = PEI_MIN_MEMORY_SIZE + ProcessorTraceTotalMemSize + CapsuleSupportMemSize;
  }
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.PlatformMemorySize, MemConfigNoCrc->PlatformMemorySize, PlatformMemorySize);

  if (BootMode != BOOT_IN_RECOVERY_MODE) {
    for (Index = 0; Index < DataSize / sizeof (EFI_MEMORY_TYPE_INFORMATION); Index++) {
      PlatformMemorySize += MemoryData[Index].NumberOfPages * EFI_PAGE_SIZE;
    }
    UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.PlatformMemorySize, MemConfigNoCrc->PlatformMemorySize, PlatformMemorySize);

    ///
    /// Build the GUID'd HOB for DXE
    ///
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      MemoryData,
      DataSize
      );
  }

  return EFI_SUCCESS;
}
