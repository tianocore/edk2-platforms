;------------------------------------------------------------------------------
; Module Name:
;
;  Flat32.asm
;
; Abstract:
;
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector, and configures the stack.
;
; Copyright (c) 2013-2015 Intel Corporation
; Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

;
; Define assembler characteristics
;

SECTION .text
;
; Include processor definitions
;
%include "Platform.inc"
%include "AmdUefiStackNasm.inc"

;
; CR0 cache control bit definition
;
%define CR0_CACHE_DISABLE       0x040000000
%define CR0_NO_WRITE            0x020000000

%define BIST_VALUE              0

;
; MmioCfgBaserAddr[Enable]
;
%define MMIO_CFG_ENABLE         1

;
; External and public declarations
;  TopOfStack is used by C code
;  SecStartup is the entry point to the C code
; Neither of these names can be modified without
; updating the C code.
;
extern   ASM_PFX(PlatformSecLibStartup)
extern   ASM_PFX(BoardBeforeTempRamInit)
extern   ASM_PFX(BoardAfterTempRamInitWrapper)


%define BSP_HEAP_STACK_BASE     FixedPcdGet32 (PcdTempRamBase)
%define BSP_HEAP_STACK_SIZE     FixedPcdGet32 (PcdTempRamSize)
%define FLASH_IMAGE_SIZE        FixedPcdGet32 (PcdFlashAreaSize)

;
; Contrary to the name, this file contains 16 bit code as well.
;

;----------------------------------------------------------------------------
;
; Procedure:    _ModuleEntryPoint
;
; Input:        None
;
; Output:       None
;
; Destroys:     Assume all registers
;
; Description:
;
;   Transition to non-paged flat-model protected mode from a
;   hard-coded GDT that provides exactly two descriptors.
;   This is a bare bones transition to protected mode only
;   used for a while in PEI and possibly DXE.
;
;   After enabling protected mode, a far jump is executed to
;   transfer to PEI using the newly loaded GDT.
;
; Return:       None
;
;----------------------------------------------------------------------------
align 16
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

