/** @file
  System Topology Report driver header file.

  Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Protocol/SysTopologyProtocol.h>

#define DEFAULT_REPORT_SIZE     0x1000
#define MAX_REPORT_SIZE         0xF4240
#define DEFAULT_ENTRY_SIZE      0x100
#define SYS_TOPOLOGY_SIGNATURE  SIGNATURE_32 ('U', 'S', 'T', 'R')
#define HOST_2_BMC_VIDDID       0x24021a03
#define HOST_2_BMC_MSG_QUEUE1   0x30000
#define HOST_2_BMC_Q_STATUS     0x30044

#define PCIE_EXT_CAP_SERIAL  0x3

#define PCIE_CAP_ID            0x10
#define PCIE_LINK_CAP_OFFSET   0xC
#define PCIE_LINK_CTRL_OFFSET  0x10

// *******************************************************
//  PCI Configuration Space Structures
// *******************************************************

typedef struct _PCI_COMMON_HEADER {
  UINT16    VendorId;
  UINT16    DeviceId;
  UINT16    Command;
  UINT16    Status;
  UINT8     RevisionID;
  UINT8     ClassCode[3];
  UINT8     CacheLineSize;
  UINT8     LatencyTimer;
  UINT8     HeaderType;
  UINT8     Bist;
} PCI_COMMON_HEADER;

#define PCIE_CAP_POINTER      0x34
#define PCIE_EXT_CAP_POINTER  0x100

typedef struct _PCI_CAPABILITY_ID {
  UINT8    CapabilityId;
  UINT8    NextId;
} PCI_CAPABILITY_ID;

typedef struct _PCI_EXT_CAPABILITY_ID {
  UINT32    ExtCapabilityId      : 16;
  UINT32    ExtCapabilityVersion : 4;
  UINT32    NextId               : 12;
} PCI_EXT_CAPABILITY_ID;

typedef struct _PCI_EXT_CAPABILITY_SERIAL_NUMBER {
  PCI_EXT_CAPABILITY_ID    Header;
  UINT32                   Lower;
  UINT32                   Upper;
} PCI_EXT_CAPABILITY_SERIAL_NUMBER;
