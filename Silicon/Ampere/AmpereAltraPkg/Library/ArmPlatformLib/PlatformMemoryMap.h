/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_MEMORY_MAP_H_
#define PLATFORM_MEMORY_MAP_H_

// *******************************************************************
// Platform Memory Map
// *******************************************************************
//
// Device Memory (Socket 0)
//
#define AC01_DEVICE_MEMORY_S0_BASE  0x100000000000ULL
#define AC01_DEVICE_MEMORY_S0_SIZE  0x102000000ULL

//
// Device Memory (Socket 1)
//
#define AC01_DEVICE_MEMORY_S1_BASE  0x500000000000ULL
#define AC01_DEVICE_MEMORY_S1_SIZE  0x101000000ULL

//
// BERT memory
//
#define AC01_BERT_MEMORY_BASE  0x88230000ULL
#define AC01_BERT_MEMORY_SIZE  0x50000ULL

// *******************************************************************
// Socket 0 PCIe Device Memory
// *******************************************************************
//
// PCIe RCA0 Device memory
//
#define AC01_RCA0_DEVICE_MEMORY_S0_BASE  0x300000000000ULL
#define AC01_RCA0_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCA0 32-bit Device memory
// 1P/PCIe consolidated to RCB2 32-bit Device memory
//
#define AC01_RCA0_32_BIT_DEVICE_MEMORY_S0_BASE  0x20000000ULL
#define AC01_RCA0_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

//
// PCIe RCA1 Device memory
//
#define AC01_RCA1_DEVICE_MEMORY_S0_BASE  0x340000000000ULL
#define AC01_RCA1_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCA1 32-bit Device memory
// 1P/PCIe consolidated to RCB2 32-bit Device memory
//
#define AC01_RCA1_32_BIT_DEVICE_MEMORY_S0_BASE  0x28000000ULL
#define AC01_RCA1_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

//
// PCIe RCA2 Device memory
//
#define AC01_RCA2_DEVICE_MEMORY_S0_BASE  0x380000000000ULL
#define AC01_RCA2_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCA2 32-bit Device memory
// 1P/PCIe consolidated to RCB3 32-bit Device memory
//
#define AC01_RCA2_32_BIT_DEVICE_MEMORY_S0_BASE  0x30000000ULL
#define AC01_RCA2_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

//
// PCIe RCA3 Device memory
//
#define AC01_RCA3_DEVICE_MEMORY_S0_BASE  0x3C0000000000ULL
#define AC01_RCA3_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCA3 32-bit Device memory
// 1P/PCIe consolidated to RCB3 32-bit Device memory
//
#define AC01_RCA3_32_BIT_DEVICE_MEMORY_S0_BASE  0x38000000ULL
#define AC01_RCA3_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

//
// PCIe RCB0 Device memory
//
#define AC01_RCB0_DEVICE_MEMORY_S0_BASE  0x200000000000ULL
#define AC01_RCB0_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCB0 32-bit Device memory
// 1P/PCIe consolidated to RCB0 32-bit Device memory
//
#define AC01_RCB0_32_BIT_DEVICE_MEMORY_S0_BASE  0x00000000ULL
#define AC01_RCB0_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

//
// PCIe RCB1 Device memory
//
#define AC01_RCB1_DEVICE_MEMORY_S0_BASE  0x240000000000ULL
#define AC01_RCB1_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCB1 32-bit Device memory
// 1P/PCIe consolidated to RCB0 32-bit Device memory
//
#define AC01_RCB1_32_BIT_DEVICE_MEMORY_S0_BASE  0x08000000ULL
#define AC01_RCB1_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

//
// PCIe RCB2 Device memory
//
#define AC01_RCB2_DEVICE_MEMORY_S0_BASE  0x280000000000ULL
#define AC01_RCB2_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCB2 32-bit Device memory
// 1P/PCIe consolidated to RCB1 32-bit Device memory
//
#define AC01_RCB2_32_BIT_DEVICE_MEMORY_S0_BASE  0x10000000ULL
#define AC01_RCB2_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

