/** @file
*
*  Copyright (c) 2021, NUVIA Inc. All rights reserved.
*  Copyright (c) 2024, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/ArmSmcLib.h>
#include <Library/ArmMonitorLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/HardwareInfoLib.h>
#include <IndustryStandard/SbsaQemuSmc.h>

/**
  Get CPU count from information passed by TF-A.

**/
UINT32
GetCpuCount (
  VOID
  )
{
  UINTN  Arg0;
  UINTN  SmcResult;

  SmcResult = ArmCallSmc0 (SIP_SVC_GET_CPU_COUNT, &Arg0, NULL, NULL);
  if (SmcResult != SMC_SIP_CALL_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: SIP_SVC_GET_CPU_COUNT call failed. We have no cpu information.\n", __FUNCTION__));
    ResetShutdown ();
  }

  DEBUG ((DEBUG_INFO, "%a: We have %d cpus.\n", __FUNCTION__, Arg0));

  return Arg0;
}

/**
  Get MPIDR for a given cpu from TF-A.

  @param [in]   CpuId    Index of cpu to retrieve MPIDR value for.

  @retval                MPIDR value of CPU at index <CpuId>
**/
UINT64
GetMpidr (
  IN UINTN  CpuId
  )
{
  UINTN  SmcResult;
  UINTN  Arg0;
  UINTN  Arg1;

  Arg0 = CpuId;

  SmcResult = ArmCallSmc0 (SIP_SVC_GET_CPU_NODE, &Arg0, &Arg1, NULL);
  if (SmcResult != SMC_SIP_CALL_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: SIP_SVC_GET_CPU_NODE call failed. We have no MPIDR for CPU%d.\n", __FUNCTION__, CpuId));
    ResetShutdown ();
  }

  DEBUG ((DEBUG_INFO, "%a: MPIDR for CPU%d: = %d\n", __FUNCTION__, CpuId, Arg1));

  return Arg1;
}

/**
  Get NUMA node id for a given cpu from TF-A.

  @param [in]   CpuId    Index of cpu to retrieve NUMA node id for.

  @retval                NUMA node id for CPU at index <CpuId>
**/
UINT64
GetCpuNumaNode (
  IN UINTN  CpuId
  )
{
  UINTN  SmcResult;
  UINTN  Arg0;
  UINTN  Arg1;

  Arg0 = CpuId;

  SmcResult = ArmCallSmc0 (SIP_SVC_GET_CPU_NODE, &Arg0, &Arg1, NULL);
  if (SmcResult != SMC_SIP_CALL_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: SIP_SVC_GET_CPU_NODE call failed. Could not find information for CPU%d.\n", __FUNCTION__, CpuId));
    return 0;
  }

  DEBUG ((DEBUG_INFO, "%a: NUMA node for CPU%d: = %d\n", __FUNCTION__, CpuId, Arg0));

  return Arg0;
}

UINT32
GetMemNodeCount (
  VOID
  )
{
  UINTN  SmcResult;
  UINTN  Arg0;

  SmcResult = ArmCallSmc0 (SIP_SVC_GET_MEMORY_NODE_COUNT, &Arg0, NULL, NULL);
  if (SmcResult != SMC_SIP_CALL_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: SIP_SVC_GET_MEMORY_NODE_COUNT call failed. We have no memory information.\n", __FUNCTION__));
    ResetShutdown ();
  }

  DEBUG ((DEBUG_INFO, "%a: The number of the memory nodes is %ld\n", __FUNCTION__, Arg0));
  return (UINT32)Arg0;
}

VOID
GetMemInfo (
  IN  UINTN       MemoryId,
  OUT MemoryInfo  *MemInfo
  )
{
  UINTN  SmcResult;
  UINTN  Arg0;
  UINTN  Arg1;
  UINTN  Arg2;

  Arg0 = MemoryId;

  SmcResult = ArmCallSmc1 (SIP_SVC_GET_MEMORY_NODE, &Arg0, &Arg1, &Arg2);
  if (SmcResult != SMC_SIP_CALL_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: SIP_SVC_GET_MEMORY_NODE call failed. We have no memory information.\n", __FUNCTION__));
    ResetShutdown ();
  } else {
    MemInfo->NodeId      = Arg0;
    MemInfo->AddressBase = Arg1;
    MemInfo->AddressSize = Arg2;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: NUMA node for System RAM:%d = 0x%lx - 0x%lx\n",
    __FUNCTION__,
    MemInfo->NodeId,
    MemInfo->AddressBase,
    MemInfo->AddressBase + MemInfo->AddressSize -1
    ));
}

