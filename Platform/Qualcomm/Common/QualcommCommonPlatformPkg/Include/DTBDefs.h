/** @file
  DtExtnLib public type definitions and constants.

  This file defines DtExtnLib error codes, blob identifiers,
  common macros, and node handle types.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

/*
 * QC/DTB-namespaced error codes. These intentionally start at 1000, well
 * above libfdt's own FDT_ERR_MAX, so they never collide with libfdt's
 * error range. BaseDtFrameworkLib.c has a STATIC_ASSERT enforcing this
 * invariant against libfdt.h's FDT_ERR_MAX - update that check if
 * libfdt's error codes are ever extended.
 */
#define DTB_ERR_NOERROR         0
#define DTB_ERR_NULLPTR         1000
#define DTB_ERR_TRUNCATED       1001
#define DTB_ERR_BUF2SMALL       1002
#define DTB_ERR_NILVALUE        1003
#define DTB_ERR_SDNULL          1004
#define DTB_ERR_BADFORMAT       1005
#define DTB_ERR_BADID           1006
#define DTB_ERR_INPUT_ARG_ERR   1007
#define DTB_ERR_REGIDX          1008
#define DTB_ERR_TARGETIDX       1009
#define DTB_ERR_MEMALLOC        1010
#define DTB_ERR_BLOBID          1011
#define DTB_ERR_NOTSUPPORTED    1012
#define DTB_ERR_NOTREADY        1013
#define DTB_ERR_NODE_DIFFERENT  1014
#define DTB_ERR_SLICE_RANGE     1015
#define DTB_ERR_SLICE_COUNT     1016
#define DTB_ERR_BAD_SELECTOR    1017
#define DTB_ERR_FDTLIB_ERROR    1018
#define DTB_ERR_OVERFLOW        1019
#define DTB_ERR_UNDERFLOW       1020

/* Blob ID */
#define DTB_DEFAULT_BLOB_ID  0
#define DTB_MAX_BLOB_ID      5

/* Init Value for FDT_NODE_HANDLE struct */
#define INIT_FDT_NODE_HANDLE  {NULL,0}

/* fdt_get_reg() size code */
#define DTB_REG_SIZE_NIL   0
#define DTB_REG_SIZE_32    1
#define DTB_REG_SIZE_64    2
#define DTB_REG_SIZE_BLOB  -1

/// Node handle used by all DTFramework node-navigation and property-access APIs.
typedef struct {
  CONST VOID  *Blob;          ///< Pointer to DTB blob, opaque to client.
  INT32       Offset;         ///< Offset of this node within DTB blob.
} FDT_NODE_HANDLE;
