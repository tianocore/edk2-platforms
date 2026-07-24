/** @file
  FDT topology discovery for the RISC-V ACPI driver.

    Walks the Flattened Device Tree to extract:
    - Hart count, Hart IDs, NUMA socket assignments
    - IMSIC base address, group stride, num-ids
    - timebase-frequency read directly
    - ISA string, Zicbom/Zicboz, MMU type
    - UART base, size, IRQ
    - RISC-V IOMMU base, size, IRQs
    - PCIe apertures via PciHostBridgeLib (not re-parsed from FDT)

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

#define IMSIC_MMIO_GROUP_MIN_SHIFT  24
#define RISCV_IRQ_S_EXT             9u
#define RISCV_IRQ_M_EXT             11u

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/**
  Read a big-endian UINT32 from a raw FDT property byte stream at Offset.
**/
STATIC
UINT32
FdtReadU32Be (
  IN CONST VOID  *Data
  )
{
  return SwapBytes32 (*(CONST UINT32 *)Data);
}

/**
  Read a big-endian UINT64 from a raw FDT property byte stream at Offset.
**/
STATIC
UINT64
FdtReadU64Be (
  IN CONST VOID  *Data
  )
{
  return SwapBytes64 (*(CONST UINT64 *)Data);
}

/**
  Read a (address-cells, size-cells) register pair from a FDT "reg" property.

  Both address-cells and size-cells are assumed to be 2 (64-bit), which is
  the standard for RISC-V platforms.  Each cell is a big-endian UINT32 pair
  that together form a UINT64.

  @param[in]   Prop         Pointer to the raw property data.
  @param[in]   PropSize     Total byte length of the property.
  @param[in]   EntryIndex   Zero-based index of the reg entry to read.
  @param[out]  Base         Receives the base address.
  @param[out]  Size         Receives the size.

  @retval EFI_SUCCESS       Entry read successfully.
  @retval EFI_NOT_FOUND     EntryIndex is out of range.
**/
STATIC
EFI_STATUS
FdtReadReg (
  IN  CONST VOID  *Prop,
  IN  UINT32      PropSize,
  IN  UINTN       EntryIndex,
  OUT UINT64      *Base,
  OUT UINT64      *Size
  )
{
  CONST UINT8  *P;
  UINTN        EntrySize;

  //
  // Each reg entry is 2 address cells + 2 size cells = 4 x UINT32 = 16 bytes.
  //
  EntrySize = 16;

  if ((EntryIndex + 1) * EntrySize > PropSize) {
    return EFI_NOT_FOUND;
  }

  P     = (CONST UINT8 *)Prop + EntryIndex * EntrySize;
  *Base = ((UINT64)FdtReadU32Be (P) << 32) | FdtReadU32Be (P + 4);
  *Size = ((UINT64)FdtReadU32Be (P + 8) << 32) | FdtReadU32Be (P + 12);
  return EFI_SUCCESS;
}

/**
  Read the first interrupt number from a FDT "interrupts" or
  "interrupts-extended" property.

  For RISC-V APLIC/PLIC platforms the interrupt cell count is 1 (plain
  "interrupts") or 2 ("interrupts-extended": phandle + irq).  We only
  need the IRQ number, so we read the last UINT32 of the first entry.

  @param[in]   Prop      Raw property data.
  @param[in]   PropSize  Byte length of the property.
  @param[in]   Extended  TRUE if this is "interrupts-extended" (phandle+irq).
  @param[out]  Irq       Receives the interrupt number.

  @retval EFI_SUCCESS    IRQ read.
  @retval EFI_NOT_FOUND  Property too short.
**/
STATIC
EFI_STATUS
FdtReadFirstIrq (
  IN  CONST VOID  *Prop,
  IN  UINT32      PropSize,
  IN  BOOLEAN     Extended,
  OUT UINT32      *Irq
  )
{
  CONST UINT8  *P = (CONST UINT8 *)Prop;

  if (Extended) {
    //
    // interrupts-extended: [phandle(4)] [irq(4)] ...
    //
    if (PropSize < 8) {
      return EFI_NOT_FOUND;
    }

    *Irq = FdtReadU32Be (P + 4);
  } else {
    //
    // interrupts: [irq(4)] ...
    //
    if (PropSize < 4) {
      return EFI_NOT_FOUND;
    }

    *Irq = FdtReadU32Be (P);
  }

  return EFI_SUCCESS;
}