UINT64
GetNumaNodeCount (
  VOID
  )
{
  UINT64      Arg;
  UINT32      Index;
  UINT32      NumberNumaNodes;
  UINT32      NumberMemNodes;
  UINT32      NumCores = GetCpuCount ();
  MemoryInfo  MemInfo;

  NumberNumaNodes = 0;
  NumberMemNodes  = GetMemNodeCount ();

  if (NumCores > 0) {
    for (Index = 0; Index < NumCores; Index++) {
      Arg = GetCpuNumaNode (Index);
      if ((NumberNumaNodes == 0) || (NumberNumaNodes < (Arg + 1))) {
        NumberNumaNodes = Arg + 1;
      }
    }
  }

  if (NumberMemNodes > 0) {
    for (Index = 0; Index < NumberMemNodes; Index++) {
      GetMemInfo (Index, &MemInfo);
      if ((NumberNumaNodes == 0) || (NumberNumaNodes < (MemInfo.NodeId + 1))) {
        NumberNumaNodes = MemInfo.NodeId + 1;
      }
    }
  }

  return NumberNumaNodes;
}

/**
  Get CPU topology.
**/
VOID
GetCpuTopology (
  OUT CpuTopology  *CpuTopo
  )
{
  ARM_MONITOR_ARGS  SmcArgs;

  SmcArgs.Arg0 = SIP_SVC_GET_CPU_TOPOLOGY;
  ArmMonitorCall (&SmcArgs);

  if (SmcArgs.Arg0 != SMC_SIP_CALL_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: SIP_SVC_GET_CPU_TOPOLOGY call failed. We have no cpu topology information.\n", __FUNCTION__));
    ResetShutdown ();
  } else {
    CpuTopo->Sockets  = SmcArgs.Arg1;
    CpuTopo->Clusters = SmcArgs.Arg2;
    CpuTopo->Cores    = SmcArgs.Arg3;
    CpuTopo->Threads  = SmcArgs.Arg4;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: CPU Topology: sockets: %d, clusters: %d, cores: %d, threads: %d\n",
    __FUNCTION__,
    CpuTopo->Sockets,
    CpuTopo->Clusters,
    CpuTopo->Cores,
    CpuTopo->Threads
    ));
}

/**
  Get GIC information from TF-A.

  If run on old platform then use values from EDK2 configuration.
**/
VOID
GetGicInformation (
  OUT GicInfo  *GicInfo
  )
{
  ARM_MONITOR_ARGS  SmcArgs;

  SmcArgs.Arg0 = SIP_SVC_GET_GIC;
  ArmMonitorCall (&SmcArgs);

  if (SmcArgs.Arg0 != SMC_SIP_CALL_SUCCESS) {
    GicInfo->DistributorBase   = PcdGet64 (PcdGicDistributorBase);
    GicInfo->RedistributorBase = PcdGet64 (PcdGicRedistributorsBase);
  } else {
    GicInfo->DistributorBase   = SmcArgs.Arg1;
    GicInfo->RedistributorBase = SmcArgs.Arg2;
  }

  SmcArgs.Arg0 = SIP_SVC_GET_GIC_ITS;
  ArmMonitorCall (&SmcArgs);

  if (SmcArgs.Arg0 != SMC_SIP_CALL_SUCCESS) {
    GicInfo->ItsBase = PcdGet64 (PcdGicItsBase);
  } else {
    GicInfo->ItsBase = SmcArgs.Arg1;
  }
}

/**
  Get Platform version from TF-A.

  If run on old platform then 0.0 value is used.
**/
VOID
GetPlatformVersion (
  OUT PlatformVersion  *PlatVer
  )
{
  ARM_MONITOR_ARGS  SmcArgs;

  SmcArgs.Arg0 = SIP_SVC_VERSION;
  ArmMonitorCall (&SmcArgs);

  if (SmcArgs.Arg0 != SMC_SIP_CALL_SUCCESS) {
    PlatVer->Major = 0;
    PlatVer->Minor = 0;
  } else {
    PlatVer->Major = SmcArgs.Arg1;
    PlatVer->Minor = SmcArgs.Arg2;
  }
}
