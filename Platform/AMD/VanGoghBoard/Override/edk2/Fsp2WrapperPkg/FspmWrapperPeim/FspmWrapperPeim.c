/** @file
  This will be invoked only once. It will call FspMemoryInit API,
  register TemporaryRamDonePpi to call TempRamExit API, and register MemoryDiscoveredPpi
  notify to call FspSiliconInit API.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Library/FspWrapperHobProcessLib.h>
#include <Library/FspWrapperMultiPhaseProcessLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/IoLib.h>
#include <Ppi/FspSiliconInitDone.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>
#include <Library/FspWrapperApiTestLib.h>
#include <FspEas.h>
#include <FspStatusCode.h>
#include <FspGlobalData.h>
#include <Library/FspCommonLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <FspmUpd.h>

extern EFI_GUID  gFspHobGuid;
extern EFI_GUID  gEfiAmdAgesaPkgTokenSpaceGuid;
extern EFI_GUID  gAmdCpmOemTablePpiGuid;

// The EDK 202208 Doesn't hold these structs.
typedef enum {
  EnumMultiPhaseGetVariableRequestInfo  = 0x2,
  EnumMultiPhaseCompleteVariableRequest = 0x3
} FSP_MULTI_PHASE_ACTION_23;

typedef enum {
  FspMultiPhaseMemInitApiIndex = 8
} FSP_API_INDEX_23;

/**
  Get the FSP M UPD Data address

  @return FSP-M UPD Data Address
**/
volatile
VOID
MakePcdNotBeingDeleted (
  VOID
  );

UINTN
GetFspmUpdDataAddress (
  VOID
  )
{
  if (PcdGet64 (PcdFspmUpdDataAddress64) != 0) {
    return (UINTN)PcdGet64 (PcdFspmUpdDataAddress64);
  } else {
    return (UINTN)PcdGet32 (PcdFspmUpdDataAddress);
  }
}

#define ACPI_MMIO_BASE         0xFED80000ul
#define SMI_BASE               0x200                     // DWORD
#define FCH_PMIOA_REG60        0x60                      // AcpiPm1EvtBlk
#define FCH_PMIOA_REG62        0x62                      // AcpiPm1CntBlk
#define FCH_PMIOA_REG64        0x64                      // AcpiPmTmrBlk
#define PMIO_BASE              0x300                     // DWORD
#define FCH_SMI_REGA0          0xA0
#define FCH_SMI_REGC4          0xC4
#define R_FCH_ACPI_PM_CONTROL  0x04

/**
 Clear all SMI enable bit in SmiControl0-SmiControl9 register

 @param [in]        None

 @retval            None
*/
VOID
ClearAllSmiControlRegisters (
  VOID
  )
{
  UINTN  SmiControlOffset;

  for (SmiControlOffset = FCH_SMI_REGA0; SmiControlOffset <= FCH_SMI_REGC4; SmiControlOffset += 4) {
    MmioWrite32 (ACPI_MMIO_BASE + SMI_BASE + SmiControlOffset, 0x00);
  }
}

/**
  Clear any SMI status or wake status left over from boot.

  @param  none

  @retval none
**/
VOID
EFIAPI
ClearSmiAndWake (
  VOID
  )
{
  UINT16  Pm1Status;
  UINT16  PmControl;
  UINT16  AcpiBaseAddr;

  AcpiBaseAddr = MmioRead16 (ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REG60);

  //
  // Read the ACPI registers
  //
  Pm1Status = IoRead16 (AcpiBaseAddr);
  PmControl = IoRead16 ((UINT16)(AcpiBaseAddr + R_FCH_ACPI_PM_CONTROL));

  //
  // Clear any SMI or wake state from the boot
  //
  Pm1Status |= 0xFF;          // clear all events
  PmControl &= 0xFFFE;        // clear Bit0(SciEn) in PmControl

  //
  // Write them back
  //
  IoWrite16 (AcpiBaseAddr, Pm1Status);
  IoWrite16 ((UINT16)(AcpiBaseAddr + R_FCH_ACPI_PM_CONTROL), PmControl);
}

