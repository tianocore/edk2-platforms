/** @file
  Implements BootMode.C

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

EFI_PEI_PPI_DESCRIPTOR         mPpiListRecoveryBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
};
STATIC EFI_PEI_PPI_DESCRIPTOR  CapsulePpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI|EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gCapsuleUpdateDetectedPpiGuid,
  NULL
};

/**

Routine Description:

  This function is used to verify if the FV header is validate.

  @param  FwVolHeader - The FV header that to be verified.

  @retval EFI_SUCCESS   - The Fv header is valid.
  @retval EFI_NOT_FOUND - The Fv header is invalid.

**/
EFI_STATUS
ValidateFvHeader (
  EFI_BOOT_MODE  *BootMode
  )
{
  UINT16  *Ptr;
  UINT16  HeaderLength;
  UINT16  Checksum;

  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;

  if (BOOT_IN_RECOVERY_MODE == *BootMode) {
    DEBUG ((DEBUG_INFO, "Boot mode recovery\n"));
    return EFI_SUCCESS;
  }

  //
  // Let's check whether FvMain header is valid, if not enter into recovery mode
  //
  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvMainBase);
  if ((FwVolHeader->Revision != EFI_FVH_REVISION) ||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength == ((UINT64)-1)) ||
      ((FwVolHeader->HeaderLength & 0x01) != 0)
      )
  {
    return EFI_NOT_FOUND;
  }

  //
  // Verify the header checksum
  //
  HeaderLength = (UINT16)(FwVolHeader->HeaderLength / 2);
  Ptr          = (UINT16 *)FwVolHeader;
  Checksum     = 0;
  while (HeaderLength > 0) {
    Checksum = Checksum +*Ptr;
    Ptr++;
    HeaderLength--;
  }

  if (Checksum != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Peform the boot mode determination logic

  @param  PeiServices General purpose services available to every PEIM.
  @param  BootMode The detected boot mode.

  @retval EFI_SUCCESS if the boot mode could be set
**/
EFI_STATUS
UpdateBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  OUT EFI_BOOT_MODE          *BootMode
  )
{
  EFI_STATUS           Status;
  UINT16               SleepType;
  CHAR16               *strBootMode;
  EFI_BOOT_MODE        NewBootMode;
  EFI_PEI_CAPSULE_PPI  *Capsule;
  BOOLEAN              IsFirstBoot;

  //
  // Let's assume things are OK if not told otherwise
  //
  NewBootMode = BOOT_WITH_FULL_CONFIGURATION;

  //
  // When this boot is WDT reset, the system needs booting with CrashDump function eanbled.
  // Check Power Button, click the power button, the system will boot in fast boot mode,
  // if it is pressed and hold for a second, it will boot in FullConfiguration/setup mode.
  //
  IsFirstBoot = PcdGetBool (PcdBootState);

  if (!IsFirstBoot) {
    NewBootMode = BOOT_WITH_MINIMAL_CONFIGURATION;
  }

  //
  // Check if we need to boot in forced recovery mode
  //
  if (ValidateFvHeader (&NewBootMode) != EFI_SUCCESS) {
    NewBootMode = BOOT_IN_RECOVERY_MODE;
    DEBUG ((DEBUG_ERROR, "RECOVERY from corrupt FV\n"));
  }

  if (NewBootMode  == BOOT_IN_RECOVERY_MODE) {
    Status = (*PeiServices)->InstallPpi (
                               PeiServices,
                               &mPpiListRecoveryBootMode
                               );
    ASSERT_EFI_ERROR (Status);
  } else {
    if (GetSleepTypeAfterWakeup (PeiServices, &SleepType)) {
      switch (SleepType) {
        case V_SLP_TYPE_S3:
          NewBootMode = BOOT_ON_S3_RESUME;
          break;

        case V_SLP_TYPE_S4:
          NewBootMode = BOOT_ON_S4_RESUME;
          break;

        case V_SLP_TYPE_S5:
          NewBootMode = BOOT_ON_S5_RESUME;
          break;
      } // switch (SleepType)
    }

    //
    // Determine if we're in capsule update mode
    //
    Status = (*PeiServices)->LocatePpi (
                               PeiServices,
                               &gEfiPeiCapsulePpiGuid,
                               0,
                               NULL,
                               (void **)&Capsule
                               );

    if (Status == EFI_SUCCESS) {
      if (Capsule->CheckCapsuleUpdate ((EFI_PEI_SERVICES **)PeiServices) == EFI_SUCCESS) {
        NewBootMode = BOOT_ON_FLASH_UPDATE;
        DEBUG ((DEBUG_ERROR, "Setting BootMode to %x\n", BOOT_ON_FLASH_UPDATE));

        (*PeiServices)->InstallPpi (PeiServices, &CapsulePpi);
      }
    }

    //
    // Check for Safe Mode
    //
  }

  switch (NewBootMode) {
    case BOOT_WITH_FULL_CONFIGURATION:
      strBootMode = L"BOOT_WITH_FULL_CONFIGURATION";
      break;
    case BOOT_WITH_MINIMAL_CONFIGURATION:
      strBootMode = L"BOOT_WITH_MINIMAL_CONFIGURATION";
      break;
    case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
      strBootMode = L"BOOT_ASSUMING_NO_CONFIGURATION_CHANGES";
      break;
    case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
      strBootMode = L"BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS";
      break;
    case BOOT_WITH_DEFAULT_SETTINGS:
      strBootMode = L"BOOT_WITH_DEFAULT_SETTINGS";
      break;
    case BOOT_ON_S4_RESUME:
      strBootMode = L"BOOT_ON_S4_RESUME";
      break;
    case BOOT_ON_S5_RESUME:
      strBootMode = L"BOOT_ON_S5_RESUME";
      break;
    case BOOT_ON_S2_RESUME:
      strBootMode = L"BOOT_ON_S2_RESUME";
      break;
    case BOOT_ON_S3_RESUME:
      strBootMode = L"BOOT_ON_S3_RESUME";
      break;
    case BOOT_ON_FLASH_UPDATE:
      strBootMode = L"BOOT_ON_FLASH_UPDATE";
      break;
    case BOOT_IN_RECOVERY_MODE:
      strBootMode = L"BOOT_IN_RECOVERY_MODE";
      break;
    default:
      strBootMode = L"Unknown boot mode";
  } // switch (BootMode)

  DEBUG ((DEBUG_INFO, "Setting BootMode to %s\n", strBootMode));
  Status = (*PeiServices)->SetBootMode (
                             PeiServices,
                             NewBootMode
                             );
  ASSERT_EFI_ERROR (Status);

  *BootMode = NewBootMode;

  return Status;
}

