/** @file

  This header file contains declarations and definitions for Boot shared
  internal memory cookies.

  These cookies are shared between boot and other subsystems.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - DLOAD - Qualcomm Download Mode
    - EDL   - Emergencey Download Mode
    - ETB   - Enhanced Trace Buffer
    - QSEE  - Qualcomm Secure Execution Environment
    - RPM   - Resource Power Manager
    - SBL   - Secondary Boot Loader

**/

#pragma once

/*
 * Following magic number indicates the boot shared imem region is initialized
 * and the content is valid
 */
#define BOOT_SHARED_IMEM_MAGIC_NUM  0xC1F8DB40

/*
 * Version number to indicate what cookie structure is being used
 */
#define BOOT_SHARED_IMEM_VERSION_NUM  0x4

/*
 * Magic number for UEFI crash dump
 */
#define UEFI_CRASH_DUMP_MAGIC_NUM  0x1

/*
 * Abnormal reset occured
 */
#define ABNORMAL_RESET_ENABLED  0x1

/*
 * Magic number for raw ram dump
 */
#define BOOT_RAW_RAM_DUMP_MAGIC_NUM  0x2

/* Default value used to initialize shared imem region */
#define SHARED_IMEM_REGION_DEF_VAL  0xFFFFFFFF

/*
 * Magic number for RPM sync
 */
#define BOOT_RPM_SYNC_MAGIC_NUM  0x112C3B1C

/* Shared IMEM offset and magic number used by QSEE to indicate to
   XBL that it needs to be backed up for delayed ramdump scenario */
#define QSEE_DLOAD_DUMP_SHARED_IMEM_OFFSET     0x748
#define QSEE_DLOAD_DUMP_SHARED_IMEM_MAGIC_NUM  0x44554d50

/* Shared IMEM offset and magic number used by QHEE to indicate to
   XBL that it needs to be backed up for delayed ramdump scenario.
   Use magic number 0x44554d50 instead of legacy 0xC1F8DB42 to enable
   dumping multiple TZ segments instead of legacy TZ binary blob
*/
#define QHEE_DLOAD_DUMP_SHARED_IMEM_MAGIC_NUM  0x44554d50
#define QHEE_DLOAD_DUMP_SHARED_IMEM_OFFSET     0xb20

#define BOOT_DUMP_NUM_ENTRIES    6
#define DLOAD_DUMP_INFO_VERSION  1

#ifdef FEATURE_TPM_HASH_POPULATE
/* offset in shared imem to store image tp hash */
#define SHARED_IMEM_TPM_HASH_REGION_OFFSET  0x834
#define SHARED_IMEM_TPM_HASH_REGION_SIZE    256

#define SMEM_TPM_HASH_REGION_SIZE  1024

#endif /* FEATURE_TPM_HASH_POPULATE */

/*
 * Following structure defines all the cookies that have been placed
 * in boot's shared imem space.
 * The size of this struct must NOT exceed SHARED_IMEM_BOOT_SIZE
 */
typedef struct {
  /* Magic number which indicates boot shared imem has been initialized
     and the content is valid.*/
  UINT32    SharedImemMagic;

  /* Number to indicate what version of this structure is being used */
  UINT32    SharedImemVersion;

  /* Pointer that points to etb ram dump buffer, should only be set by HLOS */
  UINT64    EtbBufAddr;

  /* Region where HLOS will write the l2 cache dump buffer start address */
  UINT64    L2CacheDumpBuffAddr;

  /* When SBL which is A32 allocates the 64bit pointer above it will only
     consume 4 bytes.  When HLOS running in A64 mode access this it will over
     flow into the member below it.  Adding this padding will ensure 8 bytes
     are consumed so A32 and A64 have the same view of the remaining members. */
  UINT32    A64PointerPadding;

  /* Magic number for UEFI ram dump, if this cookie is set along with dload magic numbers,
     we don't enter dload mode but continue to boot. This cookie should only be set by UEFI*/
  UINT32    UefiRamDumpMagic;

  UINT32    DdrTrainingCookie;

  /* Abnormal reset cookie used by UEFI */
  UINT32    AbnormalResetOccurred;

  /* Reset Status Register */
  UINT32    ResetStatusRegister;

  /* Cookie that will be used to sync with RPM */
  UINT32    RpmSyncCookie;

  /* Debug config used by UEFI */
  UINT32    DebugConfig;

  /* Boot Log Location Pointer to be accessed in UEFI */
  UINT64    BootLogAddr;

  /* Boot Log Size */
  UINT32    BootLogSize;

  /* Boot failure count */
  UINT32    BootFailCount;

  /* Error code delivery through EDL */
  UINT32    Sbl1ErrorType;

  /* Cookie to detect if crash occurred in UEFI or HLOS */
  UINT32    UefiImageMagic;

  /* Boot Device option */
  UINT32    BootDeviceType;

  /* Boot DT address in DDR*/
  UINT64    BootDevtreeAddr;

  /* Boot DT size */
  UINT64    BootDevtreeSize;

  /* Boot set for recovery boot */
  UINT32    BootSetType;

  /* Please add new cookie here, do NOT modify or rearrange the existing cookies*/
} BOOT_SHARED_IMEM_COOKIE_TYPE;
