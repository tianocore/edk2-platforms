;/** @file
; AMD VanGoghBoard PlatformSecLib
; Platform Specific Definitions
;
; Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
; Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

%macro JMP32 1
  lea  esp, %%target
  jmp  %1
%%target:
%endmacro

%macro RET32 0
    jmp    esp
%endmacro

;
; RTC/CMOS definitions
;
RTC_INDEX      EQU 070h
  NMI_DISABLE  EQU 080h    ; Bit7=1 disables NMI
  NMI_ENABLE  EQU 000h    ; Bit7=0 disables NMI
RTC_DATA      EQU 071h

;
; PCI Configuration definitions
;
PCI_CFG            EQU 1 << 01Fh  ; PCI configuration access mechanism
PCI_ADDRESS_PORT   EQU 0CF8h
PCI_DATA_PORT      EQU 0CFCh

;
; LPC-ISA Bridge PCI Config Registers
;
LPC_PFA            EQU 00A3h          ; B0:D14:F3 (Legacy Block)
  LPC_IO_PDE         EQU 0044h        ; IO Port Decode Enable
  LPC_IOMEM_PDE      EQU 0048h        ; IO or Memory Port Decode Enable
  LPC_BASE_WIDE_PORT EQU 0064H
  LPC_WIDE_IO_EN     EQU 0074H
  LPC_WIDE_IO2       EQU 0090H
  LPC_SPI_BASE_ADDR  EQU 00A0H        ; SPI BAR = {SPI_BaseAddr[31:8],8'b0}
                                      ; HFP BAR = {SPI_BaseAddr[31:8],8'b0} + 0x0000_1000
                                      ; eSPI BAR = {SPI_BaseAddr[31:8],8'b0} + 0x0001_0000

ESPI_SLAVE0_DECODE_EN EQU 0040h
  ESPI_SLAVE0_DECODE_EN_IO_80_EN EQU  04h  ; Port 80 decode enabled

APIC_BASE_ADDRESS       EQU 0000001Bh
APIC_BSC                EQU 8       ; Boot Strap Core