/**
  Get sleep type after wakeup

  @param PeiServices       Pointer to the PEI Service Table.
  @param SleepType         Sleep type to be returned.

  @retval TRUE              A wake event occured without power failure.
  @retval FALSE             Power failure occured or not a wakeup.

**/
BOOLEAN
GetSleepTypeAfterWakeup (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  OUT UINT16                  *SleepType
  )

{
  UINTN   AcpiPm1CntBlk;
  UINT16  Pm1Cnt;

  AcpiPm1CntBlk = MmioRead16 (ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REG62);

  Pm1Cnt = IoRead16 (AcpiPm1CntBlk);
  //
  // check power failur/loss when in S3 resume type.
  // Get sleep type if a wake event occurred and there is no power failure
  //
  if ((Pm1Cnt & B_SLP_TYPE) == V_SLP_TYPE_S3) {
    *SleepType = Pm1Cnt & B_SLP_TYPE;
    return TRUE;
  } else if ((Pm1Cnt & B_SLP_TYPE) == V_SLP_TYPE_S4) {
    *SleepType = Pm1Cnt & B_SLP_TYPE;
    return TRUE;
  }

  *SleepType = 0;
  Pm1Cnt    &= (~B_SLP_TYPE);
  IoWrite16 (AcpiPm1CntBlk, Pm1Cnt);
  return FALSE;
}
