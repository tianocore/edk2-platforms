;*****************************************************************************
; AMD Generic Encapsulated Software Architecture
;
; Copyright (C) 2008 - 2024 Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;  Workfile: AmdUefiStackNasm.inc
;  $Revision$    $Date$
;
; Description: Code to setup temporary memory access for stack usage. This code
; is to be used on memory present systems that do not need CAR
;

;============================================================================
;
; Define a  macro that allow the OEM to specify supported solutions in the
; cache-as-ram code. This will reduce the size of the assembled file.
; The macro will convert solutions into supported families.
;
;============================================================================

%include "CpStackNasm.inc"

;======================================================================
;   Reference: UEFI PI v1.2 definition:
;
;   typedef struct _UEFI_SEC_PEI_HAND_OFF {
;       UINT16  DataSize;
;       VOID    *BootFirmwareVolumeBase;
;       UINTN   BootFirmwareVolumeSize;
;       VOID    *TemporaryRamBase;
;       UINTN   TemporaryRamSize;
;       VOID    *PeiTemporaryRamBase;
;       UINTN   PeiTemporaryRamSize;
;       VOID    *StackBase;
;       UINTN   StackSize;
;   } UEFI_SEC_PEI_HAND_OFF;
;
struc UEFI_SEC_PEI_HAND_OFF
    .DATA_SIZE                   resw   1
    .ALIGNMENT_PAD               resw   1
    .BOOT_FIRMWARE_VOLUME_BASE   resd   1
    .BOOT_FIRMWARE_VOLUME_SIZE   resd   1
    .TEMPORARY_RAM_BASE          resd   1
    .TEMPORARY_RAM_SIZE          resd   1
    .PEI_TEMPORARY_RAM_BASE      resd   1
    .PEI_TEMPORARY_RAM_SIZE      resd   1
    .STACK_BASE                  resd   1
    .STACK_SIZE                  resd   1
endstruc

; Assure build option is defined, default is BIST only storage
%ifndef AMD_STACK_FRAME_PAD
%define AMD_STACK_FRAME_PAD          0
%endif


