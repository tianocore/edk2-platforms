/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "AcpiCommon.h"

#include <Library/AmlLib/AmlLib.h>
#include <Library/SortLib.h>
#include <Protocol/MpService.h>
#include <Register/Intel/Cpuid.h> // for CPUID_EXTENDED_TOPOLOGY

#define AMD_CPUID_EXTENDED_TOPOLOGY_V2                 0x26
#define AMD_CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_CCD  0x04
#define AMD_CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_CCX  0x03
#define AMD_CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE  0x05
#define CREATOR_REVISION                               2
#define DEVICE_BATTERY_BIT                             0x0010// Control Method Battery Device Only
#define DEVICE_ENABLED_BIT                             0x0002
#define DEVICE_HEALTH_BIT                              0x0008
#define DEVICE_IN_UI_BIT                               0x0004
#define DEVICE_PRESENT_BIT                             0x0001
#define MAX_TEST_CPU_STRING_SIZE                       20
#define OEM_REVISION_NUMBER                            0

EFI_PROCESSOR_INFORMATION  *mApicIdtoUidMap     = NULL;
UINT32                     mCcdOrder[16]        = { 0, 4, 8, 12, 2, 6, 10, 14, 3, 7, 11, 15, 1, 5, 9, 13 };
UINTN                      mNumberOfCpus        = 0;
UINTN                      mNumberOfEnabledCPUs = 0;

/**
  Callback compare function.
  Compares CCD number of provided arguments.

  @param[in] LocalX2ApicLeft   Pointer to Left Buffer.
  @param[in] LocalX2ApicRight  Pointer to Right Buffer.
  @return    0                 If both are same
             -1                If left value is less than righ value.
             1                 If left value is greater than righ value.

**/
INTN
EFIAPI
SortByCcd (
  IN  CONST VOID  *LocalX2ApicLeft,
  IN  CONST VOID  *LocalX2ApicRight
  )
{
  EFI_PROCESSOR_INFORMATION  *Left;
  EFI_PROCESSOR_INFORMATION  *Right;
  UINT32                     Index;
  UINT32                     LeftCcdIndex;
  UINT32                     RightCcdIndex;

  Left  = (EFI_PROCESSOR_INFORMATION *)LocalX2ApicLeft;
  Right = (EFI_PROCESSOR_INFORMATION *)LocalX2ApicRight;

  // Get the CCD Index number
  LeftCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mCcdOrder); Index++) {
    if (Left->ExtendedInformation.Location2.Die == mCcdOrder[Index]) {
      LeftCcdIndex = Index;
      break;
    }
  }

  RightCcdIndex = MAX_UINT32;
  for (Index = 0; Index < ARRAY_SIZE (mCcdOrder); Index++) {
    if (Right->ExtendedInformation.Location2.Die == mCcdOrder[Index]) {
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

EFI_STATUS
GenerateApicIdtoUidMap (
  VOID
  )
{
  EFI_MP_SERVICES_PROTOCOL  *MpService;
  EFI_STATUS                Status;
  UINTN                     Index;

  // Get MP service
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpService);
  if (EFI_ERROR (Status) || (MpService == NULL)) {
    return EFI_NOT_FOUND;
  }

  // Load MpServices
  Status = MpService->GetNumberOfProcessors (MpService, &mNumberOfCpus, &mNumberOfEnabledCPUs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_ERROR, "%a: NumberOfCpus = %d mNumberOfEnabledCPUs = %d\n", __func__, mNumberOfCpus, mNumberOfEnabledCPUs));

  mApicIdtoUidMap = AllocateZeroPool (mNumberOfCpus * sizeof (EFI_PROCESSOR_INFORMATION));
  if (mApicIdtoUidMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < mNumberOfCpus; Index++) {
    Status = MpService->GetProcessorInfo (
                          MpService,
                          Index | CPU_V2_EXTENDED_TOPOLOGY,
                          &mApicIdtoUidMap[Index]
                          );
  }

  if (FixedPcdGet32 (PcdMaxCpuSocketCount) > 1) {
    /// Sort by CCD location
    PerformQuickSort (mApicIdtoUidMap, mNumberOfCpus/2, sizeof (EFI_PROCESSOR_INFORMATION), SortByCcd);
    PerformQuickSort (mApicIdtoUidMap+(mNumberOfCpus/2), mNumberOfCpus/2, sizeof (EFI_PROCESSOR_INFORMATION), SortByCcd);
  } else {
    /// Sort by CCD location
    PerformQuickSort (mApicIdtoUidMap, mNumberOfCpus, sizeof (EFI_PROCESSOR_INFORMATION), SortByCcd);
  }

  // Now allocate the Uid
  for (Index = 0; Index < mNumberOfCpus; Index++) {
    // Now make Processor as Uid
    mApicIdtoUidMap[Index].ProcessorId = Index;
  }

  return EFI_SUCCESS;
}

