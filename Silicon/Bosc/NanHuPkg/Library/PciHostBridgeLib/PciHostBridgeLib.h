/** @file
  Main Header file for the PciHostBridgeLib

  Copyright (c) 2024, Bosc. All rights reserved.<BR>ved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __PCIHOSTBRIDGELIB_H
#define __PCIHOSTBRIDGELIB_H

/* Register definitions */
#define XILINX_PCIE_REG_RPSC           (0x00000148)

/* Root Port Status/control Register definitions */
#define XILINX_PCIE_REG_RPSC_BEN       BIT(0)

#define END_DEVICE_PATH_DEF { END_DEVICE_PATH_TYPE, \
                              END_ENTIRE_DEVICE_PATH_SUBTYPE, \
                              { END_DEVICE_PATH_LENGTH, 0 } \
                            }

#define PCI_DEVICE_PATH_NODE(Func, Dev) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      } \
    }, \
    (Func), \
    (Dev) \
  }

#endif