;======================================================================
;======================================================================
; AMD_ENABLE_UEFI_STACK2:  Setup a stack, heap & UEFI stack frame
;
;   Input condition requirements:
;       32bit protected 'flat addressing' mode
;       SS, DS, ES = segment descriptor defining 0x00000000 as the base.
;
;   Build time options:
;       AMD_STACK_FRAME_PAD EQU 00h
;              used to create a Host Env stack frame for pseudo
;              global variables - a build time option. Incremented
;              by 4 to cover the BIST storage.
;
;   Input Parameter:
;       StackLocation
;         STACK_AT_TOP
;                Indicate stack is on the top of cache as RAM.
;         STACK_AT_BOTTOM (default)
;                Indicate stack is at the bottom of cache as RAM.
;       BspStackSize = Stack size for BSP
;       BspStackAddr = Stack base address for BSP
;
;   In:
;       EAX  = BIST value collected after reset by host env
;       EBX  = Return address (preserved)
;       ECX  = size, in bytes, of the region to cache for execution.
;       EDX  = base address of region to cache, or zero for (4GB - size).
;
;   Out:
;       SS:ESP - Our new private stack location
;
;       EAX = AGESA_STATUS
;       EDX = Return status code if EAX contains a return code of higher
;             severity than AGESA_SUCCESS
;       ECX = Stack size in bytes
;       EDI  = pointer to stack frame location. Points to the
;               beginning of the UEFI structure defined by the
;               PI v1.2 spec. The Host Env stack frame follows
;               this structure.
;               [EDI]UEFI_SEC_PEI_HAND_OFF.BOOT_FIRMWARE_VOLUME_BASE = OEM_BFV_BASE
;               [EDI]UEFI_SEC_PEI_HAND_OFF.BOOT_FIRMWARE_VOLUME_SIZE = OEM_BFV_SIZE
;               [EDI+sizeof(UEFI_SEC_PEI_HAND_OFF)].OEM_DATA_DWORD[0] = BIST
;
;   Preserved:
;       EBX, EBP, DS, ES, SS
;
;   Destroyed:
;       EAX, ECX, EDX, EDI, ESI, ESP
;       MMX0, MMX1, MMX2, MMX3, MMX4, MMX5  ... MMX[0..7] are used as save/restore storage
;
;   Known Limitations:
;       *!* This routine presently is limited to a max of 64 processor cores
;
;   Description:
;       This procedure will do the following:
;       - allocate pre-defined address space for use as a stack for C code
;       - allocate pre-defined address space for use as a UEFI heap
;       - enable execution cache for a specified region
;       - create an instance of the UEFI structure UEFI_SEC_PEI_HAND_OFF on the
;           stack and populate it with values.
;
;     Stack Allocation:
;       Note: At present, the stack allocation is the same as described above in AMD_ENABLE_STACK_PRIVATE.
;           In fact, this macro uses that macro to perform the allocation.
;           The same 64 core limit applies to this implementation.
;       Future versions of this macro will expand support to 80+ cores.
;           Stack allocation will be 64k for the BSP, 16K for all APs.
;       ESP is set to point below the HostEnv stack frame.
;
;     Heap Allocation:
;       Note: At present, only the BSP will be allocated space for a UEFI heap.
;       Future versions of this macro will allcate 48K for each AP and the
;           allocation for the BSP will vary for the size of the L2 present and
;           the number of cores sharing the L2; maximizing the BSP allocation.
;
;     Execution cache:
;       The code will use Variable MTRRs 6 and 7 to define an area of memory
;       enabled for execution cache. This is presumed to include the PEI core
;       code area. The allocation is presummed to be at top-of-4G address range
;       so the smaller block, if one exists, will be allocated at the base
;       parameter (edx) and the larger block just after (edx+sizeof(smaller block))
;
;      HostEnv UEFI stack frame:
;       The code will create a stack data frame containing:
;       * a Host Env specific area for pseudo global variables.
;           o This area is at 'bottom (defalult)' so as to be retained if the PEI core decides
;               to reset the stack.
;           o The first entry in this area is the BIST value.
;       * an SEC-PEI hand-off structure (PI v1.2)
;           o populated with the stack and Temporary RAM entries.
;           o A processor's stack and heap are contiguous, with stack on 'top'.
;
;======================================================================
%macro AMD_ENABLE_UEFI_STACK2 2-3

    movd    mm1, ebp                    ; Save user requested register
    movd    mm0, ebx                    ; Logically 'push' the input parameters ( return address )
    movd    mm2, eax                    ;   ( BIST )
    movd    mm3, ecx                    ;   ( cache zone size )
    movd    mm4, edx                    ;   ( cache zone base )

    ; Short term method - need to accommodate existance of UEFI heap AND the AGESA heap.
    ;   So, use the old stack allocation process for stack, then mimick current UEFI (~v0.9)
    ;   operation to fill in the data stack frame.
    %if (%0 = 3)
        AMD_ENABLE_STACK_PRIVATE %1, %2, %3
    %else
        AMD_ENABLE_STACK_PRIVATE STACK_AT_BOTTOM, %1, %2
    %endif
    cmp     eax, AGESA_SUCCESS          ; Abort if not first entry; future versions will not allow multi-entry
    jne     %%AmdEnableUefiStackAbort

    ; review:
    ;       EAX = AGESA_STATUS
    ;       EDX = Return status code if EAX contains a return code of higher
    ;             severity than AGESA_SUCCESS
    ;       ECX = Stack size in bytes
    ;       ebx - return address parameter
    ;       ebp - user preserved register
    ;       ss:esp - stack pointer
    ;
    ;       esi -  address of start of stack block
    ;       [esp] - stack base address
    ;       [esp+4] - size of stack
    ;       [esp+8] - Marker for top-of-stack
    ;       mm0 - user return address
    ;       mm1 - user saved EBP register content
    ;       mm2 - BIST value
    ;       mm3 - cache zone size
    ;       mm5 - 32b pointer to family info struc. Set by GET_NODE_ID_CORE_ID_Fxx macros

    ; calculate stack frame pointer

    mov     ebp, [esp]
    mov     edx, ebp                            ; save stack base to edx

    ; for BSC, we divide the memory allocation zone in half and allocate 1/2 to each of stack & UEFI heap
    ; for APs, we allocate whole allocation to stack
    IS_BSC
    _if carry
        %if (%0 = 3)
            %if (%1 = STACK_AT_BOTTOM)
                shr ecx, 1
                add ebp, ecx
                shl ecx, 1
            %else
                add ebp, ecx
            %endif
        %else
            shr ecx, 1
            add ebp, ecx
            shl ecx, 1
        %endif
    _else
        add ebp, ecx
    _endif
    sub     ebp, (4 + AMD_STACK_FRAME_PAD)      ; space for BIST and additional OEM data
    movd    eax, mm2                            ; retrieve BIST data OEM acquired after reset
    mov     [ebp], eax                          ; place BIST data into first OEM data DWORD
    sub     ebp, UEFI_SEC_PEI_HAND_OFF_size     ; space for UEFI structure storage
    mov     eax, edx                            ; retrieve memory base address for passing on
    mov     esp, ebp                            ; now can update the esp
    ; fill the UEFI stack frame
    mov     [ebp + UEFI_SEC_PEI_HAND_OFF.TEMPORARY_RAM_BASE], eax
    mov     [ebp + UEFI_SEC_PEI_HAND_OFF.TEMPORARY_RAM_SIZE], ecx
    mov     word [ebp + UEFI_SEC_PEI_HAND_OFF.DATA_SIZE], UEFI_SEC_PEI_HAND_OFF_size

    ; for BSC, we divide the memory zone in half and allocate 1/2 to each of stack & UEFI heap
    IS_BSC
    _if carry
        push    ecx
        shr     ecx, 1                          ; divide the memory zone in half and allocate 1/2 to each of stack & UEFI heap
        mov     [ebp + UEFI_SEC_PEI_HAND_OFF.PEI_TEMPORARY_RAM_SIZE], ecx
        mov     [ebp + UEFI_SEC_PEI_HAND_OFF.STACK_SIZE], ecx

        %if (%0 = 3)
            %if (%1 = STACK_AT_BOTTOM)
                mov     [ebp + UEFI_SEC_PEI_HAND_OFF.STACK_BASE], eax
                add     eax, ecx                ; put PEI temporary RAM base in upper half
                mov     [ebp + UEFI_SEC_PEI_HAND_OFF.PEI_TEMPORARY_RAM_BASE], eax
            %else
                mov     [ebp + UEFI_SEC_PEI_HAND_OFF.PEI_TEMPORARY_RAM_BASE], eax
                add     eax, ecx                ; put stack base in upper half
                mov     [ebp + UEFI_SEC_PEI_HAND_OFF.STACK_BASE], eax
            %endif
        %else
            mov     [ebp + UEFI_SEC_PEI_HAND_OFF.STACK_BASE], eax
            add     eax, ecx                    ; put PEI temporary RAM base in upper half
            mov     [ebp + UEFI_SEC_PEI_HAND_OFF.PEI_TEMPORARY_RAM_BASE], eax
        %endif
        pop     ecx
    _else
    ; for APs, we allocate whole memory to stack
        mov     dword [ebp + UEFI_SEC_PEI_HAND_OFF.PEI_TEMPORARY_RAM_BASE], 0
        mov     dword [ebp + UEFI_SEC_PEI_HAND_OFF.PEI_TEMPORARY_RAM_SIZE], 0
        mov     [ebp + UEFI_SEC_PEI_HAND_OFF.STACK_SIZE], ecx
        mov     [ebp + UEFI_SEC_PEI_HAND_OFF.STACK_BASE], eax
    _endif

    ; we will use the cache zone as implied BFV,
    ; The OEM is free to override this from their code that follows
    movd    eax, mm3                            ; cache zone size
    mov     [ebp + UEFI_SEC_PEI_HAND_OFF.BOOT_FIRMWARE_VOLUME_SIZE], eax

    ; calculate the base from size
    movd    ebx, mm4
    _if ebx, e, 0
        sub     ebx, eax
        movd    mm4, ebx
    _endif
    mov     [ebp + UEFI_SEC_PEI_HAND_OFF.BOOT_FIRMWARE_VOLUME_BASE], ebx

    ; Round up the size if there are more than 2 bits set in the given cache zone size
    push    edx
    push    ecx
    push    eax

    bsr     ecx, eax
    _if nzero
        btr     eax, ecx                        ; there is one bit set in the given cache zone size
        bsr     ecx, eax
        _if nzero
            push    ecx
            btr     eax, ecx                    ; there are two bits set in the given cache zone size
            bsr     ecx, eax
            _if nzero
                pop     ecx                     ; ecx is the index of second bit set from most-significant
                pop     eax                     ; eax is the given cache zone size

                xor     edx, edx
                bts     edx, ecx
                add     eax, edx                ; round up the size
                dec     edx
                bts     edx, ecx                ; former 2nd bit spot should now be =0, clear it also
                not     edx
                and     eax, edx                ; now, eax has two bits set at most, could have only one

                push    eax
            _else
                pop     ecx                     ; balance the stack
            _endif
        _endif
    _endif

    pop     eax
    pop     ecx
    pop     edx
    movd    mm3, eax                            ; update cache zone size

    ; Check for and apply any family size limits.
    movd    edi, mm5
    mov     bx, [edi + CPU_FAMILY_INFO.L2_ALLOC_EXE]

    _if bx, a, 0                                ; if there is a family limit
        ; CPUID will destroyed EAX, EBX, ECX, EDX
        ; but we only want to preserve EAX, ECX, EDX
        push    eax
        push    ecx
        push    edx

        ; get L2 allocate execution cache = CPU_FAMILY_INFO.L2_ALLOC_EXE + (L2 cache size - CPU_FAMILY_INFO.L2_MIN_SIZE)
        AMD_CPUID   AMD_CPUID_L2Cache
        shr     ecx, 16                         ; CX = L2 cache size
        sub     cx, [edi + CPU_FAMILY_INFO.L2_MIN_SIZE]         ; CX = additional L2 size to the family limit
        mov     bx, [edi + CPU_FAMILY_INFO.L2_ALLOC_EXE]        ; use the additional L2 for exe cache
        add     bx,  cx

        ; restore EAX, ECX, EDX
        pop     edx
        pop     ecx
        pop     eax

        movzx   ebx, bx                         ;   convert the limit from K to Bytes
        shl     ebx, 10
        _if     eax, a, ebx                     ;   enforce the family limit
            ; note: SEC-PEI data is NOT updated on purpose, to allow the PEI
            ;   to see the full intended zone as the BFV


            mov     eax, ebx                    ;   set size to family limit
            movd    mm3, eax                    ;   update cache zone size
        _endif
    _endif

    ; base = 4G - size
