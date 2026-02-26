;------------------------------------------------------------------------------
;
; Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;  PeiCoreEntry.nasm
;
; Abstract:
;
;   Find and call SecStartup
;
;------------------------------------------------------------------------------

SECTION .text

extern ASM_PFX(SecStartup)
extern ASM_PFX(PlatformInit)
extern ASM_PFX(PcdGet64 (PcdFspWrapperBfvforResetVectorInFsp))

;-----------------------------------------------------------------------------
;  Macro:        PUSHA_64
;
;  Description:  Saves all registers on stack
;
;  Input:        None
;
;  Output:       None
;-----------------------------------------------------------------------------
%macro PUSHA_64   0
  push    r8
  push    r9
  push    r10
  push    r11
  push    r12
  push    r13
  push    r14
  push    r15
  push    rax
  push    rcx
  push    rdx
  push    rbx
  push    rsp
  push    rbp
  push    rsi
  push    rdi
%endmacro

;-----------------------------------------------------------------------------
;  Macro:        POPA_64
;
;  Description:  Restores all registers from stack
;
;  Input:        None
;
;  Output:       None
;-----------------------------------------------------------------------------
%macro POPA_64   0
  pop    rdi
  pop    rsi
  pop    rbp
  pop    rsp
  pop    rbx
  pop    rdx
  pop    rcx
  pop    rax
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  pop    r11
  pop    r10
  pop    r9
  pop    r8
%endmacro

global ASM_PFX(CallPeiCoreEntryPoint)
ASM_PFX(CallPeiCoreEntryPoint):
  ;
  ; Per X64 calling convention, make sure RSP is 16-byte aligned.
  ;
  mov     rax, rsp
  and     rax, 0fh
  sub     rsp, rax

  ;
  ; Platform init
  ;
  PUSHA_64
  sub     rsp, 20h
  call    ASM_PFX(PlatformInit)
  add     rsp, 20h
  POPA_64

  ;
  ; Set stack top pointer
  ;
  mov     rsp, r8

  ;
  ; Push the hob list pointer
  ;
  push    rcx

  ;
  ; RBP holds start of BFV passed from Vtf0. Save it to r10.
  ;
  mov     r10, rbp

  ;
  ; Save the value
  ;   RDX: start of range
  ;   r8: end of range
  ;
  mov     rbp, rsp
  push    rdx
  push    r8
  mov     r14, rdx
  mov     r15, r8

  ;
  ; Push processor count to stack first, then BIST status (AP then BSP)
  ;
  mov     eax, 1
  cpuid
  shr     ebx, 16
  and     ebx, 0000000FFh
  cmp     bl, 1
  jae     PushProcessorCount

  ;
  ; Some processors report 0 logical processors.  Effectively 0 = 1.
  ; So we fix up the processor count
  ;
  inc     ebx

PushProcessorCount:
  sub     rsp, 4
  mov     rdi, rsp
  mov     DWORD [rdi], ebx

  ;
  ; We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  ; for all processor threads
  ;
  xor     ecx, ecx
  mov     cl, bl
PushBist:
  sub     rsp, 4
  mov     rdi, rsp
  movd    eax, mm0
  mov     DWORD [rdi], eax
  loop    PushBist

  ;
  ; FSP saves the timestamp of the beginning of firmware execution in mm5.
  ; Get the timestamp from mm5 and then push to stack.
  ;
  movq    rax, mm5
  push    rax

  ;
  ; Per X64 calling convention, make sure RSP is 16-byte aligned.
  ;
  mov     rax, rsp
  and     rax, 0fh
  sub     rsp, rax

  ;
  ; Pass entry point of the PEI core
  ;
  mov     rdi, 0FFFFFFE0h
  mov     edi, DWORD [rdi]
  mov     r9, rdi

  ;
  ; Pass BFV into the PEI Core
  ;
#if FixedPcdGetBool (PcdFspWrapperResetVectorInFsp) == 1
  ;
  ; Reset Vector and initial SEC core (to initialize Temp Ram) is part of FSP-O.
  ; Default UefiCpuPkg Reset Vector locates FSP-O as BFV. However the actual
  ; SEC core that launches PEI is part of another FV. We need to pass that FV
  ; as BFV to PEI core.
  ;
  mov     r8, ASM_PFX (PcdGet64 (PcdFspWrapperBfvforResetVectorInFsp))
  mov     rcx, QWORD[r8]
  mov     r8,  rcx
#else
  mov     r8, r10
#endif

  ;
  ; Pass stack size into the PEI Core
  ;
  mov     rcx, r15  ; Start of TempRam
  mov     rdx, r14  ; End of TempRam

  sub     rcx, rdx  ; Size of TempRam

  ;
  ; Pass Control into the PEI Core
  ;
  sub     rsp, 20h
  call    ASM_PFX(SecStartup)

