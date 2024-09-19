/** @file

  Copyright (c) 2021 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/ArmLib/ArmLibPrivate.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NVParamLib.h>
#include <NVParamDef.h>
#include <Platform/Ac01.h>

UINT32  Ac01CoreOrderMonolithic[PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM] = {
  36,  37,  40,  41,  52,  53,  56,  57,  32, 33,
  44,  45,  48,  49,  60,  61,  20,  21,  24, 25,
  68,  69,  72,  73,  16,  17,  28,  29,  64, 65,
  76,  77,  4,   5,   8,   9,   0,   1,   12, 13,
  38,  39,  42,  43,  54,  55,  58,  59,  34, 35,
  46,  47,  50,  51,  62,  63,  22,  23,  26, 27,
  70,  71,  74,  75,  18,  19,  30,  31,  66, 67,
  78,  79,  6,   7,   10,  11,  2,   3,   14, 15,
  80,  81,  82,  83,  84,  85,  86,  87,
  88,  89,  90,  91,  92,  93,  94,  95,
  96,  97,  98,  99,  100, 101, 102, 103,
  104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 124, 125, 126, 127,
};

UINT32  Ac01CoreOrderHemisphere[PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM] = {
  32,  33,  48,  49,  16,  17,  64,  65,  36, 37,
  52,  53,  0,   1,   20,  21,  68,  69,  4,  5,
  34,  35,  50,  51,  18,  19,  66,  67,  38, 39,
  54,  55,  2,   3,   22,  23,  70,  71,  6,  7,
  44,  45,  60,  61,  28,  29,  76,  77,  40, 41,
  56,  57,  12,  13,  24,  25,  72,  73,  8,  9,
  46,  47,  62,  63,  30,  31,  78,  79,  42, 43,
  58,  59,  14,  15,  26,  27,  74,  75,  10, 11,
  80,  81,  82,  83,  84,  85,  86,  87,
  88,  89,  90,  91,  92,  93,  94,  95,
  96,  97,  98,  99,  100, 101, 102, 103,
  104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 124, 125, 126, 127,
};

UINT32  Ac01CoreOrderQuadrant[PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM] = {
  16,  17,  32,  33,  0,   1,   20,  21,  4,  5,
  18,  19,  34,  35,  2,   3,   22,  23,  6,  7,
  48,  49,  64,  65,  52,  53,  68,  69,  36, 37,
  50,  51,  66,  67,  54,  55,  70,  71,  38, 39,
  28,  29,  44,  45,  12,  13,  24,  25,  8,  9,
  30,  31,  46,  47,  14,  15,  26,  27,  10, 11,
  60,  61,  76,  77,  56,  57,  72,  73,  40, 41,
  62,  63,  78,  79,  58,  59,  74,  75,  42, 43,
  80,  81,  82,  83,  84,  85,  86,  87,
  88,  89,  90,  91,  92,  93,  94,  95,
  96,  97,  98,  99,  100, 101, 102, 103,
  104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 124, 125, 126, 127,
};

UINT32  Ac02CoreOrderMonolithic[PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM] = {
  36,  37,  40,  41,  52,  53,  56,  57,  32,  33,
  44,  45,  48,  49,  60,  61,  20,  21,  24,  25,
  68,  69,  72,  73,  16,  17,  28,  29,  64,  65,
  76,  77,  4,   5,   8,   9,   84,  85,  88,  89,
  0,   1,   12,  13,  80,  81,  92,  93,  100, 101,
  104, 105, 96,  97,  108, 109, 116, 117, 120, 121,
  112, 113, 124, 125, 38,  39,  42,  43,  54,  55,
  58,  59,  34,  35,  46,  47,  50,  51,  62,  63,
  22,  23,  26,  27,  70,  71,  74,  75,  18,  19,
  30,  31,  66,  67,  78,  79,  6,   7,   10,  11,
  86,  87,  90,  91,  2,   3,   14,  15,  82,  83,
  94,  95,  102, 103, 106, 107, 98,  99,  110, 111,
  118, 119, 122, 123, 114, 115, 126, 127,
};

UINT32  Ac02CoreOrderHemisphere[PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM] = {
  32,  33,  48,  49,  16,  17,  64,  65,  36,  37,
  52,  53,  0,   1,   20,  21,  68,  69,  80,  81,
  4,   5,   84,  85,  96,  97,  100, 101, 112, 113,
  116, 117, 34,  35,  50,  51,  18,  19,  66,  67,
  38,  39,  54,  55,  2,   3,   22,  23,  70,  71,
  82,  83,  6,   7,   86,  87,  98,  99,  102, 103,
  114, 115, 118, 119, 44,  45,  60,  61,  28,  29,
  76,  77,  40,  41,  56,  57,  12,  13,  24,  25,
  72,  73,  92,  93,  8,   9,   88,  89,  108, 109,
  104, 105, 124, 125, 120, 121, 46,  47,  62,  63,
  30,  31,  78,  79,  42,  43,  58,  59,  14,  15,
  26,  27,  74,  75,  94,  95,  10,  11,  90,  91,
  110, 111, 106, 107, 126, 127, 122, 123,
};

UINT32  Ac02CoreOrderQuadrant[PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM] = {
  16, 17, 32,  33,  0,   1,   20,  21,
  36, 37, 4,   5,   84,  85,  96,  97,
  18, 19, 34,  35,  2,   3,   22,  23,
  38, 39, 6,   7,   86,  87,  98,  99,
  48, 49, 64,  65,  52,  53,  68,  69,
  80, 81, 100, 101, 112, 113, 116, 117,
  50, 51, 66,  67,  54,  55,  70,  71,
  82, 83, 102, 103, 114, 115, 118, 119,
  28, 29, 44,  45,  12,  13,  24,  25,
  40, 41, 8,   9,   88,  89,  108, 109,
  30, 31, 46,  47,  14,  15,  26,  27,
  42, 43, 10,  11,  90,  91,  110, 111,
  60, 61, 76,  77,  56,  57,  72,  73,
  92, 93, 104, 105, 124, 125, 120, 121,
  62, 63, 78,  79,  58,  59,  74,  75,
  94, 95, 106, 107, 126, 127, 122, 123,
};

PLATFORM_INFO_HOB *
GetPlatformHob (
  VOID
  );

/**
  Get current CPU frequency.

  @param    Socket    Socket index.
  @return   UINTN     Current CPU frequency.

**/
UINTN
EFIAPI
CpuGetCurrentFreq (
  UINT8  Socket
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  ASSERT (PlatformHob != NULL);

  return PlatformHob->CpuClk;
}

