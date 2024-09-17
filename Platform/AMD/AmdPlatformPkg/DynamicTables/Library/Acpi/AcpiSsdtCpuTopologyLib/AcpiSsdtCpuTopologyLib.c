/** @file

  Generate ACPI SSDT CPU table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <Library/AmlLib/AmlLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/SortLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/MpService.h>
#include <Register/Intel/Cpuid.h> // for CPUID_EXTENDED_TOPOLOGY
#include <Protocol/AcpiSystemDescriptionTable.h>

#define DEVICE_ENABLED_BIT        0x0002
#define DEVICE_HEALTH_BIT         0x0008
#define DEVICE_IN_UI_BIT          0x0004
#define DEVICE_PRESENT_BIT        0x0001
#define MAX_TEST_CPU_STRING_SIZE  20

EFI_PROCESSOR_INFORMATION  *mApicIdtoUidMap = NULL;
UINT32                     mCcdOrder[16]    = { 0, 4, 8, 12, 2, 6, 10, 14, 3, 7, 11, 15, 1, 5, 9, 13 };

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

/**
  Generate ApicId to Processor UID map.

  @retval EFI_SUCCESS           - ApicId to Processor UID map generated successfully.
  @retval EFI_NOT_FOUND         - MP Service Protocol not found.
  @retval EFI_OUT_OF_RESOURCES  - Memory allocation failed.
**/
EFI_STATUS
GenerateApicIdToUidMap (
  VOID
  )
{
  EFI_MP_SERVICES_PROTOCOL  *MpService;
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     NumberOfCpus;
  UINTN                     NumberOfEnabledCPUs;
  UINTN                     SocketCount;

  // Get MP service
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpService);
  if (EFI_ERROR (Status) || (MpService == NULL)) {
    return EFI_NOT_FOUND;
  }

  // Load MpServices
  Status = MpService->GetNumberOfProcessors (MpService, &NumberOfCpus, &NumberOfEnabledCPUs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_ERROR, "%a: NumberOfCpus = %d NumberOfEnabledCPUs = %d\n", __func__, NumberOfCpus, NumberOfEnabledCPUs));

  mApicIdtoUidMap = AllocateZeroPool (NumberOfCpus * sizeof (EFI_PROCESSOR_INFORMATION));
  if (mApicIdtoUidMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SocketCount = 0;
  for (Index = 0; Index < NumberOfCpus; Index++) {
    Status = MpService->GetProcessorInfo (
                          MpService,
                          Index | CPU_V2_EXTENDED_TOPOLOGY,
                          &mApicIdtoUidMap[Index]
                          );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      FreePool (mApicIdtoUidMap);
      mApicIdtoUidMap = NULL;
      return Status;
    }

    if (mApicIdtoUidMap[Index].ExtendedInformation.Location2.Package > SocketCount) {
      SocketCount = mApicIdtoUidMap[Index].ExtendedInformation.Location2.Package;
    }
  }

  // increment the SocketCount by 1 because socket numbering starts from 0
  SocketCount++;

  if (SocketCount > 1) {
    /// Sort by CCD location
    PerformQuickSort (
      mApicIdtoUidMap,
      NumberOfCpus/2,
      sizeof (EFI_PROCESSOR_INFORMATION),
      SortByCcd
      );
    PerformQuickSort (
      mApicIdtoUidMap+(NumberOfCpus/2),
      NumberOfCpus/2,
      sizeof (EFI_PROCESSOR_INFORMATION),
      SortByCcd
      );
  } else {
    /// Sort by CCD location
    PerformQuickSort (
      mApicIdtoUidMap,
      NumberOfCpus,
      sizeof (EFI_PROCESSOR_INFORMATION),
      SortByCcd
      );
  }

  // Now allocate the Uid
  for (Index = 0; Index < NumberOfCpus; Index++) {
    // Now make Processor as Uid
    mApicIdtoUidMap[Index].ProcessorId = Index;
  }

  return EFI_SUCCESS;
}

