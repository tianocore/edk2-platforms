;------------------------------------------------------------------------------
; @file
; First code executed by processor after resetting.
;
; Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
; Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    16

ALIGN   16

;
; Pad the image size to 4k when page tables are in VTF0
;
; If the VTF0 image has page tables built in, then we need to make
; sure the end of VTF0 is 4k above where the page tables end.
;
; This is required so the page tables will be 4k aligned when VTF0 is
; located just below 0x100000000 (4GB) in the firmware device.
;
%ifdef ALIGN_TOP_TO_4K_FOR_PAGING
    TIMES (0x1000 - ($ - EndOfPageTables) - 0x50) DB 0
%endif

; 16 bytes reserved for Anti-rollback security level
;  - 04 bytes: Security level
;  - 12 bytes: Pad 0x00
    DD AntiRollback_SecurityLevel
    TIMES 12 DB 0

;
; 32 bytes reserved for BIOS version string and build date and time
; Signature 4 bytes: BIVS
; Pad0      1 byte,  00
; Version   8 bytes, such as UMD9B18C
; Pad1      1 byte,  00
; Date      4 bytes, such as 20191118
; Pad2      1 byte,  00
; Time      3 bytes, such as 113028
; Pad3      10 byte,  00
;

BiosVersionDateTimeSignature:
    DB 'B', 'I', 'V', 'S'

Pad0:
    DB 0

Version:
    TIMES 8 DB 0

Pad1:
    DB 0

Date:
    TIMES 4 DB 0

Pad2:
    DB 0

DateTime:
    TIMES 7 DB 0

Pad3:
    TIMES 10 DB 0

applicationProcessorEntryPoint:
;
; Application Processors entry point
;
; GenFv generates code aligned on a 4k boundary which will jump to this
; location.  (0xffffffe0)  This allows the Local APIC Startup IPI to be
; used to wake up the application processors.
;
    jmp     EarlyApInitReal16

ALIGN   8

    DD      0

;
; The VTF signature
;
; VTF-0 means that the VTF (Volume Top File) code does not require
; any fixups.
;
vtfSignature:
    DB      'V', 'T', 'F', 0

ALIGN   16

resetVector:
;
; Reset Vector
;
; This is where the processor will begin execution
;
    nop
    nop
    jmp   near  EarlyBspInitReal16

ALIGN   16

fourGigabytes:

