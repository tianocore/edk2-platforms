/** @file
  Parsing NVPARAM board settings for bifurcation programming.

  NVPARAM board settings is spec-ed within Firmware Interface Requirement.
  Bifurcation devmap is programmed before at SCP following the rule

  Root Complex Type-A devmap settings (RP == Root Port)
  -----------------------------------------
  | RP0   | RP1  | RP2  | RP3  | Devmap   |
  | (x16) | (x4) | (x8) | (x4) | (output) |
  -------------------------------------------
  |  Y    |  N   |  N   |  N   | 0        |
  |  Y    |  N   |  Y   |  N   | 1        |
  |  Y    |  N   |  Y   |  Y   | 2        |
  |  Y    |  Y   |  Y   |  Y   | 3        |
  -----------------------------------------

  Root Complex Type-B LOW (aka RCBxA) devmap settings (RP == Root Port)
  ----------------------------------------
  | RP0  | RP1  | RP2  | RP3  | Devmap   |
  | (x8) | (x2) | (x4) | (x3) | (output) |
  ----------------------------------------
  |  Y   |  N   |  N   |  N   | 0        |
  |  Y   |  N   |  Y   |  N   | 1        |
  |  Y   |  N   |  Y   |  Y   | 2        |
  |  Y   |  Y   |  Y   |  Y   | 3        |
  ----------------------------------------

  Root Complex Type-B HIGH (aka RCBxB) devmap settings (RP == Root Port)
  ----------------------------------------
  | RP4  | RP5  | RP6  | RP7  | Devmap   |
  | (x8) | (x2) | (x4) | (x3) | (output) |
  ----------------------------------------
  |  Y   |  N   |  N   |  N   | 0        |
  |  Y   |  N   |  Y   |  N   | 1        |
  |  Y   |  N   |  Y   |  Y   | 2        |
  |  Y   |  Y   |  Y   |  Y   | 3        |
  ----------------------------------------

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/PlatformInfoHob.h>
#include <Guid/RootComplexInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/NVParamLib.h>
#include <NVParamDef.h>

#include "RootComplexNVParam.h"

typedef enum {
  Gen3Preset = 0,
  Gen4Preset,
  GenPresetMax
} NVPARAM_PCIE_PRESET_TYPE;

STATIC
BOOLEAN
IsEmptyRC (
  AC01_ROOT_COMPLEX  *RootComplex
  )
{
  UINT8  Idx;

  for (Idx = PcieController0; Idx < MaxPcieController; Idx++) {
    if (RootComplex->Pcie[Idx].Active) {
      return FALSE;
    }
  }

  return TRUE;
}

VOID
SetRootComplexBifurcation (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              RPStart,
  IN DEV_MAP_MODE       DevMap
  )
{
  UINT8  MaxWidth;

  if ((RPStart != PcieController0) && (RPStart != PcieController4)) {
    return;
  }

  if ((RootComplex->Type != RootComplexTypeB) && (RPStart == PcieController4)) {
    return;
  }

  if ((RootComplex->Type == RootComplexTypeA) && (RootComplex->Pcie[RPStart].MaxWidth == LINK_WIDTH_X16)) {
    RootComplex->Pcie[RPStart + 1].MaxWidth = LINK_WIDTH_X4;
    RootComplex->Pcie[RPStart + 2].MaxWidth = LINK_WIDTH_X8;
    RootComplex->Pcie[RPStart + 3].MaxWidth = LINK_WIDTH_X4;
  }

  if ((RootComplex->Type == RootComplexTypeB) && (RootComplex->Pcie[RPStart].MaxWidth == LINK_WIDTH_X8)) {
    RootComplex->Pcie[RPStart + 1].MaxWidth = LINK_WIDTH_X2;
    RootComplex->Pcie[RPStart + 2].MaxWidth = LINK_WIDTH_X4;
    RootComplex->Pcie[RPStart + 3].MaxWidth = LINK_WIDTH_X2;
  }

  switch (DevMap) {
    case DevMapMode2:
      MaxWidth                                = (RootComplex->Type == RootComplexTypeA) ? LINK_WIDTH_X8 : LINK_WIDTH_X4;
      RootComplex->Pcie[RPStart].MaxWidth     = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart], MaxWidth);
      RootComplex->Pcie[RPStart + 1].Active   = FALSE;
      RootComplex->Pcie[RPStart + 2].MaxWidth = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart + 2], MaxWidth);
      RootComplex->Pcie[RPStart + 2].Active   = TRUE;
      RootComplex->Pcie[RPStart + 3].Active   = FALSE;
      break;

    case DevMapMode3:
      MaxWidth                                = (RootComplex->Type == RootComplexTypeA) ? LINK_WIDTH_X8 : LINK_WIDTH_X4;
      RootComplex->Pcie[RPStart].MaxWidth     = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart], MaxWidth);
      RootComplex->Pcie[RPStart + 1].Active   = FALSE;
      MaxWidth                                = (RootComplex->Type == RootComplexTypeA) ? LINK_WIDTH_X4 : LINK_WIDTH_X2;
      RootComplex->Pcie[RPStart + 2].MaxWidth = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart + 2], MaxWidth);
      RootComplex->Pcie[RPStart + 2].Active   = TRUE;
      RootComplex->Pcie[RPStart + 3].MaxWidth = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart + 3], MaxWidth);
      RootComplex->Pcie[RPStart + 3].Active   = TRUE;
      break;

    case DevMapModeAuto:
    case DevMapMode4:
      MaxWidth                                = (RootComplex->Type == RootComplexTypeA) ? LINK_WIDTH_X4 : LINK_WIDTH_X2;
      RootComplex->Pcie[RPStart].MaxWidth     = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart], MaxWidth);
      RootComplex->Pcie[RPStart + 1].MaxWidth = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart + 1], MaxWidth);
      RootComplex->Pcie[RPStart + 1].Active   = TRUE;
      RootComplex->Pcie[RPStart + 2].MaxWidth = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart + 2], MaxWidth);
      RootComplex->Pcie[RPStart + 2].Active   = TRUE;
      RootComplex->Pcie[RPStart + 3].MaxWidth = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart + 3], MaxWidth);
      RootComplex->Pcie[RPStart + 3].Active   = TRUE;
      break;

    case DevMapMode1:
    default:
      MaxWidth                              = (RootComplex->Type == RootComplexTypeA) ? LINK_WIDTH_X16 : LINK_WIDTH_X8;
      RootComplex->Pcie[RPStart].MaxWidth   = PCIE_GET_MAX_WIDTH (RootComplex->Pcie[RPStart], MaxWidth);
      RootComplex->Pcie[RPStart + 1].Active = FALSE;
      RootComplex->Pcie[RPStart + 2].Active = FALSE;
      RootComplex->Pcie[RPStart + 3].Active = FALSE;
      break;
  }
}

DEV_MAP_MODE
GetDefaultDevMap (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN BOOLEAN            IsGetDevMapLow
  )
{
  UINT8         StartIndex;
  DEV_MAP_MODE  DevMapMode;

  DevMapMode = DevMapMode4;
  StartIndex = IsGetDevMapLow ? PcieController0 : PcieController4;

  while (DevMapMode >= DevMapMode1) {
    switch (DevMapMode) {
      case DevMapMode4:
        if (  RootComplex->Pcie[StartIndex].Active
           && RootComplex->Pcie[StartIndex + 1].Active
           && RootComplex->Pcie[StartIndex + 2].Active
           && RootComplex->Pcie[StartIndex + 3].Active)
        {
          return DevMapMode4;
        }

        break;
      case DevMapMode3:
        if (  RootComplex->Pcie[StartIndex].Active
           && RootComplex->Pcie[StartIndex + 2].Active
           && RootComplex->Pcie[StartIndex + 3].Active)
        {
          return DevMapMode3;
        }

        break;
      case DevMapMode2:
        if (  RootComplex->Pcie[StartIndex].Active
           && RootComplex->Pcie[StartIndex + 2].Active)
        {
          return DevMapMode2;
        }

        break;
      default:
        return DevMapMode1;
    }

    DevMapMode--;
  }

  return DevMapMode1;
}

VOID
GetDevMap (
  IN OUT AC01_ROOT_COMPLEX  *RootComplex
  )
{
  //
  // Get default Devmap low and configure Devmap low accordingly.
  //
  if (RootComplex->DefaultDevMapLow != DevMapModeAuto) {
    RootComplex->DefaultDevMapLow = GetDefaultDevMap (RootComplex, TRUE);
  }

  if (RootComplex->DevMapLow == 0) {
    RootComplex->DevMapLow = RootComplex->DefaultDevMapLow;
  }

  //
  // Get default Devmap high and configure Devmap high accordingly.
  //
  RootComplex->DefaultDevMapHigh = IsAc01Processor () ? GetDefaultDevMap (RootComplex, FALSE) : DevMapMode1;
  if ((RootComplex->Type == RootComplexTypeB) && (RootComplex->DevMapHigh == 0)) {
    RootComplex->DevMapHigh = RootComplex->DefaultDevMapHigh;
  }

  //
  // Set bifurcation bases on Devmap high and Devmap low.
  //
  SetRootComplexBifurcation (RootComplex, PcieController0, RootComplex->DevMapLow);
  if (RootComplex->Type == RootComplexTypeB) {
    SetRootComplexBifurcation (RootComplex, PcieController4, RootComplex->DevMapHigh);
  }
}

UINT8
GetMaxController (
  IN AC01_ROOT_COMPLEX  *RootComplex
  )
{
  if (IsAc01Processor ()) {
    return MaxPcieControllerOfRootComplexA;
  }

  return RootComplex->MaxPcieController;
}

NVPARAM
CalculateNvParamOffset (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PaddingOrder,
  IN UINT8              StartIndex,
  IN UINT64             StartOffset
  )
{
  UINT8    NeededPadding;
  INT8     PositionFromStartIndex;
  NVPARAM  NvParamOffset;

  NeededPadding          = RootComplex->ID - PaddingOrder;
  PositionFromStartIndex = (RootComplex->ID - StartIndex) + NeededPadding;
  NvParamOffset          = StartOffset + PositionFromStartIndex * NV_PARAM_ENTRYSIZE;

  return NvParamOffset;
}

EFI_STATUS_CODE_TYPE
GetNvParamOffsetLane (
  IN  AC01_ROOT_COMPLEX  *RootComplex,
  OUT NVPARAM            *NvParamOffset
  )
{
  BOOLEAN  IsAc01;
  BOOLEAN  IsRootComplexTypeA;
  BOOLEAN  IsSocket0;
  UINT8    StartIndex;
  UINT64   StartOffset;
  UINT8    PaddingOrder;

  IsSocket0          = RootComplex->Socket == 0 ? TRUE : FALSE;
  IsAc01             = IsAc01Processor ();
  IsRootComplexTypeA = RootComplex->Type == RootComplexTypeA ? TRUE : FALSE;

  if (!IsAc01 && (RootComplex->ID >= MaxPcieControllerOfRootComplexA)) {
    // Because from NV_SI_RO_BOARD_S0_RCA4_CFG to NV_SI_RO_BOARD_S0_RCA7_CFG for supporting
    // Altra Max are not sequential arrangement with NV_SI_RO_BOARD_S0_RCA0_CFG
    // so the start index will be the first Root Complex ID which using these NVParams
    // (NV_SI_RO_BOARD_S0_RCA4_CFG to NV_SI_RO_BOARD_S0_RCA7_CFG) to support Altra Max processor.
    StartIndex   = 4;
    StartOffset  = IsSocket0 ? NV_SI_RO_BOARD_S0_RCA4_CFG : NV_SI_RO_BOARD_S1_RCA4_CFG;
    PaddingOrder = RootComplex->ID;
  } else {
    StartIndex   = 0;
    StartOffset  = IsSocket0 ? NV_SI_RO_BOARD_S0_RCA0_CFG : NV_SI_RO_BOARD_S1_RCA0_CFG;
    PaddingOrder = IsRootComplexTypeA ? RootComplex->ID : MaxRootComplexA;
  }

  *NvParamOffset = CalculateNvParamOffset (RootComplex, PaddingOrder, StartIndex, StartOffset);
  return EFI_SUCCESS;
}

EFI_STATUS
GetNvParamOffsetPreset (
  IN  AC01_ROOT_COMPLEX         *RootComplex,
  IN  NVPARAM_PCIE_PRESET_TYPE  PresetType,
  OUT NVPARAM                   *NvParamOffset
  )
{
  BOOLEAN  IsAc01;
  BOOLEAN  IsRootComplexTypeA;
  BOOLEAN  IsSocket0;
  UINT8    StartIndex;
  UINT64   StartOffset;
  UINT8    PaddingOrder;

  IsSocket0          = RootComplex->Socket == 0 ? TRUE : FALSE;
  IsAc01             = IsAc01Processor ();
  IsRootComplexTypeA = RootComplex->Type == RootComplexTypeA ? TRUE : FALSE;

  switch (PresetType) {
    case Gen3Preset:
      if (IsAc01) {
        StartOffset = IsSocket0 ? NV_SI_RO_BOARD_S0_RCA0_TXRX_G3PRESET :
                      NV_SI_RO_BOARD_S1_RCA2_TXRX_G3PRESET;
      } else {
        StartOffset = IsSocket0 ? NV_SI_RO_BOARD_MQ_S0_RCA0_TXRX_G3PRESET :
                      NV_SI_RO_BOARD_MQ_S1_RCA2_TXRX_G3PRESET;
      }

      break;

    case Gen4Preset:
      if (IsAc01) {
        StartOffset = IsSocket0 ? NV_SI_RO_BOARD_S0_RCA0_TXRX_G4PRESET :
                      NV_SI_RO_BOARD_S1_RCA2_TXRX_G4PRESET;
      } else {
        StartOffset = IsSocket0 ? NV_SI_RO_BOARD_MQ_S0_RCA0_TXRX_G4PRESET :
                      NV_SI_RO_BOARD_MQ_S1_RCA2_TXRX_G4PRESET;
      }

      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // For Socket 0, NVParams for all Root Complexes are supported so starting from RCA0.
  // For Socket 1, NVParams for RCA0 and RCA1 are not supported so starting from RCA2.
  //
  StartIndex = IsSocket0 ? 0 : 2;
  //
  // There're two NVParam entries per RootComplexTypeB
  // so padding need to be start from MaxRootComplexA to
  // get the first NVParam entry of RootComplexTypeB
  //
  PaddingOrder = IsRootComplexTypeA ? RootComplex->ID : MaxRootComplexA;

  *NvParamOffset = CalculateNvParamOffset (RootComplex, PaddingOrder, StartIndex, StartOffset);

  return EFI_SUCCESS;
}

VOID
GetLaneAllocation (
  IN OUT AC01_ROOT_COMPLEX  *RootComplex
  )
{
  EFI_STATUS  Status;
  INTN        RPIndex;
  NVPARAM     NvParamOffset;
  UINT32      Value;
  UINT32      Width;
  UINT32      MaxController;

  Status = GetNvParamOffsetLane (RootComplex, &NvParamOffset);
  if (!EFI_ERROR (Status)) {
    Status = NVParamGet (NvParamOffset, NV_PERM_ALL, &Value);
    if (EFI_ERROR (Status)) {
      Value = 0;
    }
  } else {
    Value = 0;
  }

  MaxController = GetMaxController (RootComplex);
  for (RPIndex = PcieController0; RPIndex < MaxController; RPIndex++) {
    Width = (Value >> (RPIndex * BITS_PER_BYTE)) & BYTE_MASK;
    switch (Width) {
      case 1:
      case 2:
      case 3:
      case 4:
        RootComplex->Pcie[RPIndex].MaxWidth = 1 << Width;
        RootComplex->Pcie[RPIndex].MaxGen   = LINK_SPEED_GEN3;
        RootComplex->Pcie[RPIndex].Active   = TRUE;
        break;

      case 0:
      default:
        RootComplex->Pcie[RPIndex].MaxWidth = LINK_WIDTH_NONE;
        RootComplex->Pcie[RPIndex].MaxGen   = LINK_SPEED_NONE;
        RootComplex->Pcie[RPIndex].Active   = FALSE;
        break;
    }
  }

  // Update RootComplex data to handle auto bifurcation mode on RCA
  if (Value == AUTO_BIFURCATION_SETTING_VALUE) {
    RootComplex->Pcie[PcieController0].MaxWidth = LINK_WIDTH_X4;
    RootComplex->Pcie[PcieController0].MaxGen   = LINK_SPEED_GEN3;
    RootComplex->Pcie[PcieController0].Active   = TRUE;
    RootComplex->DefaultDevMapLow               = DevMapModeAuto;
  }

  if (RootComplex->Type == RootComplexTypeB) {
    NvParamOffset += NV_PARAM_ENTRYSIZE;
    Status         = NVParamGet (NvParamOffset, NV_PERM_ALL, &Value);
    if (EFI_ERROR (Status)) {
      Value = 0;
    }

    for (RPIndex = MaxPcieControllerOfRootComplexA; RPIndex < MaxPcieController; RPIndex++) {
      Width = (Value >> ((RPIndex - MaxPcieControllerOfRootComplexA) * BITS_PER_BYTE)) & BYTE_MASK;
      switch (Width) {
        case 1:
        case 2:
        case 3:
        case 4:
          RootComplex->Pcie[RPIndex].MaxWidth = 1 << Width;
          RootComplex->Pcie[RPIndex].MaxGen   = LINK_SPEED_GEN3;
          RootComplex->Pcie[RPIndex].Active   = TRUE;
          break;

        case 0:
        default:
          RootComplex->Pcie[RPIndex].MaxWidth = LINK_WIDTH_NONE;
          RootComplex->Pcie[RPIndex].MaxGen   = LINK_SPEED_NONE;
          RootComplex->Pcie[RPIndex].Active   = FALSE;
          break;
      }
    }
  }

  // Do not proceed if no Root Port enabled
  if (IsEmptyRC (RootComplex)) {
    RootComplex->Active = FALSE;
  }
}

VOID
GetPresetSetting (
  AC01_ROOT_COMPLEX  *RootComplex
  )
{
  EFI_STATUS  Status;
  INTN        Index;
  NVPARAM     NvParamOffset;
  UINT32      Value;

  // Load default value
  for (Index = 0; Index < MaxPcieControllerOfRootComplexB; Index++) {
    RootComplex->PresetGen3[Index] = PRESET_INVALID;
    RootComplex->PresetGen4[Index] = PRESET_INVALID;
  }

  // Get NVParam offset of Gen3 preset
  Status = GetNvParamOffsetPreset (RootComplex, Gen3Preset, &NvParamOffset);
  if (!EFI_ERROR (Status)) {
    Status = NVParamGet (NvParamOffset, NV_PERM_ALL, &Value);
  }

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < MaxPcieControllerOfRootComplexA; Index++) {
      RootComplex->PresetGen3[Index] = (Value >> (Index * BITS_PER_BYTE)) & BYTE_MASK;
    }
  }

  if (RootComplex->Type == RootComplexTypeB) {
    NvParamOffset += NV_PARAM_ENTRYSIZE;
    Status         = NVParamGet (NvParamOffset, NV_PERM_ALL, &Value);
    if (!EFI_ERROR (Status)) {
      for (Index = MaxPcieControllerOfRootComplexA; Index < MaxPcieController; Index++) {
        RootComplex->PresetGen3[Index] = (Value >> ((Index - MaxPcieControllerOfRootComplexA) * BITS_PER_BYTE)) & BYTE_MASK;
      }
    }
  }

  // Get NVParam offset of Gen4 preset.
  Status = GetNvParamOffsetPreset (RootComplex, Gen4Preset, &NvParamOffset);
  if (!EFI_ERROR (Status)) {
    Status = NVParamGet (NvParamOffset, NV_PERM_ALL, &Value);
  }

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < MaxPcieControllerOfRootComplexA; Index++) {
      RootComplex->PresetGen4[Index] = (Value >> (Index * BITS_PER_BYTE)) & BYTE_MASK;
    }
  }

  if (RootComplex->Type == RootComplexTypeB) {
    NvParamOffset += NV_PARAM_ENTRYSIZE;
    Status         = NVParamGet (NvParamOffset, NV_PERM_ALL, &Value);
    if (!EFI_ERROR (Status)) {
      for (Index = MaxPcieControllerOfRootComplexA; Index < MaxPcieController; Index++) {
        RootComplex->PresetGen4[Index] = (Value >> ((Index - MaxPcieControllerOfRootComplexA) * BITS_PER_BYTE)) & BYTE_MASK;
      }
    }
  }
}

VOID
GetMaxSpeedGen (
  AC01_ROOT_COMPLEX  *RootComplex
  )
{
  UINT8  MaxSpeedGen[MaxPcieControllerOfRootComplexA]        = { LINK_SPEED_GEN4, LINK_SPEED_GEN4, LINK_SPEED_GEN4, LINK_SPEED_GEN4 }; // Bifurcation 0: RootComplexTypeA x16 / RootComplexTypeB x8
  UINT8  ErrataSpeedDevMap3[MaxPcieControllerOfRootComplexA] = { LINK_SPEED_GEN4, LINK_SPEED_GEN4, LINK_SPEED_GEN1, LINK_SPEED_GEN1 }; // Bifurcation 2: x8 x4 x4 (PCIE_ERRATA_SPEED1)
  UINT8  ErrataSpeedDevMap4[MaxPcieControllerOfRootComplexA] = { LINK_SPEED_GEN1, LINK_SPEED_GEN1, LINK_SPEED_GEN1, LINK_SPEED_GEN1 }; // Bifurcation 3: x4 x4 x4 x4 (PCIE_ERRATA_SPEED1)
  UINT8  ErrataSpeedRcb[MaxPcieControllerOfRootComplexA]     = { LINK_SPEED_GEN1, LINK_SPEED_GEN1, LINK_SPEED_GEN1, LINK_SPEED_GEN1 }; // RootComplexTypeB PCIE_ERRATA_SPEED1
  UINT8  Idx;
  UINT8  MaxController;
  UINT8  *MaxGen;

  ASSERT (MaxPcieControllerOfRootComplexA == 4);
  ASSERT (MaxPcieController == 8);

  //
  // Due to hardware errata, for A0/A1*
  //  RootComplexTypeB is limited to Gen1 speed.
  //  RootComplexTypeA x16, x8 port supports up to Gen4,
  //  RootComplexTypeA x4 port only supports Gen1.
  //
  MaxGen = MaxSpeedGen;
  if (RootComplex->Type == RootComplexTypeB) {
    if (RootComplex->Flags & PCIE_ERRATA_SPEED1) {
      MaxGen = ErrataSpeedRcb;
    }
  } else {
    switch (RootComplex->DevMapLow) {
      case DevMapMode3: /* x8 x4 x4 */
        if (RootComplex->Flags & PCIE_ERRATA_SPEED1) {
          MaxGen = ErrataSpeedDevMap3;
        }

        break;

      case DevMapModeAuto:
      case DevMapMode4: /* x4 x4 x4 x4 */
        if (RootComplex->Flags & PCIE_ERRATA_SPEED1) {
          MaxGen = ErrataSpeedDevMap4;
        }

        break;

      case DevMapMode2: /* x8 x8 */
      case DevMapMode1: /* x16 */
      default:
        break;
    }
  }

  MaxController = GetMaxController (RootComplex);
  for (Idx = 0; Idx < MaxController; Idx++) {
    RootComplex->Pcie[Idx].MaxGen = RootComplex->Pcie[Idx].Active ? MaxGen[Idx] : LINK_SPEED_NONE;
  }

  if (RootComplex->Type == RootComplexTypeB) {
    for (Idx = MaxPcieControllerOfRootComplexA; Idx < MaxPcieController; Idx++) {
      RootComplex->Pcie[Idx].MaxGen = RootComplex->Pcie[Idx].Active ?
                                      MaxGen[Idx - MaxPcieControllerOfRootComplexA] : LINK_SPEED_NONE;
    }
  }
}

