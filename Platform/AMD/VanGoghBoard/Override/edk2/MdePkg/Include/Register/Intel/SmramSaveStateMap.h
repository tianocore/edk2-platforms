/** @file
SMRAM Save State Map Definitions.

SMRAM Save State Map definitions based on contents of the
Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  Volume 3C, Section 34.4 SMRAM
  Volume 3C, Section 34.5 SMI Handler Execution Environment
  Volume 3C, Section 34.7 Managing Synchronous and Asynchronous SMIs

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTEL_SMRAM_SAVE_STATE_MAP_H_
#define INTEL_SMRAM_SAVE_STATE_MAP_H_
#define AMD_CPU  1
///
/// Default SMBASE address
///
#define SMM_DEFAULT_SMBASE  0x30000

///
/// Offset of SMM handler from SMBASE
///
#define SMM_HANDLER_OFFSET  0x8000

///
/// Offset of SMRAM Save State Map from SMBASE
///
#if AMD_CPU
#define SMRAM_SAVE_STATE_MAP_OFFSET  0xfe00
#else
#define SMRAM_SAVE_STATE_MAP_OFFSET  0xfc00
#endif

#pragma pack (1)

#if AMD_CPU
///
/// CPU save-state strcuture for AMD64 Architecture
///
typedef struct {
  UINT8     Reserved[0xF8];  // FE00h - FEF7h , Reserved, 248 Bytes, --
  UINT32    SMBASE;          // FEF8h, SMBASE, Doubleword, Read/Write
  UINT32    SMMRevId;        // FEFCh, SMM-Revision identifier, Doubleword, Read-Only
  UINT16    IORestart;       // FF00h, I/O Instruction Restart, Word, Read/Write
  UINT16    AutoHALTRestart; // FF02h, Auti-Halt Restart, Word, Read/Write
  UINT8     Reserved1[0x84]; // FF04h - FF87h, Reserved, 132 Bytes --
  UINT32    GdtBase;         // FF88h, GDT Base, Doubleword, Read-Only
  UINT64    Reserved2;       // FF8Ch - FF93h, Quadword, --
  UINT32    IDTBase;         // FF94h, IDT Base, Doubleword, Read-Only
  UINT8     Reserved3[0x10]; // FF98 - FFA7h, Reserved, 16Bytes, --
  UINT32    _ES;             // FFA8h, ES, Doubleword, Read-Only
  UINT32    _CS;             // FFACh, CS, Doubleword, Read-Only
  UINT32    _SS;             // FFB0h, SS, Doubleword, Read-Only
  UINT32    _DS;             // FFB4h, DS, Doubleword, Read-Only
  UINT32    _FS;             // FFB8h, FS, Doubleword, Read-Only
  UINT32    _GS;             // FFBCh, GS, Doubleword, Read-Only
  UINT32    LDTBase;         // FFC0h, LDT Base, Doubleword, Read-Only
  UINT32    _TR;             // FFC4h, TR, Doubleword, Read-Only
  UINT32    _DR7;            // FFC8h, DR7, Doubleword, Read-Only
  UINT32    _DR6;            // FFCCh, DR6, Doubleword, Read-Only
  UINT32    _EAX;            // FFD0h, EAX, Doubleword, Read/Write
  UINT32    _ECX;            // FFD4h, ECX, Doubleword, Read/Write
  UINT32    _EDX;            // FFD8h, EDX, Doubleword, Read/Write
  UINT32    _EBX;            // FFDCh, EBX, Doubleword, Read/Write
  UINT32    _ESP;            // FFE0h, ESP, Doubleword, Read/Write
  UINT32    _EBP;            // FFE4h, EBP, Doubleword, Read/Write
  UINT32    _ESI;            // FFE8h, ESI, Doubleword, Read/Write
  UINT32    _EDI;            // FFECh, EDI, Doubleword, Read/Write
  UINT32    _EIP;            // FFF0h, EIP, Doubleword, Read/Write
  UINT32    _EFLAGS;         // FFF4h, EFLAGS, Doubleword, Read/Write
  UINT32    _CR3;            // FFF8h, CR3, Doubleword, Read/Write
  UINT32    _CR0;            // FFFCh, CR0, Doubleword, Read/Write
} SMRAM_SAVE_STATE_MAP32;

typedef struct {
  UINT16    _ES;                 // FE00h, ES Selector, Word, Read-Only
  UINT16    ESAttributes;        // FE02h, ES Attributes, Word, Read-Only
  UINT32    ESLimit;             // FE04h, ES Limit, Doubleword, Read-Only
  UINT64    ESBase;              // FE08h, ES Base, Quadword, Read-Only
  UINT16    _CS;                 // FE10h, CS Selector, Word, Read-Only
  UINT16    CSAttributes;        // FE12h, CS Attributes, Word, Read-Only
  UINT32    CSLimit;             // FE14h, CS Limit, Doubleword, Read-Only
  UINT64    CSBase;              // FE18h, CS Base, Quadword, Read-Only
  UINT16    _SS;                 // FE20h, SS Selector, Word, Read-Only
  UINT16    SSAttributes;        // FE22h, SS Attributes, Word, Read-Only
  UINT32    SSLimit;             // FE24h, SS Limit, Doubleword, Read-Only
  UINT64    SSBase;              // FE28h, SS Base, Quadword, Read-Only
  UINT16    _DS;                 // FE30h, DS Selector, Word, Read-Only
  UINT16    DSAttributes;        // FE32h, DS Attributes, Word, Read-Only
  UINT32    DSLimit;             // FE34h, DS Limit, Doubleword, Read-Only
  UINT64    DSBase;              // FE38h, DS Base, Quadword, Read-Only
  UINT16    _FS;                 // FE40h, FS Selector, Word, Read-Only
  UINT16    FSAttributes;        // FE42h, FS Attributes, Word, Read-Only
  UINT32    FSLimit;             // FE44h, FS Limit, Doubleword, Read-Only
  UINT64    FSBase;              // FE48h, FS Base, Quadword, Read-Only
  UINT16    _GS;                 // FE50h, GS Selector, Word, Read-Only
  UINT16    GSAttributes;        // FE52h, GS Attributes, Word, Read-Only
  UINT32    GSLimit;             // FE54h, GS Limit, Doubleword, Read-Only
  UINT64    GSBase;              // FE58h, GS Base, Quadword, Read-Only
  UINT8     Reserved1[4];        // FE60h - FE63h, GDTR Reserved, 4 Bytes, Read-Only
  UINT16    GdtrLimit;           // FE64h, GDTR Limit, Word, Read-Only
  UINT8     Reserved2[2];        // FE66h - FE67h, Reserved, 2 Bytes, Read-Only
  //  UINT64  GDTR_Base;            // FE68h, GDTR Base, Quadword, Read-Only
  UINT32    GdtrBaseLoDword;
  UINT32    GdtrBaseHiDword;
  UINT16    LdtrSelector;        // FE70h, LDTR Selector, Word, Read-Only
  UINT16    LdtrAttributes;      // FE72h, LDTR Attributes, Word, Read-Only
  UINT32    LdtrLimit;           // FE74h, LDTR Limit, Doubleword, Read-Only
  //  UINT64  LDTR_Base;            // FE78h, LDTR Base, Quadword, Read-Only
  UINT32    LdtrBaseLoDword;
  UINT32    LdtrBaseHiDword;
  UINT8     Reserved3[4];        // FE80h - FE83h (PID: 24593 (PUB) 3.20 p.279 has a technical errors), IDTR Reserved, 4 Bytes, Read-Only
  UINT16    IdtrLimit;           // FE84h, IDTR Limit, Word, Read-Only
  UINT8     Reserved4[2];        // FE86h - FE87h (PID: 24593 (PUB) 3.20 p.279 has a technical errors), IDTR Reserved, 2 Bytes, Read-Only
  //  UINT64  IDTR_Base;            // FE88h, IDTR Base, Quadword, Read-Only
  UINT32    IdtrBaseLoDword;
  UINT32    IdtrBaseHiDword;
  UINT16    TrSelector;           // FE90h, TR Selector, Word, Read-Only
  UINT16    TrAttributes;         // FE92h, TR Attributes, Word, Read-Only
  UINT32    TrLimit;              // FE94h, TR Limit, Doubleword, Read-Only
  UINT64    TrBase;               // FE98h, TR Base, Quadword, Read-Only
  UINT64    IO_RESTART_RIP;       // FEA0h, I/O Instruction Restart RIP, Quadword, Read-Only
  UINT64    IO_RESTART_RCX;       // FEA8h, I/O Instruction Restart RCX, Quadword, Read-Only
  UINT64    IO_RESTART_RSI;       // FEB0h, I/O Instruction Restart RSI, Quadword, Read-Only
  UINT64    IO_RESTART_RDI;       // FEB8h, I/O Instruction Restart RDI, Quadword, Read-Only
  UINT32    SMM_IO_TRAP;          // FEC0h, I/O Instruction Restart Dword SMMFEC0 [SMM IO Trap Offset], Read-Only
  UINT32    LocalSmiStatus;       // FEC4h, SMMFEC4 [Local SMI Status], Doubleword, Read-Only
  UINT8     SMM_IO_RESTART;       // FEC8h, SMMFEC8 [SMM IO Restart Byte], Byte, Read/Write
  UINT8     AutoHALTRestart;      // FEC9h, SMMFEC9 [Auto Halt Restart Offset], Byte, Read/Write
  UINT8     NMI_MASK;             // FECAh, SMMFECA [NMI Mask], Byte, Read/Write
  UINT8     Reserved5[5];         // FECBh - FECFh, Reserved, 5 Bytes, --
  UINT64    EFER;                 // FED0h, EFER, Quadword, Read-Only
  UINT64    SMM_SVM_State;        // FED8h, SMMFED8 [SMM SVM State], Read-Only
  UINT64    Guest_VMCB_PHY_ADDR;  // FEE0h, Guest VMCB physical address, Read-Only
  UINT64    SVM_Virtual_INT_CTRL; // FEE8h, SVM Virtual Interrupt Control, Read-Only
  UINT8     Reserved6[12];        // FEF0h - FEFBh, Reserved, 12 Bytes (PID: 42300 Family 15h BKDG (NDA) 1.11 p.49 has a technial error), --
  UINT32    SMMRevId;             // FEFCh, SMMFEFC [SMM-Revision-Indentifier], Doubleword, Read/Write
  UINT32    SMBASE;               // FF00h, SMMFF00 [SMM Base Address (SMM_BASE)], Read/Write
  UINT8     Reserved7[28];        // FF04h, Reserved, 24 Bytes, --
  UINT64    GuestPAT;             // FF20h, Guest PAT, Quadword, Read-Only
  UINT64    HostEFER;             // FF28h, Host EFER, Quadword, Read-Only
  UINT64    HostCR4;              // FF30h, Host CR4, Quadword, Read-Only
  UINT64    NestedCR3;            // FF38h, Nested CR3, Quadword, Read-Only
  UINT64    HostCR0;              // FF40h, Host CR0, Quadword, Read-Only
  UINT64    _CR4;                 // FF48h, CR4, Quadword, Read-Only
  UINT64    _CR3;                 // FF50h, CR3, Quadword, Read-Only
  UINT64    _CR0;                 // FF58h, CR0, Quadword, Read-Only
  UINT64    _DR7;                 // FF60h, DR7, Quadword, Read-Only
  UINT64    _DR6;                 // FF68h, DR6, Quadword, Read-Only
  UINT64    _RFLAGS;              // FF70h, RFLAGS, Quadword, Read/Write
  UINT64    _RIP;                 // FF78h, RIP, Quadword, Read/Write
  UINT64    _R15;                 // FF80h, R15, Quadword, Read/Write
  UINT64    _R14;                 // FF88h, R14, Quadword, Read/Write
  UINT64    _R13;                 // FF90h, R13, Quadword, Read/Write
  UINT64    _R12;                 // FF98h, R12, Quadword, Read/Write
  UINT64    _R11;                 // FFA0h, R11, Quadword, Read/Write
  UINT64    _R10;                 // FFA8h, R10, Quadword, Read/Write
  UINT64    _R9;                  // FFB0h, R9, Quadword, Read/Write
  UINT64    _R8;                  // FFB8, R8, Quadword, Read/Write
  UINT64    _RDI;                 // FFC0h, RDI, Quadword, Read/Write
  UINT64    _RSI;                 // FFD8h, RSI, Quadword, Read/Write
  UINT64    _RBP;                 // FFD0h, RBP, Quadword, Read/Write
  UINT64    _RSP;                 // FFE0h, RSP, Quadword, Read/Write
  UINT64    _RBX;                 // FFE0h, RBX, Quadword, Read/Write
  UINT64    _RDX;                 // FFE8h, RDX, Quadword, Read/Write
  UINT64    _RCX;                 // FFF0h, RCX, Quadword, Read/Write
  UINT64    _RAX;                 // FFF8h, RAX, Quadword, Read/Write
} SMRAM_SAVE_STATE_MAP64;
#else
///
/// 32-bit SMRAM Save State Map
///
typedef struct {
  UINT8     Reserved[0x200]; // 7c00h
                             // Padded an extra 0x200 bytes so 32-bit and 64-bit
                             // SMRAM Save State Maps are the same size
  UINT8     Reserved1[0xf8]; // 7e00h
  UINT32    SMBASE;          // 7ef8h
  UINT32    SMMRevId;        // 7efch
  UINT16    IORestart;       // 7f00h
  UINT16    AutoHALTRestart; // 7f02h
  UINT8     Reserved2[0x9C]; // 7f08h
  UINT32    IOMemAddr;       // 7fa0h
  UINT32    IOMisc;          // 7fa4h
  UINT32    _ES;             // 7fa8h
  UINT32    _CS;             // 7fach
  UINT32    _SS;             // 7fb0h
  UINT32    _DS;             // 7fb4h
  UINT32    _FS;             // 7fb8h
  UINT32    _GS;             // 7fbch
  UINT32    Reserved3;       // 7fc0h
  UINT32    _TR;             // 7fc4h
  UINT32    _DR7;            // 7fc8h
  UINT32    _DR6;            // 7fcch
  UINT32    _EAX;            // 7fd0h
  UINT32    _ECX;            // 7fd4h
  UINT32    _EDX;            // 7fd8h
  UINT32    _EBX;            // 7fdch
  UINT32    _ESP;            // 7fe0h
  UINT32    _EBP;            // 7fe4h
  UINT32    _ESI;            // 7fe8h
  UINT32    _EDI;            // 7fech
  UINT32    _EIP;            // 7ff0h
  UINT32    _EFLAGS;         // 7ff4h
  UINT32    _CR3;            // 7ff8h
  UINT32    _CR0;            // 7ffch
} SMRAM_SAVE_STATE_MAP32;

///
/// 64-bit SMRAM Save State Map
///
typedef struct {
  UINT8     Reserved1[0x1d0]; // 7c00h
  UINT32    GdtBaseHiDword;   // 7dd0h
  UINT32    LdtBaseHiDword;   // 7dd4h
  UINT32    IdtBaseHiDword;   // 7dd8h
  UINT8     Reserved2[0xc];   // 7ddch
  UINT64    IO_EIP;           // 7de8h
  UINT8     Reserved3[0x50];  // 7df0h
  UINT32    _CR4;             // 7e40h
  UINT8     Reserved4[0x48];  // 7e44h
  UINT32    GdtBaseLoDword;   // 7e8ch
  UINT32    Reserved5;        // 7e90h
  UINT32    IdtBaseLoDword;   // 7e94h
  UINT32    Reserved6;        // 7e98h
  UINT32    LdtBaseLoDword;   // 7e9ch
  UINT8     Reserved7[0x38];  // 7ea0h
  UINT64    EptVmxControl;    // 7ed8h
  UINT32    EnEptVmxControl;  // 7ee0h
  UINT8     Reserved8[0x14];  // 7ee4h
  UINT32    SMBASE;           // 7ef8h
  UINT32    SMMRevId;         // 7efch
  UINT16    IORestart;        // 7f00h
  UINT16    AutoHALTRestart;  // 7f02h
  UINT8     Reserved9[0x18];  // 7f04h
  UINT64    _R15;             // 7f1ch
  UINT64    _R14;
  UINT64    _R13;
  UINT64    _R12;
  UINT64    _R11;
  UINT64    _R10;
  UINT64    _R9;
  UINT64    _R8;
  UINT64    _RAX;            // 7f5ch
  UINT64    _RCX;
  UINT64    _RDX;
  UINT64    _RBX;
  UINT64    _RSP;
  UINT64    _RBP;
  UINT64    _RSI;
  UINT64    _RDI;
  UINT64    IOMemAddr;       // 7f9ch
  UINT32    IOMisc;          // 7fa4h
  UINT32    _ES;             // 7fa8h
  UINT32    _CS;
  UINT32    _SS;
  UINT32    _DS;
  UINT32    _FS;
  UINT32    _GS;
  UINT32    _LDTR;           // 7fc0h
  UINT32    _TR;
  UINT64    _DR7;            // 7fc8h
  UINT64    _DR6;
  UINT64    _RIP;            // 7fd8h
  UINT64    IA32_EFER;       // 7fe0h
  UINT64    _RFLAGS;         // 7fe8h
  UINT64    _CR3;            // 7ff0h
  UINT64    _CR0;            // 7ff8h
} SMRAM_SAVE_STATE_MAP64;
#endif

///
/// Union of 32-bit and 64-bit SMRAM Save State Maps
///
typedef union  {
  SMRAM_SAVE_STATE_MAP32    x86;
  SMRAM_SAVE_STATE_MAP64    x64;
} SMRAM_SAVE_STATE_MAP;

///
/// Minimum SMM Revision ID that supports IOMisc field in SMRAM Save State Map
///
#define SMRAM_SAVE_STATE_MIN_REV_ID_IOMISC  0x30004

///
/// SMRAM Save State Map IOMisc I/O Length Values
///
#define  SMM_IO_LENGTH_BYTE   0x01
#define  SMM_IO_LENGTH_WORD   0x02
#define  SMM_IO_LENGTH_DWORD  0x04

///
/// SMRAM Save State Map IOMisc I/O Instruction Type Values
///
#define  SMM_IO_TYPE_IN_IMMEDIATE   0x9
#define  SMM_IO_TYPE_IN_DX          0x1
#define  SMM_IO_TYPE_OUT_IMMEDIATE  0x8
#define  SMM_IO_TYPE_OUT_DX         0x0
#define  SMM_IO_TYPE_INS            0x3
#define  SMM_IO_TYPE_OUTS           0x2
#define  SMM_IO_TYPE_REP_INS        0x7
#define  SMM_IO_TYPE_REP_OUTS       0x6

///
/// SMRAM Save State Map IOMisc structure
///
typedef union {
  struct {
    UINT32    SmiFlag   : 1;
    UINT32    Length    : 3;
    UINT32    Type      : 4;
    UINT32    Reserved1 : 8;
    UINT32    Port      : 16;
  } Bits;
  UINT32    Uint32;
} SMRAM_SAVE_STATE_IOMISC;

#pragma pack ()

#endif