BITS 16
  ;
  ; Warm Reset (INIT#) check.
  ;
  mov     si, 0x0F000
  mov     ds, si
  mov     si, 0x0FFF0
  cmp     byte [si], 0x0EA      ; Is it warm reset ?
  jne     NotWarmReset          ; JIf not.

  mov     al, 08
  mov     dx, 0x0CF9
  out     dx, al
  mov     al, 0x055
  out     0x80, al;
  jmp $
NotWarmReset:
  ;
  ; Save time-stamp counter value
  ; rdtsc load 64bit time-stamp counter to EDX:EAX
  ;
  rdtsc
  movd    mm6, edx
  movd    mm7, eax
  ;
  ; Load the GDT table in GdtDesc
  ;
  mov     esi, GdtDesc
o32 lgdt    [cs:si]

  ;
  ; Transition to 16 bit protected mode
  ;
  mov     eax, cr0                   ; Get control register 0
  or      eax, 00000003h             ; Set PE bit (bit #0) & MP bit (bit #1)
  mov     cr0, eax                   ; Activate protected mode

  ;
  ; Now we're in 16 bit protected mode
  ; Set up the selectors for 32 bit protected mode entry
  ;
  mov     ax, SYS_DATA_SEL
  mov     ds, ax
  mov     es, ax
  mov     fs, ax
  mov     gs, ax
  mov     ss, ax

  ;
  ; Transition to Flat 32 bit protected mode
  ; The jump to a far pointer causes the transition to 32 bit mode
  ;
  jmp LINEAR_CODE_SEL:dword ProtectedModeEntryPoint

;
; Protected mode portion initializes stack, configures cache, and calls C entry point
;
;----------------------------------------------------------------------------
;
; Procedure:    ProtectedModeEntryPoint
;
; Input:        Executing in 32 Bit Protected (flat) mode
;                cs: 0-4GB
;                ds: 0-4GB
;                es: 0-4GB
;                fs: 0-4GB
;                gs: 0-4GB
;                ss: 0-4GB
;
; Output:       This function never returns
;
; Destroys:
;               ecx
;               edi
;               esi
;               esp
;
; Description:
;               Perform any essential early platform initialization
;               Setup a stack
;               Call the main EDKII Sec C code
;
;----------------------------------------------------------------------------
BITS 32
ProtectedModeEntryPoint:

  JMP32  stackless_EarlyPlatformInit
  ;
  ; Early board hooks
  ;
  JMP32  ASM_PFX(BoardBeforeTempRamInit)
  ;
  ; Set UEFI stack
  ;
  mov eax, BIST_VALUE         ; BIST value
  lea ebx, [StackReturn]      ; Return address
  mov ecx, FLASH_IMAGE_SIZE   ; Size in bytes of region to cache
  xor edx, edx                ; Base of region to cache

;  ;
;  ; Configure cache-as-ram, ensuring stack does not collide with HOB allocations.
;  ; BSP_HEAP_STACK_SIZE is divided in half: 1/2 for Heap + 1/2 for Stack
;  ;
  AMD_ENABLE_UEFI_STACK2 STACK_AT_TOP, BSP_HEAP_STACK_SIZE, BSP_HEAP_STACK_BASE
;
StackReturn:
  ;
  ; Save configured stack pointer
  ;
  lea ebx, [PlatformTemporaryStackBase]
  mov [ebx], esp

  ;
  ; Early board hooks
  ;
  call ASM_PFX(BoardAfterTempRamInitWrapper)
  ;
  ; Store the BIST value in EBP
  ;
  mov     ebp, BIST_VALUE

  ;
  ; Push processor count to stack first, then BIST status (AP then BSP)
  ;
  mov     eax, 1
  cpuid
  shr     ebx, 16
  and     ebx, 0x0000000FF
  cmp     bl, 1
  jae     PushProcessorCount

  ;
  ; Some processors report 0 logical processors.  Effectively 0 = 1.
  ; So we fix up the processor count
  ;
  inc     ebx

PushProcessorCount:
  push    ebx

  ;
  ; We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  ; for all processor threads
  ;
  xor     ecx, ecx
  mov     cl, bl
PushBist:
  push    ebp
  loop    PushBist

  ; Push the Time-Stamp Counter values to the stack
  movd eax, mm6
  push eax
  movd eax, mm7
  push eax

  ;
  ; Pass Control into the PEI Core
  ;
  call ASM_PFX(PlatformSecLibStartup)

  ;
  ; PEI Core should never return to here, this is just to capture an invalid return.
  ;
  jmp     $

;----------------------------------------------------------------------------
;
; Procedure:    stackless_EarlyPlatformInit
;
; Input:        esp - Return address
;
; Output:       None
;
; Destroys:
;                eax
;                ecx
;                dx
;                ebp
;
; Description:
;        Any essential early platform initialisation required:
;        (1) Disable Cache
;        (2) Disable NMI's/SMI's
;        (3) Setup eSRAM (provide early memory to the system)
;        (4) Setup PCIEXBAR access mechanism
;        (5) Open up full SPI flash decode
;
;----------------------------------------------------------------------------
stackless_EarlyPlatformInit:

  ;
  ; Save return address
  ;
  mov  ebp, esp

  ;
  ; Ensure cache is disabled.
  ;
  mov     eax, cr0
  or      eax, CR0_CACHE_DISABLE + CR0_NO_WRITE
  invd
  mov     cr0, eax

  ;
  ; Disable NMI
  ; Good convention suggests you should read back RTC data port after
  ; accessing the RTC index port.
  ;
  mov  al, NMI_DISABLE
  mov  dx, RTC_INDEX
  out  dx, al
  mov  dx, RTC_DATA
  in   al, dx

  ;
  ; Restore return address
  ;
  mov  esp, ebp
  RET32

IssueWarmReset:
  ;
  ; Issue Warm Reset request to Remote Management Unit via iLB
  ;
  mov ax, CF9_WARM_RESET
  mov  dx, ILB_RESET_REG
  out  dx, ax
  jmp  $  ; Stay here until we are reset.

IssueColdReset:
  ;
  ; Issue Cold Reset request to Remote Management Unit via iLB
  ;
  mov ax, CF9_COLD_RESET
  mov  dx, ILB_RESET_REG
  out  dx, ax
  jmp  $  ; Stay here until we are reset.

;----------------------------------------------------------------------------
;
; VOID
; AsmSecPlatformDisableTemporaryMemory(
;    VOID
; );
;
;----------------------------------------------------------------------------
global ASM_PFX(AsmSecPlatformDisableTemporaryMemory)
ASM_PFX(AsmSecPlatformDisableTemporaryMemory):
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push ebp


; AMD_DISABLE_UEFI_STACK:  Dismantle the pre-memory cache-as-RAM mode.
;
;   In:
;       EBX  = Return address (preserved)
;
;   Out:
;       EAX = AGESA_SUCCESS
;
;   Preserved:
;       ebx, esp
;   Destroyed:
;       eax, ebx, ecx, edx, esi, ebp
;
  lea  ebx, [dis_ret_label]

  AMD_DISABLE_UEFI_STACK2

dis_ret_label:

  pop ebp
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax

  ret

;----------------------------------------------------------------------------
;
; UINT32 *
; AsmSecPlatformGetTemporaryStackBase(
;    VOID
; );
;
;----------------------------------------------------------------------------
global ASM_PFX(AsmSecPlatformGetTemporaryStackBase)
ASM_PFX(AsmSecPlatformGetTemporaryStackBase):
  push ebx

  lea ebx, [PlatformTemporaryStackBase]
  mov eax, [ebx]

  pop ebx
  ret

;----------------------------------------------------------------------------
;
; ROM-based Global-Descriptor Table for the Tiano PEI Phase
;
align 16

;
; GDT[0]: 0x00: Null entry, never used.
;
GDT_BASE:
;
; NULL data segment descriptor
;
NULL_SEL        equ     $ - GDT_BASE        ; Selector [0]
        DW      0                           ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      0                           ; present, ring 0, data, expand-up, writable
        DB      0                           ; page-granular, 32-bit
        DB      0
;
; Linear data segment descriptor
;
LINEAR_SEL      equ     $ - GDT_BASE        ; Selector [0x8]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      092h                        ; present, ring 0, data, expand-up, writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; Linear code segment descriptor
;
LINEAR_CODE_SEL equ     $ - GDT_BASE        ; Selector [0x10]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      09Bh                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; System data segment descriptor
;
SYS_DATA_SEL    equ     $ - GDT_BASE        ; Selector [0x18]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      093h                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0

;
; System code segment descriptor
;
SYS_CODE_SEL    equ     $ - GDT_BASE        ; Selector [0x20]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      09Ah                        ; present, ring 0, data, expand-up, writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; Spare segment descriptor
;
SYS16_CODE_SEL  equ     $ - GDT_BASE        ; Selector [0x28]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0Fh
        DB      09Bh                        ; present, ring 0, code, expand-up, writable
        DB      00h                         ; byte-granular, 16-bit
        DB      0
;
; Spare segment descriptor
;
SYS16_DATA_SEL  equ     $ - GDT_BASE        ; Selector [0x30]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      093h                        ; present, ring 0, data, expand-up, not-writable
        DB      00h                         ; byte-granular, 16-bit
        DB      0

;
; Spare segment descriptor
;
SPARE5_SEL      equ     $ - GDT_BASE        ; Selector [0x38]
        DW      0                           ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      0                           ; present, ring 0, data, expand-up, writable
        DB      0                           ; page-granular, 32-bit
        DB      0
GDT_SIZE        equ     $ - GDT_BASE        ; Size, in bytes

;
; GDT Descriptor
;
GdtDesc:                                    ; GDT descriptor
        DW      GDT_SIZE - 1                ; GDT limit
        DD      GDT_BASE                    ; GDT base address

ProtectedModeEntryLinearAddress:
ProtectedModeEntryLinearOffset:
  DD      ProtectedModeEntryPoint           ; Offset of our 32 bit code
  DW      LINEAR_CODE_SEL

PlatformTemporaryStackBase DD 0
