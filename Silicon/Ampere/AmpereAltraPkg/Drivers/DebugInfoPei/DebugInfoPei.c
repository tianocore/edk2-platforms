/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/NVParamLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>

#include <NVParamDef.h>

#define GB_SCALE_FACTOR   1073741824
#define MB_SCALE_FACTOR   1048576
#define KB_SCALE_FACTOR   1024
#define MHZ_SCALE_FACTOR  1000000

/**
  Print any existence NVRAM.
**/
STATIC VOID
PrintNVRAM (
  VOID
  )
{
  EFI_STATUS  Status;
  NVPARAM     Idx;
  UINT32      Val;
  UINT16      ACLRd = NV_PERM_ALL;
  BOOLEAN     Flag;

  Flag = FALSE;
  for (Idx = NV_PREBOOT_PARAM_START; Idx <= NV_PREBOOT_PARAM_MAX; Idx += NVPARAM_SIZE) {
    Status = NVParamGet (Idx, ACLRd, &Val);
    if (!EFI_ERROR (Status)) {
      if (!Flag) {
        DebugPrint (DEBUG_INIT, "Pre-boot Configuration Setting:\n");
        Flag = TRUE;
      }

      DebugPrint (DEBUG_INIT, "    %04X: 0x%X (%d)\n", (UINT32)Idx, Val, Val);
    }
  }

  Flag = FALSE;
  for (Idx = NV_MANU_PARAM_START; Idx <= NV_MANU_PARAM_MAX; Idx += NVPARAM_SIZE) {
    Status = NVParamGet (Idx, ACLRd, &Val);
    if (!EFI_ERROR (Status)) {
      if (!Flag) {
        DebugPrint (DEBUG_INIT, "Manufacturer Configuration Setting:\n");
        Flag = TRUE;
      }

      DebugPrint (DEBUG_INIT, "    %04X: 0x%X (%d)\n", (UINT32)Idx, Val, Val);
    }
  }

  Flag = FALSE;
  for (Idx = NV_USER_PARAM_START; Idx <= NV_USER_PARAM_MAX; Idx += NVPARAM_SIZE) {
    Status = NVParamGet (Idx, ACLRd, &Val);
    if (!EFI_ERROR (Status)) {
      if (!Flag) {
        DebugPrint (DEBUG_INIT, "User Configuration Setting:\n");
        Flag = TRUE;
      }

      DebugPrint (DEBUG_INIT, "    %04X: 0x%X (%d)\n", (UINT32)Idx, Val, Val);
    }
  }

  Flag = FALSE;
  for (Idx = NV_BOARD_PARAM_START; Idx <= NV_BOARD_PARAM_MAX; Idx += NVPARAM_SIZE) {
    Status = NVParamGet (Idx, ACLRd, &Val);
    if (!EFI_ERROR (Status)) {
      if (!Flag) {
        DebugPrint (DEBUG_INIT, "Board Configuration Setting:\n");
        Flag = TRUE;
      }

      DebugPrint (DEBUG_INIT, "    %04X: 0x%X (%d)\n", (UINT32)Idx, Val, Val);
    }
  }
}

STATIC
CHAR8 *
GetCCIXLinkSpeed (
  IN UINTN  Speed
  )
{
  switch (Speed) {
    case 1:
      return "2.5 GT/s";

    case 2:
      return "5 GT/s";

    case 3:
      return "8 GT/s";

    case 4:
    case 6:
      return "16 GT/s";

    case 0xa:
      return "20 GT/s";

    case 0xf:
      return "25 GT/s";
  }

  return "Unknown";
}

