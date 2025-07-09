/** @file
  Multi-processor Info 2 HOB PEIM.

  The purpose of this PEIM is to provide backwards compatibility between FSP
  binaries built with older versions of EDK II and the latest EDK II.

  Newer versions of CpuMpPei produce the gMpInformation2HobGuid. This HOB is
  required by newer implementations of the CPU DXE driver, however older
  versions of CpuMpPei do not produce it. This PEIM will check if CpuMpPei
  creates gMpInformation2HobGuid and if it does not it creates it.

Copyright (c) 2024 - 2025, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/MpServices2.h>
#include <Guid/MpInformation2.h>
#include <Register/Cpuid.h>

typedef struct {
  EDKII_PEI_MP_SERVICES2_PPI    *CpuMpPpi2;
  UINT8                         *CoreTypes;
} GET_PROCESSOR_CORE_TYPE_BUFFER;

/**
  Get CPU core type.

  @param[in, out] Buffer  Argument of the procedure.
**/
VOID
EFIAPI
GetProcessorCoreType (
  IN OUT VOID  *Buffer
  )
{
  EFI_STATUS                               Status;
  UINT8                                    *CoreTypes;
  CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX  NativeModelIdAndCoreTypeEax;
  UINTN                                    ProcessorIndex;
  GET_PROCESSOR_CORE_TYPE_BUFFER           *Params;

  Params = (GET_PROCESSOR_CORE_TYPE_BUFFER *)Buffer;
  Status = Params->CpuMpPpi2->WhoAmI (Params->CpuMpPpi2, &ProcessorIndex);
  ASSERT_EFI_ERROR (Status);

  CoreTypes = Params->CoreTypes;
  AsmCpuidEx (CPUID_HYBRID_INFORMATION, CPUID_HYBRID_INFORMATION_MAIN_LEAF, &NativeModelIdAndCoreTypeEax.Uint32, NULL, NULL, NULL);
  CoreTypes[ProcessorIndex] = (UINT8)NativeModelIdAndCoreTypeEax.Bits.CoreType;
}

