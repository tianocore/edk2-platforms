/** @file
  DSDT builder for the RISC-V ACPI driver.

  Generates the Differentiated System Description Table (DSDT) using the
  AmlLib library from DynamicTablesPkg. The DSDT describes:

    \_SB scope:
      CPU devices (ACPI0007) — one per hart, with _HID, _UID, _MAT
      UART device (RSCV0003)
      Interrupt controller — APLIC (RSCV0002), one per socket
      IOMMU device (RSCV0004)
      PCIe host bridge (_HID PNP0A03, _CID PNP0A08)

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

#include <Library/AmlLib/AmlLib.h>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/**
  Add a _CRS ResourceTemplate containing one QWordMemory range and,
  optionally, one extended interrupt to a device node.

  @param[in]  DevNode   AML Device node to attach _CRS to.
  @param[in]  Base      64-bit MMIO base address.
  @param[in]  Size      64-bit MMIO size in bytes.
  @param[in]  Gsi       Global System Interrupt array (ignored when HasIrq
                        is FALSE).
  @param[in]  NumGsi    Number of interrupts.
  @param[in]  HasIrq    TRUE to include an extended interrupt descriptor.

  @retval EFI_SUCCESS   _CRS node created and attached.
  @retval other         AmlLib error.
**/
STATIC
EFI_STATUS
AddCrsMemIrq (
  IN AML_OBJECT_NODE_HANDLE  DevNode,
  IN UINT64                  Base,
  IN UINT64                  Size,
  IN UINT32                  *Gsi,
  IN UINT8                   NumGsi,
  IN BOOLEAN                 HasIrq
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  CrsNode;

  //
  // Name (_CRS, ResourceTemplate () {})
  //
  Status = AmlCodeGenNameResourceTemplate ("_CRS", DevNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _CRS method: %r\n", __func__, Status));
    return Status;
  }

  //
  // Append QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
  //                     NonCacheable, ReadWrite, 0, Base, Base+Size-1, 0, Size)
  //
  Status = AmlCodeGenRdQWordMemory (
             FALSE,                    // ResourceConsumer = false (producer)
             TRUE,                     // PosDecode
             TRUE,                     // MinFixed
             TRUE,                     // MaxFixed
             AmlMemoryNonCacheable,
             TRUE,                     // ReadWrite
             0,                        // Granularity
             Base,                     // Minimum
             Base + Size - 1,          // Maximum
             0,                        // TranslationOffset
             Size,                     // RangeLength
             0,
             NULL,
             AmlAddressRangeMemory,
             TRUE,                     // TypeStatic
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create QWordMemory: %r\n", __func__, Status));
    return Status;
  }

  if (HasIrq) {
    //
    // Append Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { Gsi }
    //
    Status = AmlCodeGenRdInterrupt (
               TRUE,    // ResourceConsumer
               FALSE,   // EdgeTriggered = false (Level)
               FALSE,   // ActiveLow = false (ActiveHigh)
               FALSE,   // Shared = false (Exclusive)
               Gsi,
               NumGsi,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create interrupt resource: %r\n", __func__, Status));
    }
  }

  return Status;
}

