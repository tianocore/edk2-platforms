/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_AC01_H_
#define PLATFORM_AC01_H_

//
// Number of supported sockets in the platform
//
#define PLATFORM_CPU_MAX_SOCKET  2

//
// Maximum number of CPMs in the chip.
//
#define PLATFORM_CPU_MAX_CPM  (FixedPcdGet32 (PcdClusterCount))

//
// Number of cores per CPM.
//
#define PLATFORM_CPU_NUM_CORES_PER_CPM  (FixedPcdGet32 (PcdCoreCount) / PLATFORM_CPU_MAX_CPM)

//
// Maximum number of cores supported.
//
#define PLATFORM_CPU_MAX_NUM_CORES  (PLATFORM_CPU_MAX_SOCKET * PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM)

#define MAX_AMPERE_ALTRA_CORES      80
#define MAX_AMPERE_ALTRA_MAX_CORES  128

//
// Maximum number of memory region
//
#define PLATFORM_DRAM_INFO_MAX_REGION  16

//
// Maximum number of DDR slots supported
//
#define PLATFORM_DIMM_INFO_MAX_SLOT  32

//
// CSR Address base for slave socket
//
#define SLAVE_SOCKET_BASE_ADDRESS_OFFSET  0x400000000000

//
// SMpro EFUSE Shadow register
//
#define SMPRO_EFUSE_SHADOW0  (FixedPcdGet64 (PcdSmproEfuseShadow0))

//
// 2P Configuration Register
//
#define CFG2P_OFFSET  0x200

//
// Slave socket present
//
#define SLAVE_PRESENT_N  BIT1

//
// The maximum number of I2C bus
//
#define AC01_I2C_MAX_BUS_NUM  18

//
// The base address of DW I2C
//
#define AC01_I2C_BASE_ADDRESS_LIST  0x100002690000, /* Socket 0 IIC 2 */ \
                                    0x1000026A0000, /* IIC 3 */ \
                                    0x1000026B0000, /* IIC 4 */ \
                                    0x1000026C0000, /* IIC 5 */ \
                                    0x100002750000, /* IIC 6 */ \
                                    0x100002760000, /* IIC 7 */ \
                                    0x100002770000, /* IIC 8 */ \
                                    0x100002780000, /* IIC 9 */ \
                                    0x100002790000, /* IIC 10 */ \
                                    0x500002690000, /* Socket 1 IIC 2 */ \
                                    0x5000026A0000, /* IIC 3 */ \
                                    0x5000026B0000, /* IIC 4 */ \
                                    0x5000026C0000, /* IIC 5 */ \
                                    0x500002750000, /* IIC 6 */ \
                                    0x500002760000, /* IIC 7 */ \
                                    0x500002770000, /* IIC 8 */ \
                                    0x500002780000, /* IIC 9 */ \
                                    0x500002790000  /* IIC 10 */

//
// The Array of Soc Gpio Base Address
//
#define AC01_GPIO_BASE_ADDRESS_LIST  0x1000026f0000, 0x1000026e0000, 0x1000027b0000, 0x1000026d0000, 0x5000026f0000, 0x5000026e0000, 0x5000027b0000, 0x5000026d0000

//
// The Array of Soc Gpi Base Address
//
#define AC01_GPI_BASE_ADDRESS_LIST  0x1000026d0000, 0x5000026d0000

//
// Number of Pins Per Each Contoller
//
#define AC01_GPIO_PINS_PER_CONTROLLER  8

//
// Number of Pins Each Socket
//
#define AC01_GPIO_PINS_PER_SOCKET  32

//
// Maximum number of memory controller supports NVDIMM-N per socket
//
#define AC01_NVDIMM_MAX_MCU_PER_SOCKET  2

//
// Maximum number of NVDIMM-N per memory controller
//
#define AC01_NVDIMM_MAX_DIMM_PER_MCU  1

//
// Maximum number of NVDIMM region per socket
//
#define AC01_NVDIMM_MAX_REGION_PER_SOCKET  2

//
// Socket 0 base address of NVDIMM non-hashed region 0
//
#define AC01_NVDIMM_SK0_NHASHED_REGION0_BASE  0x0B0000000000ULL

//
// Socket 0 base address of NVDIMM non-hashed region 1
//
#define AC01_NVDIMM_SK0_NHASHED_REGION1_BASE  0x0F0000000000ULL

//
// Socket 1 base address of NVDIMM non-hashed region 0
//
#define AC01_NVDIMM_SK1_NHASHED_REGION0_BASE  0x430000000000ULL

