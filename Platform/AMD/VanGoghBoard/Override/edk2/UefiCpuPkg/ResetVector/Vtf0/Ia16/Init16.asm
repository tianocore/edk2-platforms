;------------------------------------------------------------------------------
; @file
; 16-bit initialization code
;
; Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
; Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------


BITS    16

ALIGN   4
SMMResumeInfo: ;; This offset can bie found as 0xFFFFFFF5 + word [0xFFFFFFF3] - 0x10(16)
      DD ADDR_OF_MEM(GDT_BASE)        ; GDT base address
      DW LINEAR_CODE_SEL              ; code segment
      DW LINEAR_SEL                   ; data segment
      DD ADDR_OF_MEM(Main32)          ; Offset of our 32 bit code
      DD SMM_RESUME_SIGNATURE

;
; @param[out] DI    'BP' to indicate boot-strap processor
;
EarlyBspInitReal16:
    mov     di, 'BP'
    jmp     short Main16

;
; @param[out] DI    'AP' to indicate application processor
;
EarlyApInitReal16:
    mov     di, 'AP'
    jmp     short Main16

;
; Modified:  EAX
;
; @param[in]  EAX   Initial value of the EAX register (BIST: Built-in Self Test)
; @param[out] ESP   Initial value of the EAX register (BIST: Built-in Self Test)
;
EarlyInit16:
    ;
    ; ESP -  Initial value of the EAX register (BIST: Built-in Self Test)
    ;
    mov     esp, eax

    debugInitialize

    OneTimeCallRet EarlyInit16

