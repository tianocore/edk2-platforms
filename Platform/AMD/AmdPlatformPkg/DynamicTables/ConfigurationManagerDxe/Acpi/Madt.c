/** @file
  Collects the information required to update the MADT table.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/MpService.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SortLib.h>
#include <Library/LocalApicLib.h>
#include <Library/AmdPlatformSocLib.h>
#include "ConfigurationManager.h"

UINT32  mMadtCcdOrder[16] = { 0, 4, 8, 12, 2, 6, 10, 14, 3, 7, 11, 15, 1, 5, 9, 13 };

/**
  Callback compare function.
  Compares CCD number of provided arguments.

  @param[in] ProcessorInfoLeft   Pointer to Left Buffer.
  @param[in] ProcessorInfoRight  Pointer to Right Buffer.
  @return    0                 If both are same
             -1                If left value is less than righ value.
             1                 If left value is greater than righ value.

**/
INTN
EFIAPI
MadtSortByCcd (
  CONST VOID  *ProcessorInfoLeft,
  CONST VOID  *ProcessorInfoRight
  )
{
  CONST EFI_PROCESSOR_INFORMATION  *Left;
  CONST EFI_PROCESSOR_INFORMATION  *Right;
  UINT32                           Index;
  UINT32                           LeftCcdIndex;
  UINT32                           RightCcdIndex;

  Left  = (EFI_PROCESSOR_INFORMATION *)ProcessorInfoLeft;
  Right = (EFI_PROCESSOR_INFORMATION *)ProcessorInfoRight;

  // Get the CCD Index number
  LeftCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mMadtCcdOrder); Index++) {
    if (Left->ExtendedInformation.Location2.Die == mMadtCcdOrder[Index]) {
      LeftCcdIndex = Index;
      break;
    }
  }

  RightCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mMadtCcdOrder); Index++) {
    if (Right->ExtendedInformation.Location2.Die == mMadtCcdOrder[Index]) {
      RightCcdIndex = Index;
      break;
    }
  }

  // Now compare for quick sort
  if (LeftCcdIndex < RightCcdIndex) {
    return -1;
  }

  if (LeftCcdIndex > RightCcdIndex) {
    return 1;
  }

  return 0;
}

