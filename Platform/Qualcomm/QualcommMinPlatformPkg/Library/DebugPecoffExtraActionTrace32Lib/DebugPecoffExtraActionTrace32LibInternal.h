/** @file
  Internal definitions for the Trace32 PE/COFF Extra Action Library.

  Defines the DDR buffer layout, CMM header structure, and helper macros
  used by DebugPecoffExtraActionTrace32Lib.

  Copyright (C) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Library/PcdLib.h>

#define ADDRESS_FORMAT_STRING  "0x%016lx"

// DDR Buffer Configuration
#define CMM_DDR_BASE       FixedPcdGet64 (PcdTrace32DdrBase)
#define CMM_DDR_SIZE       FixedPcdGet64 (PcdTrace32DdrSize)
#define CMM_MAGIC          0x314D4D43U     /* 'CMM1' */
#define CMM_FLAG_OVERFLOW  BIT0            /* Script buffer overflowed */
#define SCRIPT_INIT_STR    "local &Sf\r\n\r\n"

/* ============================================================
   Memory Layout - Firmware Buffer
   ============================================================
   Region       Offset              Size        Owner / Lifecycle
   --------------------------------------------------------
   Stop Driver  0x0000 - 0x003F     64 bytes    Written by TRACE32 before boot;
                                                read by firmware to identify the
                                                module to stop at; never cleared
                                                by firmware.
   Trap Flag    0x0040 - 0x0041      2 bytes    Written as ASCII "TF" by TRACE32
                                                to make firmware spin in
                                                BreakPointFunction; cleared by
                                                TRACE32 to release the CPU; never
                                                cleared by firmware.
   [padding]    0x0042 - 0x0047      6 bytes    Reserved; 8-byte alignment for Header.
   Header       0x0048 - 0x0057     16 bytes    Zeroed by firmware on first use
                                                (CmmInitIfNeeded); holds CMM
                                                magic, version, write offset,
                                                and overflow flag.
   Script       0x0058 - 0x3757    ~13.7KB      Append-only CMM script buffer;
                                                written by firmware, read by
                                                TRACE32 to load symbols.
   Scratchpad   0x3758 - 0x3B97    1088 bytes   Temporary buffers used by
                                                firmware during script
                                                generation; not read by TRACE32.
   ============================================================ */

// ----- TRACE32-owned Region (written by TRACE32, never cleared by firmware) -----
#define STOP_DRIVER_OFFSET  0x0000
#define STOP_DRIVER_SIZE    64

#define TRAP_FLAG_OFFSET  (STOP_DRIVER_OFFSET + STOP_DRIVER_SIZE)
#define TRAP_FLAG_SIZE    2

#define PADDING_OFFSET  (TRAP_FLAG_OFFSET + TRAP_FLAG_SIZE)
#define PADDING_SIZE    6              /* 8-byte alignment for Header */

// ----- Firmware-initialized Region (zeroed by CmmInitIfNeeded on first use) -----
#define HEADER_OFFSET  (PADDING_OFFSET + PADDING_SIZE)
#define HEADER_SIZE    16

// ----- Script Buffer -----
#define SCRIPT_DATA_OFFSET  (HEADER_OFFSET + HEADER_SIZE)
#define SCRIPT_DATA_SIZE    0x3700          /* ~13.7KB */

// ----- Scratchpad Region (1088 bytes) -----
#define SCRATCH_OFFSET  (SCRIPT_DATA_OFFSET + SCRIPT_DATA_SIZE)
#define SCRATCH_SIZE    0x440               /* 1088 bytes */

#define SCRATCH_CMD_OFFSET  (SCRATCH_OFFSET)
#define SCRATCH_CMD_SIZE    512

#define SCRATCH_PATH_OFFSET  (SCRATCH_CMD_OFFSET + SCRATCH_CMD_SIZE)
#define SCRATCH_PATH_SIZE    512

#define SCRATCH_PKG_OFFSET  (SCRATCH_PATH_OFFSET + SCRATCH_PATH_SIZE)
#define SCRATCH_PKG_SIZE    64

typedef struct {
  UINT32    Magic;        ///< CMM magic number identifying an initialized buffer
  UINT32    Version;      ///< Header version number
  UINT32    WriteOffset;  ///< Current write position in the script buffer
  UINT32    Flags;        ///< Status flags; bit0 = overflow
} CMM_DDR_HEADER;

/**
  Compute a pointer into the DDR buffer at the given byte offset.

  @param[in]  Offset  Byte offset from CMM_DDR_BASE.

  @return  Pointer to the DDR location at the specified offset.
**/
#define DDR_PTR(Offset)  ((VOID *)(CMM_DDR_BASE + (Offset)))

#if defined (__GNUC__)
/// Compiler hint to prevent inlining of the decorated function (GCC).
#define NOINLINE  __attribute__((noinline))
#elif defined (_MSC_VER)
/// Compiler hint to prevent inlining of the decorated function (MSVC).
#define NOINLINE  __declspec(noinline)
#else
/// Compiler hint to prevent inlining of the decorated function (fallback).
#define NOINLINE
#endif