/**
  Return TRUE if the ISA string contains the given underscore-prefixed
  extension token (e.g. "zicbom" matches "_zicbom" in the ISA string).
**/
STATIC
BOOLEAN
IsaHasExtension (
  IN CONST CHAR8  *Isa,
  IN CONST CHAR8  *Ext
  )
{
  CONST CHAR8  *P;
  UINTN        ExtLen;

  if ((Isa == NULL) || (Ext == NULL)) {
    return FALSE;
  }

  ExtLen = AsciiStrLen (Ext);
  P      = Isa;

  while (*P != '\0') {
    if ((*P == '_') && (AsciiStrnCmp (P + 1, Ext, ExtLen) == 0)) {
      CHAR8  Next = P[1 + ExtLen];
      if ((Next == '\0') || (Next == '_') || (Next == ',')) {
        return TRUE;
      }
    }

    P++;
  }

  return FALSE;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
  Discover the platform topology from the Flattened Device Tree.

  @param[out]  Topo  Receives the discovered topology.

  @retval EFI_SUCCESS           Topology populated successfully.
  @retval EFI_NOT_FOUND         FDT_CLIENT_PROTOCOL not available.
  @retval EFI_PROTOCOL_ERROR    Required FDT nodes or properties missing.
  @retval EFI_BUFFER_TOO_SMALL  More harts than RISCV_MAX_HARTS.
**/
EFI_STATUS
EFIAPI
DiscoverPlatformTopology (
  OUT PLATFORM_TOPOLOGY  *Topo
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  INT32                CpuNode;
  CONST VOID           *Prop;
  UINT32               PropSize;
  UINTN                HartIndex;
  BOOLEAN              FirstHart;
  UINT32               SocketIdx;
  UINTN                SatpMode;

  //
  // Initialize the topology structure to zero.  We'll fill in fields as we discover them.
  //
  ZeroMem (Topo, sizeof (*Topo));

  SatpMode = (RiscVGetSupervisorAddressTranslationRegister () & SATP64_MODE) >> SATP64_MODE_SHIFT;
  switch (SatpMode) {
    case SATP_MODE_SV39:
      Topo->MmuType =  EFI_ACPI_6_6_RHCT_MMU_TYPE_SV39;
      break;
    case SATP_MODE_SV48:
      Topo->MmuType =  EFI_ACPI_6_6_RHCT_MMU_TYPE_SV48;
      break;
    case SATP_MODE_SV57:
      Topo->MmuType =  EFI_ACPI_6_6_RHCT_MMU_TYPE_SV57;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Unsupported SATP mode %u\n", __func__, SatpMode));
      return EFI_PROTOCOL_ERROR;
  }

  //
  // Locate FDT client.
  //
  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: FDT_CLIENT_PROTOCOL not found: %r\n", __func__, Status));
    return Status;
  }

  // -------------------------------------------------------------------------
  // timebase-frequency.
  // -------------------------------------------------------------------------
  Topo->TimebaseFrequency = GetPerformanceCounterProperties (NULL, NULL);
  if (Topo->TimebaseFrequency == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read timebase-frequency\n", __func__));
    return EFI_PROTOCOL_ERROR;
  }

  // -------------------------------------------------------------------------
  // CPU nodes — hart IDs, socket IDs, ISA.
  // -------------------------------------------------------------------------
  HartIndex = 0;
  FirstHart = TRUE;

  Status = FdtClient->FindCompatibleNode (FdtClient, "riscv", &CpuNode);
  while (!EFI_ERROR (Status)) {
    UINT64  HartId;
    UINT32  SocketId;

    if (HartIndex >= RISCV_MAX_HARTS) {
      DEBUG ((DEBUG_ERROR, "%a: Too many harts (max %u)\n", __func__, RISCV_MAX_HARTS));
      return EFI_BUFFER_TOO_SMALL;
    }

    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          CpuNode,
                          "reg",
                          &Prop,
                          &PropSize
                          );
    if (EFI_ERROR (Status)) {
      goto NextCpu;
    }

    if (PropSize == sizeof (UINT32)) {
      HartId = FdtReadU32Be (Prop);
    } else if (PropSize == sizeof (UINT64)) {
      HartId = FdtReadU64Be (Prop);
    } else {
      goto NextCpu;
    }
    Topo->HartIds[HartIndex] = HartId;

    SocketId = 0;
    Status   = FdtClient->GetNodeProperty (
                            FdtClient,
                            CpuNode,
                            "numa-node-id",
                            &Prop,
                            &PropSize
                            );
    if (!EFI_ERROR (Status) && (PropSize == sizeof (UINT32))) {
      SocketId = FdtReadU32Be (Prop);
    }

    if (SocketId >= Topo->NumSockets) {
      if ((SocketId >= RISCV_MAX_SOCKETS) || (Topo->NumSockets >= RISCV_MAX_SOCKETS)) {
        DEBUG ((DEBUG_ERROR, "%a: Too many sockets (max %u)\n", __func__, RISCV_MAX_SOCKETS));
        return EFI_BUFFER_TOO_SMALL;
      }

      Topo->NumSockets = SocketId + 1;
    }

    Topo->SocketId[HartIndex] = SocketId;
    Topo->HartsPerSocket[SocketId]++;
    Topo->LocalCpuId[HartIndex] = Topo->HartsPerSocket[SocketId] - 1;

    if (FirstHart) {
      Status = FdtClient->GetNodeProperty (
                            FdtClient,
                            CpuNode,
                            "riscv,isa",
                            &Prop,
                            &PropSize
                            );
      if (!EFI_ERROR (Status) && (PropSize > 0)) {
        AsciiStrnCpyS (
          &Topo->IsaString[0],
          sizeof (Topo->IsaString),
          (CONST CHAR8 *)Prop,
          PropSize
          );
      }

      Topo->HasZicbom = IsaHasExtension (Topo->IsaString, "zicbom");
      Topo->HasZicboz = IsaHasExtension (Topo->IsaString, "zicboz");
      Status          = FdtClient->GetNodeProperty (
                                     FdtClient,
                                     CpuNode,
                                     "riscv,cbom-block-size",
                                     &Prop,
                                     &PropSize
                                     );
      if (!EFI_ERROR (Status) && (PropSize == sizeof (UINT32))) {
        UINT32  Bs = FdtReadU32Be (Prop);
        Topo->CbomBlockSize = (Bs > 0) ? (UINT32)HighBitSet32 (Bs) : 0;
      }

      Status = FdtClient->GetNodeProperty (
                            FdtClient,
                            CpuNode,
                            "riscv,cboz-block-size",
                            &Prop,
                            &PropSize
                            );
      if (!EFI_ERROR (Status) && (PropSize == sizeof (UINT32))) {
        UINT32  Bs = FdtReadU32Be (Prop);
        Topo->CbozBlockSize = (Bs > 0) ? (UINT32)HighBitSet32 (Bs) : 0;
      }

      FirstHart = FALSE;
    }

    HartIndex++;

NextCpu:
    Status = FdtClient->FindNextCompatibleNode (FdtClient, "riscv", CpuNode, &CpuNode);
  }

  Topo->NumHarts = HartIndex;
  if (Topo->NumHarts == 0) {
    DEBUG ((DEBUG_ERROR, "%a: No CPU nodes found in FDT\n", __func__));
    return EFI_PROTOCOL_ERROR;
  }

  if (Topo->NumSockets == 0) {
    Topo->NumSockets = 1;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: %u harts, %u sockets, ISA='%a', timebase=%Lu\n",
    __func__,
    (UINT32)Topo->NumHarts,
    Topo->NumSockets,
    Topo->IsaString,
    Topo->TimebaseFrequency
    ));

  // -------------------------------------------------------------------------
  // Per-socket interrupt controller resources.
  // -------------------------------------------------------------------------

  //
  // Find the S-mode IMSIC node.
  //
  // QEMU creates two IMSIC nodes with compatible "riscv,imsics":
  //   1. M-mode IMSIC  — interrupts-extended uses IRQ_M_EXT (11)
  //   2. S-mode IMSIC  — interrupts-extended uses IRQ_S_EXT (9)
  //
  // FindCompatibleNode returns the M-mode node first.  We must skip it and
  // use the S-mode node, identified by its interrupts-extended entries
  // referencing IRQ_S_EXT (9) rather than IRQ_M_EXT (11).
  //
  // IRQ_S_EXT = 9, IRQ_M_EXT = 11  (RISC-V privilege spec)
  //
  Status = FdtClient->FindCompatibleNode (FdtClient, "riscv,imsics", &Node);
  while (!EFI_ERROR (Status)) {
    CONST VOID  *IeProp;
    UINT32      IeSize;
    UINT32      IrqType;
    //
    // Each interrupts-extended entry is [phandle(4)][irq(4)] = 8 bytes.
    // Read the IRQ type from the first entry (offset 4).
    //
    IrqType = RISCV_IRQ_M_EXT;  // assume M-mode until proven otherwise
    if (!EFI_ERROR (
           FdtClient->GetNodeProperty (
                        FdtClient,
                        Node,
                        "interrupts-extended",
                        &IeProp,
                        &IeSize
                        )
           ) && (IeSize >= 8))
    {
      IrqType = FdtReadU32Be ((CONST UINT8 *)IeProp + 4);
    }

    if (IrqType == RISCV_IRQ_S_EXT) {
      break;  // found the S-mode node
    }

    Status = FdtClient->FindNextCompatibleNode (
                          FdtClient,
                          "riscv,imsics",
                          Node,
                          &Node
                          );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No S-mode 'riscv,imsics' DT node found\n",
      __func__
      ));
    return EFI_PROTOCOL_ERROR;
  } else {
    CONST VOID  *RegProp;
    UINT32      RegSize;
    UINT64      ImsicBase;
    UINT64      ImsicSize;

    //
    // Read riscv,num-ids (supervisor interrupt identities).
    //
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "riscv,num-ids",
                          &Prop,
                          &PropSize
                          );
    if (!EFI_ERROR (Status) && (PropSize == sizeof (UINT32))) {
      Topo->ImsicNumIds = FdtReadU32Be (Prop);
    } else {
      DEBUG ((DEBUG_ERROR, "%a: riscv,num-ids missing\n", __func__));
      return EFI_PROTOCOL_ERROR;
    }

    //
    // Read riscv,guest-index-bits to derive AiaGuests.
    //
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "riscv,guest-index-bits",
                          &Prop,
                          &PropSize
                          );
    if (!EFI_ERROR (Status) && (PropSize == sizeof (UINT32))) {
      UINT32  GuestIndexBits = FdtReadU32Be (Prop);
      Topo->AiaGuests = (GuestIndexBits > 0) ? ((1u << GuestIndexBits) - 1) : 0;
    } else {
      Topo->AiaGuests = 0;
    }

    //
    // Read riscv,group-index-shift.
    // Always read from FDT when present; fall back to
    // IMSIC_MMIO_GROUP_MIN_SHIFT (24) as required by the AIA spec.
    //
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "riscv,group-index-shift",
                          &Prop,
                          &PropSize
                          );
    if (!EFI_ERROR (Status) && (PropSize == sizeof (UINT32))) {
      Topo->ImsicGroupShift = FdtReadU32Be (Prop);
    } else if (Topo->NumSockets > 1) {
      DEBUG ((DEBUG_ERROR, "%a: riscv,group-index-shift missing for multi-socket\n", __func__));
      return EFI_PROTOCOL_ERROR;
    } else {
      Topo->ImsicGroupShift = IMSIC_MMIO_GROUP_MIN_SHIFT;
    }

    //
    // Read the first reg entry as the S-mode IMSIC base.
    // The reg property for a multi-socket IMSIC has one entry per socket;
    // we take the first entry as the global base and derive the group stride
    // from riscv,group-index-shift.
    //
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "reg",
                          &RegProp,
                          &RegSize
                          );
    if (!EFI_ERROR (Status)) {
      if (!EFI_ERROR (FdtReadReg (RegProp, RegSize, 0, &ImsicBase, &ImsicSize))) {
        Topo->ImsicBase         = ImsicBase;
        Topo->ImsicSize         = ImsicSize;
        Topo->ImsicGroupMaxSize = (UINT64)1 << Topo->ImsicGroupShift;
      } else {
        DEBUG ((DEBUG_ERROR, "%a: Failed to read IMSIC reg\n", __func__));
        return EFI_PROTOCOL_ERROR;
      }
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: AIA=APLIC+IMSIC, ImsicBase=0x%lx, ImsicSize=0x%lx, ImsicGroupMaxSize=0x%lx, NumIds=%u, Guests=%u\n",
      __func__,
      Topo->ImsicBase,
      Topo->ImsicSize,
      Topo->ImsicGroupMaxSize,
      Topo->ImsicNumIds,
      Topo->AiaGuests
      ));
  }

  //
  // Walk all S-mode APLIC nodes (one per socket) for APLIC
  //
  SocketIdx = 0;
  Status    = FdtClient->FindCompatibleNode (FdtClient, "riscv,aplic", &Node);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: No 'riscv,aplic' compatible DT node found (%r)\n", __func__, Status));
    return EFI_PROTOCOL_ERROR;
  }

  while (!EFI_ERROR (Status)) {
    CONST VOID  *RegProp;
    UINT32      RegSize;
    UINT64      AplicBase;
    UINT64      AplicSize;
    UINT32      NumSources;
    BOOLEAN     IsMMode;

    //
    // Skip M-mode APLIC nodes — they have no "msi-parent" and their
    // "riscv,children" points to the S-mode node.  We identify M-mode
    // nodes by the presence of "riscv,children".
    //
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "riscv,children",
                          &Prop,
                          &PropSize
                          );
    IsMMode = !EFI_ERROR (Status);

    if (!IsMMode) {
      //
      // S-mode APLIC: read reg and riscv,num-sources.
      //
      Status = FdtClient->GetNodeProperty (
                            FdtClient,
                            Node,
                            "reg",
                            &RegProp,
                            &RegSize
                            );
      if (!EFI_ERROR (Status) &&
          !EFI_ERROR (FdtReadReg (RegProp, RegSize, 0, &AplicBase, &AplicSize)))
      {
        NumSources = 0;
        Status     = FdtClient->GetNodeProperty (
                                  FdtClient,
                                  Node,
                                  "riscv,num-sources",
                                  &Prop,
                                  &PropSize
                                  );
        if (!EFI_ERROR (Status) && (PropSize == sizeof (UINT32))) {
          NumSources = FdtReadU32Be (Prop);
        }

        //
        // Assign to the next available socket slot.
        //
        if (SocketIdx < RISCV_MAX_SOCKETS) {
          Topo->Intc[SocketIdx].Base       = AplicBase;
          Topo->Intc[SocketIdx].Size       = AplicSize;
          Topo->Intc[SocketIdx].NumSources = NumSources;
          Topo->Intc[SocketIdx].GsiBase    = NumSources * SocketIdx;
          SocketIdx++;
        }

        DEBUG ((
          DEBUG_INFO,
          "%a: APLIC[%u] base=0x%lx size=0x%lx sources=%u\n",
          __func__,
          SocketIdx - 1,
          AplicBase,
          AplicSize,
          NumSources
          ));
      }
    }

    Status = FdtClient->FindNextCompatibleNode (FdtClient, "riscv,aplic", Node, &Node);
  }

  // -------------------------------------------------------------------------
  // UART — compatible "ns16550a" or "snps,dw-apb-uart".
  // -------------------------------------------------------------------------
  Status = FdtClient->FindCompatibleNode (FdtClient, "ns16550a", &Node);
  if (EFI_ERROR (Status)) {
    Status = FdtClient->FindCompatibleNode (FdtClient, "snps,dw-apb-uart", &Node);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: No 'ns16550a' or 'snps,dw-apb-uart' compatible DT node found (%r)\n", __func__, Status));
    return EFI_PROTOCOL_ERROR;
  } else {
    CONST VOID  *RegProp;
    UINT32      RegSize;

    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "reg",
                          &RegProp,
                          &RegSize
                          );
    if (!EFI_ERROR (Status)) {
      FdtReadReg (RegProp, RegSize, 0, &Topo->UartBase, &Topo->UartSize);
    }

    //
    // Read interrupt number from "interrupts" (plain) or
    // "interrupts-extended".
    //
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "interrupts",
                          &Prop,
                          &PropSize
                          );
    if (!EFI_ERROR (Status)) {
      FdtReadFirstIrq (Prop, PropSize, FALSE, &Topo->UartIrq);
    } else {
      Status = FdtClient->GetNodeProperty (
                            FdtClient,
                            Node,
                            "interrupts-extended",
                            &Prop,
                            &PropSize
                            );
      if (!EFI_ERROR (Status)) {
        FdtReadFirstIrq (Prop, PropSize, TRUE, &Topo->UartIrq);
      }
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: UART base=0x%lx size=0x%lx irq=%u\n",
      __func__,
      Topo->UartBase,
      Topo->UartSize,
      Topo->UartIrq
      ));
  }

  // -------------------------------------------------------------------------
  // RISC-V IOMMU — compatible "riscv,iommu".
  // -------------------------------------------------------------------------
  Status = FdtClient->FindCompatibleNode (FdtClient, "riscv,iommu", &Node);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: No 'riscv,iommu' compatible DT node found (%r)\n", __func__, Status));
  } else {
    CONST VOID  *RegProp;
    UINT32      RegSize;

    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "reg",
                          &RegProp,
                          &RegSize
                          );
    if (!EFI_ERROR (Status)) {
      FdtReadReg (RegProp, RegSize, 0, &Topo->IommuBase, &Topo->IommuSize);
    }

    //
    // Read IOMMU interrupts and derive how many IRQs are exposed.
    //
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "interrupts",
                          &Prop,
                          &PropSize
                          );
    if (!EFI_ERROR (Status)) {
      if (!EFI_ERROR (FdtReadFirstIrq (Prop, PropSize, FALSE, &Topo->IommuIrqBase))) {
        Topo->IommuNumIrqs = PropSize / sizeof (UINT32);
      }
    } else {
      Status = FdtClient->GetNodeProperty (
                            FdtClient,
                            Node,
                            "interrupts-extended",
                            &Prop,
                            &PropSize
                            );
      if (!EFI_ERROR (Status)) {
        if (!EFI_ERROR (FdtReadFirstIrq (Prop, PropSize, TRUE, &Topo->IommuIrqBase))) {
          Topo->IommuNumIrqs = PropSize / (2 * sizeof (UINT32));
        }
      }
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: IOMMU base=0x%lx size=0x%lx irq_base=%u num_irqs=%u\n",
      __func__,
      Topo->IommuBase,
      Topo->IommuSize,
      Topo->IommuIrqBase,
      Topo->IommuNumIrqs
      ));
  }

  // -------------------------------------------------------------------------
  // PCIe — read apertures from PciHostBridgeLib, not from FDT directly.
  // Read interrupt-map from FDT to populate PcieLinkGsi[4].
  // -------------------------------------------------------------------------
  {
    UINTN            RbCount;
    PCI_ROOT_BRIDGE  *Rb;

    Rb = PciHostBridgeGetRootBridges (&RbCount);
    if ((Rb == NULL) || (RbCount == 0)) {
      //
      // No PCIe host bridge present — leave all PCIe fields zero.
      //
      DEBUG ((DEBUG_INFO, "%a: No PCIe host bridge found\n", __func__));
    } else {
      //
      // We support exactly one root bridge (segment 0).
      //
      Topo->PcieBusMin = (UINT32)Rb[0].Bus.Base;
      Topo->PcieBusMax = (UINT32)Rb[0].Bus.Limit;

      Topo->PcieEcamBase = PcdGet64 (PcdPciExpressBaseAddress);
      Topo->PcieEcamSize = (UINT64)(Topo->PcieBusMax - Topo->PcieBusMin + 1) << 20;

      if (Rb[0].Io.Base <= Rb[0].Io.Limit) {
        Topo->PcieIoBase        = Rb[0].Io.Base;
        Topo->PcieIoSize        = Rb[0].Io.Limit - Rb[0].Io.Base + 1;
        Topo->PcieIoTranslation = PcdGet64 (PcdPciIoTranslation);
      }

      if (Rb[0].Mem.Base <= Rb[0].Mem.Limit) {
        Topo->PcieMmio32Base = Rb[0].Mem.Base;
        Topo->PcieMmio32Size = Rb[0].Mem.Limit - Rb[0].Mem.Base + 1;
      }

      if (Rb[0].MemAbove4G.Base <= Rb[0].MemAbove4G.Limit) {
        Topo->PcieMmio64Base = Rb[0].MemAbove4G.Base;
        Topo->PcieMmio64Size = Rb[0].MemAbove4G.Limit - Rb[0].MemAbove4G.Base + 1;
      }

      PciHostBridgeFreeRootBridges (Rb, RbCount);

      DEBUG ((
        DEBUG_VERBOSE,
        "%a: PCIe Bus[%u..%u] ECAM=0x%lx/0x%lx "
        "IO=0x%lx/0x%lx(+0x%lx) MMIO32=0x%lx/0x%lx MMIO64=0x%lx/0x%lx\n",
        __func__,
        Topo->PcieBusMin,
        Topo->PcieBusMax,
        Topo->PcieEcamBase,
        Topo->PcieEcamSize,
        Topo->PcieIoBase,
        Topo->PcieIoSize,
        Topo->PcieIoTranslation,
        Topo->PcieMmio32Base,
        Topo->PcieMmio32Size,
        Topo->PcieMmio64Base,
        Topo->PcieMmio64Size
        ));
    }
  }

  // -------------------------------------------------------------------------
  // PCIe interrupt-map — parse INTA..INTD GSIs from the FDT pcie node.
  //
  // QEMU's interrupt-map for a PCIe node with an APLIC parent has
  // 7 cells per entry (all big-endian UINT32):
  //   [0..2]  child unit address  (3 cells: phys.hi, phys.mid, phys.lo)
  //   [3]     child interrupt     (pin: 1=INTA, 2=INTB, 3=INTC, 4=INTD)
  //   [4]     parent phandle      (1 cell)
  //   [5]     parent interrupt    (1 cell: GSI number)
  //   [6]     Value  4            (1 cell)
  //   Total: 7 x UINT32 = 28 bytes per entry.
  //
  // We record the first GSI seen for each pin (1..4) into
  // PcieLinkGsi[pin-1], which gives the correct base GSIs for L000..L003.
  // -------------------------------------------------------------------------
  Status = FdtClient->FindCompatibleNode (FdtClient, "pci-host-ecam-generic", &Node);
  if (EFI_ERROR (Status)) {
    Status = FdtClient->FindCompatibleNode (FdtClient, "pci-host-cam-generic", &Node);
  }

  if (!EFI_ERROR (Status)) {
    CONST UINT8  *Map;
    UINT32       MapSize;
    UINT32       EntrySize;
    UINT32       Offset;
    UINTN        Pin;

    //
    // 7 cells per entry x 4 bytes per cell
    //
    EntrySize = 7 * sizeof (UINT32);

    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "interrupt-map",
                          (CONST VOID **)&Map,
                          &MapSize
                          );
    if (!EFI_ERROR (Status) && (MapSize >= (EntrySize * 4))) {
      for (Pin = 1; Pin <= 4; Pin++) {
        Offset = (Pin - 1) * EntrySize;
        //
        // Cell [5]: parent interrupt (GSI).
        //
        Topo->PcieLinkGsi[Pin - 1] = FdtReadU32Be (Map + Offset + 5 * sizeof (UINT32));
        DEBUG ((
          DEBUG_INFO,
          "%a: PCIe INT%c GSI=%u\n",
          __func__,
          (CHAR8)('A' + Pin - 1),
          Topo->PcieLinkGsi[Pin - 1]
          ));
      }
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Failed to read PCIe interrupt-map\n", __func__));
      return EFI_PROTOCOL_ERROR;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a: No PCIe FDT node\n", __func__));
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}
