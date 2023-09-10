/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef JH7110_H__
#define JH7110_H__

// #define JH7110_SOC_REGISTERS               (FixedPcdGet64 (PcdJH7110RegistersAddress))
// #define JH7110_SOC_REGISTER_LENGTH         0x02000000

/* Generic PCI addresses */
#define PCIE_TOP_OF_MEM_WIN   (FixedPcdGet64 (PcdJH7110PciBusMmioAdr))
#define PCIE_CPU_MMIO_WINDOW  (FixedPcdGet64 (PcdJH7110PciCpuMmioAdr))
#define PCIE_BRIDGE_MMIO_LEN  (FixedPcdGet32 (PcdJH7110PciBusMmioLen))

/* PCI root bridge control registers location */
#define PCIE_REG_BASE     (FixedPcdGet64 (PcdJH7110PciRegBase))
#define PCIE_CONFIG_BASE  (FixedPcdGet64 (PcdJH7110PciConfigRegBase))

#endif /* JH7110_H__ */
