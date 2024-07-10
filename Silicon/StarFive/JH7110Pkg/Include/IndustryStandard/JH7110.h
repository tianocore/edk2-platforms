/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef JH7110_H__
#define JH7110_H__

/* Generic PCI addresses */
#define PCIE_TOP_OF_MEM_WIN   (FixedPcdGet64 (PcdPciBusMmioAdr))
#define PCIE_CPU_MMIO_WINDOW  (FixedPcdGet64 (PcdPciCpuMmioAdr))
#define PCIE_BRIDGE_MMIO_LEN  (FixedPcdGet32 (PcdPciBusMmioLen))

/* PCI root bridge control registers location */
#define PCIE_REG_BASE     (FixedPcdGet64 (PcdPciRegBase))
#define PCIE_CONFIG_BASE  (FixedPcdGet64 (PcdPciConfigRegBase))

#endif /* JH7110_H__ */
