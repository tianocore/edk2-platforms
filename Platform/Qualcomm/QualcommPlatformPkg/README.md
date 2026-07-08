# Qualcomm Pre-UEFI Boot ABI

This document describes the interfaces that UEFI firmware depends on from
earlier boot stages on Qualcomm SoCs. These interfaces are established by
XBL (eXtensible Boot Loader) before UEFI executes and must be maintained
across all supported targets.

---

## Glossary

| Term   | Meaning |
|--------|---------|
| ABL    | Android Boot Loader |
| APPBL  | Application Processor Boot Loader |
| HLOS   | High Level Operating System (e.g., Linux/Android) |
| IMEM   | Internal Memory — a small on-chip SRAM region used for inter-subsystem communication |
| PASR   | Partial Array Self Refresh |
| QHEE   | Qualcomm Hypervisor Execution Environment |
| QSEE   | Qualcomm Secure Execution Environment |
| RPM    | Resource Power Manager |
| SBL    | Secondary Boot Loader |
| SMEM   | Shared Memory — a DDR region used for inter-processor communication |
| TZ     | TrustZone |
| XBL    | eXtensible Boot Loader — Qualcomm primary boot loader |

---

## Boot Chain Overview

```
Power-on
  └─► XBL (eXtensible Boot Loader)
        ├─ Initializes SMEM region in DDR
        ├─ Populates RAM Partition Table in SMEM
        ├─ Writes IMEM cookies to on-chip SRAM
        └─► UEFI (QualcommPlatformPkg / QualcommSiliconPkg)
              ├─ Reads SMEM via SmemLib
              ├─ Reads RAM Partition Table from SMEM
              ├─ Reads IMEM cookies for boot state
              └─► HLOS (Linux/Android)
```

UEFI is a consumer of all three interfaces described below. It does not
re-initialize them; it reads what XBL has already established.

---

## 1. Shared Memory (SMEM)

### Purpose

SMEM is a DDR region that acts as a message-passing bus between the
Application Processor (UEFI/HLOS), Modem, DSPs, RPM, TrustZone, and other
subsystems. Each item in SMEM is identified by a numeric ID from the
`SMEM_MEM_TYPE` enum defined in `Silicon/Qualcomm/QualcommSiliconPkg/Include/SmemType.h`.

### Hardware Configuration

UEFI locates SMEM using platform-specific PCDs:

| PCD | Type | Description |
|-----|------|-------------|
| `PcdSmemBaseAddress` | `UINT64` | Physical base address of the SMEM DDR region |
| `PcdSmemSize` | `UINT32` | Size of the SMEM region in bytes |
| `PcdSmemMaxItems` | `UINT16` | Maximum number of allocatable SMEM items |
| `PcdSmemTcsrBase` | `UINT32` | Base address of TCSR registers (spinlock support) |
| `PcdSmemMutexRegBase` | `UINT32` | Base address of hardware mutex registers |
| `PcdSmemWonceRegBase` | `UINT32` | Base address of Write-Once registers |
| `PcdSmemTargetInfoWonceReg` | `UINT32` | Offset of target info register within WONCE space |

### Initialization Contract

XBL initializes SMEM before transferring control to UEFI. UEFI calls
`SmemInit()` early in `ArmPlatformGetVirtualMemoryMap()` to set up its
local view of the SMEM region. UEFI must not call `SmemInit()` more than
once.

### SMEM Version Protocol

SMEM uses a versioning scheme to detect incompatible changes between
processors. The version ID `SMEM_VERSION_ID = 0x000C0000` encodes:

```
Bits [31:16]  Major version — incompatible change if mismatched
Bits [15:0]   Minor version — compatible API additions/deprecations
```

A major version mismatch between processors prevents SMEM from booting.
A minor version mismatch is tolerated.

### UEFI Usage

UEFI uses SMEM exclusively to retrieve the RAM Partition Table (see §2).
The relevant SMEM item ID is `SmemUsableRamPartitionTable` (value `402`).

```c
// Retrieve RAM partition table pointer from SMEM
RamPartitionTable = SmemGetAddr(SmemUsableRamPartitionTable, &BufferSize);
```

### Compatibility Requirements

- The physical address and size of SMEM **must not change** between XBL and
  UEFI for a given target. They are fixed at SoC integration time.
- The `SMEM_MEM_TYPE` enum values are an ABI shared with all subsystems.
  All entries carry **explicit numeric assignments** to prevent accidental
  value shifts when new entries are added.
- SMEM item IDs used by UEFI (`SmemUsableRamPartitionTable`) must remain
  stable across XBL versions.

---

## 2. RAM Partition Table

### Purpose

The RAM Partition Table describes the DDR memory layout of the SoC. XBL
populates it in SMEM before UEFI runs. UEFI reads it to construct the UEFI
memory map and build HOBs for the DXE phase.

