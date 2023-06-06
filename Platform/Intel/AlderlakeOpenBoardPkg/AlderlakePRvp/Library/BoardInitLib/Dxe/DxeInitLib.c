/** @file

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <MemInfoHob.h>
#include <PlatformBoardConfig.h>
#include <PlatformBoardId.h>
#include <Pins/GpioPinsVer2Lp.h>
#include <Register/PchRegs.h>
#include <Library/PciSegmentLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BoardConfigLib.h>
#include <Library/DebugLib.h>



/**
   Retimer Platform Specific Data
**/


EFI_STATUS
PatchConfigurationDataInit (
  IN CONFIG_PATCH_STRUCTURE  *ConfigPatchStruct,
  IN UINTN                   ConfigPatchStructSize
  );


VOID
AdlPInitCommonPlatformPcd (
  VOID
  )
{

  PcdSetBoolS (PcdPssReadSN, TRUE);
  PcdSet8S (PcdPssI2cSlaveAddress, 0x6E);
  PcdSet8S (PcdPssI2cBusNumber, 0x05);
  PcdSetBoolS (PcdSpdAddressOverride, FALSE);

}

VOID
AdlPUpdateDimmPopulation (
  VOID
  )
{
  MEMORY_INFO_DATA_HOB    *MemInfo;
  UINT8                   Slot0;
  UINT8                   Slot1;
  UINT8                   Slot2;
  UINT8                   Slot3;
  CONTROLLER_INFO         *ControllerInfo;
  EFI_HOB_GUID_TYPE       *GuidHob;

  GuidHob = NULL;
  MemInfo = NULL;
  GuidHob = GetFirstGuidHob (&gSiMemoryInfoDataGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    MemInfo = (MEMORY_INFO_DATA_HOB *) GET_GUID_HOB_DATA (GuidHob);
  }
  if (MemInfo != NULL) {
    if ( PcdGet8 (PcdPlatformFlavor) == FlavorDesktop ||
         PcdGet8 (PcdPlatformFlavor) == FlavorUpServer ||
         PcdGet8 (PcdPlatformFlavor) == FlavorWorkstation
        ) {
        ControllerInfo = &MemInfo->Controller[0];
        Slot0 = ControllerInfo->ChannelInfo[0].DimmInfo[0].Status;
        Slot1 = ControllerInfo->ChannelInfo[0].DimmInfo[1].Status;
        Slot2 = ControllerInfo->ChannelInfo[1].DimmInfo[0].Status;
        Slot3 = ControllerInfo->ChannelInfo[1].DimmInfo[1].Status;

      //
      // Channel 0          Channel 1
      // Slot0   Slot1      Slot0   Slot1      - Population            AIO board
      // 0          0          0          0          - Invalid        - Invalid
      // 0          0          0          1          - Valid          - Invalid
      // 0          0          1          0          - Invalid        - Valid
      // 0          0          1          1          - Valid          - Valid
      // 0          1          0          0          - Valid          - Invalid
      // 0          1          0          1          - Valid          - Invalid
      // 0          1          1          0          - Invalid        - Invalid
      // 0          1          1          1          - Valid          - Invalid
      // 1          0          0          0          - Invalid        - Valid
      // 1          0          0          1          - Invalid        - Invalid
      // 1          0          1          0          - Invalid        - Valid
      // 1          0          1          1          - Invalid        - Valid
      // 1          1          0          0          - Valid          - Valid
      // 1          1          0          1          - Valid          - Invalid
      // 1          1          1          0          - Invalid        - Valid
      // 1          1          1          1          - Valid          - Valid
      //
      if ((Slot0 && (Slot1 == 0)) || (Slot2 && (Slot3 == 0))) {
        PcdSetBoolS (PcdDimmPopulationError, TRUE);
      }
    }
  }
}

/**
  Enable Tier2 GPIO Sci wake capability.

  @retval EFI_SUCCESS   The function completed successfully.
**/
EFI_STATUS
AdlPTier2GpioWakeSupport (
  VOID
  )
{
  BOOLEAN Tier2GpioWakeEnable;

  Tier2GpioWakeEnable = FALSE;
  PcdSetBoolS (PcdGpioTier2WakeEnable, Tier2GpioWakeEnable);

  return EFI_SUCCESS;
}


/**
  A hook for board-specific initialization after PCI enumeration.

  @retval EFI_SUCCESS   The board initialization was successful.
**/
EFI_STATUS
EFIAPI
AdlPBoardInitAfterPciEnumeration (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "AdlPBoardInitAfterPciEnumeration\n"));

  AdlPTier2GpioWakeSupport ();
  AdlPInitCommonPlatformPcd ();

  return EFI_SUCCESS;
}

/**
  A hook for board-specific functionality for the ReadyToBoot event.

  @retval EFI_SUCCESS   The board initialization was successful.
**/
EFI_STATUS
EFIAPI
AdlPBoardInitReadyToBoot (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "AdlPBoardInitReadyToBoot\n"));

  AdlPUpdateDimmPopulation ();

  return EFI_SUCCESS;
}

/**
  A hook for board-specific functionality for the ExitBootServices event.

  @retval EFI_SUCCESS   The board initialization was successful.
**/
EFI_STATUS
EFIAPI
AdlPBoardInitEndOfFirmware (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "AdlPBoardInitEndOfFirmware\n"));

  return EFI_SUCCESS;
}