/**
  Get maximum CPU frequency.

  @param    Socket    Socket index.
  @return   UINTN     Maximum CPU frequency.

**/
UINTN
EFIAPI
CpuGetMaxFreq (
  UINT8  Socket
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  ASSERT (PlatformHob != NULL);

  if (PlatformHob->TurboCapability[Socket]) {
    return PlatformHob->TurboFrequency[Socket];
  }

  return PlatformHob->CpuClk;
}

/**
  Get CPU voltage.

  @param    Socket    Socket index.
  @return   UINT8     CPU voltage.

**/
UINT8
EFIAPI
CpuGetVoltage (
  UINT8  Socket
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;
  UINT8              Voltage;

  PlatformHob = GetPlatformHob ();
  ASSERT (PlatformHob != NULL);

  Voltage = 0x80 | (PlatformHob->CoreVoltage[Socket] / 100);

  return Voltage;
}

/**
  Get CPU Ecid.

  @param        SocketId   Socket index.
  @param[out]   Ecid       Pointer to contain Ecid value.

**/
VOID
EFIAPI
CpuGetEcid (
  UINT8   SocketId,
  UINT32  **Ecid
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob != NULL) {
    *Ecid = (UINT32 *)(PlatformHob->Ecid[SocketId]);
  }
}