;    push    edx
;    xor     edx, edx
;    sub     edx, eax
;    movd    mm4, edx
;    pop     edx
    ; review:
    ;       eax - Cache zone size
    ;       ebx -
    ;       ecx - Stack size in bytes
    ;       edx - Return status code if EAX contains a return code of higher
    ;             severity than AGESA_SUCCESS
    ;       ebp - Stack Frame pointer
    ;
    ;       esi -  address of start of stack block
    ;       mm0 - user return address
    ;       mm1 - user saved EBP register content
    ;       mm3 - cache zone size
    ;       mm4 - cache zone base
    ;       mm5 - 32b pointer to family info struc. Set by GET_NODE_ID_CORE_ID_Fxx macros

    ; Cross check the cache zone for proper base/length values,
    push    edx
    push    ecx

    and     eax, 0FFFF8000h                     ; size must be >= 32K

    ; Size a Power of Two? We can pull the two largest blocks from the size
    ; then set first avaible vMTRR to cover those blocks of the zone. The zone is presumed
    ; to be at the top of 4G memory space, so the blocks are allocated in a
    ; 'top down' manner, smaller first at base address then the larger.
    bsr     ecx, eax
    _if nzero                                   ; Is parameter non-zero?
        push    ecx                             ; save size of larger block
        btr     eax, ecx                        ; reduce zone size by 1st 2**N
        push    eax

        ; skip vMTRR setting if it's not a primary thread
        pushad
        AMD_CPUID   CPUID_MODEL
        shr     ebx, LOCAL_APIC_ID
        and     ebx, 0FFh ; ebx - initial local APIC physical ID
        push    ebx

        AMD_CPUID   AMD_CPUID_EXT_APIC

        pop     eax ; eax - initial local APIC physical ID
        shr     ebx, 8
        and     ebx, 0FFh
        inc     bl ; bl - ThreadsPerCore
        div     bl
        cmp     ah, 0
        popad
        jnz %%AmdSkipvMtrrSetting

        ; calculate upper mask value - needs to match the CPU address bus size
        movzx   ax, [edi + CPU_FAMILY_INFO.SIZE_ADDRESS_BUS]
        movzx   eax, ax
        xor     edx, edx

        ; find out the first vMTRR which is available
        push eax
        push ecx
        push edx
        mov ecx, AMD_MTRR_VARIABLE_MASK0
        _while ecx, be, AMD_MTRR_VARIABLE_MASK7
            rdmsr
            _if eax, e, 0
                mov edi, ecx
                dec edi                         ; now edi points to AMD_MTRR_VARIABLE_BASEx
                jmp %%AmdvMtrrFound
            _endif
            add ecx, 2
        _endw
