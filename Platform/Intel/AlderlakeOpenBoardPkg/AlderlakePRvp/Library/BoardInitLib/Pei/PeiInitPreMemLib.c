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
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/PeiServicesLib.h>
#include <Library/GpioLib.h>
#include <Library/BoardConfigLib.h>
#include <Library/TimerLib.h>
#include <PlatformBoardId.h>
#include <Library/IoLib.h>
#include <Pins/GpioPinsVer2Lp.h>
#include <Library/PchInfoLib.h>

/**
  Get Vpd binary address

  Parse through each FV for VPD FFS file and return the address

  @retval Address on VPD FFS detection else returns 0

**/
UINTN
EFIAPI
GetVpdFfsAddress (
  )
{
  EFI_STATUS            Status;
  VOID                  *Address;
  UINTN                  Instance;
  EFI_PEI_FV_HANDLE      VolumeHandle;
  EFI_PEI_FILE_HANDLE    FileHandle;

  Address = NULL;

  VolumeHandle = NULL;
  Instance = 0;
  while (TRUE) {
    //
    // Traverse all firmware volume instances.
    //
    Status = PeiServicesFfsFindNextVolume (Instance, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    FileHandle = NULL;
    Status = PeiServicesFfsFindFileByName (&gVpdFfsGuid, VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      //
      // Search RAW section.
      //
      Status = PeiServicesFfsFindSectionData (EFI_SECTION_RAW, FileHandle, &Address);
      if (!EFI_ERROR (Status)) {
        return (UINTN)Address;
      }
    }

    //
    // Search the next volume.
    //
    Instance++;
  }

  DEBUG ((DEBUG_ERROR, " PEI get VPD address: %r\n", EFI_NOT_FOUND));
  return 0;
}

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
  UINTN                           VpdBaseAddress;

  VpdBaseAddress = (UINTN) PcdGet64 (PcdVpdBaseAddress64);
  DEBUG ((DEBUG_INFO, "VpdFfsAddress: %x\n", VpdBaseAddress));
  if (VpdBaseAddress == 0) {
    VpdBaseAddress= (UINTN) GetVpdFfsAddress();
    PcdSet64S (PcdVpdBaseAddress64,VpdBaseAddress);
    DEBUG ((DEBUG_INFO, "VpdFfsAddress updated: %x\n", VpdBaseAddress));
  }
  PcdSet32S (PcdStackBase, PcdGet32 (PcdTemporaryRamBase) + PcdGet32 (PcdTemporaryRamSize) - (PcdGet32 (PcdFspTemporaryRamSize) + PcdGet32 (PcdFspReservedBufferSize)));
  PcdSet32S (PcdStackSize, PcdGet32 (PcdFspTemporaryRamSize));

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
AdlPSaDisplayConfigInit (
  VOID
  );

EFI_STATUS
AdlPRootPortClkInfoInit (
  VOID
  );

VOID
AdlPGpioGroupTierInit (
  VOID
  );


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
  DEBUG ((DEBUG_INFO, "AdlPBoardInitBeforeMemoryInit\n"));

  AdlPInitPreMem ();

  AdlPGpioGroupTierInit ();

  AdlPMrcConfigInit ();
  AdlPSaMiscConfigInit ();
  AdlPRootPortClkInfoInit ();
  AdlPSaDisplayConfigInit ();
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
  DEBUG ((DEBUG_INFO, "AdlPBoardBootModeDetect\n"));
  return BOOT_WITH_FULL_CONFIGURATION;
}