//
// PCIe RCB3 Device memory
//
#define AC01_RCB3_DEVICE_MEMORY_S0_BASE  0x2C0000000000ULL
#define AC01_RCB3_DEVICE_MEMORY_S0_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket0 RCB3 32-bit Device memory
// 1P/PCIe consolidated to RCB1 32-bit Device memory
//
#define AC01_RCB3_32_BIT_DEVICE_MEMORY_S0_BASE  0x18000000ULL
#define AC01_RCB3_32_BIT_DEVICE_MEMORY_S0_SIZE  0x8000000ULL

// *******************************************************************
// Socket 1 PCIe Device Memory
// *******************************************************************
//
// PCIe RCA0 Device memory
//
#define AC01_RCA0_DEVICE_MEMORY_S1_BASE  0x700000000000ULL
#define AC01_RCA0_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCA0 32-bit Device memory
// 1P/PCIe consolidated to RCB2 32-bit Device memory
//
#define AC01_RCA0_32_BIT_DEVICE_MEMORY_S1_BASE  0x60000000ULL
#define AC01_RCA0_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

//
// PCIe RCA1 Device memory
//
#define AC01_RCA1_DEVICE_MEMORY_S1_BASE  0x740000000000ULL
#define AC01_RCA1_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCA1 32-bit Device memory
// 1P/PCIe consolidated to RCB2 32-bit Device memory
//
#define AC01_RCA1_32_BIT_DEVICE_MEMORY_S1_BASE  0x68000000ULL
#define AC01_RCA1_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

//
// PCIe RCA2 Device memory
//
#define AC01_RCA2_DEVICE_MEMORY_S1_BASE  0x780000000000ULL
#define AC01_RCA2_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCA2 32-bit Device memory
// 1P/PCIe consolidated to RCB3 32-bit Device memory
//
#define AC01_RCA2_32_BIT_DEVICE_MEMORY_S1_BASE  0x70000000ULL
#define AC01_RCA2_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

//
// PCIe RCA3 Device memory
//
#define AC01_RCA3_DEVICE_MEMORY_S1_BASE  0x7C0000000000ULL
#define AC01_RCA3_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCA3 32-bit Device memory
// 1P/PCIe consolidated to RCB3 32-bit Device memory
//
#define AC01_RCA3_32_BIT_DEVICE_MEMORY_S1_BASE  0x78000000ULL
#define AC01_RCA3_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

//
// PCIe RCB0 Device memory
//
#define AC01_RCB0_DEVICE_MEMORY_S1_BASE  0x600000000000ULL
#define AC01_RCB0_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCB0 32-bit Device memory
// 1P/PCIe consolidated to RCB0 32-bit Device memory
//
#define AC01_RCB0_32_BIT_DEVICE_MEMORY_S1_BASE  0x40000000ULL
#define AC01_RCB0_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

//
// PCIe RCB1 Device memory
//
#define AC01_RCB1_DEVICE_MEMORY_S1_BASE  0x640000000000ULL
#define AC01_RCB1_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCB1 32-bit Device memory
// 1P/PCIe consolidated to RCB0 32-bit Device memory
//
#define AC01_RCB1_32_BIT_DEVICE_MEMORY_S1_BASE  0x48000000ULL
#define AC01_RCB1_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

//
// PCIe RCB2 Device memory
//
#define AC01_RCB2_DEVICE_MEMORY_S1_BASE  0x680000000000ULL
#define AC01_RCB2_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCB2 32-bit Device memory
// 1P/PCIe consolidated to RCB1 32-bit Device memory
//
#define AC01_RCB2_32_BIT_DEVICE_MEMORY_S1_BASE  0x50000000ULL
#define AC01_RCB2_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

//
// PCIe RCB3 Device memory
//
#define AC01_RCB3_DEVICE_MEMORY_S1_BASE  0x6C0000000000ULL
#define AC01_RCB3_DEVICE_MEMORY_S1_SIZE  0x40000000000ULL

//
// 2P/PCIe Socket1 RCB3 32-bit Device memory
// 1P/PCIe consolidated to RCB1 32-bit Device memory
//
#define AC01_RCB3_32_BIT_DEVICE_MEMORY_S1_BASE  0x58000000ULL
#define AC01_RCB3_32_BIT_DEVICE_MEMORY_S1_SIZE  0x8000000ULL

#endif /* PLATFORM_MEMORY_MAP_H_ */