/**
  Install CPU devices scoped under \_SB into DSDT

  Determine all the CPU threads and create ACPI Device nodes for each thread.

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
GenerateAcpiSsdtCpuTable (
  VOID
  )
{
  AML_OBJECT_NODE_HANDLE       CpuInstanceNode;
  AML_OBJECT_NODE_HANDLE       CpuNode;
  AML_OBJECT_NODE_HANDLE       ScopeNode;
  AML_ROOT_NODE_HANDLE         RootNode;
  CHAR8                        *String;
  CHAR8                        Identifier[MAX_TEST_CPU_STRING_SIZE];
  EFI_ACPI_DESCRIPTION_HEADER  *Table;
  EFI_ACPI_SDT_HEADER          *DsdtTable;
  EFI_ACPI_SDT_HEADER          *ReplacementAcpiTable;
  EFI_ACPI_SDT_PROTOCOL        *AcpiSdtProtocol;
  EFI_ACPI_TABLE_PROTOCOL      *AcpiTableProtocol;
  EFI_ACPI_TABLE_VERSION       DsdtVersion;
  EFI_MP_SERVICES_PROTOCOL     *MpServices;
  EFI_STATUS                   Status;
  EFI_STATUS                   Status1;
  UINT32                       ReplacementAcpiTableLength;
  UINTN                        DeviceStatus;
  UINTN                        DsdtTableKey;
  UINTN                        Index;
  UINTN                        NumberOfEnabledProcessors;
  UINTN                        NumberOfLogicProcessors;
  UINTN                        TableHandle;

  DEBUG ((DEBUG_INFO, "Generating ACPI CPU SSDT Table. \n"));

  // Get Acpi Table Protocol
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Locate ACPI SDT protocol
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&AcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to locate ACPI SDT Protocol. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Find the DSDT table and append to it
  for (Index = 0; ; Index++) {
    Status = AcpiSdtProtocol->GetAcpiTable (
                                Index,
                                &DsdtTable,
                                &DsdtVersion,
                                &DsdtTableKey
                                );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: ACPI DSDT table not found. Status(%r)\n",
        Status
        ));
      return Status;
    }

    if (DsdtTable->Signature == EFI_ACPI_6_5_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      break;
    }
  }

  String = &Identifier[0];

  // Get MP service
  MpServices = NULL;
  Status     = gBS->LocateProtocol (
                      &gEfiMpServiceProtocolGuid,
                      NULL,
                      (VOID **)&MpServices
                      );
  if (EFI_ERROR (Status) || (MpServices == NULL)) {
    return EFI_NOT_FOUND;
  }

  // Load MpServices
  Status = MpServices->GetNumberOfProcessors (
                         MpServices,
                         &NumberOfLogicProcessors,
                         &NumberOfEnabledProcessors
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Generate ACPI UID Map
  Status = GenerateApicIdToUidMap ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Could not generate ApicId to ProcessorUid map.\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "AMD   ",
             "CPU TOPO",
             0x00,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    FreePool (mApicIdtoUidMap);
    return Status;
  }

  Status = AmlCodeGenScope ("\\_SB_", RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  CpuNode = ScopeNode;

  for (Index = 0; Index < NumberOfLogicProcessors; Index++) {
    // Check for valid Processor under the current socket
    if (!mApicIdtoUidMap[Index].StatusFlag) {
      continue;
    }

    AsciiSPrint (String, MAX_TEST_CPU_STRING_SIZE, "C%03X", Index);
    Status = AmlCodeGenDevice (String, CpuNode, &CpuInstanceNode);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // _HID
    Status = AmlCodeGenNameString ("_HID", "ACPI0007", CpuInstanceNode, NULL);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    DeviceStatus = DEVICE_PRESENT_BIT | DEVICE_IN_UI_BIT;
    if (mApicIdtoUidMap[Index].StatusFlag & PROCESSOR_ENABLED_BIT) {
      DeviceStatus |= DEVICE_ENABLED_BIT;
    }

    if (mApicIdtoUidMap[Index].StatusFlag & PROCESSOR_HEALTH_STATUS_BIT) {
      DeviceStatus |= DEVICE_HEALTH_BIT;
    }

    // _UID - Must match ACPI Processor UID in MADT
    Status = AmlCodeGenNameInteger (
               "_UID",
               mApicIdtoUidMap[Index].ProcessorId,
               CpuInstanceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // _STA - As defined by 6.3.7
    Status = AmlCodeGenMethodRetInteger (
               "_STA",
               DeviceStatus,
               0,
               FALSE,
               0,
               CpuInstanceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // PACK -> Package
    Status = AmlCodeGenNameInteger (
               "PACK",
               mApicIdtoUidMap[Index].ExtendedInformation.Location2.Package,
               CpuInstanceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // CCD_ -> Ccd
    Status = AmlCodeGenNameInteger (
               "CCD_",
               mApicIdtoUidMap[Index].ExtendedInformation.Location2.Die,
               CpuInstanceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // CCX_ -> Ccx
    Status = AmlCodeGenNameInteger (
               "CCX_",
               mApicIdtoUidMap[Index].ExtendedInformation.Location2.Module,
               CpuInstanceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // CORE -> Core Number
    Status = AmlCodeGenNameInteger (
               "CORE",
               mApicIdtoUidMap[Index].ExtendedInformation.Location2.Core,
               CpuInstanceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // THRD  -> Thread
    Status = AmlCodeGenNameInteger (
               "THRD",
               mApicIdtoUidMap[Index].ExtendedInformation.Location2.Thread,
               CpuInstanceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
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
    goto exit_handler;
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
      FreePool (mApicIdtoUidMap);
      return Status1;
    }
  }

  // Update the Table header
  Table->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  Table->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  CopyMem (&Table->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Table->OemId));

  // Calculate new DSDT Length and allocate space
  ReplacementAcpiTableLength = DsdtTable->Length + (UINT32)(Table->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER));
  ReplacementAcpiTable       =  AllocatePool (ReplacementAcpiTableLength);
  if (ReplacementAcpiTable == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    DEBUG ((DEBUG_ERROR, "ERROR: Unable to allocate Replacement Table space.\n"));
    FreePool (mApicIdtoUidMap);
    return EFI_OUT_OF_RESOURCES;
  }

  // Copy the old DSDT to the new buffer
  CopyMem (ReplacementAcpiTable, DsdtTable, DsdtTable->Length);

  // Append new data to DSDT
  CopyMem (
    (UINT8 *)ReplacementAcpiTable + DsdtTable->Length,
    (UINT8 *)Table + sizeof (EFI_ACPI_DESCRIPTION_HEADER),
    Table->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)
    );

  ReplacementAcpiTable->Length = ReplacementAcpiTableLength;

  // Uninstall the original DSDT
  Status = AcpiTableProtocol->UninstallAcpiTable (
                                AcpiTableProtocol,
                                DsdtTableKey
                                );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Unable to uninstall original DSDT table. Status(%r)\n",
      Status
      ));
  } else {
    // Install ACPI table
    Status = AcpiTableProtocol->InstallAcpiTable (
                                  AcpiTableProtocol,
                                  ReplacementAcpiTable,
                                  ReplacementAcpiTableLength,
                                  &TableHandle
                                  );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Unable to re-install ACPI DSDT Table. Status(%r)\n",
        Status
        ));
    }
  }

  FreePool (ReplacementAcpiTable);
  FreePool (mApicIdtoUidMap);
  return Status;

exit_handler:
  ASSERT_EFI_ERROR (Status);
  AmlDeleteTree (RootNode);
  FreePool (mApicIdtoUidMap);
  return Status;
}

/**
  Event notification function for AcpiSsdtCpuTopologyLib.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context, which is
                      implementation-dependent.
**/
STATIC
VOID
EFIAPI
AcpiSsdtCpuLibEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  /// Close the Event
  gBS->CloseEvent (Event);

  /// Update the CPU SSDT Table
  GenerateAcpiSsdtCpuTable ();
}

/**
  Implementation of AcpiSsdtCpuTopologyLibConstructor for AMD platforms.
  This is library constructor for AcpiSsdtCpuTopologyLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated ACPI SSDT CPU Table.
  @retval Others          Failed to generate ACPI SSDT CPU Table.
**/
EFI_STATUS
EFIAPI
AcpiSsdtCpuTopologyLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  //
  // Register notify function
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiSsdtCpuLibEvent,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create gEfiEventReadyToBootGuid event. Status(%r)\n",
      __func__,
      Status
      ));
  }

  return Status;
}

/**
  Implementation of AcpiSsdtCpuTopologyLibDestructor for AMD platforms.
  This is library destructor for AcpiSsdtCpuTopologyLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully destroyed ACPI SSDT CPU Table.
  @retval Others          Failed to destroy ACPI SSDT CPU Table.
**/
EFI_STATUS
EFIAPI
AcpiSsdtCpuTopologyLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
