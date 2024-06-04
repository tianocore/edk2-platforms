;------------------------------------------------------------------------------
; Module Name:
;   Platform.inc
;
; Abstract:
;
;   Platform Specific Definitions
;
; Copyright (c) 2013-2015 Intel Corporation
; Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%macro JMP32 1
  lea  esp, [%%return_address]
  jmp  %1
%%return_address:
%endmacro

%macro RET32 0
    jmp    esp
%endmacro

;
; RTC/CMOS definitions
;
RTC_INDEX         equ 0x70
  NMI_DISABLE     equ 0x80    ; Bit7=1 disables NMI
  NMI_ENABLE      equ 0x00    ; Bit7=0 disables NMI
RTC_DATA          equ 0x71

;
; PCI Configuration definitions
;
PCI_CFG           equ 1 << 0x1F  ; PCI configuration access mechanism
PCI_ADDRESS_PORT  equ 0xCF8
PCI_DATA_PORT     equ 0xCFC

;
; ILB Reset Register
;
ILB_RESET_REG       equ 0xCF9
  CF9_WARM_RESET    equ 0x2
  CF9_COLD_RESET    equ 0x8

