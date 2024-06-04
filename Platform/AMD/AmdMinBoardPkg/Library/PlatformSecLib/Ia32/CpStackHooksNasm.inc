;*****************************************************************************
; AMD Generic Encapsulated Software Architecture
;
; Copyright (C) 2008 - 2024 Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; $Workfile:: CpStackHooksNasm.inc
; $Revision$    $Date$
;
; Description: CpStackHooksNasm.inc - AGESA stack setup family hooks
;

%include "NasmBase.inc"

%define STACK_SIZE_4M            400000h        ; 4MB
%define STACK_SIZE_2M            200000h        ; 2MB
%define STACK_SIZE_1M            100000h        ; 1MB
%define STACK_SIZE_256K          40000h         ; 256KB
%define STACK_SIZE_192K          30000h         ; 192KB
%define STACK_SIZE_128K          20000h         ; 128KB
%define STACK_SIZE_64K           10000h         ; 64KB
%define STACK_SIZE_32K           8000h          ; 32KB
%define STACK_SIZE_16K           4000h          ; 16KB
%define STACK_SIZE_4K            1000h          ; 4KB

%define CORE0_STACK_SIZE         STACK_SIZE_16K ; 16KB for primary cores
%define CORE1_STACK_SIZE         STACK_SIZE_4K  ; 4KB for each AP cores

%define BSP_STACK_BASE_ADDR      30000h         ; Base address for core 0 stack
%define BSP_STACK_BASE_ADDR_4M   400000h        ; Base address at 4MB
%define CORE0_STACK_BASE_ADDR    80000h         ; Base address for primary cores stack
;
;CORE1_STACK_BASE_ADDR = BSP_STACK_BASE_ADDR + BSP_STACK_SIZE
;
%define BSP_CACHE_TYPE_POSITION  3

;============================================================================
;
; Define a  macro that allow the exclusion of processor families from
; the cache-as-ram code. This will reduce the size of the assembled file.
;
;============================================================================

;---------------------------------------------------
;
; AMD_ENABLE_STACK_FAMILY_HOOK Macro - Stackless
;
;   Set any family specific controls needed to enable the use of
;   cache as general storage before main memory is available.
;
; Inputs:
;       ESI - node#, core#, flags from GET_NODE_ID_CORE_ID
; Outputs:
;       none
; Destroyed:
;       eax, ebx, ecx, edx
;---------------------------------------------------
%macro AMD_ENABLE_STACK_FAMILY_HOOK 0

    AMD_ENABLE_STACK_FAMILY_HOOK_F19
    AMD_ENABLE_STACK_FAMILY_HOOK_F1A

%endmacro

;----------------------------------------------
;
; AMD_DISABLE_STACK_FAMILY_HOOK Macro - Stackless
;
;   Return any family specific controls to their 'standard'
;   settings for using cache with main memory.
;
; Inputs:
;       ESI - node#, core#, flags from GET_NODE_ID_CORE_ID
; Outputs:
;       none
; Destroyed:
;       eax, ebx, ecx, edx
;----------------------------------------------
%macro AMD_DISABLE_STACK_FAMILY_HOOK 0

    AMD_DISABLE_STACK_FAMILY_HOOK_F19
    AMD_DISABLE_STACK_FAMILY_HOOK_F1A

%endmacro

;---------------------------------------------------
;
; GET_NODE_ID_CORE_ID Macro - Stackless
;
;   Read family specific values to determine the node and core
;   numbers for the core executing this code.
;
; Inputs:
;     none
; Outputs:
;     SI[7:0] = Core# (0..N, relative to node)
;     SI[15:8]= Node# (0..N)
;     SI[23:16]= reserved
;     SI[24]=   flag: 1=Family Unrecognized
;     SI[25]=   flag: 1=Interface re-entry call
;     SI[26]=   flag: 1=Core is primary of compute unit
;     SI[31:27]= reserved, =0
;
; Destroyed:
;       eax, ebx, ecx, edx, esi
;---------------------------------------------------
%macro GET_NODE_ID_CORE_ID 0

    mov     si, -1
    GET_NODE_ID_CORE_ID_F19
    GET_NODE_ID_CORE_ID_F1A

    ;
    ; Check for unrecognized Family
    ;
    _if si, e, -1 ; Has family (node/core) been discovered?
        mov    esi, ( (1 << FLAG_UNKNOWN_FAMILY)+(1 << FLAG_IS_PRIMARY) ) ; No, Set error code, Only let BSP continue
        mov    ecx, APIC_BASE_ADDRESS  ; MSR:0000_001B
        rdmsr
        bt     eax, APIC_BSC           ; Is this the BSC?
        _if ncarry
            ;
            ; No, this is an AP
            ;
            hlt                        ; Kill APs
        _endif
    _endif
%endmacro

;;***************************************************************************
;;                      Family 19h MACROS
;;***************************************************************************
;---------------------------------------------------
;
; AMD_ENABLE_STACK_FAMILY_HOOK_F19 Macro - Stackless
;
;   Set any family specific controls needed to enable the use of
;   cache as general storage before main memory is available.
;
; Inputs:
;       ESI - node#, core#, flags from GET_NODE_ID_CORE_ID
; Outputs:
;       none
; Destroyed:
;       eax, ebx, ecx, edx
;---------------------------------------------------
%macro AMD_ENABLE_STACK_FAMILY_HOOK_F19 0

    AMD_CPUID CPUID_MODEL
    mov    ebx, eax                       ; Save revision info to EBX
    shr    eax, 20                        ; AL = cpu extended family
    cmp    al, 0Ah                        ; Is this family 19h?
    jnz    %%fam19_enable_stack_hook_exit ; Br if no

    ;
    ; Set TOP_MEM (C001_001A) for non-shared cores to 16M. This will be increased at heap init.
    ;  - not strictly needed since the FixedMTRRs take presedence.
    ;
    mov    ecx, TOP_MEM                   ; MSR:C001_001A
    rdmsr
    test    eax, eax
    _if zero
        dec    eax
        wrmsr
    _endif

%%fam19_enable_stack_hook_exit:
%endmacro

;----------------------------------------------
;
; AMD_DISABLE_STACK_FAMILY_HOOK_F19 Macro - Stackless
;
;   Return any family specific controls to their 'standard'
;   settings for using cache with main memory.
;
; Inputs:
;       ESI - [31:24] flags; [15:8]= Node#; [7:0]= core#
; Outputs:
;       none
; Destroyed:
;       eax, ebx, ecx, edx
;---------------------------------------------------
%macro AMD_DISABLE_STACK_FAMILY_HOOK_F19 0

    AMD_CPUID   CPUID_MODEL
    mov    ebx, eax                        ; Save revision info to EBX
    shr    eax, 20                         ; AL = cpu extended family
    cmp    al, 0Ah                         ; Is this family 19h?
    jnz    %%fam19_disable_stack_hook_exit ; Br if no

%%fam19_disable_stack_hook_exit:
%endmacro

;---------------------------------------------------
;
; GET_NODE_ID_CORE_ID_F19 Macro - Stackless
;
;   Read family specific values to determine the node and core
;   numbers for the core executing this code.
;
; Inputs:
;     none
; Outputs:
;     ESI = core#, node# & flags (see GET_NODE_ID_CORE_ID macro above)
;     MM5 = 32b pointer to family info structure
; Destroyed:
;       eax, ebx, ecx, edx, esi, mm5
;---------------------------------------------------
%macro GET_NODE_ID_CORE_ID_F19 0

        jmp     %%end_of_f19h_data
        ; Family 19h Info Structure: L2Size,  #SharedCores, AllocMem, AllocExe, SzAddrBus, pad
        %%FAM19H_INFO_STRUCT istruc CPU_FAMILY_INFO
        at  CPU_FAMILY_INFO.L2_MIN_SIZE,      dw 2048
        at  CPU_FAMILY_INFO.NUM_SHARED_CORES, db 2
        at  CPU_FAMILY_INFO.L2_ALLOC_MEM,     db 0
        at  CPU_FAMILY_INFO.L2_ALLOC_EXE,     dw 0
        at  CPU_FAMILY_INFO.SIZE_ADDRESS_BUS, db 48
        at  CPU_FAMILY_INFO.FAMILY_RESERVED,  db 0
        iend
%%end_of_f19h_data:
        jmp     %%end_of_f19h_zen4_data
        ; Family 19h Info Structure: L2Size,  #SharedCores, AllocMem, AllocExe, SzAddrBus, pad
        %%FAM19H_ZEN4_INFO_STRUCT istruc CPU_FAMILY_INFO
        at  CPU_FAMILY_INFO.L2_MIN_SIZE,      dw 2048
        at  CPU_FAMILY_INFO.NUM_SHARED_CORES, db 2
        at  CPU_FAMILY_INFO.L2_ALLOC_MEM,     db 0
        at  CPU_FAMILY_INFO.L2_ALLOC_EXE,     dw 0
        at  CPU_FAMILY_INFO.SIZE_ADDRESS_BUS, db 52
        at  CPU_FAMILY_INFO.FAMILY_RESERVED,  db 0
        iend
