;*****************************************************************************
; AMD Generic Encapsulated Software Architecture
;
; Copyright (C) 2008 - 2024 Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; $Workfile:: EarlyCpuSupportNasm.inc
;
;  $Revision$    $Date$
;
; Description:
;

%define STACK_AT_TOP               0           ; Stack is at the top of CAR
%define STACK_AT_BOTTOM            1           ; Stack is at the bottom of CAR

%define APIC_BASE_ADDRESS        0000001Bh
%define     APIC_BSC                 8       ; Boot Strap Core

%define APIC_MSG_REG             380h        ; Location of BSC message
%define     APIC_MSG             00DE00ADh   ; Message data
%define     APIC_INVD_ALL_DONE_MSG       00AD00DEh ; Indicate all cores have invalidated
%define APIC_CMD_LO_REG          300h        ; APIC command low
%define APIC_CMD_HI_REG          310h        ; APIC command high
%define     CMD_REG_TO_READ_DATA  00000338h  ; APIC command for remote read of APIC_MSG_REG
%define     REMOTE_READ_STS        00030000h ; Remote read status mask
%define     REMOTE_DELIVERY_PEND   00010000h ; Remote read is pending
%define     REMOTE_DELIVERY_DONE   00020000h ; Remote read is complete
%define     DELIVERY_STS_BIT     12          ; Delivery status valid bit
%define APIC_ID_REG              0020h       ; Local APIC ID offset
%define     APIC20_APICID        24
%define APIC_REMOTE_READ_REG     00C0h       ; Remote read offset

%define AMD_MTRR_VARIABLE_BASE0  0200h
%define AMD_MTRR_VARIABLE_BASE6  020Ch
%define AMD_MTRR_VARIABLE_BASE7  020Eh
%define     VMTRR_VALID                  11
%define     MTRR_TYPE_WB                 06h
%define     MTRR_TYPE_WP                 05h
%define     MTRR_TYPE_WT                 04h
%define     MTRR_TYPE_UC                 00h
%define AMD_MTRR_VARIABLE_MASK0  0201h
%define AMD_MTRR_VARIABLE_MASK7  020Fh
%define AMD_MTRR_FIX64k_00000    0250h
%define AMD_MTRR_FIX16K_80000    0258h
%define AMD_MTRR_FIX16K_A0000    0259h
%define AMD_MTRR_FIX4K_C0000     0268h
%define AMD_MTRR_FIX4K_C8000     0269h
%define AMD_MTRR_FIX4K_D0000     026Ah
%define AMD_MTRR_FIX4K_D8000     026Bh
%define AMD_MTRR_FIX4K_E0000     026Ch
%define AMD_MTRR_FIX4K_E8000     026Dh
%define AMD_MTRR_FIX4K_F0000     026Eh
%define AMD_MTRR_FIX4K_F8000     026Fh
%define CPU_LIST_TERMINAL        0FFFFFFFFh

%define AMD_MTRR_DEFTYPE         02FFh
%define     WB_DRAM_TYPE             1Eh     ; MemType - memory type
%define     MTRR_DEF_TYPE_EN         11      ; MtrrDefTypeEn - variable and fixed MTRRs default enabled
%define     MTRR_DEF_TYPE_FIX_EN     10      ; MtrrDefTypeEn - fixed MTRRs default enabled

%define     INVD_WBINVD              4       ;   INVD to WBINVD conversion

%define IORR_BASE                0C0010016h  ; IO Range Regusters Base/Mask, 2 pairs
                                        ;   uses 16h - 19h
%define TOP_MEM                  0C001001Ah  ; Top of Memory
%define TOP_MEM2                 0C001001Dh  ; Top of Memory2

%define CR0_PE                   0           ; Protection Enable
%define CR0_NW                   29          ; Not Write-through
%define CR0_CD                   30          ; Cache Disable
%define CR0_PG                   31          ; Paging Enable

; CPUID Functions

%define CPUID_MODEL              1
            ; In EAX
%define     EXT_MODEL            16          ; Extended model
%define     EXT_FAMILY           20          ; Extended family
            ; In EBX
%define     PROCESSOR_COUNT      16          ; Logical processor count
%define     LOCAL_APIC_ID        24          ; LocalApicId - initial local APIC physical ID
            ; In EDX
%define      PYSYCAL_ADDRESS_EXT 6           ; Physical addresss extensions.


%define AMD_CPUID_L2Cache        80000006h   ; L2/L3 cache info
%define AMD_CPUID_APIC           80000008h   ; Long Mode and APIC info., core count
%define     APIC_ID_CORE_ID_SIZE      12     ; ApicIdCoreIdSize bit position
%define AMD_CPUID_EXT_APIC       8000001Eh   ; Extended APICID information

%define AMD_CPUID_CAP_EXT        80000008h   ; Processor capability parameter and
                                             ; Extended feature identification.
%define         PHYSICAL_ADDR_MASK    0FFh   ;