%%AmdvMtrrFound:
        pop edx
        pop ecx
        pop eax
        _if edi, a, AMD_MTRR_VARIABLE_BASE7
            jmp %%AmdSkipvMtrrSetting           ; There's no enough vMTRR register pairs for ROM cache
        _endif


        _if al, be, 64
            bts     edx, eax
        _endif
        dec     edx                             ; edx = upper mask (e.g. 0x000FFFFF)
        pop     eax                             ; retrieve zone size (minus large block)
        bsr     ecx, eax
        _if nzero
            push    edx                             ; save upper mask, make room to calc new base
            ; set vMTRR[x] for Smaller block, if it exists
            xor     ebx, ebx
            dec     ebx                             ; ebx = all ones
            btr     ebx, ecx
            inc     ebx                             ; ebx = MTRR mask ( e.g 0xFFF80000)
            movd    eax, mm4                        ; cache zone base
            and     eax, ebx                        ; use mask to align base
            xor     edx, edx
            bts     edx, ecx                        ; edx = block size
            add     edx, eax                        ; add block size to base - for next block's base
            movd    mm4, edx                        ; update stored base value
            mov     al,  MTRR_TYPE_WP
            mov     ecx, edi                        ; use vMTRR pair # which is found above
            add     edi, 2                          ; point to the next vMTRR
            xor     edx, edx                        ; clear upper base
            wrmsr                                   ; set the vMTRR[6] Base
            mov     eax, ebx                        ; now build the mask
            pop     edx                             ; retrieve upper mask value
            bts     eax, VMTRR_VALID
            inc     ecx
            wrmsr                                   ; set the vMTRR[6] Mask + Valid
        _endif                                  ; Any remaining size is abandoned. We can only use 2 vMTRRs
        pop     ecx                             ; retrieve size of larger block
        push    edx                             ; save upper mask value
        ; set vMTRR[x + 1] for Larger block, if it exists
        _if edi, a, AMD_MTRR_VARIABLE_BASE7
            jmp %%AmdSkipvMtrrSetting           ; There's no enough vMTRR register pairs for ROM cache
        _endif

        xor     ebx, ebx
        dec     ebx                             ; ebx=all ones
        btr     ebx, ecx
        inc     ebx                             ; ebx = MTRR mask ( e.g 0xFFF00000)
        movd    eax, mm4                        ; cache zone base
        and     eax, ebx                        ; use mask to align base
        xor     edx, edx                        ; clear upper base
        mov     al,  MTRR_TYPE_WP
        mov     ecx, edi
        wrmsr                                   ; set the vMTRR[7] Base
        mov     eax, ebx                        ; now build the mask
        bts     eax, VMTRR_VALID
        pop     edx                             ; retrieve upper mask value
        inc     ecx
        wrmsr                                   ; set the vMTRR[7] Mask + Valid
    _endif

    ; prepare to exit
