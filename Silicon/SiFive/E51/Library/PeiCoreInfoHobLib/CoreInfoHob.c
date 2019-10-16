/**@file
  Build up platform processor information.

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

#include <IndustryStandard/RiscVOpensbi.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FirmwareContextProcessorSpecificLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/ResourcePublicationLib.h>

#include <ProcessorSpecificHobData.h>
#include <RiscVImpl.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_platform.h>
#include <SmbiosProcessorSpecificData.h>

/**
  Function to build core specific information HOB. RISC-V SMBIOS DXE driver collect
  this information and build SMBIOS Type44.

  @param  ParentProcessorGuid    Parent processor od this core. ParentProcessorGuid
                                 could be the same as CoreGuid if one processor has
                                 only one core.
  @param  ParentProcessorUid     Unique ID of pysical processor which owns this core.
  @param  HartId                 Hart ID of this core.
  @param  IsBootHart             TRUE means this is the boot HART.
  @param  GuidHobData            Pointer to receive   EFI_HOB_GUID_TYPE.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
CreateE51CoreProcessorSpecificDataHob (
  IN EFI_GUID  *ParentProcessorGuid,
  IN UINTN     ParentProcessorUid,
  IN UINTN     HartId,
  IN BOOLEAN   IsBootHart,
  OUT RISC_V_PROCESSOR_SPECIFIC_HOB_DATA **GuidHobData
  )
{
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA *CoreGuidHob;
  EFI_GUID *ProcessorSpecDataHobGuid;
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA ProcessorSpecDataHob;
  struct sbi_scratch *ThisHartSbiScratch;
  struct sbi_platform *ThisHartSbiPlatform;
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContext;
  EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC *FirmwareContextHartSpecific;

  DEBUG ((DEBUG_INFO, "%a: Entry.\n", __FUNCTION__));

  if (GuidHobData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ThisHartSbiScratch = sbi_hart_id_to_scratch (sbi_scratch_thishart_ptr(), (UINT32)HartId);
  DEBUG ((DEBUG_INFO, "    SBI Scratch is at 0x%x.\n", ThisHartSbiScratch));
  ThisHartSbiPlatform = (struct sbi_platform *)sbi_platform_ptr(ThisHartSbiScratch);
  DEBUG ((DEBUG_INFO, "    SBI platform is at 0x%x.\n", ThisHartSbiPlatform));
  FirmwareContext = (EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *)ThisHartSbiPlatform->firmware_context;
  DEBUG ((DEBUG_INFO, "    Firmware Context is at 0x%x.\n", FirmwareContext));
  FirmwareContextHartSpecific = FirmwareContext->HartSpecific[HartId];
  DEBUG ((DEBUG_INFO, "    Firmware Context Hart specific is at 0x%x.\n", FirmwareContextHartSpecific));

  //
  // Build up RISC_V_PROCESSOR_SPECIFIC_HOB_DATA.
  //
  CommonFirmwareContextHartSpecificInfo (
      FirmwareContextHartSpecific,
      ParentProcessorGuid,
      ParentProcessorUid,
      (EFI_GUID *)PcdGetPtr (PcdSiFiveE51CoreGuid),
      HartId,
      IsBootHart,
      &ProcessorSpecDataHob
      );
  ProcessorSpecDataHob.ProcessorSpecificData.MModeExcepDelegation.Value64_L     = TO_BE_FILLED;
  ProcessorSpecDataHob.ProcessorSpecificData.MModeExcepDelegation.Value64_H     = TO_BE_FILLED;
  ProcessorSpecDataHob.ProcessorSpecificData.MModeInterruptDelegation.Value64_L = TO_BE_FILLED;
  ProcessorSpecDataHob.ProcessorSpecificData.MModeInterruptDelegation.Value64_H = TO_BE_FILLED;
  ProcessorSpecDataHob.ProcessorSpecificData.HartXlen                 = RegisterLen64;
  ProcessorSpecDataHob.ProcessorSpecificData.MachineModeXlen          = RegisterLen64;
  ProcessorSpecDataHob.ProcessorSpecificData.SupervisorModeXlen       = RegisterUnsupported;
  ProcessorSpecDataHob.ProcessorSpecificData.UserModeXlen             = RegisterLen64;

  DEBUG ((DEBUG_INFO, "        *HartId = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.HartId.Value64_L));
  DEBUG ((DEBUG_INFO, "        *Is Boot Hart? = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.BootHartId));
  DEBUG ((DEBUG_INFO, "        *PrivilegeModeSupported = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.PrivilegeModeSupported));
  DEBUG ((DEBUG_INFO, "        *MModeExcepDelegation = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.MModeExcepDelegation.Value64_L));
  DEBUG ((DEBUG_INFO, "        *MModeInterruptDelegation = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.MModeInterruptDelegation.Value64_L));
  DEBUG ((DEBUG_INFO, "        *HartXlen = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.HartXlen ));
  DEBUG ((DEBUG_INFO, "        *MachineModeXlen = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.MachineModeXlen));
  DEBUG ((DEBUG_INFO, "        *SupervisorModeXlen = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.SupervisorModeXlen));
  DEBUG ((DEBUG_INFO, "        *UserModeXlen = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.UserModeXlen));
  DEBUG ((DEBUG_INFO, "        *InstSetSupported = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.InstSetSupported));
  DEBUG ((DEBUG_INFO, "        *MachineVendorId = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.MachineVendorId.Value64_L));
  DEBUG ((DEBUG_INFO, "        *MachineArchId = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.MachineArchId.Value64_L));
  DEBUG ((DEBUG_INFO, "        *MachineImplId = 0x%x\n", ProcessorSpecDataHob.ProcessorSpecificData.MachineImplId.Value64_L));

  //
  // Build GUID HOB for E51 core, this is for SMBIOS type 44
  //
  ProcessorSpecDataHobGuid = PcdGetPtr (PcdProcessorSpecificDataGuidHobGuid);
  CoreGuidHob = (RISC_V_PROCESSOR_SPECIFIC_HOB_DATA *)BuildGuidDataHob (ProcessorSpecDataHobGuid, (VOID *)&ProcessorSpecDataHob, sizeof (RISC_V_PROCESSOR_SPECIFIC_HOB_DATA));
  if (CoreGuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive E51 core.\n"));
    ASSERT (FALSE);
  }
  *GuidHobData = CoreGuidHob;
  return EFI_SUCCESS;
}

/**
  Function to build processor related SMBIOS information. RISC-V SMBIOS DXE driver collect
  this information and build SMBIOS Type4 and Type7 record.

  @param  ProcessorUid    Unique ID of pysical processor which owns this core.
  @param  SmbiosHobPtr    Pointer to receive RISC_V_PROCESSOR_SMBIOS_HOB_DATA. The pointers
                          maintained in this structure is only valid before memory is discovered.
                          Access to those pointers after memory is installed will cause unexpected issues.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
CreateE51ProcessorSmbiosDataHob (
  IN UINTN     ProcessorUid,
  OUT RISC_V_PROCESSOR_SMBIOS_HOB_DATA **SmbiosHobPtr
  )
{
  EFI_GUID *GuidPtr;
  RISC_V_PROCESSOR_TYPE4_HOB_DATA ProcessorDataHob;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA L1InstCacheDataHob;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA SmbiosDataHob;
  RISC_V_PROCESSOR_TYPE4_HOB_DATA *ProcessorDataHobPtr;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA *L1InstCacheDataHobPtr;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA *SmbiosDataHobPtr;

  if (SmbiosHobPtr == NULL) {
      return EFI_INVALID_PARAMETER;
  }
  //
  // Build up SMBIOS type 7 L1 instruction cache record.
  //
  ZeroMem((VOID *)&L1InstCacheDataHob, sizeof (RISC_V_PROCESSOR_TYPE7_HOB_DATA));
  CopyGuid (&L1InstCacheDataHob.PrcessorGuid, (EFI_GUID *)PcdGetPtr (PcdSiFiveE51CoreGuid));
  L1InstCacheDataHob.ProcessorUid = ProcessorUid;
  L1InstCacheDataHob.SmbiosType7Cache.SocketDesignation = TO_BE_FILLED_BY_VENDOR;
  L1InstCacheDataHob.SmbiosType7Cache.CacheConfiguration = RISC_V_CACHE_CONFIGURATION_CACHE_LEVEL_1 | \
      RISC_V_CACHE_CONFIGURATION_LOCATION_INTERNAL | \
      RISC_V_CACHE_CONFIGURATION_ENABLED | \
      RISC_V_CACHE_CONFIGURATION_MODE_UNKNOWN;
  L1InstCacheDataHob.SmbiosType7Cache.MaximumCacheSize = TO_BE_FILLED_BY_VENDOR;
  L1InstCacheDataHob.SmbiosType7Cache.InstalledSize = TO_BE_FILLED_BY_VENDOR;
  L1InstCacheDataHob.SmbiosType7Cache.SupportedSRAMType.Unknown = 1;
  L1InstCacheDataHob.SmbiosType7Cache.CurrentSRAMType.Unknown = 1;
  L1InstCacheDataHob.SmbiosType7Cache.CacheSpeed = TO_BE_FILLED_BY_VENDOR;
  L1InstCacheDataHob.SmbiosType7Cache.ErrorCorrectionType = TO_BE_FILLED_BY_VENDOR;
  L1InstCacheDataHob.SmbiosType7Cache.SystemCacheType = CacheTypeInstruction;
  L1InstCacheDataHob.SmbiosType7Cache.Associativity = TO_BE_FILLED_BY_VENDOR;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosType7GuidHobGuid);
  L1InstCacheDataHobPtr = (RISC_V_PROCESSOR_TYPE7_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&L1InstCacheDataHob, sizeof (RISC_V_PROCESSOR_TYPE7_HOB_DATA));
  if (L1InstCacheDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive E51 core L1 instruction cache RISC_V_PROCESSOR_TYPE7_HOB_DATA.\n"));
    ASSERT (FALSE);
  }

  //
  // Build up SMBIOS type 4 record.
  //
  ZeroMem((VOID *)&ProcessorDataHob, sizeof (RISC_V_PROCESSOR_TYPE4_HOB_DATA));
  CopyGuid (&ProcessorDataHob.PrcessorGuid, (EFI_GUID *)PcdGetPtr (PcdSiFiveE51CoreGuid));
  ProcessorDataHob.ProcessorUid = ProcessorUid;
  ProcessorDataHob.SmbiosType4Processor.Socket = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.ProcessorType = CentralProcessor;
  ProcessorDataHob.SmbiosType4Processor.ProcessorFamily = ProcessorFamilyIndicatorFamily2;
  ProcessorDataHob.SmbiosType4Processor.ProcessorManufacture = TO_BE_FILLED_BY_VENDOR;
  SetMem ((VOID *)&ProcessorDataHob.SmbiosType4Processor.ProcessorId, sizeof (PROCESSOR_ID_DATA), TO_BE_FILLED_BY_CODE);
  ProcessorDataHob.SmbiosType4Processor.ProcessorVersion = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.Voltage.ProcessorVoltageCapability3_3V = 1;
  ProcessorDataHob.SmbiosType4Processor.ExternalClock = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.MaxSpeed = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.CurrentSpeed = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.Status = TO_BE_FILLED_BY_CODE;
  ProcessorDataHob.SmbiosType4Processor.ProcessorUpgrade = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.L1CacheHandle = TO_BE_FILLED_BY_RISC_V_SMBIOS_DXE_DRIVER;
  ProcessorDataHob.SmbiosType4Processor.L2CacheHandle = 0xffff;
  ProcessorDataHob.SmbiosType4Processor.L3CacheHandle = 0xffff;
  ProcessorDataHob.SmbiosType4Processor.SerialNumber = TO_BE_FILLED_BY_CODE;
  ProcessorDataHob.SmbiosType4Processor.AssetTag = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.PartNumber = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.CoreCount = 1;
  ProcessorDataHob.SmbiosType4Processor.EnabledCoreCount = 1;
  ProcessorDataHob.SmbiosType4Processor.ThreadCount = 1;
  ProcessorDataHob.SmbiosType4Processor.ProcessorCharacteristics = (UINT16)(1 << 2); // 64-bit capable
  ProcessorDataHob.SmbiosType4Processor.ProcessorFamily2 = ProcessorFamilyRiscVRV64;
  ProcessorDataHob.SmbiosType4Processor.CoreCount2 = 0;
  ProcessorDataHob.SmbiosType4Processor.EnabledCoreCount2 = 0;
  ProcessorDataHob.SmbiosType4Processor.ThreadCount2 = 0;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosType4GuidHobGuid);
  ProcessorDataHobPtr = (RISC_V_PROCESSOR_TYPE4_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&ProcessorDataHob, sizeof (RISC_V_PROCESSOR_TYPE4_HOB_DATA));
  if (ProcessorDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive E51 core RISC_V_PROCESSOR_TYPE4_HOB_DATA.\n"));
    ASSERT (FALSE);
  }

  ZeroMem((VOID *)&SmbiosDataHob, sizeof (RISC_V_PROCESSOR_SMBIOS_HOB_DATA));
  SmbiosDataHob.Processor = ProcessorDataHobPtr;
  SmbiosDataHob.L1InstCache = L1InstCacheDataHobPtr;
  SmbiosDataHob.L1DataCache = NULL;
  SmbiosDataHob.L2Cache = NULL;
  SmbiosDataHob.L3Cache = NULL;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosGuidHobGuid);
  SmbiosDataHobPtr = (RISC_V_PROCESSOR_SMBIOS_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&SmbiosDataHob, sizeof (RISC_V_PROCESSOR_SMBIOS_HOB_DATA));
  if (SmbiosDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive E51 core RISC_V_PROCESSOR_SMBIOS_HOB_DATA.\n"));
    ASSERT (FALSE);
  }
  *SmbiosHobPtr = SmbiosDataHobPtr;
  return EFI_SUCCESS;
}


