/** @file

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BiosIdLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiLib.h>
#include <Guid/MemoryOverwriteControl.h>
#include <PlatformBoardConfig.h>
#include <Library/PchCycleDecodingLib.h>
#include <Register/PmcRegs.h>
#include <Library/PmcLib.h>
#include <Library/PeiBootModeLib.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/PeiServicesLib.h>
#include <Library/GpioLib.h>
#include <Library/BoardConfigLib.h>
#include <Library/TimerLib.h>
#include <PlatformBoardId.h>
#include <Library/IoLib.h>
#include <Pins/GpioPinsVer2Lp.h>
#include <Library/PchInfoLib.h>

static EFI_PEI_PPI_DESCRIPTOR mSetupVariablesReadyPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gSetupVariablesReadyPpiGuid,
  NULL
};

/**
  Alderlake P boards configuration init function for PEI pre-memory phase.

  @retval EFI_SUCCESS             The function completed successfully.
**/
EFI_STATUS
EFIAPI
AdlPInitPreMem (
  VOID
  )
{
  UINT8                             MorControl;

  //
  // MOR
  //
  MorControl = 0;
  if (MOR_CLEAR_MEMORY_VALUE (MorControl)) {
    PcdSet8S (PcdCleanMemory, MorControl & MOR_CLEAR_MEMORY_BIT_MASK);
  }

  PcdSet32S (PcdStackBase, PcdGet32 (PcdTemporaryRamBase) + PcdGet32 (PcdTemporaryRamSize) - (PcdGet32 (PcdFspTemporaryRamSize) + PcdGet32 (PcdFspReservedBufferSize)));
  PcdSet32S (PcdStackSize, PcdGet32 (PcdFspTemporaryRamSize));

  PcdSet8S (PcdCpuRatio, 0x0);

  return EFI_SUCCESS;
}


VOID
AdlPMrcConfigInit (
  VOID
  );

VOID
AdlPSaMiscConfigInit (
  VOID
  );

VOID
AdlPSaGpioConfigInit (
  VOID
  );

VOID
AdlPSaDisplayConfigInit (
  VOID
  );

VOID
AdlPSaUsbConfigInit (
  VOID
  );

EFI_STATUS
AdlPRootPortClkInfoInit (
  VOID
  );

EFI_STATUS
AdlPUsbConfigInit (
  VOID
  );

VOID
AdlPGpioGroupTierInit (
  VOID
  );


/**
  Notifies the gPatchConfigurationDataPreMemPpiGuid has been Installed

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
**/
EFI_STATUS
EFIAPI
AdlPBoardPatchConfigurationDataPreMemCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PeiServicesInstallPpi (&mSetupVariablesReadyPpi);

  return RETURN_SUCCESS;
}

/**
  Board Misc init function for PEI pre-memory phase.
**/
VOID
AdlPBoardMiscInitPreMem (
  VOID
  )
{
  UINT16    BoardId;
  BoardId = PcdGet16(PcdBoardId);

  //
  // OddPower Init
  //
  PcdSetBoolS (PcdOddPowerInitEnable, FALSE);

  //
  // Pc8374SioKbc Present
  //
  PcdSetBoolS (PcdPc8374SioKbcPresent, FALSE);

  //
  // Smbus Alert function Init.
  //
  PcdSetBoolS (PcdSmbusAlertEnable, FALSE);
}


/**
  A hook for board-specific initialization prior to memory initialization.

  @retval EFI_SUCCESS   The board initialization was successful.
**/
EFI_STATUS
EFIAPI
AdlPBoardInitBeforeMemoryInit (
  VOID
  )
{
  EFI_STATUS        Status;

  DEBUG ((DEBUG_INFO, "AdlPBoardInitBeforeMemoryInit\n"));

  GetBiosId (NULL);

  AdlPInitPreMem ();

  AdlPBoardMiscInitPreMem ();

  AdlPGpioGroupTierInit ();


  AdlPMrcConfigInit ();
  AdlPSaGpioConfigInit ();
  AdlPSaMiscConfigInit ();
  Status = AdlPRootPortClkInfoInit ();
  Status = AdlPUsbConfigInit ();
  AdlPSaDisplayConfigInit ();
  AdlPSaUsbConfigInit ();
  if (PcdGetPtr (PcdBoardGpioTableEarlyPreMem) != 0) {
    GpioInit (PcdGetPtr (PcdBoardGpioTableEarlyPreMem));

    MicroSecondDelay (15 * 1000); // 15 ms Delay
  }
  // Configure GPIO Before Memory
  GpioInit (PcdGetPtr (PcdBoardGpioTablePreMem));

  return EFI_SUCCESS;
}


/**
  This board service initializes board-specific debug devices.

  @retval EFI_SUCCESS   Board-specific debug initialization was successful.
**/
EFI_STATUS
EFIAPI
AdlPBoardDebugInit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "AdlPBoardDebugInit\n"));

  return EFI_SUCCESS;
}

/**
  This board service detects the boot mode.

  @retval EFI_BOOT_MODE The boot mode.
**/
EFI_BOOT_MODE
EFIAPI
AdlPBoardBootModeDetect (
  VOID
  )
{
  EFI_BOOT_MODE                             BootMode;

  DEBUG ((DEBUG_INFO, "AdlPBoardBootModeDetect\n"));
  BootMode = DetectBootMode ();
  DEBUG ((DEBUG_INFO, "BootMode: 0x%02X\n", BootMode));

  return BootMode;
}