//
// Socket 1 base address of NVDIMM non-hashed region 1
//
#define AC01_NVDIMM_SK1_NHASHED_REGION1_BASE  0x470000000000ULL

//
// DIMM ID of NVDIMM-N device 1
//
#define AC01_NVDIMM_NVD1_DIMM_ID  6

//
// DIMM ID of NVDIMM-N device 2
//
#define AC01_NVDIMM_NVD2_DIMM_ID  14

//
// DIMM ID of NVDIMM-N device 3
//
#define AC01_NVDIMM_NVD3_DIMM_ID  22

//
// DIMM ID of NVDIMM-N device 4
//
#define AC01_NVDIMM_NVD4_DIMM_ID  30

//
// NFIT device handle of NVDIMM-N device 1
//
#define AC01_NVDIMM_NVD1_DEVICE_HANDLE  0x0330

//
// NFIT device handle of NVDIMM-N device 2
//
#define AC01_NVDIMM_NVD2_DEVICE_HANDLE  0x0770

//
// NFIT device handle of NVDIMM-N device 3
//
#define AC01_NVDIMM_NVD3_DEVICE_HANDLE  0x1330

//
// NFIT device handle of NVDIMM-N device 4
//
#define AC01_NVDIMM_NVD4_DEVICE_HANDLE  0x1770

//
// Interleave ways of non-hashed NVDIMM-N
//
#define AC01_NVDIMM_NHASHED_INTERLEAVE_WAYS  1

//
// Interleave ways of hashed NVDIMM-N
//
#define AC01_NVDIMM_HASHED_INTERLEAVE_WAYS  2

//
// Region offset of hashed NVDIMM-N
//
#define AC01_NVDIMM_HASHED_REGION_OFFSET  512

//
// The base address of GIC distributor registers
//
#define AC01_GICD_MASTER_BASE_ADDRESS  0x100100000000

//
// The base address of master socket GIC redistributor registers
//
#define AC01_GICR_MASTER_BASE_ADDRESS  0x100100140000

//
// The base address of slave socket GIC distributor registers
//
#define AC01_GICD_SLAVE_BASE_ADDRESS  0x500100000000

//
// The base address of slave socket GIC redistributor registers
//
#define AC01_GICR_SLAVE_BASE_ADDRESS  0x500100140000

//
// Socket 0 first RC
//
#define SOCKET0_FIRST_RC  2

//
// Socket 0 last RC
//
#define SOCKET0_LAST_RC  7

//
// Socket 1 first RC
//
#define SOCKET1_FIRST_RC  10

//
// Socket 1 last RC
//
#define SOCKET1_LAST_RC  15

//
// Socket bit offset of core UID.
//
#define PLATFORM_SOCKET_UID_BIT_OFFSET  16

//
// CPM bit offset of core UID.
//
#define PLATFORM_CPM_UID_BIT_OFFSET  8

//
// MPIDR manipulation
//
#define AC01_GET_MPIDR(SocketId, ClusterId, CoreId) \
          (((SocketId) << 32) | ((ClusterId) << 16) | ((CoreId) << 8))
#define AC01_GET_SOCKET_ID(Mpidr)   (((Mpidr) & ARM_CORE_AFF3) >> 32)
#define AC01_GET_CLUSTER_ID(Mpidr)  (((Mpidr) & ARM_CORE_AFF2) >> 16)
#define AC01_GET_CORE_ID(Mpidr)     (((Mpidr) & ARM_CORE_AFF1) >> 8)

//
// Max number for AC01 PCIE Root Complexes
//
#define AC01_PCIE_MAX_ROOT_COMPLEX  16

//
// Max number for AC01 PCIE Root Complexes per socket
//
#define AC01_PCIE_MAX_RCS_PER_SOCKET  8

//
// The size of IO space
//
#define AC01_PCIE_IO_SIZE  0x2000

//
// The base address of {TCU, CSR, MMCONFIG} Registers
//
#define AC01_PCIE_CSR_BASE_LIST  0x33FFE0000000, 0x37FFE0000000, 0x3BFFE0000000, 0x3FFFE0000000, 0x23FFE0000000, 0x27FFE0000000, 0x2BFFE0000000, 0x2FFFE0000000, 0x73FFE0000000, 0x77FFE0000000, 0x7BFFE0000000, 0x7FFFE0000000, 0x63FFE0000000, 0x67FFE0000000, 0x6BFFE0000000, 0x6FFFE0000000

