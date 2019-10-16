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
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>
#include <SiFiveU5MCCoreplex.h>
#include <Library/SiFiveE51.h>
#include <Library/SiFiveU54.h>

/**
  Build up processor-specific HOB for U5MC Coreplex

  @param  UniqueId      Unique ID of this U5MC Coreplex processor

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
CreateU5MCCoreplexProcessorSpecificDataHob (
  IN UINTN UniqueId
  )
{
  EFI_STATUS Status;
  UINT32 HartIdNumber;
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA *GuidHobData;
  EFI_GUID *ParentCoreGuid;
  BOOLEAN MCSupport;

  DEBUG ((DEBUG_INFO, "Building U5 Coreplex processor information HOB\n"));

  HartIdNumber = 0;
  ParentCoreGuid = PcdGetPtr(PcdSiFiveU5MCCoreplexGuid);
  MCSupport = PcdGetBool (PcdE5MCSupported);
  if (MCSupport == TRUE) {
    Status = CreateE51CoreProcessorSpecificDataHob (ParentCoreGuid, UniqueId, HartIdNumber, FALSE, &GuidHobData);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Faile to build U5MC processor informatino HOB\n"));
      ASSERT (FALSE);
    }
    HartIdNumber ++;
    DEBUG ((DEBUG_INFO, "Support E5 Monitor core on U5 platform, HOB at address 0x%x\n", GuidHobData));
  }
  for (; HartIdNumber < (FixedPcdGet32 (PcdNumberofU5Cores) + (UINT32)MCSupport); HartIdNumber ++) {
    Status = CreateU54CoreProcessorSpecificDataHob (ParentCoreGuid, UniqueId, HartIdNumber, (HartIdNumber == FixedPcdGet32 (PcdBootHartId))? TRUE: FALSE, &GuidHobData);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Faile to build U5MC processor informatino HOB\n"));
      ASSERT (FALSE);
    }
    DEBUG ((DEBUG_INFO, "Support U5 application core on U5 platform, HOB Data at address 0x%x\n", GuidHobData));
  }
  DEBUG ((DEBUG_INFO, "Support %d U5 application cores on U5 platform\n", HartIdNumber - (UINT32)MCSupport));

  if (HartIdNumber != FixedPcdGet32 (PcdHartCount)) {
    DEBUG ((DEBUG_ERROR, "Improper core settings...\n"));
    DEBUG ((DEBUG_ERROR, "    PcdHartCount\n"));
    DEBUG ((DEBUG_ERROR, "    PcdNumberofU5Cores\n"));
    DEBUG ((DEBUG_ERROR, "    PcdE5MCSupported\n\n"));
    ASSERT (FALSE);
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
CreateU5MCProcessorSmbiosDataHob (
  IN UINTN     ProcessorUid,
  OUT RISC_V_PROCESSOR_SMBIOS_HOB_DATA **SmbiosHobPtr
  )
{
  EFI_GUID *GuidPtr;
  RISC_V_PROCESSOR_TYPE4_HOB_DATA ProcessorDataHob;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA L2CacheDataHob;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA SmbiosDataHob;
  RISC_V_PROCESSOR_TYPE4_HOB_DATA *ProcessorDataHobPtr;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA *L2CacheDataHobPtr;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA *SmbiosDataHobPtr;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  if (SmbiosHobPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Build up SMBIOS type 7 L2 cache record.
  //
  ZeroMem((VOID *)&L2CacheDataHob, sizeof (RISC_V_PROCESSOR_TYPE7_HOB_DATA));
  L2CacheDataHob.PrcessorGuid = *((EFI_GUID *)PcdGetPtr (PcdSiFiveU5MCCoreplexGuid));
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
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive U5 MC Coreplex L2 cache RISC_V_PROCESSOR_TYPE7_HOB_DATA.\n"));
    ASSERT (FALSE);
  }

  //
  // Build up SMBIOS type 4 record.
  //
  ZeroMem((VOID *)&ProcessorDataHob, sizeof (RISC_V_PROCESSOR_TYPE4_HOB_DATA));
  ProcessorDataHob.PrcessorGuid = *((EFI_GUID *)PcdGetPtr (PcdSiFiveU5MCCoreplexGuid));
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
  ProcessorDataHob.SmbiosType4Processor.CoreCount = (UINT8)FixedPcdGet32 (PcdNumberofU5Cores) + (UINT8)PcdGetBool (PcdE5MCSupported);
  ProcessorDataHob.SmbiosType4Processor.EnabledCoreCount = (UINT8)FixedPcdGet32 (PcdNumberofU5Cores) + (UINT8)PcdGetBool (PcdE5MCSupported);
  ProcessorDataHob.SmbiosType4Processor.ThreadCount = (UINT8)FixedPcdGet32 (PcdNumberofU5Cores) + (UINT8)PcdGetBool (PcdE5MCSupported);
  ProcessorDataHob.SmbiosType4Processor.ProcessorCharacteristics = (UINT16)(1 << 2); // 64-bit capable
  ProcessorDataHob.SmbiosType4Processor.ProcessorFamily2 = ProcessorFamilyRiscVRV64;
  ProcessorDataHob.SmbiosType4Processor.CoreCount2 = 0;
  ProcessorDataHob.SmbiosType4Processor.EnabledCoreCount2 = 0;
  ProcessorDataHob.SmbiosType4Processor.ThreadCount2 = 0;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosType4GuidHobGuid);
  ProcessorDataHobPtr = (RISC_V_PROCESSOR_TYPE4_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&ProcessorDataHob, sizeof (RISC_V_PROCESSOR_TYPE4_HOB_DATA));
  if (ProcessorDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive U5MC Coreplex RISC_V_PROCESSOR_TYPE4_HOB_DATA.\n"));
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
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of SiFive U5MC Coreplex RISC_V_PROCESSOR_SMBIOS_HOB_DATA.\n"));
    ASSERT (FALSE);
  }
  *SmbiosHobPtr = SmbiosDataHobPtr;
  DEBUG ((DEBUG_INFO, "%a: Exit\n", __FUNCTION__));
  return EFI_SUCCESS;
}