/// AMD CPM OEM TABLE PPI Definition

typedef struct _AMD_CPM_OEM_TABLE_PPI {
  UINTN     Revision;                         ///< Revision Number
  UINT16    PlatformId;                       ///< Current Platform Id
  VOID      *TableList;                       ///< The Point of CPM Definition Table List
} AMD_CPM_OEM_TABLE_PPI;

// Report FSP-O PEI FV manually.
EFI_STATUS
EFIAPI
GetFspoPeiFv (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **FspoPeiFvHeader
  )
{
 #ifdef COMPRESS_FSP_REGION
  BuildMemoryAllocationHob (
    (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFspoPeiBaseAddressInMemory),
    PcdGet32 (PcdFspoPeiRegionSize),
    EfiACPIMemoryNVS
    );
  // Workaround for PSP FV sig check.
  CopyMem (
    (VOID *)PcdGet32 (FspoPeiWorkaroundShadowCopyAddress),
    (VOID *)PcdGet32 (PcdFspoPeiBaseAddressInMemory),
    PcdGet32 (PcdFspoPeiRegionSize)
    );
 #else
  CopyMem (
    (VOID *)PcdGet32 (FspoPeiWorkaroundShadowCopyAddress),
    (VOID *)PcdGet32 (PcdFspoPeiBaseAddressInFlash),
    PcdGet32 (PcdFspoPeiRegionSize)
    );
 #endif

  BuildMemoryAllocationHob (
    (EFI_PHYSICAL_ADDRESS)PcdGet32 (FspoPeiWorkaroundShadowCopyAddress),
    PcdGet32 (PcdFspoPeiRegionSize),
    EfiACPIMemoryNVS
    );
  *FspoPeiFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (FspoPeiWorkaroundShadowCopyAddress);

  return EFI_SUCCESS;
}