%%AmdSkipvMtrrSetting:
    mov     edi, ebp                    ; place stack frame pointer for return
    movd    ebp, mm1                    ; Restore saved user requested register
    movd    ebx, mm0                    ;   and the return address

    pop     ecx
    pop     edx
    mov     eax, AGESA_SUCCESS
%%AmdEnableUefiStackAbort:
%endmacro


;======================================================================
; AMD_DISABLE_UEFI_STACK2:  Dismantle the pre-memory cache-as-RAM mode.
;
;   In:
;       EBX  = Return address (preserved)
;
;   Out:
;       EAX = AGESA_SUCCESS
;
;   Description:
;       It is expected that the UEFI PEI core has relocated the stack to main
;       RAM by this time and the MTRR map has been sync'd. Therefore, this
;       routine will not modify the MTRR settings; but rather, just disable
;       the CAR mode. Cache tags will be invalidated.
;
;   Preserved:
;       ebx, esp
;   Destroyed:
;       eax, ebx, ecx, edx, esi, ebp
;======================================================================
%macro AMD_DISABLE_UEFI_STACK2 0

    mov     ebp, ebx                    ; Save return address

    ; get node/core/flags of current executing core
    GET_NODE_ID_CORE_ID                 ; Sets ESI[15,8]= Node#; ESI[7,0]= core# (relative to node); flags

    AMD_DISABLE_STACK_FAMILY_HOOK       ; Re-Enable 'normal' cache operations

    mov     ebx, ebp                    ; restore return address (ebx)
    xor     eax, AGESA_SUCCESS

%endmacro

;======================================================================
; IS_BSC:  Determine if this is Boot Strap Core
;
;   In:
;       NULL
;
;   Out:
;       CF = 1, it's BSC
;       CF = 0, it's AP
;
;   Destroyed:
;       CF
;======================================================================
%macro IS_BSC 0
    pushad
    mov     ecx, APIC_BASE_ADDRESS      ; MSR:0000_001B
    rdmsr
    bt      eax, APIC_BSC               ; Is this the BSC?
    popad

%endmacro