/**
  Print system info
**/
STATIC VOID
PrintSystemInfo (
  VOID
  )
{
  UINTN              Idx;
  VOID               *Hob;
  PLATFORM_INFO_HOB  *PlatformHob;

  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  DebugPrint (DEBUG_INIT, "SCP FW version    : %a\n", (const CHAR8 *)PlatformHob->SmPmProVer);
  DebugPrint (DEBUG_INIT, "SCP FW build date : %a\n", (const CHAR8 *)PlatformHob->SmPmProBuild);

  DebugPrint (DEBUG_INIT, "Failsafe status                 : %d\n", PlatformHob->FailSafeStatus);
  DebugPrint (DEBUG_INIT, "Reset status                    : %d\n", PlatformHob->ResetStatus);
  DebugPrint (DEBUG_INIT, "CPU info\n");
  DebugPrint (DEBUG_INIT, "    CPU ID                      : %X\n", ArmReadMidr ());
  DebugPrint (DEBUG_INIT, "    CPU Clock                   : %d MHz\n", PlatformHob->CpuClk / MHZ_SCALE_FACTOR);
  DebugPrint (DEBUG_INIT, "    Number of active sockets    : %d\n", GetNumberOfActiveSockets ());
  DebugPrint (DEBUG_INIT, "    Number of active cores      : %d\n", GetNumberOfActiveCores ());
  if (IsSlaveSocketActive ()) {
    DebugPrint (
      DEBUG_INIT,
      "    Inter Socket Connection 0   : Width: x%d / Speed %a\n",
      PlatformHob->Link2PWidth[0],
      GetCCIXLinkSpeed (PlatformHob->Link2PSpeed[0])
      );
    DebugPrint (
      DEBUG_INIT,
      "    Inter Socket Connection 1   : Width: x%d / Speed %a\n",
      PlatformHob->Link2PWidth[1],
      GetCCIXLinkSpeed (PlatformHob->Link2PSpeed[1])
      );
  }

  for (Idx = 0; Idx < GetNumberOfActiveSockets (); Idx++) {
    DebugPrint (DEBUG_INIT, "    Socket[%d]: Core voltage     : %d\n", Idx, PlatformHob->CoreVoltage[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: SCU ProductID    : %X\n", Idx, PlatformHob->ScuProductId[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: Max cores        : %d\n", Idx, PlatformHob->MaxNumOfCore[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: Warranty         : %d\n", Idx, PlatformHob->Warranty[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: Subnuma          : %d\n", Idx, PlatformHob->SubNumaMode[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: RC disable mask  : %X\n", Idx, PlatformHob->RcDisableMask[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: AVS enabled      : %d\n", Idx, PlatformHob->AvsEnable[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: AVS voltage      : %d\n", Idx, PlatformHob->AvsVoltageMV[Idx]);
  }

  DebugPrint (DEBUG_INIT, "SOC info\n");
  DebugPrint (DEBUG_INIT, "    DDR Frequency               : %d MHz\n", PlatformHob->DramInfo.MaxSpeed);
  for (Idx = 0; Idx < GetNumberOfActiveSockets (); Idx++) {
    DebugPrint (DEBUG_INIT, "    Socket[%d]: Soc voltage      : %d\n", Idx, PlatformHob->SocVoltage[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: DIMM1 voltage    : %d\n", Idx, PlatformHob->Dimm1Voltage[Idx]);
    DebugPrint (DEBUG_INIT, "    Socket[%d]: DIMM2 voltage    : %d\n", Idx, PlatformHob->Dimm2Voltage[Idx]);
  }

  DebugPrint (DEBUG_INIT, "    PCP Clock                   : %d MHz\n", PlatformHob->PcpClk / MHZ_SCALE_FACTOR);
  DebugPrint (DEBUG_INIT, "    SOC Clock                   : %d MHz\n", PlatformHob->SocClk / MHZ_SCALE_FACTOR);
  DebugPrint (DEBUG_INIT, "    SYS Clock                   : %d MHz\n", PlatformHob->SysClk / MHZ_SCALE_FACTOR);
  DebugPrint (DEBUG_INIT, "    AHB Clock                   : %d MHz\n", PlatformHob->AhbClk / MHZ_SCALE_FACTOR);
}

/**
  Entry point function for the PEIM

  @param FileHandle      Handle of the file being invoked.
  @param PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS    If we installed our PPI

**/
EFI_STATUS
EFIAPI
DebugInfoPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  PrintSystemInfo ();
  PrintNVRAM ();

  return EFI_SUCCESS;
}
