/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

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
#define AC01_RCA0_DEVICE_MEMORY_S0_BASE  0x33FFE0000000ULL
#define AC01_RCA0_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

//
// PCIe RCA1 Device memory
//
#define AC01_RCA1_DEVICE_MEMORY_S0_BASE  0x37FFE0000000ULL
#define AC01_RCA1_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

//
// PCIe RCA2 Device memory
//
#define AC01_RCA2_DEVICE_MEMORY_S0_BASE  0x3BFFE0000000ULL
#define AC01_RCA2_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

//
// PCIe RCA3 Device memory
//
#define AC01_RCA3_DEVICE_MEMORY_S0_BASE  0x3FFFE0000000ULL
#define AC01_RCA3_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

//
// PCIe RCB0 Device memory
//
#define AC01_RCB0_DEVICE_MEMORY_S0_BASE  0x23FFE0000000ULL
#define AC01_RCB0_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

//
// PCIe RCB1 Device memory
//
#define AC01_RCB1_DEVICE_MEMORY_S0_BASE  0x27FFE0000000ULL
#define AC01_RCB1_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

//
// PCIe RCB2 Device memory
//
#define AC01_RCB2_DEVICE_MEMORY_S0_BASE  0x2BFFE0000000ULL
#define AC01_RCB2_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

//
// PCIe RCB3 Device memory
//
#define AC01_RCB3_DEVICE_MEMORY_S0_BASE  0x2FFFE0000000ULL
#define AC01_RCB3_DEVICE_MEMORY_S0_SIZE  0x000020000000ULL

// *******************************************************************
// Socket 1 PCIe Device Memory
// *******************************************************************
//
// PCIe RCA0 Device memory
//
#define AC01_RCA0_DEVICE_MEMORY_S1_BASE  0x73FFE0000000ULL
#define AC01_RCA0_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

//
// PCIe RCA1 Device memory
//
#define AC01_RCA1_DEVICE_MEMORY_S1_BASE  0x77FFE0000000ULL
#define AC01_RCA1_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

//
// PCIe RCA2 Device memory
//
#define AC01_RCA2_DEVICE_MEMORY_S1_BASE  0x7BFFE0000000ULL
#define AC01_RCA2_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

//
// PCIe RCA3 Device memory
//
#define AC01_RCA3_DEVICE_MEMORY_S1_BASE  0x7FFFE0000000ULL
#define AC01_RCA3_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

//
// PCIe RCB0 Device memory
//
#define AC01_RCB0_DEVICE_MEMORY_S1_BASE  0x63FFE0000000ULL
#define AC01_RCB0_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

//
// PCIe RCB1 Device memory
//
#define AC01_RCB1_DEVICE_MEMORY_S1_BASE  0x67FFE0000000ULL
#define AC01_RCB1_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

//
// PCIe RCB2 Device memory
//
#define AC01_RCB2_DEVICE_MEMORY_S1_BASE  0x6BFFE0000000ULL
#define AC01_RCB2_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

//
// PCIe RCB3 Device memory
//
#define AC01_RCB3_DEVICE_MEMORY_S1_BASE  0x6FFFE0000000ULL
#define AC01_RCB3_DEVICE_MEMORY_S1_SIZE  0x000020000000ULL

#endif /* PLATFORM_MEMORY_MAP_H_ */
