/** @file

   Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>

   SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#ifndef PCIE_HOT_PLUG_H_
#define PCIE_HOT_PLUG_H_

#define PCIE_HOT_PLUG_SPCI_CMD_ALERT_IRQ      1   // Alert IRQ
#define PCIE_HOT_PLUG_SPCI_CMD_START          2   // Stat monitor event
#define PCIE_HOT_PLUG_SPCI_CMD_CHG            3   // Indicate PCIE port change state explicitly
#define PCIE_HOT_PLUG_SPCI_CMD_LED            4   // Control LED state
#define PCIE_HOT_PLUG_SPCI_CMD_PORT_MAP_CLR   5   // Clear all port map
#define PCIE_HOT_PLUG_SPCI_CMD_PORT_MAP_SET   6   // Set port map
#define PCIE_HOT_PLUG_SPCI_CMD_PORT_MAP_LOCK  7   // Lock port map
#define PCIE_HOT_PLUG_SPCI_CMD_GPIO_MAP       8   // Set GPIO reset map

#define LED_FAULT  1      // LED_CMD: LED type - Fault
#define LED_ATT    2      // LED_CMD: LED type - Attention

#define LED_SET_ON     1
#define LED_SET_OFF    2
#define LED_SET_BLINK  3

// DEN0077A Arm Secure Partition Client Interface Specification 1.0 Beta_0_0

// Client ID used for SPCI calls
#define SPCI_CLIENT_ID  0x0000ACAC

// SPCI error codes.
#define SPCI_SUCCESS            0
#define SPCI_NOT_SUPPORTED      -1
#define SPCI_INVALID_PARAMETER  -2
#define SPCI_NO_MEMORY          -3
#define SPCI_BUSY               -4
#define SPCI_QUEUED             -5
#define SPCI_DENIED             -6
#define SPCI_NOT_PRESENT        -7

// Bit definitions inside the function id as per the SMC calling convention
#define FUNCID_CC_SHIFT   30
#define FUNCID_OEN_SHIFT  24

#define SMC_64  1
#define SMC_32  0

// Definitions to build the complete SMC ID
#define SPCI_FID_MISC_FLAG   (0 << 27)
#define SPCI_FID_MISC_SHIFT  20
#define SPCI_FID_TUN_FLAG    (1 << 27)
#define SPCI_FID_TUN_SHIFT   24

#define OEN_SPCI_START  0x30
#define OEN_SPCI_END    0x3F

#define SPCI_SMC(spci_fid)      ((OEN_SPCI_START << FUNCID_OEN_SHIFT) | \
                                 (1U << 31) | (spci_fid))
#define SPCI_MISC_32(misc_fid)  ((SMC_32 << FUNCID_CC_SHIFT) |  \
                                 SPCI_FID_MISC_FLAG |           \
                                 SPCI_SMC ((misc_fid) << SPCI_FID_MISC_SHIFT))
#define SPCI_MISC_64(misc_fid)  ((SMC_64 << FUNCID_CC_SHIFT) |  \
                                 SPCI_FID_MISC_FLAG |           \
                                 SPCI_SMC ((misc_fid) << SPCI_FID_MISC_SHIFT))
#define SPCI_TUN_64(tun_fid)    ((SMC_64 << FUNCID_CC_SHIFT) |  \
                                 SPCI_FID_TUN_FLAG |            \
                                 SPCI_SMC ((tun_fid) << SPCI_FID_TUN_SHIFT))

// SPCI miscellaneous functions
#define SPCI_FID_SERVICE_HANDLE_OPEN       0x2
#define SPCI_FID_SERVICE_HANDLE_CLOSE      0x3
#define SPCI_FID_SERVICE_REQUEST_BLOCKING  0x7
#define SPCI_FID_SERVICE_REQUEST_START     0x8

// SPCI tunneling functions
#define SPCI_FID_SERVICE_TUN_REQUEST_BLOCKING  0x2

// Complete SMC IDs and associated values
#define SPCI_SERVICE_HANDLE_OPEN                   \
                            SPCI_MISC_32 (SPCI_FID_SERVICE_HANDLE_OPEN)
#define SPCI_SERVICE_HANDLE_CLOSE                  \
                            SPCI_MISC_32 (SPCI_FID_SERVICE_HANDLE_CLOSE)
#define SPCI_SERVICE_REQUEST_BLOCKING_AARCH64      \
                            SPCI_MISC_64 (SPCI_FID_SERVICE_REQUEST_BLOCKING)
#define SPCI_SERVICE_REQUEST_START_AARCH64         \
                            SPCI_MISC_64 (SPCI_FID_SERVICE_REQUEST_START)
#define SPCI_SERVICE_TUN_REQUEST_BLOCKING_AARCH64  \
                            SPCI_TUN_64 (SPCI_FID_SERVICE_TUN_REQUEST_BLOCKING)

#pragma pack(1)

typedef struct {
  UINT64    Token;
  UINT32    HandleId;
  UINT64    SpciCommand;
  UINT64    SpciParam1;
  UINT64    SpciParam2;
  UINT64    SpciParam3;
  UINT64    SpciParam4;
  UINT64    SpciParam5;
} AMPERE_SPCI_ARGS;

#pragma pack()

/**
  Set GPIO pins used for PCIe reset. This command
  limits the number of GPIO[16:21] for reset purpose.
**/
VOID
PcieHotPlugSetGpioMap (
  VOID
  );

/**
  Lock current Portmap table.
**/
VOID
PcieHotPlugSetLockPortMap (
  VOID
  );

/**
  Start Hot plug service.
**/
VOID
PcieHotPlugSetStart (
  VOID
  );

/**
  Clear current configuration of Portmap table.
**/
VOID
PcieHotPlugSetClear (
  VOID
  );

/**
  Set configuration for Portmap table.
**/
VOID
PcieHotPlugSetPortMap (
  VOID
  );

/**
  This function will start Hotplug service after following steps:
  - Open handle to make a SPCI call.
  - Set GPIO pins for PCIe reset.
  - Set configuration for Portmap table.
  - Lock current Portmap table.
  - Start Hot plug service.
  - Close handle.
**/
VOID
PcieHotPlugStart (
  VOID
  );

#endif