/**
  Call FspMemoryInit API.

  @return Status returned by FspMemoryInit API.
**/
EFI_STATUS
PeiFspMemoryInit (
  VOID
  )
{
  FSP_INFO_HEADER  *FspmHeaderPtr;
  EFI_STATUS       Status;
  UINT64           TimeStampCounterStart;
  VOID             *FspHobListPtr;
  VOID             *HobData;
  VOID             *FspmUpdDataPtr;
  UINTN            *SourceData;
  UINT32           FspmBaseAddress;

  DEBUG ((DEBUG_INFO, "PeiFspMemoryInit enter\n"));

  FspHobListPtr  = NULL;
  FspmUpdDataPtr = NULL;
  // Copied from PlatformInit.
  ClearSmiAndWake ();
  ClearAllSmiControlRegisters ();
  FspmBaseAddress = (UINT32)(UINTN)PcdGet32 (PcdFspmBaseAddressInMemory);
 #ifndef COMPRESS_FSP_REGION
  CopyMem ((VOID *)PcdGet32 (PcdFspmBaseAddressInMemory), (VOID *)PcdGet32 (PcdFspmBaseAddressInFlash), (UINT32)PcdGet32 (PcdFspmRegionSize));
 #endif
  FspmHeaderPtr = (FSP_INFO_HEADER *)FspFindFspHeader ((EFI_PHYSICAL_ADDRESS)(UINTN)FspmBaseAddress);
  DEBUG ((DEBUG_INFO, "Fspm Base Address - 0x%x\n", FspmBaseAddress));
  DEBUG ((DEBUG_INFO, "FspmHeaderPtr - 0x%x\n", FspmHeaderPtr));
  if (FspmHeaderPtr == NULL) {
    return EFI_DEVICE_ERROR;
  }

  BuildMemoryAllocationHob (
    (EFI_PHYSICAL_ADDRESS)FspmBaseAddress,
    (UINT32)PcdGet32 (PcdFspmRegionSize),
    EfiACPIMemoryNVS
    );
  FspmHeaderPtr->ImageBase = (UINTN)FspmBaseAddress;

  if ((GetFspmUpdDataAddress () == 0) && (FspmHeaderPtr->CfgRegionSize != 0) && (FspmHeaderPtr->CfgRegionOffset != 0)) {
    //
    // Copy default FSP-M UPD data from Flash
    //
    FspmUpdDataPtr = AllocateZeroPool ((UINTN)FspmHeaderPtr->CfgRegionSize);
    ASSERT (FspmUpdDataPtr != NULL);
    SourceData = (UINTN *)((UINTN)FspmHeaderPtr->ImageBase + (UINTN)FspmHeaderPtr->CfgRegionOffset);
    CopyMem (FspmUpdDataPtr, SourceData, (UINTN)FspmHeaderPtr->CfgRegionSize);
  } else {
    //
    // External UPD is ready, get the buffer from PCD pointer.
    //
    FspmUpdDataPtr = (VOID *)GetFspmUpdDataAddress ();
    ASSERT (FspmUpdDataPtr != NULL);
  }

  DEBUG ((DEBUG_INFO, "UpdateFspmUpdData enter\n"));
  UpdateFspmUpdData (FspmUpdDataPtr);
  if (((FSPM_UPD_COMMON *)FspmUpdDataPtr)->FspmArchUpd.Revision >= 3) {
    DEBUG ((DEBUG_INFO, "  StackBase           - 0x%lx\n", ((FSPM_UPD_COMMON_FSP24 *)FspmUpdDataPtr)->FspmArchUpd.StackBase));
    DEBUG ((DEBUG_INFO, "  StackSize           - 0x%lx\n", ((FSPM_UPD_COMMON_FSP24 *)FspmUpdDataPtr)->FspmArchUpd.StackSize));
    DEBUG ((DEBUG_INFO, "  BootLoaderTolumSize - 0x%x\n", ((FSPM_UPD_COMMON_FSP24 *)FspmUpdDataPtr)->FspmArchUpd.BootLoaderTolumSize));
    DEBUG ((DEBUG_INFO, "  BootMode            - 0x%x\n", ((FSPM_UPD_COMMON_FSP24 *)FspmUpdDataPtr)->FspmArchUpd.BootMode));
  } else {
    DEBUG ((DEBUG_INFO, "  NvsBufferPtr        - 0x%x\n", ((FSPM_UPD_COMMON *)FspmUpdDataPtr)->FspmArchUpd.NvsBufferPtr));
    DEBUG ((DEBUG_INFO, "  StackBase           - 0x%x\n", ((FSPM_UPD_COMMON *)FspmUpdDataPtr)->FspmArchUpd.StackBase));
    DEBUG ((DEBUG_INFO, "  StackSize           - 0x%x\n", ((FSPM_UPD_COMMON *)FspmUpdDataPtr)->FspmArchUpd.StackSize));
    DEBUG ((DEBUG_INFO, "  BootLoaderTolumSize - 0x%x\n", ((FSPM_UPD_COMMON *)FspmUpdDataPtr)->FspmArchUpd.BootLoaderTolumSize));
    DEBUG ((DEBUG_INFO, "  BootMode            - 0x%x\n", ((FSPM_UPD_COMMON *)FspmUpdDataPtr)->FspmArchUpd.BootMode));
  }

  DEBUG ((DEBUG_INFO, "  HobListPtr          - 0x%x\n", &FspHobListPtr));

  // Report FSP-O PEI manually.
  EFI_FIRMWARE_VOLUME_HEADER  *Header = NULL;
  if (GetFspoPeiFv (&Header) == EFI_SUCCESS) {
    ((FSPM_UPD *)FspmUpdDataPtr)->FspmConfig.fsp_o_pei_volume_address = (UINT32)(UINTN)Header;
    DEBUG ((DEBUG_INFO, "  FSP-O Fv 0x%p\n", Header));
  }

  TimeStampCounterStart = AsmReadTsc ();
  Status                = CallFspMemoryInit (FspmUpdDataPtr, &FspHobListPtr);

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FspMemoryInitApi requested reset %r\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  if ((Status != FSP_STATUS_VARIABLE_REQUEST) && EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - Failed to execute FspMemoryInitApi(), Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "FspMemoryInit status: %r\n", Status));
  if (Status == FSP_STATUS_VARIABLE_REQUEST) {
    //
    // call to Variable request handler
    //
    FspWrapperVariableRequestHandler (&FspHobListPtr, FspMultiPhaseMemInitApiIndex);
  }

  //
  // See if MultiPhase process is required or not
  //
  FspWrapperMultiPhaseHandler (&FspHobListPtr, FspMultiPhaseMemInitApiIndex);    // FspM MultiPhase

  //
  // Create hobs after memory initialization and not in temp RAM. Hence passing the recorded timestamp here
  //
  PERF_START_EX (&gFspApiPerformanceGuid, "EventRec", NULL, TimeStampCounterStart, FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  PERF_END_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  DEBUG ((DEBUG_INFO, "Total time spent executing FspMemoryInitApi: %d millisecond\n", DivU64x32 (GetTimeInNanoSecond (AsmReadTsc () - TimeStampCounterStart), 1000000)));

  Status = TestFspMemoryInitApiOutput (FspmUpdDataPtr, &FspHobListPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - TestFspMemoryInitApiOutput () fail, Status = %r\n", Status));
  }

  DEBUG ((DEBUG_INFO, "  FspHobListPtr (returned) - 0x%x\n", FspHobListPtr));
  ASSERT (FspHobListPtr != NULL);

  PostFspmHobProcess (FspHobListPtr);

  //
  // FspHobList is not complete at this moment.
  // Save FspHobList pointer to hob, so that it can be got later
  //
  HobData = BuildGuidHob (
              &gFspHobGuid,
              sizeof (VOID *)
              );
  ASSERT (HobData != NULL);
  CopyMem (HobData, &FspHobListPtr, sizeof (FspHobListPtr));
  return Status;
}