/** The UpdateMadtTable function updates the MADT table.

    @param [in, out]  PlatformRepo  Pointer to the platform repository information.

    @retval EFI_SUCCESS            The MADT table is updated successfully.
    @retval EFI_INVALID_PARAMETER  The input parameter is invalid.
**/
EFI_STATUS
EFIAPI
UpdateMadtTable (
  IN OUT EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  )
{
  EFI_STATUS                         Status;
  EFI_MP_SERVICES_PROTOCOL           *MpService;
  UINTN                              NumberOfProcessors;
  UINTN                              NumberOfEnabledProcessors;
  UINT32                             Index;
  EFI_PROCESSOR_INFORMATION          *ProcessorInfoBuffer;
  UINTN                              NumSocket;
  UINTN                              ThreadsPerCore;
  UINTN                              Socket;
  CM_X64_LOCAL_APIC_X2APIC_INFO      *LocalApicInfo;
  CM_X64_LOCAL_APIC_X2APIC_INFO      *TempLocalApicInfo;
  CM_X64_LOCAL_APIC_X2APIC_INFO      *SrcLocalApicInfo;
  CM_X64_LOCAL_APIC_X2APIC_INFO      *DstLocalApicInfo;
  UINT8                              ApicMode;
  CM_X64_IO_APIC_INFO                *CmIoApicInfo;
  EFI_ACPI_6_5_IO_APIC_STRUCTURE     *IoApicInfo;
  UINTN                              IoApicCount;
  CM_X64_INTR_SOURCE_OVERRIDE_INFO   *IntrSourceOverrideInfo;
  UINTN                              IntrSourceOverrideCount;
  CM_X64_LOCAL_APIC_X2APIC_NMI_INFO  *LocalApicNmiInfo;
  UINTN                              LocalApicNmiCount;

  if (PlatformRepo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ApicMode                                = (UINT8)GetApicMode ();
  PlatformRepo->MadtInfo.LocalApicAddress = PcdGet32 (PcdCpuLocalApicBaseAddress);
  PlatformRepo->MadtInfo.Flags            = EFI_ACPI_6_5_PCAT_COMPAT;
  PlatformRepo->MadtInfo.ApicMode         = ApicMode;

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpService);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to locate MP Services Protocol. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = MpService->GetNumberOfProcessors (
                        MpService,
                        &NumberOfProcessors,
                        &NumberOfEnabledProcessors
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to get number of processors. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ProcessorInfoBuffer = AllocateZeroPool (NumberOfProcessors * sizeof (EFI_PROCESSOR_INFORMATION));
  if (ProcessorInfoBuffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to allocate memory for Processor Information Buffer. Status(%r)\n",
      __func__,
      __LINE__,
      Status
      ));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  /// Get ProcessorInfoBuffer
  NumSocket      = 0;
  ThreadsPerCore = 0;
  for (Index = 0; Index < NumberOfProcessors; Index++) {
    Status = MpService->GetProcessorInfo (
                          MpService,
                          Index|CPU_V2_EXTENDED_TOPOLOGY,
                          &ProcessorInfoBuffer[Index]
                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%d Failed to get Processor Information. Status(%r)\n",
        __func__,
        __LINE__,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      FreePool (ProcessorInfoBuffer);
      return Status;
    }

    if (ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Package > NumSocket) {
      NumSocket = ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Package;
    }

    if (ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Thread > ThreadsPerCore) {
      ThreadsPerCore = ProcessorInfoBuffer[Index].ExtendedInformation.Location2.Thread;
    }
  }

  /// Increment the NumSocket and ThreadsPerCore by 1, as it is 0 based
  NumSocket++;
  ThreadsPerCore++;

  /// Sort by CCD location
  if (NumSocket > 1) {
    PerformQuickSort (
      ProcessorInfoBuffer,
      NumberOfProcessors/2,
      sizeof (EFI_PROCESSOR_INFORMATION),
      MadtSortByCcd
      );
    PerformQuickSort (
      ProcessorInfoBuffer+(NumberOfProcessors/2),
      NumberOfProcessors/2,
      sizeof (EFI_PROCESSOR_INFORMATION),
      MadtSortByCcd
      );
  } else {
    PerformQuickSort (
      ProcessorInfoBuffer,
      NumberOfProcessors,
      sizeof (EFI_PROCESSOR_INFORMATION),
      MadtSortByCcd
      );
  }

  LocalApicInfo = AllocateZeroPool (sizeof (CM_X64_LOCAL_APIC_X2APIC_INFO) * NumberOfProcessors);
  if (LocalApicInfo == NULL) {
    FreePool (ProcessorInfoBuffer);
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Socket = 0; Socket < NumSocket; Socket++) {
    for (Index = 0; Index < NumberOfProcessors; Index++) {
      if (ProcessorInfoBuffer[Index].ProcessorId > MAX_UINT32) {
        DEBUG ((DEBUG_ERROR, "%a:%d ProcessorId is greater than MAX_UINT32\n", __func__, __LINE__));
        FreePool (ProcessorInfoBuffer);
        ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
        return EFI_INVALID_PARAMETER;
      }

      LocalApicInfo[Index].ApicId = (UINT32)ProcessorInfoBuffer[Index].ProcessorId;
      if ((ProcessorInfoBuffer[Index].StatusFlag & PROCESSOR_ENABLED_BIT) != 0) {
        LocalApicInfo[Index].Flags = 1;
      }

      LocalApicInfo[Index].AcpiProcessorUid = Index;
    }
  }

  FreePool (ProcessorInfoBuffer);

  /// Now separate the first thread list
  TempLocalApicInfo = NULL;
  if (ThreadsPerCore > 1) {
    TempLocalApicInfo = AllocateCopyPool ((sizeof (CM_X64_LOCAL_APIC_X2APIC_INFO) * NumberOfProcessors), (const void *)LocalApicInfo);
    if (TempLocalApicInfo == NULL) {
      FreePool (LocalApicInfo);
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    SrcLocalApicInfo = TempLocalApicInfo;
    DstLocalApicInfo = LocalApicInfo;
    for (Index = 0; Index < NumberOfProcessors; Index++) {
      if ((SrcLocalApicInfo->ApicId & 0x1) == 0) {
        CopyMem (DstLocalApicInfo, SrcLocalApicInfo, sizeof (CM_X64_LOCAL_APIC_X2APIC_INFO));
        SrcLocalApicInfo++;
        DstLocalApicInfo++;
      } else {
        SrcLocalApicInfo++;
      }
    }

    SrcLocalApicInfo = TempLocalApicInfo;
    for (Index = 0; Index < NumberOfProcessors; Index++) {
      if ((SrcLocalApicInfo->ApicId & 0x1) == 1) {
        CopyMem (DstLocalApicInfo, SrcLocalApicInfo, sizeof (CM_X64_LOCAL_APIC_X2APIC_INFO));
        SrcLocalApicInfo++;
        DstLocalApicInfo++;
      } else {
        SrcLocalApicInfo++;
      }
    }
  }

  if (TempLocalApicInfo != NULL) {
    FreePool (TempLocalApicInfo);
  }

  /// Now copy the LocalApicInfo to PlatformRepo
  PlatformRepo->LocalApicX2ApicInfoCount = NumberOfProcessors;
  PlatformRepo->LocalApicX2ApicInfo      = LocalApicInfo;

  /// Get the IO APIC Information

  IoApicInfo  = NULL;
  IoApicCount = 0;
  Status      = GetIoApicInfo (&IoApicInfo, (UINT8 *)&IoApicCount);
  if ((EFI_ERROR (Status) || (IoApicInfo == NULL) || (IoApicCount == 0))) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d GetIoApicInfo() failed. Status (%r).\n",
      __func__,
      __LINE__,
      Status
      ));
    return Status;
  }

  CmIoApicInfo = NULL;
  CmIoApicInfo = AllocateZeroPool (sizeof (CM_X64_IO_APIC_INFO) * IoApicCount);
  if (CmIoApicInfo == NULL) {
    FreePool (IoApicInfo);
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  /// Copy the IO APIC Information
  for (Index = 0; Index < IoApicCount; Index++) {
    CmIoApicInfo[Index].IoApicId                  = IoApicInfo[Index].IoApicId;
    CmIoApicInfo[Index].IoApicAddress             = IoApicInfo[Index].IoApicAddress;
    CmIoApicInfo[Index].GlobalSystemInterruptBase = IoApicInfo[Index].GlobalSystemInterruptBase;
  }

  FreePool (IoApicInfo);

  PlatformRepo->IoApicInfoCount = IoApicCount;
  PlatformRepo->IoApicInfo      = CmIoApicInfo;

  /// Get the Interrupt Source Override Information
  IntrSourceOverrideCount = 2;
  IntrSourceOverrideInfo  = AllocateZeroPool (sizeof (CM_X64_INTR_SOURCE_OVERRIDE_INFO) * IntrSourceOverrideCount);
  if (IntrSourceOverrideInfo == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  IntrSourceOverrideInfo[0].Bus                   = 0x0;
  IntrSourceOverrideInfo[0].Source                = 0x0;
  IntrSourceOverrideInfo[0].GlobalSystemInterrupt = 0x2;
  IntrSourceOverrideInfo[0].Flags                 = 0xF;

  IntrSourceOverrideInfo[1].Bus                   = 0x0;
  IntrSourceOverrideInfo[1].Source                = 0x9;
  IntrSourceOverrideInfo[1].GlobalSystemInterrupt = 0x9;
  IntrSourceOverrideInfo[1].Flags                 = 0xF;

  PlatformRepo->IntrSourceOverrideInfoCount = IntrSourceOverrideCount;
  PlatformRepo->IntrSourceOverrideInfo      = IntrSourceOverrideInfo;

  /// Get the Local APIC NMI Information
  LocalApicNmiCount = 1;
  LocalApicNmiInfo  = AllocateZeroPool (sizeof (CM_X64_LOCAL_APIC_X2APIC_NMI_INFO) * LocalApicNmiCount);
  if (LocalApicNmiInfo == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  LocalApicNmiInfo[0].Flags = 0x5;
  if (ApicMode == LOCAL_APIC_MODE_X2APIC) {
    LocalApicNmiInfo[0].AcpiProcessorUid = MAX_UINT32;
  } else {
    LocalApicNmiInfo[0].AcpiProcessorUid = MAX_UINT8;
  }

  LocalApicNmiInfo[0].LocalApicLint = 0x1;

  /// update the PlatformRepo
  PlatformRepo->LocalApicX2ApicNmiInfoCount = LocalApicNmiCount;
  PlatformRepo->LocalApicX2ApicNmiInfo      = LocalApicNmiInfo;

  return EFI_SUCCESS;
}