VOID
ParseRootComplexNVParamData (
  IN OUT AC01_ROOT_COMPLEX  *RootComplex
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;
  UINT32             EFuse;
  UINT8              RootComplexID;
  VOID               *Hob;

  EFuse = 0;
  Hob   = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob != NULL) {
    PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);
    EFuse       = PlatformHob->RcDisableMask[0] | (PlatformHob->RcDisableMask[1] << AC01_PCIE_MAX_RCS_PER_SOCKET);
    DEBUG ((
      DEBUG_INFO,
      "RcDisableMask[0]: 0x%x [1]: 0x%x\n",
      PlatformHob->RcDisableMask[0],
      PlatformHob->RcDisableMask[1]
      ));

    // Update errata flags for Ampere Altra
    if ((PlatformHob->ScuProductId[0] & 0xff) == 0x01) {
      if (  (PlatformHob->AHBCId[0] == 0x20100)
         || (PlatformHob->AHBCId[0] == 0x21100)
         || (  IsSlaveSocketActive ()
            && (  (PlatformHob->AHBCId[1] == 0x20100)
               || (PlatformHob->AHBCId[1] == 0x21100))))
      {
        RootComplex->Flags |= PCIE_ERRATA_SPEED1;
        DEBUG ((DEBUG_INFO, "RootComplex[%d]: Flags 0x%x\n", RootComplex->ID, RootComplex->Flags));
      }
    }
  }

  RootComplexID              = RootComplex->Socket * AC01_PCIE_MAX_RCS_PER_SOCKET + RootComplex->ID;
  RootComplex->DefaultActive = !(EFuse & BIT (RootComplexID)) ? TRUE : FALSE;
  if (!IsSlaveSocketActive () && (RootComplex->Socket == 1)) {
    RootComplex->DefaultActive = FALSE;
  }

  RootComplex->Active = RootComplex->Active && RootComplex->DefaultActive;

  GetPresetSetting (RootComplex);
  GetLaneAllocation (RootComplex);
  GetDevMap (RootComplex);
  GetMaxSpeedGen (RootComplex);
}