/**
  BuildUpdHob

  @return Status returned by FspMemoryInit API.
**/
VOID *
BuildUpdHob (
  VOID  *FspmBaseAddress
  )
{
  VOID             *FspmUpdDataPtr;
  FSP_INFO_HEADER  *FspmHeaderPtr;
  UINTN            *SourceData;

  FspmHeaderPtr = (FSP_INFO_HEADER *)FspFindFspHeader ((EFI_PHYSICAL_ADDRESS)(UINTN)FspmBaseAddress);
  DEBUG ((DEBUG_INFO, "Fspm Base Address - 0x%x\n", FspmBaseAddress));
  DEBUG ((DEBUG_INFO, "FspmHeaderPtr - 0x%x\n", FspmHeaderPtr));
  ASSERT (FspmHeaderPtr != NULL);

  FspmHeaderPtr->ImageBase = (UINTN)FspmBaseAddress;

  if ((GetFspmUpdDataAddress () == 0) && (FspmHeaderPtr->CfgRegionSize != 0) && (FspmHeaderPtr->CfgRegionOffset != 0)) {
    //
    // Copy default FSP-M UPD data from Flash
    //
    FspmUpdDataPtr = AllocateZeroPool ((UINTN)FspmHeaderPtr->CfgRegionSize);
    ASSERT (FspmUpdDataPtr != NULL);
    SourceData = (UINTN *)((UINTN)FspmHeaderPtr->ImageBase + (UINTN)FspmHeaderPtr->CfgRegionOffset);
    CopyMem (FspmUpdDataPtr, SourceData, (UINTN)FspmHeaderPtr->CfgRegionSize);
  } else {
    //
    // External UPD is ready, get the buffer from PCD pointer.
    //
    FspmUpdDataPtr = (VOID *)GetFspmUpdDataAddress ();
    ASSERT (FspmUpdDataPtr != NULL);
  }

  return BuildGuidDataHob (&gAmdFspUpdGuid, &FspmUpdDataPtr, sizeof (VOID *));
}

