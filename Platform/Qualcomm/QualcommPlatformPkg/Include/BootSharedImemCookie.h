/** @file

  Declarations and definitions for Boot Shared Internal Memory (IMEM) cookies.

  These cookies are written by the Qualcomm boot subsystem (XBL/SBL) and read
  by UEFI and other subsystems (HLOS, QSEE, QHEE) to communicate boot state
  and configuration across processor boundaries.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - A32   - ARM 32-bit execution state (AArch32)
    - A64   - ARM 64-bit execution state (AArch64)
    - DLOAD - Qualcomm Download Mode
    - EDL   - Emergency Download Mode
    - ETB   - Enhanced Trace Buffer
    - HLOS  - High Level Operating System
    - IMEM  - Internal Memory; a small on-chip SRAM region used for
              inter-image communication during and after boot
    - QHEE  - Qualcomm Hypervisor Execution Environment
    - QSEE  - Qualcomm Secure Execution Environment
    - RPM   - Resource Power Manager
    - SBL   - Secondary Boot Loader
    - XBL   - eXtensible Boot Loader (Qualcomm primary boot loader)

**/

#pragma once

/** Magic number indicating the boot shared IMEM region is initialized
    and the content is valid. */
#define QUALCOMM_BOOT_SHARED_IMEM_MAGIC_NUM  0xC1F8DB40

/** Version number indicating which cookie structure is in use. */
#define QUALCOMM_BOOT_SHARED_IMEM_VERSION_NUM  0x4

/** Magic number for UEFI crash dump. */
#define QUALCOMM_UEFI_CRASH_DUMP_MAGIC_NUM  0x1

/** Abnormal reset occurred. */
#define QUALCOMM_ABNORMAL_RESET_ENABLED  0x1

/** Magic number for raw RAM dump. */
#define QUALCOMM_BOOT_RAW_RAM_DUMP_MAGIC_NUM  0x2

/** Default value used to initialize the shared IMEM region. */
#define QUALCOMM_SHARED_IMEM_REGION_DEF_VAL  0xFFFFFFFF

/** Magic number for RPM sync. */
#define QUALCOMM_BOOT_RPM_SYNC_MAGIC_NUM  0x112C3B1C

#define QUALCOMM_BOOT_DUMP_NUM_ENTRIES    6
#define QUALCOMM_DLOAD_DUMP_INFO_VERSION  1

/** Offset in shared IMEM to store image TPM hash. */
#define QUALCOMM_SHARED_IMEM_TPM_HASH_REGION_OFFSET  0x834
#define QUALCOMM_SHARED_IMEM_TPM_HASH_REGION_SIZE    256

#define QUALCOMM_SMEM_TPM_HASH_REGION_SIZE  1024

/**
  Structure defining all cookies placed in the boot shared IMEM space.

  The size of this struct must NOT exceed SHARED_IMEM_BOOT_SIZE.
  Do NOT modify or rearrange existing fields; add new fields at the end.
**/
typedef struct {
  /** Set by XBL to indicate the shared IMEM region is initialized and valid. */
  UINT32    SharedImemMagic;

  /** Cookie structure version number. */
  UINT32    SharedImemVersion;

  /** ETB RAM dump buffer address; written by HLOS. */
  UINT64    EtbBufAddr;

  /**
    L2 cache dump buffer start address; written by HLOS.
    On Qualcomm SoCs this refers to the last-level cache shared by the
    application processor cluster.
  **/
  UINT64    L2CacheDumpBuffAddr;

  UINT32    Reserved;

  /** Set by UEFI to request a RAM dump without entering DLOAD mode. */
  UINT32    UefiRamDumpMagic;

  UINT32    DdrTrainingCookie;

  /** Set by UEFI to indicate an abnormal reset. */
  UINT32    AbnormalResetOccurred;

  UINT32    ResetStatusRegister;

  /** Used to synchronize with RPM. */
  UINT32    RpmSyncCookie;

  /** Debug configuration used by UEFI. */
  UINT32    DebugConfig;

  /** Address of the boot log buffer; read by UEFI. */
  UINT64    BootLogAddr;

  /** Size of the boot log buffer in bytes. */
  UINT32    BootLogSize;

  /** Number of consecutive boot failures. */
  UINT32    BootFailCount;

  /** Error code delivered through EDL. */
  UINT32    Sbl1ErrorType;

  /** Set to detect whether a crash occurred in UEFI or HLOS. */
  UINT32    UefiImageMagic;

  UINT32    BootDeviceType;

  /** Boot device tree address in DDR. */
  UINT64    BootDevtreeAddr;

  /** Boot device tree size in bytes. */
  UINT64    BootDevtreeSize;

  /** Set for recovery boot. */
  UINT32    BootSetType;

  /* Add new cookies here. Do not modify or rearrange existing fields. */
} QUALCOMM_BOOT_SHARED_IMEM_COOKIE_TYPE;
