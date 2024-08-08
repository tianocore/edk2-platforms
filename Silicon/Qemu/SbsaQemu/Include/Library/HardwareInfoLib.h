/** @file
*
*  Copyright (c) 2024, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef HARDWARE_INFO_LIB
#define HARDWARE_INFO_LIB

typedef struct {
  UINT32    NodeId;
  UINT64    AddressBase;
  UINT64    AddressSize;
} MemoryInfo;

/**
  Sockets: the number of sockets on sbsa-ref platform.
  Clusters: the number of clusters in one socket.
  Cores: the number of cores in one cluster.
  Threads: the number of threads in one core.
**/
typedef struct {
  UINT32    Sockets;
  UINT32    Clusters;
  UINT32    Cores;
  UINT32    Threads;
} CpuTopology;

typedef struct {
  UINTN    DistributorBase;
  UINTN    RedistributorBase;
  UINTN    ItsBase;
} GicInfo;

typedef struct {
  UINT32    Major;
  UINT32    Minor;
} PlatformVersion;

/**
  Get CPU count from information passed by Qemu.

**/
UINT32
GetCpuCount (
  VOID
  );

/**
  Get MPIDR for a given cpu from device tree passed by Qemu.

  @param [in]   CpuId    Index of cpu to retrieve MPIDR value for.

  @retval                MPIDR value of CPU at index <CpuId>
**/
UINT64
GetMpidr (
  IN UINTN  CpuId
  );

/**
  Get NUMA node id for a given cpu from device tree passed by Qemu.

  @param [in]   CpuId    Index of cpu to retrieve NUMA node id for.

  @retval                NUMA node id for CPU at index <CpuId>
**/
UINT64
GetCpuNumaNode (
  IN UINTN  CpuId
  );

/**
  Get the number of memory node from device tree passed by Qemu.

  @retval                   the number of memory nodes.
**/
UINT32
GetMemNodeCount (
  VOID
  );

/**
  Get memory information(node-id, addressbase, addresssize) for a given memory node from device tree passed by Qemu.

  @param [in]   MemoryId    Index of memory to retrieve memory information.
  @param [out]  MemInfo     A pointer to the memory information of given memory-id.


  @retval                   memory infomation for given memory node.
**/
VOID
GetMemInfo (
  IN  UINTN       MemoryId,
  OUT MemoryInfo  *MemInfo
  );

/**
  Get the number of numa node from device tree passed by Qemu.

  @retval                the number of numa node.
**/
UINT64
GetNumaNodeCount (
  VOID
  );

/**
  Get cpu topology (sockets, clusters, cores, threads) from TF-A.

  @param [out]  CpuTopo     A pointer to the cpu topology.


  @retval                   the information of cpu topology.
**/
VOID
GetCpuTopology (
  OUT CpuTopology  *CpuTopo
  );

/**
  Get GIC information (base of GICD, GICR, GICI) from TF-A.

  @param [out]  GicInfo     A pointer to the GIC information.
**/
VOID
GetGicInformation (
  OUT GicInfo  *GicInfo
  );

/**
  Get Platform version from TF-A.

  @param [out]  PlatVer     A pointer to the Platform version.
**/
VOID
GetPlatformVersion (
  OUT PlatformVersion  *PlatVer
  );

#endif /* HARDWARE_INFO_LIB */
