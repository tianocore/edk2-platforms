/** @file
  ACPI table generation for the RISC-V platform.

    FADT  - Fixed ACPI Description Table (HW-reduced profile)
    MADT  - Multiple APIC Description Table
              RINTC structure        (per hart)
              IMSIC structure        (one per platform)
              APLIC structure        (one per socket)
    MCFG  - PCI Express Memory Mapped Configuration Space Base Address Description Table
    RHCT  - RISC-V Hart Capabilities Table
              ISA string node  (type 0)
              CMO node         (type 1)
              MMU node         (type 2)
              Hart Info node   (type 0xFFFF) per hart
    SPCR  - Serial Port Console Redirection Table
    DSDT  - Differentiated System Description Table
              CPU devices (ACPI0007)
              UART (RSCV0003)
              APLIC interrupt controller devices (RSCV0002)
              IOMMU (RSCV0004)
              PCIe host bridge

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <PiDxe.h>

#include <Register/RiscV64/RiscVEncoding.h>
#include <IndustryStandard/Acpi66.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/TimerLib.h>

#include <Library/PciHostBridgeLib.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FdtClient.h>

#define RISCV_MAX_SOCKETS   16
#define RISCV_MAX_HARTS     512

// ---------------------------------------------------------------------------
// Per-socket interrupt controller resource
// ---------------------------------------------------------------------------

typedef struct {
  UINT64    Base;          ///< MMIO base address from FDT reg
  UINT64    Size;          ///< MMIO size from FDT reg
  UINT32    NumSources;    ///< riscv,num-sources (APLIC)
  UINT32    GsiBase;       ///< Accumulated GSI base for this socket
} INTC_RESOURCE;

// ---------------------------------------------------------------------------
// Platform topology discovered from the FDT
// ---------------------------------------------------------------------------

typedef struct {
  //
  // CPU / hart topology
  //
  UINTN            NumHarts;                        ///< Total hart count
  UINT64           HartIds[RISCV_MAX_HARTS];        ///< Hart IDs (arch_id)
  UINT32           SocketId[RISCV_MAX_HARTS];       ///< NUMA socket per hart
  UINT32           LocalCpuId[RISCV_MAX_HARTS];     ///< Hart index within socket
  UINT32           NumSockets;                      ///< Number of NUMA sockets
  UINT32           HartsPerSocket[RISCV_MAX_HARTS]; ///< Hart count per socket

  //
  // ISA / extension capabilities
  //
  UINT64           TimebaseFrequency;            ///< cpus timebase-frequency
  CHAR8            IsaString[1024];              ///< ISA string from first hart
  BOOLEAN          HasZicbom;                    ///< Zicbom present
  BOOLEAN          HasZicboz;                    ///< Zicboz present
  UINT32           CbomBlockSize;                ///< log2 of CBOM block size
  UINT32           CbozBlockSize;                ///< log2 of CBOZ block size
  UINT8            MmuType;                      ///< 0=Sv39,1=Sv48,2=Sv57, 0xFF=no MMU

  //
  // Interrupt architecture
  //
  UINT32           AiaGuests;                    ///< IMSIC guest count
  UINT64           ImsicBase;                    ///< S-mode IMSIC base (FDT reg)
  UINT64           ImsicSize;                    ///< S-mode IMSIC size (FDT reg)
  UINT64           ImsicGroupMaxSize;            ///< Per-socket IMSIC stride
  UINT32           ImsicGroupShift;              ///< riscv,group-index-shift
  UINT32           ImsicNumIds;                  ///< riscv,num-ids (supervisor)
  INTC_RESOURCE    Intc[RISCV_MAX_SOCKETS];      ///< Per-socket APLIC

  //
  // UART
  //
  UINT64           UartBase;                     ///< UART MMIO base
  UINT64           UartSize;                     ///< UART MMIO size
  UINT32           UartIrq;                      ///< UART GSI

  //
  // IOMMU
  //
  UINT64           IommuBase;                    ///< IOMMU MMIO base
  UINT64           IommuSize;                    ///< IOMMU MMIO size
  UINT32           IommuIrqBase;                 ///< First IOMMU IRQ from FDT
  UINT32           IommuNumIrqs;                 ///< Number of IOMMU IRQs from FDT

  //
  // PCIe host bridge (only one supported)
  //
  UINT64           PcieEcamBase;                ///< ECAM base  (= PcdPciExpressBaseAddress)
  UINT64           PcieEcamSize;                ///< ECAM size  (= (Bus.Limit+1) << 20)
  UINT64           PcieIoBase;                  ///< PCI I/O aperture base  (PCI-side; 0 = absent)
  UINT64           PcieIoSize;                  ///< PCI I/O aperture size
  UINT64           PcieIoTranslation;           ///< CPU address = PCI address + Translation
  UINT64           PcieMmio32Base;              ///< 32-bit MMIO aperture base  (0 = absent)
  UINT64           PcieMmio32Size;              ///< 32-bit MMIO aperture size
  UINT64           PcieMmio64Base;              ///< 64-bit MMIO aperture base  (0 = absent)
  UINT64           PcieMmio64Size;              ///< 64-bit MMIO aperture size
  UINT32           PcieBusMin;                  ///< First bus number
  UINT32           PcieBusMax;                  ///< Last bus number
  UINT32           PcieLinkGsi[4];              ///< GSIs for INTx lines INTA..INTD (from FDT interrupt-map)
} PLATFORM_TOPOLOGY;

// ---------------------------------------------------------------------------
// OEM identifiers
// ---------------------------------------------------------------------------

#define ACPI_OEM_ID            "PLATRV"
#define ACPI_OEM_TABLE_ID      "RISCVDYN"
#define ACPI_CREATOR_ID        "UEFI"
#define ACPI_CREATOR_REVISION  0x00000001

// ---------------------------------------------------------------------------
// Helper macro: fill EFI_ACPI_DESCRIPTION_HEADER
// ---------------------------------------------------------------------------

#define ACPI_HEADER(Sig, Rev)                                     \
  {                                                               \
    Sig,                          /* Signature */                 \
    0,                            /* Length (filled later) */     \
    (Rev),                        /* Revision */                  \
    0,                            /* Checksum (filled later) */   \
    { ACPI_OEM_ID },              /* OemId */                     \
    { ACPI_OEM_TABLE_ID },        /* OemTableId */                \
    0x00000001,                   /* OemRevision */               \
    { ACPI_CREATOR_ID },          /* CreatorId */                 \
    ACPI_CREATOR_REVISION         /* CreatorRevision */           \
  }

