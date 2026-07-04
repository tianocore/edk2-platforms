/** @file
  Internal header for the RISC-V IOMMU DXE driver.

  Defines the RISC-V IOMMU MMIO register layout as specified in the
  RISC-V IOMMU Architecture Specification (version 1.0), and the
  internal driver context structure.

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Protocol/IoMmu.h>
#include <Protocol/FdtClient.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

// ---------------------------------------------------------------------------
// RISC-V IOMMU MMIO register offsets
// (RISC-V IOMMU Spec 1.0, Section 5 - Register Interface)
// ---------------------------------------------------------------------------

///
/// capabilities (RO) - offset 0x000
/// Reports hardware capabilities: version, supported modes, etc.
///
#define RISCV_IOMMU_REG_CAPABILITIES  0x000

///
/// ddtp (RW) - offset 0x010
/// Device Directory Table Pointer.
/// Bits [3:0] = iommu_mode field.
/// Bits [63:10] = PPN of the root DDT page (when mode != Bare/Off).
///
#define RISCV_IOMMU_REG_DDTP  0x010

// ---------------------------------------------------------------------------
// ddtp.iommu_mode field values (bits [3:0] of ddtp register)
// (RISC-V IOMMU Spec 1.0, Table 5.5)
// ---------------------------------------------------------------------------

///
/// Bare: No address translation or protection.
/// Device physical address == host physical address.
/// All DMA transactions pass through without restriction.
/// This is the mode used during firmware/boot.
///
#define RISCV_IOMMU_DDTP_MODE_BARE  0x1ULL

///
/// Mask for the iommu_mode field in ddtp (bits [3:0])
///
#define RISCV_IOMMU_DDTP_MODE_MASK  0xFULL

///
/// Busy bit in ddtp (bit 4): set by hardware while processing a ddtp write.
/// Software must poll until this bit clears before issuing another ddtp write.
///
#define RISCV_IOMMU_DDTP_BUSY  BIT4

// ---------------------------------------------------------------------------
// capabilities register field masks
// ---------------------------------------------------------------------------

///
/// capabilities.version field (bits [7:0])
///
#define RISCV_IOMMU_CAP_VERSION_MASK  0xFFULL

// ---------------------------------------------------------------------------
// Timeout for ddtp.busy polling (in microseconds)
// ---------------------------------------------------------------------------
#define RISCV_IOMMU_DDTP_BUSY_TIMEOUT_US  1000000    ///< 1 second

// ---------------------------------------------------------------------------
// MAP_INFO: tracks an active DMA mapping
// ---------------------------------------------------------------------------

#define RISCV_IOMMU_MAP_INFO_SIGNATURE  SIGNATURE_32 ('R','I','M','I')

///
/// Tracks a single active DMA mapping created by RiscVIoMmuMap().
/// In bare mode, DeviceAddress == HostAddress (no translation).
///
typedef struct {
  ///
  /// Signature for validation in Unmap()
  ///
  UINT32                   Signature;

  ///
  /// Link in the global mMapInfoList
  ///
  LIST_ENTRY               Link;

  ///
  /// The DMA operation type (read / write / common buffer)
  ///
  EDKII_IOMMU_OPERATION    Operation;

  ///
  /// Original host virtual address passed to Map()
  ///
  EFI_PHYSICAL_ADDRESS     HostAddress;

  ///
  /// Device-visible address (== HostAddress in bare mode)
  ///
  EFI_PHYSICAL_ADDRESS     DeviceAddress;

  ///
  /// Number of bytes mapped
  ///
  UINTN                    NumberOfBytes;
} RISCV_IOMMU_MAP_INFO;

// ---------------------------------------------------------------------------
// Driver context
// ---------------------------------------------------------------------------

#define RISCV_IOMMU_CONTEXT_SIGNATURE  SIGNATURE_32 ('R','I','O','M')

typedef struct {
  UINT32                  Signature;

  ///
  /// Virtual address of the IOMMU MMIO register base
  ///
  UINT64                  *RegisterBase;

  ///
  /// Cached capabilities register value read at init
  ///
  UINT64                  Capabilities;

  ///
  /// The EDKII_IOMMU_PROTOCOL instance installed on the handle
  ///
  EDKII_IOMMU_PROTOCOL    IoMmu;

  ///
  /// List of all active MAP_INFO structures
  ///
  LIST_ENTRY              MapInfoList;
} RISCV_IOMMU_CONTEXT;

EFI_STATUS
EFIAPI
RiscVIoMmuEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
