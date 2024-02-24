/*
  Copyright (c) 2022 Ventana Micro Systems Inc.
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

 */

#include "SecMain.h"

ASM_FUNC (_ModuleEntryPoint)
  /* Use Temp memory as the stack for calling to C code */
  li    a4, FixedPcdGet32 (PcdRiscVSecFvBase)
  li    a5, FixedPcdGet32 (PcdRiscVSecFvSize)

  /* Use Temp memory as the stack for calling to C code */
  add   sp, a4, a5

  call SecStartup
