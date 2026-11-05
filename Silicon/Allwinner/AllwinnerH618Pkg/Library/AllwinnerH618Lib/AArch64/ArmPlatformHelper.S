/** @file
  ArmPlatformHelper implementation for Allwinner H616/H618 SoCs.

  Copyright (c) 2026, Khai Do <dovankhai97@gmail.com>. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AsmMacroLib.h>

ASM_FUNC(ArmPlatformPeiBootAction)
    //
    // Quiesce MMU and caches before page tables are initialized.
    //
    mrs   x0, CurrentEL
    cmp   x0, #0x8
    b.eq  1f

    // EL1
    mrs   x0, sctlr_el1
    bic   x0, x0, #(1 << 0)             // Disable MMU
    bic   x0, x0, #(1 << 2)             // Disable D-Cache
    msr   sctlr_el1, x0
    isb
    tlbi  vmalle1
    b     2f

    // EL2
1:
    mrs   x0, sctlr_el2
    bic   x0, x0, #(1 << 0)             // Disable MMU
    bic   x0, x0, #(1 << 2)             // Disable D-Cache
    msr   sctlr_el2, x0
    isb
    tlbi  alle2

2:
    dsb   sy
    isb
    ic    iallu
    dsb   sy
    isb
    ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