/**
  Do FSP initialization.

  @return FSP initialization status.
**/
EFI_STATUS
EFIAPI
FspmWrapperInit (
  VOID
  )
{
  EFI_STATUS                                             Status;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI  *MeasurementExcludedFvPpi;
  EFI_PEI_PPI_DESCRIPTOR                                 *MeasurementExcludedPpiList;

  MeasurementExcludedFvPpi = AllocatePool (sizeof (*MeasurementExcludedFvPpi));
  ASSERT (MeasurementExcludedFvPpi != NULL);
  MeasurementExcludedFvPpi->Count          = 1;
  MeasurementExcludedFvPpi->Fv[0].FvBase   = PcdGet32 (PcdFspmBaseAddressInMemory);
  MeasurementExcludedFvPpi->Fv[0].FvLength = (UINT32)PcdGet32 (PcdFspmRegionSize);

  MeasurementExcludedPpiList = AllocatePool (sizeof (*MeasurementExcludedPpiList));
  ASSERT (MeasurementExcludedPpiList != NULL);
  MeasurementExcludedPpiList->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  MeasurementExcludedPpiList->Guid  = &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid;
  MeasurementExcludedPpiList->Ppi   = MeasurementExcludedFvPpi;

  Status = EFI_SUCCESS;

  if (PcdGet8 (PcdFspModeSelection) == 1) {
    Status = PeiFspMemoryInit ();
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = PeiServicesInstallPpi (MeasurementExcludedPpiList);
    ASSERT_EFI_ERROR (Status);
    VOID  *FspmBaseAddress = (VOID *)(UINTN)PcdGet32 (PcdFspmBaseAddressInMemory);
 #ifndef COMPRESS_FSP_REGION
    CopyMem (FspmBaseAddress, (VOID *)PcdGet32 (PcdFspmBaseAddressInFlash), (UINT32)PcdGet32 (PcdFspmRegionSize));
 #endif
    // Build a Upd address pointer guid hob for FSP.
    VOID  **upd_guid_hob = BuildUpdHob (FspmBaseAddress);
    DEBUG ((DEBUG_INFO, "upd_guid_hob: 0x%x\n", *upd_guid_hob));
    ASSERT (upd_guid_hob != NULL);
    // Update UPD variables according to OEM requirement
    // Sample code
    //  FSPM_UPD * volatile fsp_m_upd = *upd_guid_hob;
    //  FSP_M_CONFIG * volatile fsp_m_cfg = &fsp_m_upd->FspmConfig;
    //  fsp_m_cfg->DbgFchUsbUsb0DrdMode = xx;

    BuildMemoryAllocationHob (
      (UINTN)FspmBaseAddress,
      PcdGet32 (PcdFspmRegionSize),
      EfiACPIMemoryNVS
      );
    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *)(UINTN)FspmBaseAddress,
      PcdGet32 (PcdFspmRegionSize),
      NULL,
      NULL
      );
    BuildFvHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)FspmBaseAddress,
      PcdGet32 (PcdFspmRegionSize)
      );

    EFI_FIRMWARE_VOLUME_HEADER  *FspoBaseAddress = NULL;
    Status = GetFspoPeiFv (&FspoBaseAddress);
    PeiServicesInstallFvInfoPpi (
      NULL,
      FspoBaseAddress,
      PcdGet32 (PcdFspoPeiRegionSize),
      NULL,
      NULL
      );
    BuildFvHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)FspoBaseAddress,
      PcdGet32 (PcdFspoPeiRegionSize)
      );
  }

  return Status;
}

/**
  This is the entrypoint of PEIM

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
FspmWrapperPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  DEBUG ((DEBUG_INFO, "FspmWrapperPeimEntryPoint\n"));

  FspmWrapperInit ();
  return EFI_SUCCESS;
}