//
// The base address of MMIO Registers
//
#define AC01_PCIE_MMIO_BASE_LIST  0x300000000000, 0x340000000000, 0x380000000000, 0x3C0000000000, 0x200000000000, 0x240000000000, 0x280000000000, 0x2C0000000000, 0x700000000000, 0x740000000000, 0x780000000000, 0x7C0000000000, 0x600000000000, 0x640000000000, 0x680000000000, 0x6C0000000000

//
// The size of MMIO space
//
#define AC01_PCIE_MMIO_SIZE_LIST  0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000, 0x3FFE0000000

//
// The base address of MMIO32 Registers
//
#define AC01_PCIE_MMIO32_BASE_LIST  0x000020000000, 0x000028000000, 0x000030000000, 0x000038000000, 0x000004000000, 0x000008000000, 0x000010000000, 0x000018000000, 0x000060000000, 0x000068000000, 0x000070000000, 0x000078000000, 0x000040000000, 0x000048000000, 0x000050000000, 0x000058000000

//
// The size of MMIO32 space
//
#define AC01_PCIE_MMIO32_SIZE_LIST  0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x4000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000, 0x8000000

//
// The base address of MMIO32 Registers
//
#define AC01_PCIE_MMIO32_BASE_1P_LIST  0x000040000000, 0x000050000000, 0x000060000000, 0x000070000000, 0x000008000000, 0x000010000000, 0x000020000000, 0x000030000000, 0, 0, 0, 0, 0, 0, 0, 0

//
// The size of MMIO32 1P space
//
#define AC01_PCIE_MMIO32_SIZE_1P_LIST  0x10000000, 0x10000000, 0x10000000, 0x10000000, 0x8000000, 0x10000000, 0x10000000, 0x10000000, 0, 0, 0, 0, 0, 0, 0, 0

//
// DSDT RCA2 PCIe MMIO32 Attribute
//
#define AC01_PCIE_RCA2_QMEM_LIST  0x0000000000000000, 0x0000000060000000, 0x000000006FFFFFFF, 0x0000000000000000, 0x0000000010000000

//
// DSDT RCA3 PCIe MMIO32 Attribute
//
#define AC01_PCIE_RCA3_QMEM_LIST  0x0000000000000000, 0x0000000070000000, 0x000000007FFFFFFF, 0x0000000000000000, 0x0000000010000000

//
// DSDT RCB0 PCIe MMIO32 Attribute
//
#define AC01_PCIE_RCB0_QMEM_LIST  0x0000000000000000, 0x0000000008000000, 0x000000000FFFFFFF, 0x0000000000000000, 0x0000000008000000

//
// DSDT RCB1 PCIe MMIO32 Attribute
//
#define AC01_PCIE_RCB1_QMEM_LIST  0x0000000000000000, 0x0000000010000000, 0x000000001FFFFFFF, 0x0000000000000000, 0x0000000010000000

//
// DSDT RCB2 PCIe MMIO32 Attribute
//
#define AC01_PCIE_RCB2_QMEM_LIST  0x0000000000000000, 0x0000000020000000, 0x000000002FFFFFFF, 0x0000000000000000, 0x0000000010000000

//
// DSDT RCB3 PCIe MMIO32 Attribute
//
#define AC01_PCIE_RCB3_QMEM_LIST  0x0000000000000000, 0x0000000030000000, 0x000000003FFFFFFF, 0x0000000000000000, 0x0000000010000000

//
// TBU PMU IRQ array
//
#define AC01_SMMU_TBU_PMU_IRQS_LIST  224, 230, 236, 242, 160, 170, 180, 190, 544, 550, 556, 562, 480, 490, 500, 510

//
// TCU PMU IRQ array
//
#define AC01_SMMU_TCU_PMU_IRQS_LIST  256, 257, 258, 259, 260, 261, 262, 263, 576, 577, 578, 579, 580, 581, 582, 583

//
// Max TBU PMU of Root Complex A
//
#define AC01_RCA_MAX_TBU_PMU  6

//
// Max TBU PMU of Root Complex B
//
#define AC01_RCB_MAX_TBU_PMU  10

//
// TBU Base offset of Root Complex A
//
#define AC01_RCA_TBU_PMU_OFFSET_LIST  0x40000, 0x60000, 0xA0000, 0xE0000, 0x100000, 0x140000

//
// TBU Base offset of Root Complex B
//
#define AC01_RCB_TBU_PMU_OFFSET_LIST  0x40000, 0x60000, 0xA0000, 0xE0000, 0x120000, 0x160000, 0x180000, 0x1C0000, 0x200000, 0x240000

#endif /* PLATFORM_AC01_H_ */