### Location

Stored in SMEM at item ID `SmemUsableRamPartitionTable` (value `402`).

### Structure

Two table formats are supported, selected by the `Version` field in the
table header.

#### Common Header (both versions)

```c
UINT32  Magic1;         // Must equal RAM_PART_MAGIC1 (0x9DA5E0A8)
UINT32  Magic2;         // Must equal RAM_PART_MAGIC2 (0xAF9EC4E2)
UINT32  Version;        // Table format version (1 = v1, ≥2 = current)
UINT32  Reserved1;
UINT32  NumPartitions;  // Number of valid entries that follow
UINT32  Reserved2;      // Padding for 8-byte header alignment
```

#### Version 1 Entry (`RAM_PARTITION_ENTRY_V1`)

Used when `Version == 1`. Produced by older XBL versions (32-bit SBL era).

| Field | Type | Description |
|-------|------|-------------|
| `Name` | `CHAR8[16]` | Partition name string |
| `StartAddress` | `UINT64` | Physical start address |
| `Length` | `UINT64` | Total partition length in bytes |
| `PartitionAttribute` | `UINT32` | Read-only / read-write |
| `PartitionCategory` | `UINT32` | SDRAM, IRAM, IMEM, etc. |
| `PartitionDomain` | `UINT32` | APPS, MODEM, etc. |
| `PartitionType` | `UINT32` | SysMemory, AppsMemory, ToolsFv, etc. |
| `NumPartitions` | `UINT32` | Total partitions on device |
| `HwInfo` | `UINT32` | DDR type and frequency |
| `Reserved4/5` | `UINT32` | Reserved |

#### Current Version Entry (`RAM_PARTITION_ENTRY`, Version ≥ 2)

Extends v1 with additional fields:

| Field | Type | Description |
|-------|------|-------------|
| *(all v1 fields)* | | |
| `HighestBankBit` | `UINT8` | Highest bit corresponding to a DRAM bank |
| `Reserve0/1/2` | `UINT8` | Reserved |
| `MinPasrSize` | `UINT32` | Minimum PASR size in MB |
| `AvailableLength` | `UINT64` | Available (usable) length in bytes |

> **Note:** `AvailableLength` may differ from `Length` when XBL has
> reserved a portion of the partition for its own use.

### Partition Types Used by UEFI

| `PartitionType` | `PartitionCategory` | UEFI Action |
|-----------------|---------------------|-------------|
| `RamPartitionSysMemory` | `RamPartitionSdram` | Added to UEFI memory map as `EfiConventionalMemory` |
| `RamPartitionAppsMemory` | `RamPartitionSdram` | Treated as pre-loaded (reserved) region |
| `RamPartitionToolsFvMemory` | `RamPartitionSdram` | Treated as pre-loaded (reserved) region |
| `RamPartitionQuantumFvMemory` | `RamPartitionSdram` | Treated as pre-loaded (reserved) region |
| `RamPartitionQuestFvMemory` | `RamPartitionSdram` | Treated as pre-loaded (reserved) region |
| `RamPartitionAblMemory` | `RamPartitionSdram` | Treated as pre-loaded (reserved) region |

### Version Handling

```
Version == 1  →  HandlePartitionVer1()  (uses RAM_PARTITION_ENTRY_V1)
Version >= 2  →  InitRamPartitionTableLib() main path (uses RAM_PARTITION_ENTRY)
Version < 1   →  CpuDeadLoop() — deprecated, unsupported
```

### Compatibility Requirements

- Magic numbers `RAM_PART_MAGIC1` and `RAM_PART_MAGIC2` must be present and
  correct. UEFI returns `EFI_NOT_FOUND` if they are absent.
- The `Version` field must be `>= 1`. Version `0` causes `CpuDeadLoop()`.
- The `NumPartitions` field must not exceed `RAM_NUM_PART_ENTRIES` (32).
- `PartitionType` and `PartitionCategory` enum values are a shared ABI.
  Their numeric values must not change between XBL and UEFI versions.
- `AvailableLength` (v2+) is used for system memory sizing; `Length` is
  used for pre-loaded region sizing. XBL must populate both correctly.

---

## 3. Boot Shared IMEM Cookie

### Purpose

The Boot Shared IMEM Cookie is a fixed-layout structure written by XBL/SBL
into on-chip SRAM (IMEM) before UEFI executes. It communicates boot state,
addresses, and configuration flags across the XBL → UEFI → HLOS boundary.

### Location

The structure is mapped at a fixed physical address configured by:

| PCD | Type | Description |
|-----|------|-------------|
| `PcdIMemCookiesBase` | `UINT64` | Physical base address of the IMEM cookie region |
| `PcdIMemCookiesSize` | `UINT64` | Size of the IMEM cookie region in bytes |