// ---------------------------------------------------------------------------
// Helper APIs to build ACPI tables
// ---------------------------------------------------------------------------

/**
  Compute the number of bits needed to represent values 0..N-1.
  Returns 0 for N <= 1.

  @param[in]  N  Upper bound (exclusive).

  @return  Bit width required.
**/
UINT8
EFIAPI
ImsicNumBits (
  IN UINT32  N
  );

/**
  Generate the bytes for an RINTC structure.

  @param[in]  Topo             Platform topology.
  @param[in]  Rintc            RINTC structure to fill.
  @param[in]  GuestIndexBits   Number of bits in the IMSIC guest index
  @param[in]  HartIdx          Index of the hart for which to generate bytes.

**/
VOID
EFIAPI
GenerateRintcBytes (
  IN PLATFORM_TOPOLOGY             *Topo,
  IN EFI_ACPI_6_6_RINTC_STRUCTURE  *Rintc,
  IN UINT8                         GuestIndexBits,
  IN UINTN                         HartIdx
  );

// ---------------------------------------------------------------------------
// Forward declarations for table builders
// ---------------------------------------------------------------------------

EFI_STATUS
EFIAPI
DiscoverPlatformTopology (
  OUT PLATFORM_TOPOLOGY  *Topo
  );

EFI_STATUS
EFIAPI
InstallFadt (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  );

EFI_STATUS
EFIAPI
InstallMadt (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  );

EFI_STATUS
EFIAPI
InstallRhct (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  );

EFI_STATUS
EFIAPI
InstallMcfg (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  );

EFI_STATUS
EFIAPI
InstallRimt (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  );

EFI_STATUS
EFIAPI
InstallSpcr (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  );

EFI_STATUS
EFIAPI
InstallDsdt (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  );

// ---------------------------------------------------------------------------
// AcpiTable protocol wrapper and checksumming helpers
// ---------------------------------------------------------------------------

VOID
EFIAPI
AcpiTableChecksum (
  IN OUT UINT8  *Buffer,
  IN     UINTN  Size
  );

EFI_STATUS
EFIAPI
AllocateAndInstallAcpiTable (
  IN  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN  VOID                     *TableData,
  IN  UINTN                    TableSize,
  OUT UINTN                    *TableKey
  );
