;/** @file
; AMD VanGoghBoard PlatformSecLib
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector, configures the stack.
;
; Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
; Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

;
; Include processor definitions
;
%use masm


%include "Platform.inc"

;
; CR0 cache control bit definition
;
CR0_CACHE_DISABLE       EQU 040000000h
CR0_NO_WRITE            EQU 020000000h
BSP_STACK_BASE_ADDR     EQU FixedPcdGet32 (PcdPeiCorePeiPreMemoryStackBaseAddress)     ; Base address for core 0 stack
PRE_MEM_STACK_SIZE      EQU FixedPcdGet32 (PcdPeiCorePeiPreMemoryStackSize)
PCIEX_LENGTH_BIT_SETTING EQU 011000b

MSR_IA32_EFER           EQU  0c0000080h       ; Extended Feature Enable Register
MSR_IA32_EFER_LME       EQU  8                ; Long Mode Enable

MSR_SMM_BASE            EQU  0c0010111h       ; SMBASE Register

SMM_BASE_DEFAULT        EQU  30000h           ; reset value of MSR MSR_SMM_BASE

SMMMASK_ADDRESS         EQU  0c0010113h       ; SMM TSeg Base Address
SMMMASK_ADDRESS_AE      EQU  0                ; Aseg Address Range Enable
SMMMASK_ADDRESS_TE      EQU  1                ; Tseg Address Range Enable

;
; In Modified Conventional Resume S3 Design:
;   With Modified Conventional Resume path, the x86 resumes from sleep,
; begins executing code from a predefined SMM resume vector and then
; jump to ROM code to continue conventional resume.
; EDX is filled with special signature "0x55AABB66" when jump to Sec,
; this signature can be used to identify if resume back from SMM resume.
;
SMM_RESUME_SIGNATURE    EQU  055AABB66h

PCAT_RTC_ADDRESS_REGISTER  EQU  0x70
PCAT_RTC_DATA_REGISTER     EQU  0x71

NMI_DISABLE_BIT         EQU  0x80

RTC_ADDRESS_REGISTER_A  EQU  0x0A  ; R/W[0..6]  R0[7]
RTC_ADDRESS_REGISTER_B  EQU  0x0B  ; R/W
RTC_ADDRESS_REGISTER_C  EQU  0x0C  ; RO
RTC_ADDRESS_REGISTER_D  EQU  0x0D  ; R/W

;
; External and public declarations
;  TopOfStack is used by C code
;  SecStartup is the entry point to the C code
; Neither of these names can be modified without
; updating the C code.
;
extern   ASM_PFX(SecStartup)

SECTION .text
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
;                esi
;                esp
;
; Description:
;                Perform any essential early platform initilaisation
;               Setup a stack
;               Call the main EDKII Sec C code
;
;----------------------------------------------------------------------------

global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  ;
  ; Check if system resumes from S3 SMM mode, if yes, continue to use S3 preserved stack setting
  ;
  cmp     edi, SMM_RESUME_SIGNATURE
  je      S3_SecRoutine

  JMP32   ASM_PFX(stackless_EarlyPlatformInit)
  mov     esp, BSP_STACK_BASE_ADDR+PRE_MEM_STACK_SIZE

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
  push    ebx

  movd    eax, mm0  ; BIST saved in mm0 at reset vector.
  ;
  ; We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  ; for all processor threads
  ;
  mov     ecx, ebx
PushBist:
  push    eax
  loop    PushBist

  ;Clear Long Mode Enable
  mov     ecx, MSR_IA32_EFER
  rdmsr
  btr     eax, MSR_IA32_EFER_LME          ; Set LME=0
  wrmsr

  ;Open smm ram
  mov     ecx, SMMMASK_ADDRESS
  rdmsr
  btr     eax, SMMMASK_ADDRESS_AE          ; Set AValid=0
  btr     eax, SMMMASK_ADDRESS_TE          ; Set TValid=0
  wrmsr

  ;Rebase SMRAM Base Address to power on default value
  mov     ecx, MSR_SMM_BASE
  rdmsr
  mov     eax, SMM_BASE_DEFAULT
  wrmsr

  mov     ecx, APIC_BASE_ADDRESS
  rdmsr
  bt      eax, APIC_BSC                     ; Is this the BSC?
  jc      IsBsp

IsAp:
  cli                                       ; Family 17h AP just halt here
  hlt
  jmp IsAp

IsBsp:
  push ebp  ; BFV base address
  ;
  ; Pass stack base into the PEI Core
  ;
  push    BSP_STACK_BASE_ADDR

  ;
  ; Pass stack size into the PEI Core
  ;
  push    PRE_MEM_STACK_SIZE
  ;
  ; Pass Control into the PEI Core
  ;
  ; UefiCpuPkg\SecCore\SecMain.c:
  ;
  ; VOID
  ; EFIAPI
  ; SecStartup (
  ;   IN UINT32                   SizeOfRam,
  ;   IN UINT32                   TempRamBase,
  ;   IN VOID                     *BootFirmwareVolume
  ;   );
  call ASM_PFX(SecStartup)

  ;
  ; Sec Core should never return to here, this is just to capture an invalid return.
  ;
  jmp     $

S3_SecRoutine:
  ;Clear Long Mode Enable
  mov     ecx, MSR_IA32_EFER
  rdmsr
  btr     eax, MSR_IA32_EFER_LME          ; Set LME=0
  wrmsr

  ;Open smm ram
  mov     ecx, SMMMASK_ADDRESS
  rdmsr
  btr     eax, SMMMASK_ADDRESS_AE          ; Set AValid=0
  btr     eax, SMMMASK_ADDRESS_TE          ; Set TValid=0
  wrmsr

  ;Rebase SMRAM Base Address to power on default value
  mov     ecx, MSR_SMM_BASE
  rdmsr
  mov     eax, SMM_BASE_DEFAULT
  wrmsr

  mov     ecx, APIC_BASE_ADDRESS
  rdmsr
  bt      eax, APIC_BSC                     ; Is this the BSC?
  jc      IsBspInS3

IsApInS3:
  cli                                       ; Family 17h AP just halt here
  hlt
  jmp     IsApInS3

IsBspInS3:

  ;;
  ;; Enable eSPI port 80 and FCH UART2 during S3 resume start
  ;;
  push  eax
  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi

  mov     eax, FixedPcdGet64 (PcdPciExpressBaseAddress)
  or      eax, (PCIEX_LENGTH_BIT_SETTING | 1)
  xor     edx, edx
  mov     ecx, 0C0010058h
  wrmsr

  ;
  ; Enable port 80 decode to eSPI ;
  ;
  mov  ebx, FixedPcdGet64 (PcdPciExpressBaseAddress) | (LPC_PFA << 12) | LPC_SPI_BASE_ADDR  ; PCI Configuration address
  mov  ebx, dword ptr [ebx]
  and  ebx, 0xFFFFFF00
  add  ebx, 0x10000 ; Get the eSPI base address
  add  ebx, ESPI_SLAVE0_DECODE_EN
  mov  eax, dword ptr [ebx]
  or   eax, ESPI_SLAVE0_DECODE_EN_IO_80_EN
  mov  dword ptr [ebx], eax

  ;
  ; Program IOMUX for eSPI port 80, GPIO 30 and 31 to function 1
  ;
  mov  ebx, 0xFED80D1E
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  mov  ebx, 0xFED80D1F
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  mov  al, 0x33
  out  0x80, al

  ;
  ; Program IOMUX for FCH UART2, GPIO 136 and 138 to function 1
  ;
  mov  ebx, 0xFED80D88
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  mov  ebx, 0xFED80D8A
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  pop  eax
  ;;
  ;; Enable eSPI port 80 and FCH UART2 during S3 resume end
  ;;

  pop     ebx
  mov     edi, ebx

  pop     ebx
  mov     esi, ebx

  pop     edx
  pop     eax

  pop     ebx
  mov     esp, ebx

  push    1    ; set Processor Count to 1 for S3 resume path which is not used finally.
  push    0    ; set BIST to 0

  push    ebp  ; BFV base address

  ;
  ; Pass stack base into the PEI Core
  ;
  sub     ebx, PRE_MEM_STACK_SIZE
  push    ebx

  ;
  ; Pass stack size into the PEI Core
  ;
  push    PRE_MEM_STACK_SIZE

  ;
  ; Pass Control into the PEI Core
  ;
  call    ASM_PFX(SecStartup)

  ;
  ; Sec Core should never return to here, this is just to capture an invalid return.
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
;        (1) Setup PCIEXBAR access mechanism
;        (2) enable IO port 80 to eSPI
;
;----------------------------------------------------------------------------
global ASM_PFX(stackless_EarlyPlatformInit)
ASM_PFX(stackless_EarlyPlatformInit):

  ;
  ;PcRtcInit start
  ;
  ;;
  ;; Initialize RTC Register
  ;;
  ;; Make sure Division Chain is properly configured,
  ;; or RTC clock won't "tick" -- time won't increment
  ;;
  in      al,  PCAT_RTC_ADDRESS_REGISTER

  mov     al,  RTC_ADDRESS_REGISTER_A | NMI_DISABLE_BIT
  out     PCAT_RTC_ADDRESS_REGISTER,  al

  mov     al,  FixedPcdGet8 (PcdInitialValueRtcRegisterA)
  out     PCAT_RTC_DATA_REGISTER,  al


  ;;
  ;; Read Register B
  ;;
  in      al,  PCAT_RTC_ADDRESS_REGISTER

  mov     al,  RTC_ADDRESS_REGISTER_B | NMI_DISABLE_BIT
  out     PCAT_RTC_ADDRESS_REGISTER,  al

  in      al,  PCAT_RTC_DATA_REGISTER


  ;;
  ;; Clear RTC flag register
  ;;
  in      al,  PCAT_RTC_ADDRESS_REGISTER

  mov     al,  RTC_ADDRESS_REGISTER_C | NMI_DISABLE_BIT
  out     PCAT_RTC_ADDRESS_REGISTER,  al

  in      al,  PCAT_RTC_DATA_REGISTER


  ;;
  ;; Clear RTC register D
  ;;
  in      al,  PCAT_RTC_ADDRESS_REGISTER

  mov     al,  RTC_ADDRESS_REGISTER_D | NMI_DISABLE_BIT
  out     PCAT_RTC_ADDRESS_REGISTER,  al

  mov     al,  FixedPcdGet8 (PcdInitialValueRtcRegisterD)
  out     PCAT_RTC_DATA_REGISTER,  al
  ;
  ;PcRtcInit end
  ;

  mov     eax, FixedPcdGet64 (PcdPciExpressBaseAddress)
  or      eax, (PCIEX_LENGTH_BIT_SETTING | 1)
  xor     edx, edx
  mov     ecx, 0C0010058h
  wrmsr

  ;
  ; Enable port 80 decode to eSPI ;
  ;
  mov  ebx, FixedPcdGet64 (PcdPciExpressBaseAddress) | (LPC_PFA << 12) | LPC_SPI_BASE_ADDR  ; PCI Configuration address
  mov  ebx, dword ptr [ebx]
  and  ebx, 0xFFFFFF00
  add  ebx, 0x10000 ; Get the eSPI base address
  add  ebx, ESPI_SLAVE0_DECODE_EN
  mov  eax, dword ptr [ebx]
  or   eax, ESPI_SLAVE0_DECODE_EN_IO_80_EN
  mov  dword ptr [ebx], eax

  ;
  ; Program IOMUX for eSPI port 80, GPIO 30 and 31 to function 1
  ;
  mov  ebx, 0xFED80D1E
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  mov  ebx, 0xFED80D1F
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  mov  al, 0x88
  out  0x80, al


  ;
  ; Program IOMUX for FCH UART2, GPIO 136 and 138 to function 1
  ;
  mov  ebx, 0xFED80D88
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  mov  ebx, 0xFED80D8A
  mov  eax, dword ptr [ebx]
  or   eax, 1
  mov  dword ptr [ebx], eax

  RET32

;----------------------------------------------------------------------------
;
; Procedure:    stackless_PCIConfig_Write
;
; Input:        esp - return address
;                eax - Data to write
;                ebx - PCI Config Address
;
; Output:       None
;
; Destroys:
;                dx
;
; Description:
;        Perform a DWORD PCI Configuration write
;
;----------------------------------------------------------------------------
global ASM_PFX(stackless_PCIConfig_Write)
ASM_PFX(stackless_PCIConfig_Write):

  ;
  ; Write the PCI Config Address to the address port
  ;
  xchg  eax, ebx
  mov  dx, PCI_ADDRESS_PORT
  out  dx, eax
  xchg  eax, ebx

  ;
  ; Write the PCI DWORD Data to the data port
  ;
  mov  dx, PCI_DATA_PORT
  out  dx, eax

  RET32


;----------------------------------------------------------------------------
;
; Procedure:    stackless_PCIConfig_Read
;
; Input:        esp - return address
;                ebx - PCI Config Address
;
; Output:       eax - Data read
;
; Destroys:
;                eax
;                dx
;
; Description:
;        Perform a DWORD PCI Configuration read
;
;----------------------------------------------------------------------------
global ASM_PFX(stackless_PCIConfig_Read)
ASM_PFX(stackless_PCIConfig_Read):
  ;
  ; Write the PCI Config Address to the address port
  ;
  xchg  eax, ebx
  mov  dx, PCI_ADDRESS_PORT
  out  dx, eax
  xchg  eax, ebx

  ;
  ; Read the PCI DWORD Data from the data port
  ;
  mov  dx, PCI_DATA_PORT
  in  eax, dx

  RET32