The region is mapped as `ARM_MEMORY_REGION_ATTRIBUTE_DEVICE` (non-cacheable,
non-bufferable) in the UEFI page table.

### Validity Check

Before reading any cookie fields, consumers must verify:

```c
Cookie->SharedImemMagic   == QUALCOMM_BOOT_SHARED_IMEM_MAGIC_NUM   // 0xC1F8DB40
Cookie->SharedImemVersion == QUALCOMM_BOOT_SHARED_IMEM_VERSION_NUM // 0x4
```

If either check fails, the cookie region has not been initialized by XBL
and no fields should be trusted.

### Structure Layout (`QUALCOMM_BOOT_SHARED_IMEM_COOKIE_TYPE`)

The structure is **append-only**. Fields must never be reordered or removed.
New fields are added at the end only.

| Offset | Field | Type | Written by | Description |
|--------|-------|------|-----------|-------------|
| +0x00 | `SharedImemMagic` | `UINT32` | XBL | Validity magic (`0xC1F8DB40`) |
| +0x04 | `SharedImemVersion` | `UINT32` | XBL | Structure version (`0x4`) |
| +0x08 | `EtbBufAddr` | `UINT64` | HLOS | ETB RAM dump buffer address |
| +0x10 | `L2CacheDumpBuffAddr` | `UINT64` | HLOS | Last-level cache dump buffer address |
| +0x18 | `A64PointerPadding` | `UINT32` | — | Layout padding (see note below) |
| +0x1C | `UefiRamDumpMagic` | `UINT32` | UEFI | Set to request RAM dump without DLOAD mode |
| +0x20 | `DdrTrainingCookie` | `UINT32` | XBL | DDR training state |
| +0x24 | `AbnormalResetOccurred` | `UINT32` | UEFI | Set on abnormal reset |
| +0x28 | `ResetStatusRegister` | `UINT32` | XBL | Hardware reset status |
| +0x2C | `RpmSyncCookie` | `UINT32` | UEFI | RPM synchronization cookie |
| +0x30 | `DebugConfig` | `UINT32` | UEFI | Debug configuration flags |
| +0x34 | `BootLogAddr` | `UINT64` | XBL | Address of XBL boot log buffer in DDR |
| +0x3C | `BootLogSize` | `UINT32` | XBL | Size of XBL boot log buffer in bytes |
| +0x40 | `BootFailCount` | `UINT32` | XBL | Consecutive boot failure counter |
| +0x44 | `Sbl1ErrorType` | `UINT32` | XBL | Error code for EDL delivery |
| +0x48 | `UefiImageMagic` | `UINT32` | UEFI | Crash location indicator (UEFI vs HLOS) |
| +0x4C | `BootDeviceType` | `UINT32` | XBL | Boot device type |
| +0x50 | `BootDevtreeAddr` | `UINT64` | XBL | Boot device tree (DTB) address in DDR |
| +0x58 | `BootDevtreeSize` | `UINT64` | XBL | Boot device tree size in bytes |
| +0x60 | `BootSetType` | `UINT32` | XBL | Recovery boot indicator |

> **`A64PointerPadding` note:** SBL runs in A32 (AArch32) mode and writes
> only the lower 4 bytes of the 64-bit `L2CacheDumpBuffAddr` field. This
> 4-byte pad ensures that A32 (SBL) and A64 (UEFI/HLOS) processors share
> the same field layout for all subsequent fields.

### DLOAD / Ramdump Signalling

Two additional fixed offsets in the IMEM region (outside the main cookie
structure) are used by QSEE and QHEE to signal XBL that a delayed ramdump
backup is needed:

| Signal | Offset | Magic Value |
|--------|--------|-------------|
| QSEE ramdump request | `QUALCOMM_QSEE_DLOAD_DUMP_SHARED_IMEM_OFFSET` (`0x748`) | `0x44554d50` |
| QHEE ramdump request | `QUALCOMM_QHEE_DLOAD_DUMP_SHARED_IMEM_OFFSET` (`0xB20`) | `0x44554d50` |

These offsets are fixed by the IMEM layout and must not change.

### Compatibility Requirements

- The structure layout is **frozen**. Fields must not be reordered, resized,
  or removed. New fields are appended only.
- `SharedImemMagic` and `SharedImemVersion` must be validated before
  accessing any other field.
- `BootDevtreeAddr` and `BootDevtreeSize` are used by UEFI to locate the
  UEFI device tree (DTB). XBL must populate these before transferring
  control to UEFI.
- The DLOAD/ramdump offsets (`0x748`, `0xB20`) are fixed by the IMEM
  hardware layout and are shared with QSEE and QHEE firmware. They must
  not change.
- The IMEM region must be mapped as device memory (non-cacheable) by UEFI.
  Cached access to IMEM produces undefined behavior.

---
