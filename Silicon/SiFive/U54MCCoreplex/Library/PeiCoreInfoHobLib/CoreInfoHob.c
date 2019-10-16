/**@file
  Build up platform processor information.

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/SiFiveE51.h>
#include <Library/SiFiveU54.h>
#include <Library/SiFiveU54MCCoreplex.h>

/**
  Build up processor-specific HOB for U54MC Coreplex

  @param  UniqueId      Unique ID of this U54MC Coreplex processor

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
CreateU54MCCoreplexProcessorSpecificDataHob (
  IN UINTN UniqueId
  )
{
  EFI_STATUS Status;
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA *ThisGuidHobData;
  EFI_GUID *ParentProcessorGuid;

  DEBUG ((DEBUG_INFO, "%a: Entry.\n", __FUNCTION__));

  ParentProcessorGuid = PcdGetPtr (PcdSiFiveU54MCCoreplexGuid);
  Status = CreateE51CoreProcessorSpecificDataHob (ParentProcessorGuid, UniqueId, SIFIVE_U54MC_COREPLEX_E51_HART_ID, FALSE, &ThisGuidHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Faile to build E51 core information HOB for U54 Coreplex.\n", __FUNCTION__));
    return Status;
  }
  Status = CreateU54CoreProcessorSpecificDataHob (ParentProcessorGuid, UniqueId, SIFIVE_U54_COREPLEX_U54MC_0_HART_ID, TRUE, &ThisGuidHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Faile to build U54 core information HOB for U54 Coreplex.\n", __FUNCTION__));
    return Status;
  }
  Status = CreateU54CoreProcessorSpecificDataHob (ParentProcessorGuid, UniqueId, SIFIVE_U54_COREPLEX_U54MC_1_HART_ID, FALSE, &ThisGuidHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Faile to build U54 core information HOB for U54 Coreplex.\n", __FUNCTION__));
    return Status;
  }
  Status = CreateU54CoreProcessorSpecificDataHob (ParentProcessorGuid, UniqueId, SIFIVE_U54_COREPLEX_U54MC_2_HART_ID, FALSE, &ThisGuidHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Faile to build U54 core information HOB for U54 Coreplex.\n", __FUNCTION__));
    return Status;
  }
  Status = CreateU54CoreProcessorSpecificDataHob (ParentProcessorGuid, UniqueId, SIFIVE_U54_COREPLEX_U54MC_3_HART_ID, FALSE, &ThisGuidHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Faile to build U54 core information HOB for U54 Coreplex.\n", __FUNCTION__));
    return Status;
  }
  return Status;
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
CreateU54MCProcessorSmbiosDataHob (
  IN UINTN     ProcessorUid,
  IN RISC_V_PROCESSOR_SMBIOS_HOB_DATA **SmbiosHobPtr
  )
{
  EFI_GUID *GuidPtr;
  RISC_V_PROCESSOR_TYPE4_HOB_DATA ProcessorDataHob;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA L2CacheDataHob;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA SmbiosDataHob;
  RISC_V_PROCESSOR_TYPE4_HOB_DATA *ProcessorDataHobPtr;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA *L2CacheDataHobPtr;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA *SmbiosDataHobPtr;

  if (SmbiosHobPtr == NULL) {
      return EFI_INVALID_PARAMETER;
  }

  //
  // Build up SMBIOS type 7 L2 cache record.
  //
  ZeroMem((VOID *)&L2CacheDataHob, sizeof (RISC_V_PROCESSOR_TYPE7_HOB_DATA));
  L2CacheDataHob.PrcessorGuid = *((EFI_GUID *)PcdGetPtr (PcdSiFiveU54MCCoreplexGuid));
  L2CacheDataHob.ProcessorUid = ProcessorUid;
  L2CacheDataHob.SmbiosType7Cache.SocketDesignation = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.CacheConfiguration = RISC_V_CACHE_CONFIGURATION_CACHE_LEVEL_2 | \
      RISC_V_CACHE_CONFIGURATION_LOCATION_EXTERNAL | \
      RISC_V_CACHE_CONFIGURATION_ENABLED | \
      RISC_V_CACHE_CONFIGURATION_MODE_UNKNOWN;
  L2CacheDataHob.SmbiosType7Cache.MaximumCacheSize = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.InstalledSize = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.SupportedSRAMType.Unknown = 1;
  L2CacheDataHob.SmbiosType7Cache.CurrentSRAMType.Unknown = 1;
  L2CacheDataHob.SmbiosType7Cache.CacheSpeed = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.ErrorCorrectionType = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.SystemCacheType = CacheTypeUnified;
  L2CacheDataHob.SmbiosType7Cache.Associativity = TO_BE_FILLED_BY_VENDOR;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosType7GuidHobGuid);
  L2CacheDataHobPtr = (RISC_V_PROCESSOR_TYPE7_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&L2CacheDataHob, sizeof (RISC_V_PROCESSOR_TYPE7_HOB_DATA));
  if (L2CacheDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive U54 MC Coreplex L2 cache RISC_V_PROCESSOR_TYPE7_HOB_DATA.\n"));
    ASSERT (FALSE);
  }

  //
  // Build up SMBIOS type 4 record.
  //
  ZeroMem((VOID *)&ProcessorDataHob, sizeof (RISC_V_PROCESSOR_TYPE4_HOB_DATA));
  ProcessorDataHob.PrcessorGuid = *((EFI_GUID *)PcdGetPtr (PcdSiFiveU54MCCoreplexGuid));
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
  ProcessorDataHob.SmbiosType4Processor.L2CacheHandle = TO_BE_FILLED_BY_RISC_V_SMBIOS_DXE_DRIVER;
  ProcessorDataHob.SmbiosType4Processor.L3CacheHandle = 0xffff;
  ProcessorDataHob.SmbiosType4Processor.SerialNumber = TO_BE_FILLED_BY_CODE;
  ProcessorDataHob.SmbiosType4Processor.AssetTag = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.PartNumber = TO_BE_FILLED_BY_VENDOR;
  ProcessorDataHob.SmbiosType4Processor.CoreCount = 5;
  ProcessorDataHob.SmbiosType4Processor.EnabledCoreCount = 5;
  ProcessorDataHob.SmbiosType4Processor.ThreadCount = 5;
  ProcessorDataHob.SmbiosType4Processor.ProcessorCharacteristics = (UINT16)(1 << 2); // 64-bit capable
  ProcessorDataHob.SmbiosType4Processor.ProcessorFamily2 = ProcessorFamilyRiscVRV64;
  ProcessorDataHob.SmbiosType4Processor.CoreCount2 = 0;
  ProcessorDataHob.SmbiosType4Processor.EnabledCoreCount2 = 0;
  ProcessorDataHob.SmbiosType4Processor.ThreadCount2 = 0;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosType4GuidHobGuid);
  ProcessorDataHobPtr = (RISC_V_PROCESSOR_TYPE4_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&ProcessorDataHob, sizeof (RISC_V_PROCESSOR_TYPE4_HOB_DATA));
  if (ProcessorDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive U54 MC Coreplex RISC_V_PROCESSOR_TYPE4_HOB_DATA.\n"));
    ASSERT (FALSE);
  }

  ZeroMem((VOID *)&SmbiosDataHob, sizeof (RISC_V_PROCESSOR_SMBIOS_HOB_DATA));
  SmbiosDataHob.Processor = ProcessorDataHobPtr;
  SmbiosDataHob.L1InstCache = NULL;
  SmbiosDataHob.L1DataCache = NULL;
  SmbiosDataHob.L2Cache = L2CacheDataHobPtr;
  SmbiosDataHob.L3Cache = NULL;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosGuidHobGuid);
  SmbiosDataHobPtr = (RISC_V_PROCESSOR_SMBIOS_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&SmbiosDataHob, sizeof (RISC_V_PROCESSOR_SMBIOS_HOB_DATA));
  if (SmbiosDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive U54MC Coreplex RISC_V_PROCESSOR_SMBIOS_HOB_DATA.\n"));
    ASSERT (FALSE);
  }
  *SmbiosHobPtr = SmbiosDataHobPtr;
  return EFI_SUCCESS;
}