/**
  Get max cores of socket.

  @param    SocketId   Socket index.
  @return   UINT8      Max cores of socket.

**/
UINT8
EFIAPI
GetSkuMaxCore (
  UINT8  SocketId
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    return 0;
  }

  return PlatformHob->SkuMaxCore[SocketId];
}

/**
  Get max turbo of socket.

  @param    SocketId   Socket index.
  @return   UINT8      Max turbo of socket.

**/
UINT8
EFIAPI
GetSkuMaxTurbo (
  UINT8  SocketId
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    return 0;
  }

  return PlatformHob->SkuMaxTurbo[SocketId];
}

/**
  Get CPU Core order number.

  @return   UINT32*      The order number.

**/
UINT32 *
CpuGetCoreOrder (
  VOID
  )
{
  UINT32   *CoreOrder;
  UINT8    SubNumaMode;
  BOOLEAN  IsAc01;

  SubNumaMode = CpuGetSubNumaMode ();
  IsAc01      = IsAc01Processor ();

  switch (SubNumaMode) {
    case SUBNUMA_MODE_MONOLITHIC:
    default:
      CoreOrder = IsAc01 ? Ac01CoreOrderMonolithic : Ac02CoreOrderMonolithic;
      break;

    case SUBNUMA_MODE_HEMISPHERE:
      CoreOrder = IsAc01 ? Ac01CoreOrderHemisphere : Ac02CoreOrderHemisphere;
      break;

    case SUBNUMA_MODE_QUADRANT:
      CoreOrder = IsAc01 ? Ac01CoreOrderQuadrant : Ac02CoreOrderQuadrant;
      break;
  }

  return CoreOrder;
}

/**
  Get the SubNUMA mode.

  @return   UINT8      The SubNUMA mode.

**/
UINT8
EFIAPI
CpuGetSubNumaMode (
  VOID
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    return SUBNUMA_MODE_MONOLITHIC;
  }

  return PlatformHob->SubNumaMode[0];
}

/**
  Get the number of SubNUMA region.

  @return   UINT8      The number of SubNUMA region.

**/
UINT8
EFIAPI
CpuGetNumberOfSubNumaRegion (
  VOID
  )
{
  UINT8  SubNumaMode;
  UINT8  NumberOfSubNumaRegion;

  SubNumaMode = CpuGetSubNumaMode ();
  ASSERT (SubNumaMode <= SUBNUMA_MODE_QUADRANT);

  switch (SubNumaMode) {
    case SUBNUMA_MODE_MONOLITHIC:
      NumberOfSubNumaRegion = MONOLITIC_NUM_OF_REGION;
      break;

    case SUBNUMA_MODE_HEMISPHERE:
      NumberOfSubNumaRegion = HEMISPHERE_NUM_OF_REGION;
      break;

    case SUBNUMA_MODE_QUADRANT:
      NumberOfSubNumaRegion = QUADRANT_NUM_OF_REGION;
      break;

    default:
      // Should never reach there.
      NumberOfSubNumaRegion = 0;
      ASSERT (FALSE);
      break;
  }

  return NumberOfSubNumaRegion;
}

STATIC
UINT8
CpuGetLogicCoreId (
  UINT32  PhyCoreId
  )
{
  UINT32  *CoreOrder;
  UINT8   LogicCoreId;
  UINT8   SktMaxCoreNum;

  CoreOrder     = CpuGetCoreOrder ();
  SktMaxCoreNum = PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM;

  for (LogicCoreId = 0; LogicCoreId < SktMaxCoreNum; LogicCoreId++) {
    if (CoreOrder[LogicCoreId] == PhyCoreId) {
      return LogicCoreId;
    }
  }

  return 0;
}

