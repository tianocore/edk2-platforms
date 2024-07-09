/** @file

  Generate ACPI SSDT PCI table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <Library/AcpiHelperLib.h>
#include <Library/AmdPlatformSocLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Register/IoApic.h>
#include <Uefi/UefiSpec.h>

/// "CEDT" CXL Early Discovery Table
#define CXL_EARLY_DISCOVERY_TABLE_SIGNATURE  SIGNATURE_32 ('C', 'E', 'D', 'T')
#define MAX_PCI_BUS_NUMBER_PER_SEGMENT       0x100

EFI_ACPI_SDT_PROTOCOL    *mAcpiSdtProtocol;
EFI_ACPI_TABLE_PROTOCOL  *mAcpiTableProtocol;
EFI_HANDLE               mDriverHandle;

/**
  Collect and sort the root bridge devices

  Does not include the Root Bridge resources

  @param[in, out]   RootBridge        - RootBridge information pointer
  @param[in, out]   RootBridgeCount   - Number of root bridges
  @retval           EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
InternalCollectSortedRootBridges (
  IN OUT  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  **RootBridge,
  IN OUT  UINTN                                *RootBridgeCount
  )
{
  UINTN                                Index;
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *LocalRootBridge;  // do not free
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *SortedRb;
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *UnsortedRb;
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  TempRootBridge;
  UINTN                                LocalRootBridgeCount;
  UINTN                                SortedIndex;
  UINTN                                UnsortedIndex;
  EFI_STATUS                           Status;

  if ((RootBridge == NULL) || (RootBridgeCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  LocalRootBridge      = NULL;
  LocalRootBridgeCount = 0;
  Status               = GetPcieInfo (&LocalRootBridge, &LocalRootBridgeCount);
  if (EFI_ERROR (Status) || (LocalRootBridge == NULL) || (LocalRootBridgeCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Cannot obtain Platform PCIe configuration information.\n",
      __func__,
      __LINE__
      ));
    return EFI_NOT_FOUND;
  }

  // Sort by PCIe bus number
  for (SortedIndex = 0, SortedRb = LocalRootBridge;
       SortedIndex < LocalRootBridgeCount;
       SortedIndex++, SortedRb++)
  {
    for (UnsortedIndex = 0, UnsortedRb = LocalRootBridge;
         UnsortedIndex < LocalRootBridgeCount;
         UnsortedIndex++, UnsortedRb++)
    {
      if (SortedRb->Object->BaseBusNumber < UnsortedRb->Object->BaseBusNumber) {
        CopyMem (&TempRootBridge, UnsortedRb, sizeof (AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE));
        CopyMem (UnsortedRb, SortedRb, sizeof (AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE));
        CopyMem (SortedRb, &TempRootBridge, sizeof (AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE));
      }
    }
  }

  for (Index = 0; Index < LocalRootBridgeCount; Index++) {
    // Assign Uid values
    LocalRootBridge[Index].Uid = Index;
  }

  *RootBridge      = LocalRootBridge;
  *RootBridgeCount = LocalRootBridgeCount;
  return EFI_SUCCESS;
}

/**
  Insert Root Bridge interrupts into AML table

  @param[in]      RootBridge  - Single Root Bridge instance
  @param[in, out] GlobalInterruptBase  - Global interrupt base
  @param[in, out] PciNode     - AML tree node

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
InternalInsertRootBridgeInterrupts (
  IN      AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridge,
  IN OUT  UINTN                                *GlobalInterruptBase,
  IN OUT  AML_OBJECT_NODE_HANDLE               PciNode
  )
{
  AML_OBJECT_NODE_HANDLE  PrtNode;
  EFI_STATUS              Status;
  UINTN                   Index;

  Status = AmlCodeGenNamePackage ("_PRT", NULL, &PrtNode);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((RootBridge->Object->BaseBusNumber == 0) && (RootBridge->Object->Segment == 0)) {
    // Package () {0x0014FFFF, 0, 0, 16},  // 0 + 16
    Status = AmlAddPrtEntry (
               0x0014FFFF,
               0,
               NULL,
               16,
               PrtNode
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Package () {0x0014FFFF, 1, 0, 17},  // 0 + 17
    Status = AmlAddPrtEntry (
               0x0014FFFF,
               1,
               NULL,
               17,
               PrtNode
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Package () {0x0014FFFF, 2, 0, 18},  // 0 + 18
    Status = AmlAddPrtEntry (
               0x0014FFFF,
               2,
               NULL,
               18,
               PrtNode
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Package () {0x0014FFFF, 3, 0, 19},  // 0 + 19
    Status = AmlAddPrtEntry (
               0x0014FFFF,
               3,
               NULL,
               19,
               PrtNode
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  }

  /// Add interrupt for Device 0 function 3 (generic to all function)
  Status = AmlAddPrtEntry (
             0xFFFF,
             0,
             NULL,
             (UINT32)(RootBridge->GlobalInterruptStart + 1),
             PrtNode
             );
  ASSERT_EFI_ERROR (Status);

  for (Index = 1; Index <= RootBridge->RootPortCount; Index++) {
    if ((RootBridge->RootPort[Index]->PortPresent == 0) &&
        (RootBridge->RootPort[Index]->Enabled == 0))
    {
      continue;
    }

    // Only insert for Functions 1 - 4 (minus 1)
    if (((RootBridge->RootPort[Index]->Function - 1) & ~0x3) == 0) {
      Status = AmlAddPrtEntry (
                 (UINT32)((RootBridge->RootPort[Index]->Device << 16) | 0x0000FFFF),
                 (UINT8)(RootBridge->RootPort[Index]->Function - 1),
                 NULL,
                 (UINT32)(RootBridge->GlobalInterruptStart + RootBridge->RootPort[Index]->EndpointInterruptArray[RootBridge->RootPort[Index]->Function - 1]),
                 PrtNode
                 );
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }
    }
  }

  // Attach the _PRT entry.
  Status = AmlAttachNode (PciNode, PrtNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  PrtNode = NULL;

  return Status;

exit_handler:
  ASSERT_EFI_ERROR (Status);
  AmlDeleteTree (PrtNode);
  return Status;
}

/**
  Insert Root Bridge resources into AML table

  @param[in]      RootBridge  - Single Root Bridge instance
  @param[in, out] CrsNode     - AML tree node

  @retval         EFI_SUCCESS, various EFI FAILURES.
**/
EFI_STATUS
EFIAPI
InternalInsertRootBridgeResources (
  IN      AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridge,
  IN OUT  AML_OBJECT_NODE_HANDLE               CrsNode
  )
{
  EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR  *LocalBuffer;
  EFI_HANDLE                               *HandleBuffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *Io;
  EFI_STATUS                               Status;
  UINTN                                    BaseBusNumber;
  UINTN                                    Index;
  UINTN                                    NumHandles;
  VOID                                     *Configuration;  // Never free this buffer

  BaseBusNumber = ~(UINTN)0;
  // Get EFI Pci Root Bridge I/O Protocols
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Locate the Root Bridge IO protocol for this root bridge.
  LocalBuffer   = NULL;
  Configuration = NULL;
  for (Index = 0; Index < NumHandles; Index++) {
    Status = gBS->OpenProtocol (
                    HandleBuffer[Index],
                    &gEfiPciRootBridgeIoProtocolGuid,
                    (VOID **)&Io,
                    mDriverHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      FreePool (HandleBuffer);
      return Status;
    }

    if (Io->SegmentNumber == RootBridge->Object->Segment) {
      Status = Io->Configuration (Io, &Configuration);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: Retrieve Root Bridge Configuration failed\n",
          __func__
          ));
        FreePool (HandleBuffer);
        return Status;
      }

      LocalBuffer = Configuration;
      while (TRUE) {
        if (LocalBuffer->Header.Header.Byte == ACPI_END_TAG_DESCRIPTOR) {
          LocalBuffer = NULL;
          break;
        } else if (LocalBuffer->Header.Header.Byte == ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR) {
          if ((LocalBuffer->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) &&
              (LocalBuffer->AddrRangeMin == RootBridge->Object->BaseBusNumber))
          {
            BaseBusNumber = (UINTN)LocalBuffer->AddrRangeMin;
            break;
          }
        }

        LocalBuffer++;
      }

      if (BaseBusNumber == RootBridge->Object->BaseBusNumber) {
        break;
      }
    }
  }

  FreePool (HandleBuffer);
  if ((Configuration == NULL) || (LocalBuffer == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ERROR: Retrieve Root Bridge Configuration failed\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  LocalBuffer = Configuration;

  // All Elements are sizeof (EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR) except
  // for the End Tag
  // Parse through Root Bridge resources and insert them in the ACPI Table
  while (TRUE) {
    if (LocalBuffer->Header.Header.Byte == ACPI_END_TAG_DESCRIPTOR) {
      break;
    } else if (LocalBuffer->Header.Header.Byte == ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR) {
      if (LocalBuffer->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
        BaseBusNumber = (UINTN)LocalBuffer->AddrRangeMin;
        Status        = AmlCodeGenRdWordBusNumber (
                          FALSE,
                          TRUE,
                          TRUE,
                          TRUE,
                          0,
                          (UINT16)LocalBuffer->AddrRangeMin,
                          (UINT16)LocalBuffer->AddrRangeMax,
                          (UINT16)LocalBuffer->AddrTranslationOffset,
                          (UINT16)LocalBuffer->AddrLen,
                          0,
                          NULL,
                          CrsNode,
                          NULL
                          );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      } else if (LocalBuffer->ResType == ACPI_ADDRESS_SPACE_TYPE_IO) {
        Status = AmlCodeGenRdWordIo (
                   FALSE,
                   TRUE,
                   TRUE,
                   TRUE,
                   3, // entire ranges
                   0,
                   (UINT16)LocalBuffer->AddrRangeMin,
                   (UINT16)LocalBuffer->AddrRangeMax,
                   (UINT16)LocalBuffer->AddrTranslationOffset,
                   (UINT16)LocalBuffer->AddrLen,
                   0,
                   NULL,
                   TRUE,
                   TRUE,
                   CrsNode,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      } else if (LocalBuffer->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        Status = AmlCodeGenRdQWordMemory (
                   FALSE,
                   TRUE,
                   TRUE,
                   TRUE,
                   FALSE, // non cacheable
                   TRUE,
                   0,
                   LocalBuffer->AddrRangeMin,
                   LocalBuffer->AddrRangeMax,
                   LocalBuffer->AddrTranslationOffset,
                   LocalBuffer->AddrLen,
                   0,
                   NULL,
                   0,
                   TRUE,
                   CrsNode,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
    } else {
      DEBUG ((DEBUG_ERROR, "%a: ERROR: Invalid Configuration Entry\n", __func__));
      return EFI_NOT_FOUND;
    }

    LocalBuffer++;
  }

  if ((RootBridge->Object->Segment == 0) && (BaseBusNumber == 0)) {
    Status = AmlCodeGenRdWordIo (
               FALSE,
               TRUE,
               TRUE,
               FALSE,
               3, // entire ranges
               0,
               0,
               0x0FFF,
               0,
               0x1000,
               0,
               NULL,
               TRUE,
               TRUE,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenRdQWordMemory (
               FALSE,
               TRUE,
               TRUE,
               TRUE,
               FALSE, // non cacheable
               TRUE,
               0,
               PcdGet32 (PcdIoApicBaseAddress),
               0xFED3FFFF,
               0x0,
               0x140000,
               0,
               NULL,
               0,
               TRUE,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenRdQWordMemory (
               FALSE,
               TRUE,
               TRUE,
               TRUE,
               FALSE, // non cacheable
               TRUE,
               0,
               0xFED45000,
               0xFEDC1FFF,
               0x0,
               0x7D000,
               0,
               NULL,
               0,
               TRUE,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenRdQWordMemory (
               FALSE,
               TRUE,
               TRUE,
               TRUE,
               FALSE, // non cacheable
               TRUE,
               0,
               0xFEDC7000,
               0xFEDCAFFF,
               0x0,
               0x4000,
               0,
               NULL,
               0,
               TRUE,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenRdQWordMemory (
               FALSE,
               TRUE,
               TRUE,
               TRUE,
               FALSE, // non cacheable
               TRUE,
               0,
               0xFEDCC000,
               0xFEDFFFFF,
               0x0,
               0x34000,
               0,
               NULL,
               0,
               TRUE,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenRdQWordMemory (
               FALSE,
               TRUE,
               TRUE,
               TRUE,
               FALSE, // non cacheable
               TRUE,
               0,
               0xFEE01000,
               0xFEFFFFFF,
               0x0,
               0x1FF000,
               0,
               NULL,
               0,
               TRUE,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Insert Root Port into the AML table

  @param[in]      RootBridge  - Single Root Bridge instance
  @param[in]      GlobalInterruptBase - Base to add to IOAPIC interrupt offset
  @param[in,out]  PciNode    - AmlLib table node

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
InternalInsertRootPorts (
  IN      AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridge,
  IN      UINTN                                GlobalInterruptBase,
  IN OUT  AML_OBJECT_NODE_HANDLE               PciNode
  )
{
  AML_METHOD_PARAM        MethodParam[7];
  AML_OBJECT_NODE_HANDLE  DeviceNode;
  AML_OBJECT_NODE_HANDLE  DsmMethod;
  AML_OBJECT_NODE_HANDLE  OstMethod;
  AML_OBJECT_NODE_HANDLE  PrtNode;
  CHAR8                   NameSeg[5];
  CHAR8                   RpName[15];
  EFI_STATUS              Status;
  UINTN                   Index;
  UINTN                   RPIndex;

  Status = EFI_SUCCESS;
  for (RPIndex = 1; RPIndex <= RootBridge->RootPortCount; RPIndex++) {
    if ((RootBridge->RootPort[RPIndex]->PortPresent == 0) &&
        (RootBridge->RootPort[RPIndex]->Enabled == 0))
    {
      continue;
    }

    CopyMem (NameSeg, "RPxx", AML_NAME_SEG_SIZE + 1);
    NameSeg[AML_NAME_SEG_SIZE - 2] = AsciiFromHex (RootBridge->RootPort[RPIndex]->Device & 0xF);
    NameSeg[AML_NAME_SEG_SIZE - 1] = AsciiFromHex (RootBridge->RootPort[RPIndex]->Function & 0xF);

    Status = AmlCodeGenDevice (NameSeg, PciNode, &DeviceNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenNameInteger (
               "_ADR",
               (RootBridge->RootPort[RPIndex]->Device << 16) + RootBridge->RootPort[RPIndex]->Function,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Insert Slot User Number _SUN Record.
    if (RootBridge->RootPort[RPIndex]->SlotNum != 0) {
      Status = AmlCodeGenNameInteger (
                 "_SUN",
                 RootBridge->RootPort[RPIndex]->SlotNum,
                 DeviceNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    // _DSM and _OST, handling for root port EDR feature.
    // Device 1 to 4 are external PCIe ports, only include them.
    if ((RootBridge->RootPort[RPIndex]->Device > 0) &&
        (RootBridge->RootPort[RPIndex]->Device < 5))
    {
      DEBUG ((
        DEBUG_INFO,
        "%a:Add EDR support for Uid 0x%x Addr 0x%x\n",
        __func__,
        RootBridge->Uid,
        ((RootBridge->RootPort[RPIndex]->Device << 16) + RootBridge->RootPort[RPIndex]->Function)
        ));

      AsciiSPrint (
        RpName,
        5,
        "P%01X%01X%01X",
        RootBridge->Uid,
        RootBridge->RootPort[RPIndex]->Device,
        RootBridge->RootPort[RPIndex]->Function
        );

      Status = AmlCodeGenNameString (
                 "RSTR",
                 RpName,
                 DeviceNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = AmlCodeGenNameInteger (
                 "BRB_",
                 RootBridge->Object->BaseBusNumber,
                 DeviceNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      /// Create a _DSM method for the root port
      Status = AmlCodeGenMethodRetNameString (
                 "_DSM",
                 NULL,
                 4,
                 TRUE,
                 0,
                 DeviceNode,
                 &DsmMethod
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      /// fill the AML_METHOD_PARAM structure to call the \\_SB.HDSM method
      ZeroMem (MethodParam, sizeof (MethodParam));
      MethodParam[0].Type         = AmlMethodParamTypeArg;
      MethodParam[0].Data.Arg     = 0x0; // Arg0 is the first argument to the method
      MethodParam[1].Type         = AmlMethodParamTypeArg;
      MethodParam[1].Data.Arg     = 0x1; // Arg1 is the second argument to the method
      MethodParam[2].Type         = AmlMethodParamTypeArg;
      MethodParam[2].Data.Arg     = 0x2; // Arg2 is the third argument to the method
      MethodParam[3].Type         = AmlMethodParamTypeArg;
      MethodParam[3].Data.Arg     = 0x3; // Arg3 is the fourth argument to the method
      MethodParam[4].Type         = AmlMethodParamTypeInteger;
      MethodParam[4].Data.Integer = RootBridge->Object->BaseBusNumber;
      MethodParam[5].Type         = AmlMethodParamTypeInteger;
      MethodParam[5].Data.Integer = (RootBridge->RootPort[RPIndex]->Device << 16) + RootBridge->RootPort[RPIndex]->Function;
      MethodParam[6].Type         = AmlMethodParamTypeString;
      MethodParam[6].Data.Buffer  = RpName;
      /// Call the \\_SB.HDSM method
      Status = AmlCodeGenInvokeMethod (
                 "\\_SB.HDSM",
                 7,
                 MethodParam,
                 DsmMethod
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      /// Create a _OST method for the root port
      Status = AmlCodeGenMethodRetNameString (
                 "_OST",
                 NULL,
                 3,
                 TRUE,
                 0,
                 DeviceNode,
                 &OstMethod
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      // fill the AML_METHOD_PARAM structure to call the \\_SB._OST method
      ZeroMem (MethodParam, sizeof (MethodParam));
      MethodParam[0].Type         = AmlMethodParamTypeArg;
      MethodParam[0].Data.Arg     = 0x0; // Arg0 is the first argument to the method
      MethodParam[1].Type         = AmlMethodParamTypeArg;
      MethodParam[1].Data.Arg     = 0x1; // Arg1 is the second argument to the method
      MethodParam[2].Type         = AmlMethodParamTypeInteger;
      MethodParam[2].Data.Integer = RootBridge->Object->BaseBusNumber;
      MethodParam[3].Type         = AmlMethodParamTypeInteger;
      MethodParam[3].Data.Integer = (RootBridge->RootPort[RPIndex]->Device << 16) + RootBridge->RootPort[RPIndex]->Function;
      // call the \\_SB._OST method
      Status = AmlCodeGenInvokeMethod (
                 "\\_SB.HOST",
                 4,
                 MethodParam,
                 OstMethod
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    // Build Root Port _PRT entry and insert in main ACPI Object list
    Status = AmlCodeGenNamePackage ("_PRT", NULL, &PrtNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    for (Index = 0; Index <= 3; Index++) {
      Status = AmlAddPrtEntry (
                 0x0000FFFF,
                 (UINT8)Index,
                 NULL,
                 (UINT32)(GlobalInterruptBase + RootBridge->RootPort[RPIndex]->EndpointInterruptArray[Index]),
                 PrtNode
                 );
      if (EFI_ERROR (Status)) {
        AmlDeleteTree (PrtNode);
        return Status;
      }
    }

    // Attach the _PRT entry.
    Status = AmlAttachNode (DeviceNode, PrtNode);
    if (EFI_ERROR (Status)) {
      AmlDeleteTree (PrtNode);
      return Status;
    }

    PrtNode = NULL;
  }

  return Status;
}

/**
  Insert CXL Root Bridge into the AML table

  @param[in]      RootBridgeHead  - RootBridge information pointer
  @param[in]      RootBridgeCount - Number of root bridges
  @param[in,out]  PciNode         - AmlLib table node

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
InternalInsertCxlRootBridge (
  IN          AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridgeHead,
  IN          UINTN                                RootBridgeCount,
  IN OUT      AML_OBJECT_NODE_HANDLE               PciNode
  )
{
  AMD_PCI_ADDR                         PciAddr;
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridge;
  AML_METHOD_PARAM                     MethodParam[7];
  AML_OBJECT_NODE_HANDLE               CrsNode;
  AML_OBJECT_NODE_HANDLE               DeviceNode;
  AML_OBJECT_NODE_HANDLE               OscMethod;
  AML_OBJECT_NODE_HANDLE               PackageNode;
  CHAR8                                NameSeg[5];
  EFI_STATUS                           Status;
  UINT32                               EisaId;
  UINT8                                DevIndex;
  UINT8                                Index;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __func__));
  ZeroMem ((VOID *)&PciAddr, sizeof (PciAddr));

  //
  // Populate the data structure for the CXL devices in the system to add to
  // the ACPI Table
  //
  DevIndex = 0;
  Status   = EFI_SUCCESS;
  for (Index = 0, RootBridge = RootBridgeHead; Index < RootBridgeCount; Index++, RootBridge++) {
    if ((RootBridge->CxlCount == 0) || (RootBridge->CxlPortInfo.IsCxl2 == TRUE)) {
      continue;
    }

    DevIndex++;

    CopyMem (NameSeg, "CXLx", AML_NAME_SEG_SIZE + 1);
    if (DevIndex < 0x10) {
      NameSeg[AML_NAME_SEG_SIZE - 1] = AsciiFromHex (DevIndex);
    } else {
      NameSeg[AML_NAME_SEG_SIZE - 2] = AsciiFromHex (DevIndex);
    }

    Status = AmlCodeGenDevice (NameSeg, PciNode, &DeviceNode); // RootBridge
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenNameString ("_HID", "ACPI0016", DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlCodeGenNamePackage ("_CID", DeviceNode, &PackageNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_CID, EISAID("PNP0A03"))
    Status = AmlGetEisaIdFromString ("PNP0A03", &EisaId);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlAddIntegerToNamedPackage (EisaId, PackageNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_CID, EISAID("PNP0A03"))
    Status = AmlGetEisaIdFromString ("PNP0A08", &EisaId);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AmlAddIntegerToNamedPackage (EisaId, PackageNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_ADR, <address>)
    Status = AmlCodeGenNameInteger (
               "_ADR",
               (RootBridge->CxlPortInfo.EndPointBDF.Address.Device << 16) + RootBridge->CxlPortInfo.EndPointBDF.Address.Function,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_UID, <root bridge number>)
    Status = AmlCodeGenNameInteger ("_UID", DevIndex, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_BBN, <base bus number>)
    Status = AmlCodeGenNameInteger (
               "_BBN",
               RootBridge->CxlPortInfo.EndPointBDF.Address.Bus,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_SEG, <segment number>)
    Status = AmlCodeGenNameInteger (
               "_SEG",
               RootBridge->Object->BaseBusNumber / MAX_PCI_BUS_NUMBER_PER_SEGMENT,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_PXM, <RootBridge->SocketId>)
    PciAddr.Address.Bus     = (UINT32)RootBridge->Object->BaseBusNumber;
    PciAddr.Address.Segment = (UINT32)RootBridge->Object->Segment;

    Status = AmlCodeGenNameInteger (
               "_PXM",
               RootBridge->PxmDomain,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Name (_CRS, <CRS Resource Template>)
    Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = InternalInsertRootBridgeResources (RootBridge, CrsNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    /// Create AML code for below method
    ///             Method (_OSC, 4, NotSerialized, 4)  // _OSC: Operating System Capabilities
    ///        {
    ///            \_SB.OSCI (Arg0, Arg1, Arg2, Arg3)
    ///        }
    Status = AmlCodeGenMethodRetNameString (
               "_OSC",
               NULL,
               4,
               TRUE,
               0,
               DeviceNode,
               &OscMethod
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // fill the AML_METHOD_PARAM structure to call the \\_SB.OSCI method
    ZeroMem (MethodParam, sizeof (MethodParam));
    MethodParam[0].Type     = AmlMethodParamTypeArg;
    MethodParam[0].Data.Arg = 0x0; // Arg0 is the first argument to the method
    MethodParam[1].Type     = AmlMethodParamTypeArg;
    MethodParam[1].Data.Arg = 0x1; // Arg1 is the second argument to the method
    MethodParam[2].Type     = AmlMethodParamTypeArg;
    MethodParam[2].Data.Arg = 0x2; // Arg2 is the third argument to the method
    MethodParam[3].Type     = AmlMethodParamTypeArg;
    MethodParam[3].Data.Arg = 0x3; // Arg3 is the fourth argument to the method
    // call the \\_SB.OSCI method
    Status = AmlCodeGenInvokeMethod (
               "\\_SB.OSCI",
               4,
               MethodParam,
               OscMethod
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}

/**
  Insert Pcie base size into the AML table

  @param[in,out]  CrsNode    - AmlLib table node

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
InternalInsertPciExpressBaseSize (
  IN OUT  AML_OBJECT_NODE_HANDLE  CrsNode
  )
{
  EFI_STATUS  Status;
  UINT64      RangeLen;
  UINT64      RangeMax;
  UINT64      RangeMin;

  RangeMin = PcdGet64 (PcdPciExpressBaseAddress);
  RangeLen = PcdGet64 (PcdPciExpressBaseSize);
  RangeMax = RangeMin + RangeLen - 1;

  Status = AmlCodeGenRdQWordMemory (
             FALSE,
             TRUE,
             TRUE,
             TRUE,
             FALSE, // non cacheable
             TRUE,
             0x0,
             RangeMin,
             RangeMax,
             0x0,
             RangeLen,
             0,
             NULL,
             0,
             TRUE,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Locates an existing ACPI Table

  @param[in]      Signature     - The Acpi table signature
  @param[in]      OemTableId    - The Acpi table OEM Table ID. Ignored if 0
  @param[out]     Table         - Table if Found or NULL

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
GetExistingAcpiTable (
  IN      UINT32            Signature,
  IN      UINT64            OemTableId,
  OUT  EFI_ACPI_SDT_HEADER  **Table
  )
{
  EFI_ACPI_SDT_HEADER     *LocalTable;
  EFI_ACPI_TABLE_VERSION  LocalVersion;
  EFI_STATUS              Status;
  UINTN                   Index;
  UINTN                   LocalTableKey;

  Status = EFI_NOT_FOUND;
  *Table = NULL;

  for (Index = 0; ; Index++) {
    Status = mAcpiSdtProtocol->GetAcpiTable (Index, &LocalTable, &LocalVersion, &LocalTableKey);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!(LocalTable->Signature == Signature)) {
      continue;
    }

    // Accept table if OemTableId is zero.
    if ((OemTableId == 0) ||
        (CompareMem (&LocalTable->OemTableId, &OemTableId, 8) == 0))
    {
      *Table = LocalTable;
      return EFI_SUCCESS;
    }
  }
}

/**
  Install PCI devices scoped under \_SB into DSDT

  Determine all the PCI Root Bridges and PCI root ports and install resources
  including needed _HID, _CID, _UID, _ADR, _CRS and _PRT Nodes.

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
GenerateAcpiSsdtPciTable (
  VOID
  )
{
  AMD_PCI_ADDR                         PciAddr;
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridge;
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridgeHead;
  AML_METHOD_PARAM                     MethodParam[7];
  AML_OBJECT_NODE_HANDLE               AmdmNode;
  AML_OBJECT_NODE_HANDLE               CrsNode;
  AML_OBJECT_NODE_HANDLE               CxldNode;
  AML_OBJECT_NODE_HANDLE               DsmMethod;
  AML_OBJECT_NODE_HANDLE               OscMethod;
  AML_OBJECT_NODE_HANDLE               PackageNode;
  AML_OBJECT_NODE_HANDLE               PciNode;
  AML_OBJECT_NODE_HANDLE               ScopeNode;
  AML_ROOT_NODE_HANDLE                 RootNode;
  CHAR8                                AslName[AML_NAME_SEG_SIZE + 1];
  EFI_ACPI_DESCRIPTION_HEADER          *Table;
  EFI_ACPI_SDT_HEADER                  *DsdtTable;
  EFI_ACPI_SDT_HEADER                  *ReplacementAcpiTable;
  EFI_ACPI_SDT_HEADER                  *SdtTable;
  EFI_ACPI_TABLE_VERSION               DsdtVersion;
  EFI_STATUS                           Status;
  EFI_STATUS                           Status1;
  UINT32                               EisaId;
  UINT32                               ReplacementAcpiTableLength;
  UINTN                                DsdtTableKey;
  UINTN                                GlobalInterruptBase;
  UINTN                                Index;
  UINTN                                RbIndex;
  UINTN                                RootBridgeCount;
  UINTN                                TableHandle;

  DEBUG ((DEBUG_INFO, "Generating ACPI SSDT PCI Table.\n"));

  // Get Acpi Table Protocol
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Get Acpi SDT Protocol
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Find the DSDT table and append to it
  for (Index = 0; ; Index++) {
    Status = mAcpiSdtProtocol->GetAcpiTable (
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

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "AMD   ",
             "PCIE DEV",
             0x00,
             &RootNode
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem ((VOID *)&PciAddr, sizeof (PciAddr));
  GlobalInterruptBase = 0;

  Status = InternalCollectSortedRootBridges (&RootBridgeHead, &RootBridgeCount);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Status = AmlCodeGenScope ("\\_SB_", RootNode, &ScopeNode);  // START: Scope (\_SB)
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Create Root Bridge PCXX devices
  for (RbIndex = 0, RootBridge = RootBridgeHead;
       RbIndex < RootBridgeCount;
       RbIndex++, RootBridge++)
  {
    GlobalInterruptBase = RootBridge->GlobalInterruptStart;
    // Make sure there is always PCI0 since this is a defacto standard. And
    // therefore PCI0-PCIF and then PC10-PCFF
    CopyMem (AslName, "PCIx", AML_NAME_SEG_SIZE + 1);
    AslName[AML_NAME_SEG_SIZE - 1] = AsciiFromHex (RootBridge->Uid & 0xF);
    if (RootBridge->Uid > 0xF) {
      AslName[AML_NAME_SEG_SIZE - 2] = AsciiFromHex ((RootBridge->Uid >> 4) & 0xF);
    }

    Status = AmlCodeGenDevice (AslName, ScopeNode, &PciNode); // RootBridge
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    if ((RootBridge->CxlCount > 0) && (RootBridge->CxlPortInfo.IsCxl2 == TRUE)) {
      Status = AmlCodeGenNameString ("_HID", "ACPI0016", PciNode, NULL);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      Status = AmlCodeGenNamePackage ("_CID", PciNode, &PackageNode);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      // Name (_CID, EISAID("PNP0A03"))
      Status = AmlGetEisaIdFromString ("PNP0A03", &EisaId);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      Status = AmlAddIntegerToNamedPackage (EisaId, PackageNode);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      // Name (_CID, EISAID("PNP0A03"))
      Status = AmlGetEisaIdFromString ("PNP0A08", &EisaId);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      Status = AmlAddIntegerToNamedPackage (EisaId, PackageNode);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }
    } else {
      // Name (_HID, EISAID("PNP0A08"))
      Status = AmlGetEisaIdFromString ("PNP0A08", &EisaId);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      Status = AmlCodeGenNameInteger ("_HID", EisaId, PciNode, NULL);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      // Name (_CID, EISAID("PNP0A03"))
      Status = AmlGetEisaIdFromString ("PNP0A03", &EisaId);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }

      Status = AmlCodeGenNameInteger ("_CID", EisaId, PciNode, NULL);
      if (EFI_ERROR (Status)) {
        goto exit_handler;
      }
    }

    // Name (_UID, <root bridge number>)
    Status = AmlCodeGenNameInteger ("_UID", RootBridge->Uid, PciNode, NULL);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Name (_BBN, <base bus number>)
    Status = AmlCodeGenNameInteger (
               "_BBN",
               RootBridge->Object->BaseBusNumber,
               PciNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Name (_SEG, <segment number>)
    Status = AmlCodeGenNameInteger (
               "_SEG",
               RootBridge->Object->Segment,
               PciNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Name (_PXM, <RootBridge->SocketId>)
    PciAddr.Address.Bus     = (UINT32)RootBridge->Object->BaseBusNumber;
    PciAddr.Address.Segment = (UINT32)RootBridge->Object->Segment;

    Status = AmlCodeGenNameInteger (
               "_PXM",
               RootBridge->PxmDomain,
               PciNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Name (_CRS, <CRS Resource Template>)
    Status = AmlCodeGenNameResourceTemplate ("_CRS", PciNode, &CrsNode);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    Status = InternalInsertRootBridgeResources (RootBridge, CrsNode);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Name (_PRT, <Interrupt Packages>)
    Status = InternalInsertRootBridgeInterrupts (RootBridge, &GlobalInterruptBase, PciNode);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Create Root Port PXXX devices
    // Name (_ADR, <pci address>)
    // Name (_PRT, <Interrupt Packages>)
    //   Needs to be offset by previous IOAPICs interrupt count
    Status = InternalInsertRootPorts (RootBridge, RootBridge->GlobalInterruptStart, PciNode);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    /// AML code to generate _OSC method
    ///             Method (_OSC, 4, NotSerialized, 4)  // _OSC: Operating System Capabilities
    ///        {
    ///            \_SB.OSCI (Arg0, Arg1, Arg2, Arg3)
    ///        }
    Status = AmlCodeGenMethodRetNameString (
               "_OSC",
               NULL,
               4,
               TRUE,
               0,
               PciNode,
               &OscMethod
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // fill the AML_METHOD_PARAM structure to call the \\_SB.OSCI method
    ZeroMem (MethodParam, sizeof (MethodParam));
    MethodParam[0].Type     = AmlMethodParamTypeArg;
    MethodParam[0].Data.Arg = 0x0; // Arg0 is the first argument to the method
    MethodParam[1].Type     = AmlMethodParamTypeArg;
    MethodParam[1].Data.Arg = 0x1; // Arg1 is the second argument to the method
    MethodParam[2].Type     = AmlMethodParamTypeArg;
    MethodParam[2].Data.Arg = 0x2; // Arg2 is the third argument to the method
    MethodParam[3].Type     = AmlMethodParamTypeArg;
    MethodParam[3].Data.Arg = 0x3; // Arg3 is the fourth argument to the method
    // call the \\_SB.OSCI method
    Status = AmlCodeGenInvokeMethod (
               "\\_SB.OSCI",
               4,
               MethodParam,
               OscMethod
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  }

  Status = GetExistingAcpiTable (
             CXL_EARLY_DISCOVERY_TABLE_SIGNATURE,
             0,
             &SdtTable
             );
  if (!EFI_ERROR (Status)) {
    // CXL Root Device Specific Methods (_DSM)
    Status = AmlCodeGenDevice ("CXLD", ScopeNode, &CxldNode);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // _DSM Functions that are associated with the CXL Root Device (HID="ACPI0017")
    Status = AmlCodeGenNameString ("_HID", "ACPI0017", CxldNode, NULL);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // Create a _DSM method
    Status = AmlCodeGenMethodRetNameString (
               "_DSM",
               NULL,
               4,
               TRUE,
               0,
               CxldNode,
               &DsmMethod
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    // fill the AML_METHOD_PARAM structure to call the \\_SB.HDSM method
    ZeroMem (MethodParam, sizeof (MethodParam));
    MethodParam[0].Type     = AmlMethodParamTypeArg;
    MethodParam[0].Data.Arg = 0x0;     // Arg0 is the first argument to the method
    MethodParam[1].Type     = AmlMethodParamTypeArg;
    MethodParam[1].Data.Arg = 0x1;     // Arg1 is the second argument to the method
    MethodParam[2].Type     = AmlMethodParamTypeArg;
    MethodParam[2].Data.Arg = 0x2;     // Arg2 is the third argument to the method
    MethodParam[3].Type     = AmlMethodParamTypeArg;
    MethodParam[3].Data.Arg = 0x3;     // Arg3 is the fourth argument to the method
    //
    // Call the \\_SB.HDSM method
    // The CXL DSM will look for UUID: f365f9a6-a7de-4071-a66a-b40c0b4f8e52
    //
    Status = AmlCodeGenInvokeMethod (
               "\\_SB.HDSM",
               4,
               MethodParam,
               DsmMethod
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  }

  //
  // CXL device are added as Root Bridges but are not part of
  // the AMD PCI Resource Protocol
  //
  Status = InternalInsertCxlRootBridge (RootBridgeHead, RootBridgeCount, ScopeNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Add Pcie Base Size
  Status = AmlCodeGenDevice ("AMDM", ScopeNode, &AmdmNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Name (_HID, EISAID("PNP0C02"))
  Status = AmlGetEisaIdFromString ("PNP0C02", &EisaId);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Status = AmlCodeGenNameInteger ("_HID", EisaId, AmdmNode, NULL);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Name (_UID, <root bridge number>)
  Status = AmlCodeGenNameInteger ("_UID", 0, AmdmNode, NULL);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Status = AmlCodeGenNameResourceTemplate ("_CRS", AmdmNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Status = InternalInsertPciExpressBaseSize (CrsNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
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
      return Status1;
    }
  }

  FreePool (RootBridgeHead);

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
    FreePool (Table);
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
  Status = mAcpiTableProtocol->UninstallAcpiTable (
                                 mAcpiTableProtocol,
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
    Status = mAcpiTableProtocol->InstallAcpiTable (
                                   mAcpiTableProtocol,
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
  FreePool (Table);
  return Status;

exit_handler:
  ASSERT_EFI_ERROR (Status);
  AmlDeleteTree (RootNode);
  return Status;
}

/**
  Event notification function for AcpiSsdtPciLib.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context, which is
                      implementation-dependent.
**/
STATIC
VOID
EFIAPI
AcpiSsdtPciLibEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  /// Close the Event
  gBS->CloseEvent (Event);

  /// Update the PCI SSDT Table
  GenerateAcpiSsdtPciTable ();
}

/**
  Implementation of AcpiSsdtPciLibConstructor for AMD platforms.
  This is library constructor for AcpiSsdtPciLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated ACPI SSDT PCI Table.
  @retval Others          Failed to generate ACPI SSDT PCI Table.
**/
EFI_STATUS
EFIAPI
AcpiSsdtPciLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  mDriverHandle = ImageHandle;

  //
  // Register notify function
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiSsdtPciLibEvent,
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
  Implementation of AcpiSsdtPciLibDestructor for AMD platforms.
  This is library destructor for AcpiSsdtPciLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully destroyed ACPI SSDT PCI Table.
  @retval Others          Failed to destroy ACPI SSDT PCI Table.
**/
EFI_STATUS
EFIAPI
AcpiSsdtPciLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
