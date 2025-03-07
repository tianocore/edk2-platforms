;------------------------------------------------------------------------------
; @file
; Sets the CR3 register for 64-bit paging
;
; Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
; Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; Modified:  EAX
;
SetCr3ForPageTables64:

    ;
    ; These pages are built into the ROM image in X64/PageTables.asm
    ;
    mov     eax, ADDR_OF(TopLevelPageDirectory)
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64