UINT8
EFIAPI
CpuGetSubNumNode (
  UINT8   SocketId,
  UINT16  Cpm
  )
{
  UINT8  LogicCoreId;
  UINT8  MaxFamliyCore;
  UINT8  MaxNumCorePerNode;
  UINT8  SubNumaNode;

  LogicCoreId   = CpuGetLogicCoreId (Cpm * PLATFORM_CPU_NUM_CORES_PER_CPM);
  MaxFamliyCore = IsAc01Processor () ? MAX_AMPERE_ALTRA_CORES : MAX_AMPERE_ALTRA_MAX_CORES;

  switch (CpuGetSubNumaMode ()) {
    case SUBNUMA_MODE_MONOLITHIC:
      SubNumaNode = (SocketId == 0) ? 0 : 1;
      break;

    case SUBNUMA_MODE_HEMISPHERE:
      MaxNumCorePerNode = MaxFamliyCore / HEMISPHERE_NUM_OF_REGION;
      SubNumaNode       = LogicCoreId / MaxNumCorePerNode;
      if (SocketId == 1) {
        SubNumaNode += HEMISPHERE_NUM_OF_REGION;
      }

      break;

    case SUBNUMA_MODE_QUADRANT:
      MaxNumCorePerNode = MaxFamliyCore / QUADRANT_NUM_OF_REGION;
      SubNumaNode       = LogicCoreId / MaxNumCorePerNode;
      if (SocketId == 1) {
        SubNumaNode += QUADRANT_NUM_OF_REGION;
      }

      break;

    default:
      // Should never reach there.
      SubNumaNode = (SocketId == 0) ? 0 : 1;
      ASSERT (FALSE);
      break;
  }

  return SubNumaNode;
}

/**
  Get the number of supported socket.

  @return   UINT8      Number of supported socket.

**/
UINT8
EFIAPI
GetNumberOfSupportedSockets (
  VOID
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    //
    // By default, the number of supported sockets is 1.
    //
    return 1;
  }

  return (sizeof (PlatformHob->ClusterEn) / sizeof (PLATFORM_CLUSTER_EN));
}

/**
  Get the number of active socket.

  @return   UINT8      Number of active socket.

**/
UINT8
EFIAPI
GetNumberOfActiveSockets (
  VOID
  )
{
  UINT8                NumberOfActiveSockets, Count, Index, Index1;
  PLATFORM_CLUSTER_EN  *Socket;
  PLATFORM_INFO_HOB    *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    //
    // By default, the number of active sockets is 1.
    //
    return 1;
  }

  NumberOfActiveSockets = 0;

  for (Index = 0; Index < GetNumberOfSupportedSockets (); Index++) {
    Socket = &PlatformHob->ClusterEn[Index];
    Count  = ARRAY_SIZE (Socket->EnableMask);
    for (Index1 = 0; Index1 < Count; Index1++) {
      if (Socket->EnableMask[Index1] != 0) {
        NumberOfActiveSockets++;
        break;
      }
    }
  }

  return NumberOfActiveSockets;
}

/**
  Get the number of active CPM per socket.

  @param    SocketId    Socket index.
  @return   UINT16      Number of CPM.

**/
UINT16
EFIAPI
GetNumberOfActiveCPMsPerSocket (
  UINT8  SocketId
  )
{
  UINT16               NumberOfCPMs, Count, Index;
  UINT32               Val32;
  PLATFORM_CLUSTER_EN  *Socket;
  PLATFORM_INFO_HOB    *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    return 0;
  }

  if (SocketId >= GetNumberOfActiveSockets ()) {
    return 0;
  }

  NumberOfCPMs = 0;
  Socket       = &PlatformHob->ClusterEn[SocketId];
  Count        = ARRAY_SIZE (Socket->EnableMask);
  for (Index = 0; Index < Count; Index++) {
    Val32 = Socket->EnableMask[Index];
    while (Val32 > 0) {
      if ((Val32 & 0x1) != 0) {
        NumberOfCPMs++;
      }

      Val32 >>= 1;
    }
  }

  return NumberOfCPMs;
}

