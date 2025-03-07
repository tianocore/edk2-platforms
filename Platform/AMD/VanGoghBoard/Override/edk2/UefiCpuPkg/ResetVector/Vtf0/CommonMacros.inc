;------------------------------------------------------------------------------
; @file
; Common macros used in the ResetVector VTF module.
;
; Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
; Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%define ADDR16_OF(x) (0x10000 - fourGigabytes + x)
%define ADDR_OF(x) (0x100000000 - fourGigabytes + x)
%define ADDR_OF_MEM(x) (VIRTUAL4G - fourGigabytes + x)
%define SMM_RESUME_SIGNATURE 0x55AABB66
%macro  OneTimeCall 1
    jmp     %1
%1 %+ OneTimerCallReturn:
%endmacro

%macro  OneTimeCallRet 1
    jmp     %1 %+ OneTimerCallReturn
%endmacro

StartOfResetVectorCode:

%define ADDR_OF_START_OF_RESET_CODE ADDR_OF(StartOfResetVectorCode)