%%end_of_f19h_zen4_data:

    cmp    si, -1                       ; Has node/core already been discovered?
    jnz    %%node_core_f19_exit         ; Br if yes

    AMD_CPUID   CPUID_MODEL
    shr    ebx, 16                      ; BH = LocalApicId
    mov    bl, al
    shr    bl, 4                        ; BL = cpu basic model
    shr    eax, 12                      ; AH = cpu extended family
    cmp    ah, 0Ah                      ; Is this family 19h?
    jnz    %%node_core_f19_exit         ; Br if no
    or     bl, al                       ; BL = cpu model

    cmp    bl, 010h                     ; Is this RS
    jz     %%load_family_19_zen4_info   ; Br if yes
    cmp    bl, 011h                     ; Is this RS B0
    jz     %%load_family_19_zen4_info   ; Br if yes
    cmp    bl, 018h                     ; Is this STP
    jz     %%load_family_19_zen4_info   ; Br if yes
    cmp    bl, 0A0h                     ; Is this Bergamo
    jz     %%load_family_19_zen4_info   ; Br if yes
    jmp    %%load_family_19_info

%%load_family_19_zen4_info:
    ; TODO, Need to check whether 52 bus is enabled or not
    ; Assuming its enabled
    LoadTableAddress %%FAM19H_ZEN4_INFO_STRUCT
    jmp %%load_family_19_end
%%load_family_19_info:
    LoadTableAddress %%FAM19H_INFO_STRUCT
%%load_family_19_end:
    movd   mm5, eax                    ; load pointer to Family Info Struc

    xor    esi, esi                    ; Assume BSC, clear local flags
    mov    ecx, APIC_BASE_ADDRESS      ; MSR:0000_001B
    rdmsr
    bt     eax, APIC_BSC               ; Is this the BSC?
    _if ncarry
        shr    bx, 4
        shr    bl, 4
        mov    si, bx                  ; SI = [15:8]= Node# = 0; [7:0]= core#
    _endif                             ; end

    ;
    ;   determine if this core shares MTRRs
    ;

    AMD_CPUID AMD_CPUID_EXT_APIC
    _if bh, ne, 0
        bt    si, 0
        _if ncarry
            bts    esi, FLAG_IS_PRIMARY ; Set shared flag into return value
        _endif
    _else
        bts    esi, FLAG_IS_PRIMARY     ; Set shared flag into return value
    _endif

    bts    esi, FLAG_DRAM_AVAILABLE
%%node_core_f19_exit:
%endmacro

;;***************************************************************************
;;                      Family 1Ah MACROS
;;***************************************************************************
;---------------------------------------------------
;
; AMD_ENABLE_STACK_FAMILY_HOOK_F1A Macro - Stackless
;
;   Set any family specific controls needed to enable the use of
;   cache as general storage before main memory is available.
;
; Inputs:
;       ESI - node#, core#, flags from GET_NODE_ID_CORE_ID
; Outputs:
;       none
; Destroyed:
;       eax, ebx, ecx, edx
;
;---------------------------------------------------
%macro AMD_ENABLE_STACK_FAMILY_HOOK_F1A 0

    AMD_CPUID   CPUID_MODEL
    mov    ebx, eax                        ; Save revision info to EBX
    shr    eax, EXT_FAMILY                 ; AL = cpu extended family
    cmp    al, 0bh                         ; Is this family 1Ah?
    jnz    %%fam1A_enable_stack_hook_exit  ; Br if no

    ;
    ; Set TOP_MEM (C001_001A) for non-shared cores to 16M. This will be increased at heap init.
    ;  - not strictly needed since the FixedMTRRs take presedence.
    ;
    mov    ecx, TOP_MEM                    ; MSR:C001_001A
    rdmsr
    test   eax, eax

    _if zero
        dec    eax
        wrmsr
    _endif

%%fam1A_enable_stack_hook_exit:
%endmacro

;----------------------------------------------
;
; AMD_DISABLE_STACK_FAMILY_HOOK_F1A Macro - Stackless
;
;   Return any family specific controls to their 'standard'
;   settings for using cache with main memory.
;
; Inputs:
;       ESI - [31:24] flags; [15:8]= Node#; [7:0]= core#
; Outputs:
;       none
; Destroyed:
;       eax, ebx, ecx, edx
;
;---------------------------------------------------
%macro AMD_DISABLE_STACK_FAMILY_HOOK_F1A 0

    AMD_CPUID CPUID_MODEL
    mov    ebx, eax                        ; Save revision info to EBX
    shr    eax, EXT_FAMILY                 ; AL = cpu extended family
    cmp    al, 0bh                         ; Is this family 1ah?
    jnz    %%fam1A_disable_stack_hook_exit ; Br if no

%%fam1A_disable_stack_hook_exit:
%endmacro

;---------------------------------------------------
;
; GET_NODE_ID_CORE_ID_F1A Macro - Stackless
;
;   Read family specific values to determine the node and core
;   numbers for the core executing this code.
;
; Inputs:
;     none
; Outputs:
;     ESI = core#, node# & flags (see GET_NODE_ID_CORE_ID macro above)
;     MM5 = 32b pointer to family info structure
;
; Destroyed:
;       eax, ebx, ecx, edx, esi, mm5
;---------------------------------------------------
%macro GET_NODE_ID_CORE_ID_F1A  0

    jmp    %%end_of_f1ah_48bit_data

    ; Family 1Ah Info Structure for 48 bits bus: L2Size,  #SharedCores, AllocMem, AllocExe, SzAddrBus, pad
    %%FAM1AH_48BIT_INFO_STRUCT istruc CPU_FAMILY_INFO
    at  CPU_FAMILY_INFO.L2_MIN_SIZE,      dw 2048
    at  CPU_FAMILY_INFO.NUM_SHARED_CORES, db 2
    at  CPU_FAMILY_INFO.L2_ALLOC_MEM,     db 0
    at  CPU_FAMILY_INFO.L2_ALLOC_EXE,     dw 0
    at  CPU_FAMILY_INFO.SIZE_ADDRESS_BUS, db 48
    at  CPU_FAMILY_INFO.FAMILY_RESERVED,  db 0
%%end_of_f1ah_48bit_data:
    jmp    %%end_of_f1ah_52bit_data

    ; Family 1Ah Info Structure for 52 bits bus: L2Size,  #SharedCores, AllocMem, AllocExe, SzAddrBus, pad
    %%FAM1AH_52BIT_INFO_STRUCT istruc CPU_FAMILY_INFO
    at  CPU_FAMILY_INFO.L2_MIN_SIZE,      dw 2048
    at  CPU_FAMILY_INFO.NUM_SHARED_CORES, db 2
    at  CPU_FAMILY_INFO.L2_ALLOC_MEM,     db 0
    at  CPU_FAMILY_INFO.L2_ALLOC_EXE,     dw 0
    at  CPU_FAMILY_INFO.SIZE_ADDRESS_BUS, db 52
    at  CPU_FAMILY_INFO.FAMILY_RESERVED,  db 0
%%end_of_f1ah_52bit_data:

    cmp    si, -1                       ; Has node/core already been discovered?
    jnz    %%node_core_f1A_exit         ; Br if yes

    AMD_CPUID AMD_CPUID_CAP_EXT         ; CPUID function 8000_0008h
    and    eax, PHYSICAL_ADDR_MASK      ;
    cmp    al, 52                       ; If the maximum physical address is 52 bits.
    je     %%load_family_1a_52bit_info  ; Load 52-bit table if PAE is set

    ; Load 48-bit table.
    LoadTableAddress %%FAM1AH_48BIT_INFO_STRUCT
    jmp    %%load_family_1a_info_end

%%load_family_1a_52bit_info:
    ; Load 52-bit table.
    LoadTableAddress %%FAM1AH_52BIT_INFO_STRUCT

%%load_family_1a_info_end:
    movd    mm5, eax                   ; load pointer to Family Info Struc

    AMD_CPUID   CPUID_MODEL
    shr    eax, EXT_FAMILY             ; AL = cpu extended family
    cmp    al, 0bh                     ; Is this family 1ah?
    jnz    %%node_core_f1A_exit        ; Br if no
    shr    ebx, PROCESSOR_COUNT        ; BH = LocalApicId (Node #), BL = Logical processor count (Core #)
    xor    esi, esi                    ; Assume BSC, clear local flags
    mov    ecx, APIC_BASE_ADDRESS      ; MSR:0000_001B
    rdmsr
    bt     eax, APIC_BSC               ; Is this the BSC?
    _if ncarry
        mov    si, bx                  ; SI = [15:8]= Node# = 0; [7:0]= core#
    _endif                             ; end

    ;
    ;   determine if this core shares MTRRs
    ;
    AMD_CPUID AMD_CPUID_EXT_APIC
    _if bh, ne, 0
        bt    si, 0
        _if ncarry
            bts    esi, FLAG_IS_PRIMARY ; Set shared flag into return value
        _endif
    _else
        bts    esi, FLAG_IS_PRIMARY     ; Set shared flag into return value
    _endif

    bts    esi, FLAG_DRAM_AVAILABLE
%%node_core_f1A_exit:
%endmacro