/**
  Get the number of configured CPM per socket. This number
  should be the same for all sockets.

  @param    SocketId    Socket index.
  @return   UINT8       Number of configured CPM.

**/
UINT16
EFIAPI
GetNumberOfConfiguredCPMs (
  UINT8  SocketId
  )
{
  EFI_STATUS  Status;
  UINT32      Value;
  UINT32      Param, ParamStart, ParamEnd;
  UINT16      Count;

  Count      = 0;
  ParamStart = NV_SI_S0_PCP_ACTIVECPM_0_31 + SocketId * NV_PARAM_ENTRYSIZE * (PLATFORM_CPU_MAX_CPM / 32);
  ParamEnd   = ParamStart + NV_PARAM_ENTRYSIZE * (PLATFORM_CPU_MAX_CPM / 32);
  for (Param = ParamStart; Param < ParamEnd; Param += NV_PARAM_ENTRYSIZE) {
    Status = NVParamGet (
               Param,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &Value
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    while (Value != 0) {
      if ((Value & 0x01) != 0) {
        Count++;
      }

      Value >>= 1;
    }
  }

  return Count;
}

/**
  Get version of SCP.

  @param[out]   ScpVer   Pointer to contain version of SCP value.
**/
VOID
EFIAPI
GetScpVersion (
  UINT8  **ScpVer
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob != NULL) {
    *ScpVer = (UINT8 *)PlatformHob->SmPmProVer;
  } else {
    *ScpVer = NULL;
  }
}

/**
  Get date of build release for SCP.

  @param[out]   ScpBuild   Pointer to contain date of build release for SCP.
**/
VOID
EFIAPI
GetScpBuild (
  UINT8  **ScpBuild
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob != NULL) {
    *ScpBuild = (UINT8 *)PlatformHob->SmPmProBuild;
  } else {
    *ScpBuild = NULL;
  }
}

/**
  Get information of DIMM List.

  @param[out]   DimmList   Pointer contains information of DIMM List.
**/
VOID
EFIAPI
GetDimmList (
  PLATFORM_DIMM_LIST  **DimmList
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob != NULL) {
    *DimmList = &PlatformHob->DimmList;
  } else {
    *DimmList = NULL;
  }
}

/**
  Get information of DRAM.

  @param[out]   DramInfo   Pointer contains information of DRAM.
**/
VOID
EFIAPI
GetDramInfo (
  PLATFORM_DRAM_INFO  **DramInfo
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob != NULL) {
    *DramInfo = &PlatformHob->DramInfo;
  } else {
    *DramInfo = NULL;
  }
}

/**
  Set the number of configured CPM per socket.

  @param    SocketId        Socket index.
  @param    NumberOfCPMs    Number of CPM to be configured.
  @return   EFI_SUCCESS     Operation succeeded.
  @return   Others          An error has occurred.

**/
EFI_STATUS
EFIAPI
SetNumberOfConfiguredCPMs (
  UINT8   SocketId,
  UINT16  NumberOfCPMs
  )
{
  EFI_STATUS  Status;
  UINT32      Value;
  UINT32      Param, ParamStart, ParamEnd;
  BOOLEAN     IsClear;

  IsClear = FALSE;
  if (NumberOfCPMs == 0) {
    IsClear = TRUE;
  }

  Status = EFI_SUCCESS;

  ParamStart = NV_SI_S0_PCP_ACTIVECPM_0_31 + SocketId * NV_PARAM_ENTRYSIZE * (PLATFORM_CPU_MAX_CPM / 32);
  ParamEnd   = ParamStart + NV_PARAM_ENTRYSIZE * (PLATFORM_CPU_MAX_CPM / 32);
  for (Param = ParamStart; Param < ParamEnd; Param += NV_PARAM_ENTRYSIZE) {
    if (NumberOfCPMs >= 32) {
      Value         = 0xffffffff;
      NumberOfCPMs -= 32;
    } else {
      Value = 0;
      while (NumberOfCPMs > 0) {
        Value |= (1 << (--NumberOfCPMs));
      }
    }

    if (IsClear) {
      /* Clear this param */
      Status = NVParamClr (
                 Param,
                 NV_PERM_BIOS | NV_PERM_MANU
                 );
    } else {
      Status = NVParamSet (
                 Param,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 Value
                 );
    }
  }

  return Status;
}