/**
  Create gMpInformation2HobGuid.
**/
VOID
BuildMpInformationHob (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *CpuMpPpi2
  )
{
  GET_PROCESSOR_CORE_TYPE_BUFFER  Buffer;
  EFI_STATUS                      Status;
  UINTN                           ProcessorIndex;
  UINTN                           NumberOfProcessors;
  UINTN                           NumberOfEnabledProcessors;
  UINTN                           NumberOfProcessorsInHob;
  UINTN                           MaxProcessorsPerHob;
  MP_INFORMATION2_HOB_DATA        *MpInformation2HobData;
  MP_INFORMATION2_ENTRY           *MpInformation2Entry;
  UINTN                           Index;
  UINT8                           *CoreTypes;
  UINT32                          CpuidMaxInput;
  UINTN                           CoreTypePages;

  ProcessorIndex        = 0;
  MpInformation2HobData = NULL;
  MpInformation2Entry   = NULL;
  CoreTypes             = NULL;
  CoreTypePages         = 0;

  Status = CpuMpPpi2->GetNumberOfProcessors (
                        CpuMpPpi2,
                        &NumberOfProcessors,
                        &NumberOfEnabledProcessors
                        );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get Processors CoreType
  //
  AsmCpuid (CPUID_SIGNATURE, &CpuidMaxInput, NULL, NULL, NULL);
  if (CpuidMaxInput >= CPUID_HYBRID_INFORMATION) {
    CoreTypePages = EFI_SIZE_TO_PAGES (sizeof (UINT8) * NumberOfProcessors);
    CoreTypes     = AllocatePages (CoreTypePages);
    ASSERT (CoreTypes != NULL);
    if (CoreTypes == NULL) {
      goto Done;
    }

    Buffer.CoreTypes = CoreTypes;
    Buffer.CpuMpPpi2 = CpuMpPpi2;
    Status           = CpuMpPpi2->StartupAllCPUs (
                                    CpuMpPpi2,
                                    GetProcessorCoreType,
                                    0,
                                    (VOID *)&Buffer
                                    );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  MaxProcessorsPerHob     = ((MAX_UINT16 & ~7) - sizeof (EFI_HOB_GUID_TYPE) - sizeof (MP_INFORMATION2_HOB_DATA)) / sizeof (MP_INFORMATION2_ENTRY);
  NumberOfProcessorsInHob = MaxProcessorsPerHob;

  //
  // Create MP_INFORMATION2_HOB. when the max HobLength 0xFFF8 is not enough, there
  // will be a MP_INFORMATION2_HOB series in the HOB list.
  // In the HOB list, there is a gMpInformation2HobGuid with 0 value NumberOfProcessors
  // fields to indicate it's the last MP_INFORMATION2_HOB.
  //
  while (NumberOfProcessorsInHob != 0) {
    NumberOfProcessorsInHob = MIN (NumberOfProcessors - ProcessorIndex, MaxProcessorsPerHob);
    MpInformation2HobData   = BuildGuidHob (
                                &gMpInformation2HobGuid,
                                sizeof (MP_INFORMATION2_HOB_DATA) + sizeof (MP_INFORMATION2_ENTRY) * NumberOfProcessorsInHob
                                );
    ASSERT (MpInformation2HobData != NULL);
    if (MpInformation2HobData == NULL) {
      goto Done;
    }

    MpInformation2HobData->Version            = MP_INFORMATION2_HOB_REVISION;
    MpInformation2HobData->ProcessorIndex     = ProcessorIndex;
    MpInformation2HobData->NumberOfProcessors = (UINT16)NumberOfProcessorsInHob;
    MpInformation2HobData->EntrySize          = sizeof (MP_INFORMATION2_ENTRY);

    DEBUG ((DEBUG_INFO, "Creating MpInformation2 HOB...\n"));

    for (Index = 0; Index < NumberOfProcessorsInHob; Index++) {
      MpInformation2Entry = &MpInformation2HobData->Entry[Index];
      Status              = CpuMpPpi2->GetProcessorInfo (
                                         CpuMpPpi2,
                                         (Index + ProcessorIndex) | CPU_V2_EXTENDED_TOPOLOGY,
                                         &MpInformation2Entry->ProcessorInfo
                                         );
      ASSERT_EFI_ERROR (Status);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      MpInformation2Entry->CoreType = (CoreTypes != NULL) ? CoreTypes[Index + ProcessorIndex] : 0;

      DEBUG ((
        DEBUG_INFO,
        "  Processor[%04d]: ProcessorId = 0x%lx, StatusFlag = 0x%x, CoreType = 0x%x\n",
        Index + ProcessorIndex,
        MpInformation2Entry->ProcessorInfo.ProcessorId,
        MpInformation2Entry->ProcessorInfo.StatusFlag,
        MpInformation2Entry->CoreType
        ));
      DEBUG ((
        DEBUG_INFO,
        "    Location = Package:%d Core:%d Thread:%d\n",
        MpInformation2Entry->ProcessorInfo.Location.Package,
        MpInformation2Entry->ProcessorInfo.Location.Core,
        MpInformation2Entry->ProcessorInfo.Location.Thread
        ));
      DEBUG ((
        DEBUG_INFO,
        "    Location2 = Package:%d Die:%d Tile:%d Module:%d Core:%d Thread:%d\n",
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Package,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Die,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Tile,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Module,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Core,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Thread
        ));
    }

    ProcessorIndex += NumberOfProcessorsInHob;
  }

Done:
  if (CoreTypes != NULL) {
    FreePages (CoreTypes, CoreTypePages);
  }
}

/**
  Check if CpuMpPei creates gMpInformation2HobGuid and if it does not it
  creates it.

  @param[in] ImageHandle    Handle for the image of this driver
  @param[in] SystemTable    Pointer to the EFI System Table

  @retval    EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
MpInfo2HobPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_PPI_DESCRIPTOR      *TempPpiDescriptor;
  EDKII_PEI_MP_SERVICES2_PPI  *CpuMpPpi2;
  EFI_HOB_GUID_TYPE           *GuidHob;

  Status = EFI_SUCCESS;
  GuidHob = GetFirstGuidHob (&gMpInformation2HobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_INFO, "gMpInformation2HobGuid was not created by CpuMpPei, creating now\n"));

    Status = PeiServicesLocatePpi (
              &gEdkiiPeiMpServices2PpiGuid,
              0,
              &TempPpiDescriptor,
              (VOID **)&CpuMpPpi2
              );
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      BuildMpInformationHob (CpuMpPpi2);
    }
  }

  return Status;
}