/**
  Build and attach four PNP0C0F PCI Interrupt Link devices (L000..L003)
  under the given \_SB scope node.

  @param[in]  SbNode   \_SB scope node to attach link devices to.
  @param[in]  LinkGsi  Array of 4 GSIs: LinkGsi[i] = PCIE_IRQ + i.

  @retval EFI_SUCCESS   All four link devices created and attached.
  @retval other         AmlLib error.
**/
STATIC
EFI_STATUS
AddPcieLinkDevices (
  IN AML_OBJECT_NODE_HANDLE  SbNode,
  IN UINT32                  *LinkGsi
  )
{
  EFI_STATUS              Status;
  UINT32                  LinkIdx;
  AML_OBJECT_NODE_HANDLE  LnkNode;
  AML_OBJECT_NODE_HANDLE  CrsNode;
  AML_OBJECT_NODE_HANDLE  PrsNode;
  CHAR8                   LnkName[5];
  UINT32                  Gsi;

  for (LinkIdx = 0; LinkIdx < 4; LinkIdx++) {
    AsciiSPrint (LnkName, sizeof (LnkName), "L00%u", LinkIdx);
    Gsi = LinkGsi[LinkIdx];

    Status = AmlCodeGenDevice (LnkName, SbNode, &LnkNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create %a device: %r\n", __func__, LnkName, Status));
      return Status;
    }

    Status = AmlCodeGenNameString ("_HID", "PNP0C0F", LnkNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _HID PNP0C0F: %r\n", __func__, Status));
      return Status;
    }

    Status = AmlCodeGenNameInteger ("_UID", LinkIdx, LnkNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _UID name integer %d: %r\n", __func__, LinkIdx, Status));
      return Status;
    }

    //
    // _PRS: possible resource settings.
    //
    Status = AmlCodeGenNameResourceTemplate ("_PRS", LnkNode, &PrsNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _PRS: %r\n", __func__, Status));
      return Status;
    }

    Status = AmlCodeGenRdInterrupt (
               TRUE,    // ResourceConsumer
               FALSE,   // Level triggered
               FALSE,   // ActiveHigh
               FALSE,   // Exclusive
               &Gsi,
               1,
               PrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create interrupt resource: %r\n", __func__, Status));
      return Status;
    }

    //
    // _CRS: current resource setting, identical to _PRS.
    //
    Status = AmlCodeGenNameResourceTemplate ("_CRS", LnkNode, &CrsNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _CRS: %r\n", __func__, Status));
      return Status;
    }

    Status = AmlCodeGenRdInterrupt (
               TRUE,    // ResourceConsumer
               FALSE,   // Level triggered
               FALSE,   // ActiveHigh
               FALSE,   // Exclusive
               &Gsi,
               1,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create interrupt resource: %r\n", __func__, Status));
      return Status;
    }

    //
    // _SRS: no-op empty method, 1 argument.
    //
    Status = AmlCodeGenMethod ("_SRS", 1, FALSE, 0, LnkNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _SRS method: %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Build and attach Name (_PRT, Package (128) { ... }) to a PCIe host bridge
  device node.

  Each entry references one of the four PNP0C0F link devices (LNK0..LNK3)
  under \_SB as the interrupt Source, with SourceIndex=0.

  @param[in]  PcieNode  AML Device node of the PCIe host bridge (PCI0).

  @retval EFI_SUCCESS   _PRT node created and attached.
  @retval other         AmlLib or parse error.
**/
STATIC
EFI_STATUS
AddPciePrt (
  IN AML_OBJECT_NODE_HANDLE  PcieNode
  )
{
  EFI_STATUS              Status;
  UINT32                  DevNum;
  UINT8                   Pin;
  UINT32                  Addr;
  UINT32                  LinkIdx;
  AML_OBJECT_NODE_HANDLE  PrtNode;
  CHAR8                   LinkName[5];

  PrtNode = NULL;

  //
  // Name (_PRT, Package () { ... })
  //
  Status = AmlCodeGenNamePackage ("_PRT", NULL, &PrtNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _PRT: %r\n", __func__, Status));
    return Status;
  }

  //
  // Emit 128 Package(4){Address, Pin, L00x, Zero} entries.
  //
  for (DevNum = 0; DevNum < 32; DevNum++) {
    for (Pin = 0; Pin < 4; Pin++) {
      Addr    = (DevNum << 16) | 0xFFFFu;
      LinkIdx = (DevNum + Pin) % 4;

      AsciiSPrint (LinkName, sizeof (LinkName), "L00%u", LinkIdx);
      Status = AmlAddPrtEntry (
                 Addr,
                 Pin,
                 LinkName,
                 0,
                 PrtNode
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: AML: Failed to add PRT entry dev %d pin %d : %r\n", __func__, DevNum, Pin, Status));
        AmlDeleteTree (PrtNode);
        return Status;
      }
    }
  }

  Status = AmlAttachNode ((AML_NODE_HANDLE)PcieNode, (AML_NODE_HANDLE)PrtNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to attach _PRT to PCIe node: %r\n", __func__, Status));
    AmlDeleteTree (PrtNode);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Add CPU device nodes.

  @param[in]       Topo      Platform topology.
  @param[in, out]  SbNode    \_SB scope node.

  @retval EFI_SUCCESS        CPU device nodes were successfully.
  @retval other              Failed to add CPU device nodes.
**/
STATIC
EFI_STATUS
AddCpuDevices (
  IN PLATFORM_TOPOLOGY           *Topo,
  IN OUT AML_OBJECT_NODE_HANDLE  SbNode
  )
{
  EFI_STATUS                    Status;
  UINTN                         HartIdx;
  AML_OBJECT_NODE_HANDLE        CpuNode;
  CHAR8                         DevName[5];
  UINT8                         GuestIndexBits;
  EFI_ACPI_6_6_RINTC_STRUCTURE  RintcBytes;

  for (HartIdx = 0; HartIdx < Topo->NumHarts; HartIdx++) {
    AsciiSPrint (DevName, sizeof (DevName), "C%03X", (UINT32)HartIdx);

    //
    // Device ("C<NNN>") {}
    //
    Status = AmlCodeGenDevice (DevName, SbNode, &CpuNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create %a device: %r\n", __func__, DevName, Status));
      return Status;
    }

    //
    // Name (_HID, "ACPI0007")
    //
    Status = AmlCodeGenNameString ("_HID", "ACPI0007", CpuNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _HID ACPI0007 name: %r\n", __func__, Status));
      return Status;
    }

    //
    // Name (_UID, HartIdx)
    //
    Status = AmlCodeGenNameInteger ("_UID", HartIdx, CpuNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _UID %d name: %r\n", __func__, HartIdx, Status));
      return Status;
    }

    //
    // Build the raw RINTC bytes for _MAT.
    //
    GuestIndexBits = (UINT8)(Topo->AiaGuests > 0
                            ? ImsicNumBits (Topo->AiaGuests + 1)
                            : 0);

    GenerateRintcBytes (
      Topo,
      &RintcBytes,
      GuestIndexBits,
      HartIdx
      );

    //
    // Name (_MAT, Buffer (sizeof(EFI_ACPI_6_6_RINTC_STRUCTURE)) { <rintc bytes> })
    //
    Status = AmlCodeGenMethodRetBuffer ("_MAT", (UINT8 *)&RintcBytes, sizeof (RintcBytes), 0, FALSE, 0, CpuNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _MAT method: %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Build UART device node.

  @param[in]       Topo     Platform topology.
  @param[in, out]  SbNode   The SB node to add the UART device to.

  @retval EFI_SUCCESS       UART node added successfully.
  @retval other             Error occurred adding UART node.

**/
STATIC
EFI_STATUS
AddUartDevice (
  IN PLATFORM_TOPOLOGY           *Topo,
  IN OUT AML_OBJECT_NODE_HANDLE  SbNode
  )
{
  AML_OBJECT_NODE_HANDLE  UartNode;
  EFI_STATUS              Status;

  Status = AmlCodeGenDevice ("COM0", SbNode, &UartNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create COM0 device: %r\n", __func__, Status));
    return Status;
  }

  Status = AmlCodeGenNameString ("_HID", "RSCV0003", UartNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _HID RSCV0003 name: %r\n", __func__, Status));
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_UID", 0, UartNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _UID 0 name: %r\n", __func__, Status));
    return Status;
  }

  Status = AddCrsMemIrq (
             UartNode,
             Topo->UartBase,
             Topo->UartSize,
             &Topo->UartIrq,
             1,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _CRS resource: %r\n", __func__, Status));
  }

  return Status;
}

/**
  Build APLIC nodes.

  @param[in]        Topo      Platform topology.
  @param[in, out]   SbNode    The SB node to add the APLIC nodes to.

  @retval EFI_SUCCESS         APLIC nodes added successfully.
  @retval other               Error occurred adding APLIC nodes.

**/
STATIC
EFI_STATUS
AddAplicDevices (
  IN PLATFORM_TOPOLOGY           *Topo,
  IN OUT AML_OBJECT_NODE_HANDLE  SbNode
  )
{
  EFI_STATUS              Status;
  UINT32                  SocketIdx;
  AML_OBJECT_NODE_HANDLE  IcNode;
  CHAR8                   IcName[5];

  for (SocketIdx = 0; SocketIdx < Topo->NumSockets; SocketIdx++) {
    AsciiSPrint (IcName, sizeof (IcName), "IC%02X", SocketIdx);

    Status = AmlCodeGenDevice (IcName, SbNode, &IcNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create %a device: %r\n", __func__, IcName, Status));
      return Status;
    }

    Status = AmlCodeGenNameString ("_HID", "RSCV0002", IcNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _HID RSCV0002 name: %r\n", __func__, Status));
      return Status;
    }

    Status = AmlCodeGenNameInteger ("_UID", SocketIdx, IcNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _UID %d name: %r\n", __func__, SocketIdx, Status));
      return Status;
    }

    //
    // Name (_GSB, GsiBase) — Global System Interrupt Base
    //
    Status = AmlCodeGenNameInteger (
               "_GSB",
               Topo->Intc[SocketIdx].GsiBase,
               IcNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _GSB %x name: %r\n", __func__, Topo->Intc[SocketIdx].GsiBase, Status));
      return Status;
    }

    Status = AddCrsMemIrq (
               IcNode,
               Topo->Intc[SocketIdx].Base,
               Topo->Intc[SocketIdx].Size,
               0,
               0,
               FALSE
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _CRS resource: %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Build IOMMU nodes. Only one IOMMU node for now.

  @param[in]        Topo    Platform topology.
  @param[in, out]   SbNode  The SB node to add the IOMMU nodes to.

  @retval EFI_SUCCESS       IOMMU node added successfully.
  @retval other             Error occurred adding IOMMU node.

**/
STATIC
EFI_STATUS
AddIoMmuDevices (
  IN PLATFORM_TOPOLOGY           *Topo,
  IN OUT AML_OBJECT_NODE_HANDLE  SbNode
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  IommuNode;
  UINT32                  *Irqs;
  UINT32                  IrqIdx;

  Status = AmlCodeGenDevice ("IMU0", SbNode, &IommuNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create IMU0 device: %r\n", __func__, Status));
    return Status;
  }

  Status = AmlCodeGenNameString ("_HID", "RSCV0004", IommuNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _HID RSCV0004 name: %r\n", __func__, Status));
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_UID", 0, IommuNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _UID 0 name: %r\n", __func__, Status));
    return Status;
  }

  //
  // _CRS: one QWordMemory + IOMMU interrupts discovered from FDT.
  //
  if (Topo->IommuNumIrqs == 0) {
    DEBUG ((DEBUG_ERROR, "%a: IOMMU has no IRQs in topology\n", __func__));
    return EFI_PROTOCOL_ERROR;
  }

  Irqs = AllocatePool (sizeof (UINT32) * Topo->IommuNumIrqs);
  if (Irqs == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for IOMMU IRQs\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  for (IrqIdx = 0; IrqIdx < Topo->IommuNumIrqs; IrqIdx++) {
    Irqs[IrqIdx] = Topo->IommuIrqBase + IrqIdx;
  }

  Status = AddCrsMemIrq (
             IommuNode,
             Topo->IommuBase,
             Topo->IommuSize,
             Irqs,
             (UINT8)Topo->IommuNumIrqs,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _CRS resource: %r\n", __func__, Status));
  }

  return EFI_SUCCESS;
}

/**
  Build PCIe Host Bridge nodes. Only one host bridge is supported for now.

  @param[in]        Topo    Platform topology.
  @param[in, out]   SbNode  The SB node to add the PCIe Host Bridge nodes to.

  @retval EFI_SUCCESS       PCIe Host Bridge nodes added successfully.
  @retval other             Error occurred adding PCIe Host Bridge nodes.

**/
STATIC
EFI_STATUS
AddPcieHostBridgeDevices (
  IN PLATFORM_TOPOLOGY           *Topo,
  IN OUT AML_OBJECT_NODE_HANDLE  SbNode
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  PcieNode;
  AML_OBJECT_NODE_HANDLE  CrsNode;

  if (!Topo->PcieEcamBase) {
    DEBUG ((DEBUG_ERROR, "%a: PcieEcamBase is zero\n", __func__));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenDevice ("PCI0", SbNode, &PcieNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create PCI0 device: %r\n", __func__, Status));
    return Status;
  }

  //
  // _HID "PNP0A03"
  //
  Status = AmlCodeGenNameString ("_HID", "PNP0A03", PcieNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _HID PNP0A03 name: %r\n", __func__, Status));
    return Status;
  }

  //
  // _CID "PNP0A08" — PCIe compatible ID, advertises PCIe capability.
  //
  Status = AmlCodeGenNameString ("_CID", "PNP0A08", PcieNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _CID PNP0A08 name: %r\n", __func__, Status));
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_UID", 0, PcieNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _UID 0 name: %r\n", __func__, Status));
    return Status;
  }

  //
  // _SEG 0 — PCI segment group number.
  //
  Status = AmlCodeGenNameInteger ("_SEG", 0, PcieNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _SEG 0 name: %r\n", __func__, Status));
    return Status;
  }

  //
  // _BBN — base bus number of this root bridge.
  //
  Status = AmlCodeGenNameInteger ("_BBN", Topo->PcieBusMin, PcieNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _BBN %d name: %r\n", __func__, Topo->PcieBusMin, Status));
    return Status;
  }

  //
  // _PRT — PCI Interrupt Routing Table, references \_SB.L000..\_SB.L003.
  //
  Status = AddPciePrt (PcieNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _PRT resource: %r\n", __func__, Status));
    return Status;
  }

  //
  // _CRS: bus range + I/O aperture + ECAM window + MMIO apertures.
  //
  Status = AmlCodeGenNameResourceTemplate ("_CRS", PcieNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create _CRS resource: %r\n", __func__, Status));
    return Status;
  }

  //
  // WordBusNumber: [PcieBusMin .. PcieBusMax] from PciHostBridgeLib.
  //
  Status = AmlCodeGenRdWordBusNumber (
             FALSE,                                              // ResourceProducer
             TRUE,                                               // MinFixed
             TRUE,                                               // MaxFixed
             FALSE,                                              // ISAOnlyRanges
             0,                                                  // AddressGranularity
             (UINT16)Topo->PcieBusMin,                           // AddressMinimum
             (UINT16)Topo->PcieBusMax,                           // AddressMaximum
             0,                                                  // AddressTranslation
             (UINT16)(Topo->PcieBusMax - Topo->PcieBusMin + 1),  // RangeLength
             0,
             NULL,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create WordBusNumber resource: %r\n", __func__, Status));
    return Status;
  }

  //
  // QWordMemory: ECAM configuration space window, ResourceConsumer.
  //
  Status = AmlCodeGenRdQWordMemory (
             TRUE,                    // ResourceConsumer
             TRUE,                    // PosDecode
             TRUE,                    // MinFixed
             TRUE,                    // MaxFixed
             AmlMemoryNonCacheable,
             TRUE,                    // ReadWrite
             0,                       // Granularity
             Topo->PcieEcamBase,
             Topo->PcieEcamBase + Topo->PcieEcamSize - 1,
             0,                       // TranslationOffset
             Topo->PcieEcamSize,
             0,
             NULL,
             AmlAddressRangeMemory,
             TRUE,                    // TypeStatic
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create QWordMemory resource: %r\n", __func__, Status));
    return Status;
  }

  //
  // DWordIo: I/O aperture, ResourceProducer.
  //
  if (Topo->PcieIoSize != 0) {
    Status = AmlCodeGenRdDWordIo (
               FALSE,                                              // ResourceProducer
               TRUE,                                               // MinFixed
               TRUE,                                               // MaxFixed
               FALSE,                                              // ISAOnlyRanges
               FALSE,                                              // EntireRange
               0,                                                  // AddressGranularity
               (UINT32)Topo->PcieIoBase,                           // AddressMinimum (PCI-side)
               (UINT32)(Topo->PcieIoBase + Topo->PcieIoSize - 1),  // AddressMaximum
               (UINT32)Topo->PcieIoTranslation,                    // AddressTranslation
               (UINT32)Topo->PcieIoSize,                           // RangeLength
               0,
               NULL,
               TRUE,
               TRUE,                                // TypeStatic
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create DWordIo resource: %r\n", __func__, Status));
      return Status;
    }
  }

  //
  // QWordMemory: 32-bit MMIO aperture, ResourceProducer.
  //
  if (Topo->PcieMmio32Size != 0) {
    Status = AmlCodeGenRdQWordMemory (
               FALSE,                   // ResourceProducer
               TRUE,                    // PosDecode
               TRUE,                    // MinFixed
               TRUE,                    // MaxFixed
               AmlMemoryNonCacheable,
               TRUE,                    // ReadWrite
               0,                       // Granularity
               Topo->PcieMmio32Base,
               Topo->PcieMmio32Base + Topo->PcieMmio32Size - 1,
               0,                       // TranslationOffset
               Topo->PcieMmio32Size,
               0,
               NULL,
               AmlAddressRangeMemory,
               TRUE,                    // TypeStatic
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create QWordMemory 32-bit MMIO resource: %r\n", __func__, Status));
      return Status;
    }
  }

  //
  // QWordMemory: 64-bit MMIO aperture, ResourceProducer.
  //
  if (Topo->PcieMmio64Size != 0) {
    Status = AmlCodeGenRdQWordMemory (
               FALSE,                   // ResourceProducer
               TRUE,                    // PosDecode
               TRUE,                    // MinFixed
               TRUE,                    // MaxFixed
               AmlMemoryCacheable,
               TRUE,                    // ReadWrite
               0,                       // Granularity
               Topo->PcieMmio64Base,
               Topo->PcieMmio64Base + Topo->PcieMmio64Size - 1,
               0,                       // TranslationOffset
               Topo->PcieMmio64Size,
               0,
               NULL,
               AmlAddressRangeMemory,
               TRUE,                    // TypeStatic
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AML: Failed to create QWordMemory 64-bit MMIO resource: %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

// ---------------------------------------------------------------------------
// DSDT entry point
// ---------------------------------------------------------------------------

/**
  Build and install the Differentiated System Description Table (DSDT).

  @param[in]  AcpiTable   The ACPI table protocol.
  @param[in]  Topo        Platform topology.

  @retval EFI_SUCCESS     DSDT installed.
  @retval other           Installation failed.
**/
EFI_STATUS
EFIAPI
InstallDsdt (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  )
{
  EFI_STATUS                   Status;
  UINTN                        TableKey;
  AML_ROOT_NODE_HANDLE         RootNode;
  AML_OBJECT_NODE_HANDLE       SbNode;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;

  RootNode = NULL;
  Table    = NULL;

  Status = AmlCodeGenDefinitionBlock (
             "DSDT",
             ACPI_OEM_ID,
             ACPI_OEM_TABLE_ID,
             0x00000001,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: AmlCodeGenDefinitionBlock failed: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Scope (\_SB) {}
  //
  Status = AmlCodeGenScope ("\\_SB", RootNode, &SbNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: AmlCodeGenScope failed: %r\n",
      __func__,
      Status
      ));
    goto Done;
  }

  // -------------------------------------------------------------------------
  // CPU devices — one per hart.
  // -------------------------------------------------------------------------
  Status = AddCpuDevices (Topo, SbNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: AddCpuDevices failed: %r\n",
      __func__,
      Status
      ));
    goto Done;
  }

  // -------------------------------------------------------------------------
  // UART device — one per platform.
  // -------------------------------------------------------------------------
  Status = AddUartDevice (Topo, SbNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: AddCpuUart failed: %r\n",
      __func__,
      Status
      ));
    goto Done;
  }

  // -------------------------------------------------------------------------
  // APLIC devices — one per socket.
  // -------------------------------------------------------------------------
  Status = AddAplicDevices (Topo, SbNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: AddAplicDevices failed: %r\n",
      __func__,
      Status
      ));
    goto Done;
  }

  // -------------------------------------------------------------------------
  // IOMMU device - one per platform for now.
  // -------------------------------------------------------------------------
  if (Topo->IommuBase) {
    Status = AddIoMmuDevices (Topo, SbNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: AddIoMmuDevices failed: %r\n",
        __func__,
        Status
        ));
      goto Done;
    }
  }

  if (Topo->PcieEcamBase != 0) {
    // -------------------------------------------------------------------------
    // PCI Interrupt Link devices.
    // -------------------------------------------------------------------------
    Status = AddPcieLinkDevices (SbNode, Topo->PcieLinkGsi);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: AddPcieLinkDevices failed: %r\n",
        __func__,
        Status
        ));
      goto Done;
    }

    // -------------------------------------------------------------------------
    // PCIe host bridge. Only one host bridge supported for now.
    // -------------------------------------------------------------------------
    Status = AddPcieHostBridgeDevices (Topo, SbNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: AddPcieHostBridgeDevices failed: %r\n",
        __func__,
        Status
        ));
      goto Done;
    }
  }

  // -------------------------------------------------------------------------
  // Serialize the AML tree to a flat table buffer and install
  // -------------------------------------------------------------------------
  Status = AmlSerializeDefinitionBlock (RootNode, &Table);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: AmlSerializeDefinitionBlock failed: %r\n",
      __func__,
      Status
      ));
    goto Done;
  }

  AcpiTableChecksum ((UINT8 *)Table, Table->Length);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)Table,
                        Table->Length,
                        &TableKey
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: InstallAcpiTable failed: %r\n",
      __func__,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: DSDT installed (%u harts)\n",
      __func__,
      (UINT32)Topo->NumHarts
      ));
  }

  FreePool (Table);

Done:
  if (RootNode != NULL) {
    AmlDeleteTree (RootNode);
  }

  return Status;
}