/**
  Get the maximum number of core per socket.

  @return   UINT16      Maximum number of core.

**/
UINT16
EFIAPI
GetMaximumNumberOfCores (
  VOID
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    return 0;
  }

  return PlatformHob->MaxNumOfCore[0];
}

/**
  Get the number of active cores of a sockets.

  @param    SocketId    Socket Index.
  @return   UINT16      Number of active core.

**/
UINT16
EFIAPI
GetNumberOfActiveCoresPerSocket (
  UINT8  SocketId
  )
{
  return GetNumberOfActiveCPMsPerSocket (SocketId) * PLATFORM_CPU_NUM_CORES_PER_CPM;
}

/**
  Get the number of active cores of all sockets.

  @return   UINT16      Number of active core.

**/
UINT16
EFIAPI
GetNumberOfActiveCores (
  VOID
  )
{
  UINT16  NumberOfActiveCores;
  UINT8   Index;

  NumberOfActiveCores = 0;

  for (Index = 0; Index < GetNumberOfActiveSockets (); Index++) {
    NumberOfActiveCores += GetNumberOfActiveCoresPerSocket (Index);
  }

  return NumberOfActiveCores;
}

/**
  Check if the logical CPU is enabled or not.

  @param    CpuId       The logical Cpu ID. Started from 0.
  @return   BOOLEAN     TRUE if the Cpu enabled
                        FALSE if the Cpu disabled.

**/
BOOLEAN
EFIAPI
IsCpuEnabled (
  UINT16  CpuId
  )
{
  PLATFORM_CLUSTER_EN  *Socket;
  PLATFORM_INFO_HOB    *PlatformHob;
  UINT8                SocketId;
  UINT16               ClusterId;

  SocketId  = SOCKET_ID (CpuId);
  ClusterId = CLUSTER_ID (CpuId);

  PlatformHob = GetPlatformHob ();
  if (PlatformHob == NULL) {
    return FALSE;
  }

  if (SocketId >= GetNumberOfActiveSockets ()) {
    return FALSE;
  }

  Socket = &PlatformHob->ClusterEn[SocketId];
  if ((Socket->EnableMask[ClusterId / 32] & (1 << (ClusterId % 32))) != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check if the slave socket is present

  @return   BOOLEAN     TRUE if the Slave Cpu is present
                        FALSE if the Slave Cpu is not present

**/
BOOLEAN
EFIAPI
IsSlaveSocketAvailable (
  VOID
  )
{
  UINT32  Value;

  Value = MmioRead32 (SMPRO_EFUSE_SHADOW0 + CFG2P_OFFSET);

  return ((Value & SLAVE_PRESENT_N) != 0) ? FALSE : TRUE;
}

/**
  Check if the slave socket is active

  @return   BOOLEAN     TRUE if the Slave CPU Socket is active.
                        FALSE if the Slave CPU Socket is not active.

**/
BOOLEAN
EFIAPI
IsSlaveSocketActive (
  VOID
  )
{
  return (GetNumberOfActiveSockets () > 1) ? TRUE : FALSE;
}

/**
  Check if the CPU product ID is Ac01
  @return   BOOLEAN     TRUE if the Product ID is Ac01
                        FALSE otherwise.

**/
BOOLEAN
EFIAPI
IsAc01Processor (
  VOID
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;

  PlatformHob = GetPlatformHob ();
  ASSERT (PlatformHob != NULL);

  if (PlatformHob != NULL) {
    if ((PlatformHob->ScuProductId[0] & 0xFF) == 0x01) {
      return TRUE;
    }
  }

  return FALSE;
}
