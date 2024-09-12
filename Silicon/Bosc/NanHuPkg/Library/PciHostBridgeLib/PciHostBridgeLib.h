/** @file
  Main Header file for the PciHostBridgeLib

  Copyright (c) 2024, Bosc. All rights reserved.<BR>ved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __PCIHOSTBRIDGELIB_H
#define __PCIHOSTBRIDGELIB_H

#define BIT(nr)         (1UL << (nr))
#define GENMASK(h, l) \
    (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

/* Register definitions */
#define XILINX_PCIE_REG_RPSC           (0x00000148)

/* Root Port Status/control Register definitions */
#define XILINX_PCIE_REG_RPSC_BEN       BIT(0)

#define END_DEVICE_PATH_DEF { END_DEVICE_PATH_TYPE, \
                              END_ENTIRE_DEVICE_PATH_SUBTYPE, \
                              { END_DEVICE_PATH_LENGTH, 0 } \
                            }

#define ACPI_DEVICE_PATH_DEF(UID) {{ ACPI_DEVICE_PATH, ACPI_DP, \
                                     { (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
                                       (UINT8) (sizeof (ACPI_HID_DEVICE_PATH) >> 8)} \
                                     }, \
                                     EISA_PNP_ID (0x0A03), UID \
                                  }
#endif