/**
  Install CPU devices scoped under \_SB into DSDT

  Determine all the CPU threads and create ACPI Device nodes for each thread.
  AGESA will scope to these CPU records when installing CPU power and
  performance capabilities.

  @param[in]      ImageHandle   - Standard UEFI entry point Image Handle
  @param[in]      SystemTable   - Standard UEFI entry point System Table

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
InstallCpuAcpi (
  IN      EFI_HANDLE        ImageHandle,
  IN      EFI_SYSTEM_TABLE  *SystemTable
  )
{
  AML_OBJECT_NODE_HANDLE       CpuInstanceNode;
  AML_OBJECT_NODE_HANDLE       CpuNode;
  AML_OBJECT_NODE_HANDLE       ScopeNode;
  AML_ROOT_NODE_HANDLE         RootNode;
  CHAR8                        *String;
  CHAR8                        Identifier[MAX_TEST_CPU_STRING_SIZE];
  EFI_ACPI_DESCRIPTION_HEADER  *Table;
  EFI_MP_SERVICES_PROTOCOL     *MpServices;
  EFI_STATUS                   Status;
  EFI_STATUS                   Status1;
  UINTN                        DeviceStatus;
  UINTN                        Index;
  UINTN                        NumberOfEnabledProcessors;
  UINTN                        NumberOfLogicProcessors;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  String = &Identifier[0];

  // Get MP service
  MpServices = NULL;
  Status     = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices);
  if (EFI_ERROR (Status) || (MpServices == NULL)) {
    return EFI_NOT_FOUND;
  }

  // Generate ACPI UID Map
  Status = GenerateApicIdtoUidMap ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Could not generate ApicId to ProcessorUid map.\n", __func__));
    return EFI_NOT_FOUND;
  }

  // Load MpServices
  Status = MpServices->GetNumberOfProcessors (MpServices, &NumberOfLogicProcessors, &NumberOfEnabledProcessors);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "AMD   ",
             "SSDTPROC",
             0x00,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlCodeGenScope ("\\_SB_", RootNode, &ScopeNode);  // START: Scope (\_SB)
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  CpuNode = ScopeNode;

  for (Index = 0; Index < NumberOfLogicProcessors; Index++) {
    // Check for valid Processor under the current socket
    if (!mApicIdtoUidMap[Index].StatusFlag) {
      continue;
    }

    // Assumption is that AGESA will have to do the same thing.
    AsciiSPrint (String, MAX_TEST_CPU_STRING_SIZE, "C%03X", Index);
    Status = AmlCodeGenDevice (String, CpuNode, &CpuInstanceNode); // START: Device (CXXX)
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // _HID
    Status = AmlCodeGenNameString ("_HID", "ACPI0007", CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    DeviceStatus = DEVICE_PRESENT_BIT | DEVICE_IN_UI_BIT;
    if (mApicIdtoUidMap[Index].StatusFlag & PROCESSOR_ENABLED_BIT) {
      DeviceStatus |= DEVICE_ENABLED_BIT;
    }

    if (mApicIdtoUidMap[Index].StatusFlag & PROCESSOR_HEALTH_STATUS_BIT) {
      DeviceStatus |= DEVICE_HEALTH_BIT;
    }

    // _UID - Must match ACPI Processor UID in MADT
    Status = AmlCodeGenNameInteger ("_UID", mApicIdtoUidMap[Index].ProcessorId, CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // _STA - As defined by 6.3.7
    Status = AmlCodeGenMethodRetInteger ("_STA", DeviceStatus, 0, FALSE, 0, CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // PACK -> Package
    Status = AmlCodeGenNameInteger ("PACK", mApicIdtoUidMap[Index].ExtendedInformation.Location2.Package, CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // CCD_ -> Ccd
    Status = AmlCodeGenNameInteger ("CCD_", mApicIdtoUidMap[Index].ExtendedInformation.Location2.Die, CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // CCX_ -> Ccx
    Status = AmlCodeGenNameInteger ("CCX_", mApicIdtoUidMap[Index].ExtendedInformation.Location2.Module, CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // CORE -> Core Number
    Status = AmlCodeGenNameInteger ("CORE", mApicIdtoUidMap[Index].ExtendedInformation.Location2.Core, CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // THRD  -> Thread
    Status = AmlCodeGenNameInteger ("THRD", mApicIdtoUidMap[Index].ExtendedInformation.Location2.Thread, CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  Table = NULL;
  // Serialize the tree.
  Status = AmlSerializeDefinitionBlock (
             RootNode,
             &Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PCI: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
    return (Status);
  }

  // Cleanup
  Status1 = AmlDeleteTree (RootNode);
  if (EFI_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PCI: Failed to cleanup AML tree."
      " Status = %r\n",
      Status1
      ));
    // If Status was success but we failed to delete the AML Tree
    // return Status1 else return the original error code, i.e. Status.
    if (!EFI_ERROR (Status)) {
      return Status1;
    }
  }

  Status = AppendExistingAcpiTable (
             EFI_ACPI_6_5_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
             AMD_DSDT_OEMID,
             Table
             );

  return Status;
}