%define MTRR_SYS_CFG             0C0010010h  ; System Configuration Register
%define   SYS_UC_LOCK_EN             17      ;   SysUcLockEn      System lock command enable
%define   MTRR_FIX_DRAM_EN           18      ;   MtrrFixDramEn    MTRR fixed RdDram and WrDram attributes enable
%define   MTRR_FIX_DRAM_MOD_EN       19      ;   MtrrFixDramModEn MTRR fixed RdDram and WrDram modification enable
%define   MTRR_VAR_DRAM_EN           20      ;   MtrrVarDramEn    MTRR variable DRAM enable

; Local use flags, in upper most byte of ESI
%define FLAG_UNKNOWN_FAMILY                24    ; Signals that the family# of the installed processor is not recognized
%define FLAG_STACK_REENTRY                 25    ; Signals that the environment has made a re-entry (2nd) call to set up the stack
%define FLAG_IS_PRIMARY                    26    ; Signals that this core is the primary within the compute unit
%define FLAG_CORE_NOT_IDENTIFIED           27    ; Signals that the cores/compute units of the installed processor is not recognized
%define FLAG_FORCE_32K_STACK               28    ; Signals to force 32KB stack size for BSP core
%define FLAG_DRAM_AVAILABLE                29    ; Signals that DRAM is present

; Error code returned in EDX by AMD_ENABLE_STACK_PRIVATE
%ifndef CPU_EVENT_UNKNOWN_PROCESSOR_FAMILY
%define CPU_EVENT_UNKNOWN_PROCESSOR_FAMILY     008010500h
%endif
%ifndef CPU_EVENT_STACK_REENTRY
%define CPU_EVENT_STACK_REENTRY                008020500h
%endif
%ifndef CPU_EVENT_CORE_NOT_IDENTIFIED
%define CPU_EVENT_CORE_NOT_IDENTIFIED          008030500h
%endif
%ifndef CPU_EVENT_STACK_BASE_OUT_OF_BOUNDS
%define CPU_EVENT_STACK_BASE_OUT_OF_BOUNDS     008040500h
%endif
%ifndef CPU_EVENT_STACK_SIZE_INVALID
%define CPU_EVENT_STACK_SIZE_INVALID           008050500h
%endif

; AGESA_STATUS values
%ifndef AGESA_SUCCESS
%define AGESA_SUCCESS  0
%endif
%ifndef AGESA_WARNING
%define AGESA_WARNING  4
%endif
%ifndef AGESA_FATAL
%define AGESA_FATAL    7
%endif
;;***************************************************************************
;;
;;                      CPU MACROS - PUBLIC
;;
;;***************************************************************************

%macro AMD_CPUID 0-1
  %if (%0 = 0)
    mov   eax, 1
    db    0Fh, 0A2h                     ; Execute instruction
    bswap eax
    xchg  al, ah                        ; Ext model in al now
    rol   eax, 8                        ; Ext model in ah, model in al
    and   ax, 0FFCFh                    ; Keep 23:16, 7:6, 3:0
  %else
    mov   eax, %1
    db    0Fh, 0A2h
  %endif
%endmacro

%macro MAKE_SBDFO 5
    mov   eax, ((%1 << 28) | (%2 << 20) | (%3 << 15) | (%4 << 12) | (%5))
%endmacro

%macro MAKE_EXT_PCI_ADDR 5
    mov   eax, ((1 << 31) | (%1 << 28) | (((%5 & 0F00h) >> 8) << 24) | (%2 << 16) | (%3 << 11) | (%4 << 8) | (%5 & 0FCh))
%endmacro

;---------------------------------------------------
; LoadTableAddress
;       Due to the various assembly methodologies used by BIOS vendors, this macro is needed to abstract the
;       loading of the address of a table. The default is the standard LEA instruction with table address.
;       The IBV that needs to use an alternative method can define their version of the macro prior to including
;       this file into their source.
;       An alternative example:
;         LoadTableAddress  MACRO MyTable
;           LEA   eax, -(LAST_ADDRESS - MyTable)
;---------------------------------------------------
%ifnmacro LoadTableAddress 1
%macro  LoadTableAddress 1
       lea   eax, [%1]
%endmacro
%endif


;;***************************************************************************
;;
;;                      CPU STRUCTURES - PUBLIC
;;
;;***************************************************************************
struc CPU_FAMILY_INFO
    .L2_MIN_SIZE         resw    1       ; Minimum size of the L2 cache for this family, in K
    .NUM_SHARED_CORES    resb    1       ; Number of cores sharing an L2 cache
    .L2_ALLOC_MEM        resb    1       ; L2 space reserved for memory training, in K
    .L2_ALLOC_EXE        resw    1       ; L2 space reserved for EXE CACHE, in K. 0 means unlimited.
    .SIZE_ADDRESS_BUS    resb    1       ; Number of address bits supported by this family
    .FAMILY_RESERVED     resb    1       ; reserved, pad to DWORD size
endstruc


%macro CPU_DEADLOOP 0

%%DeadLoop:
    out     84h, al
    jmp     %%DeadLoop

%endmacro





