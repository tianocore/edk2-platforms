/** @file
  This file initialises and Installs GopPolicy Protocol.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/GraphicsInfoLib.h>
#include <Library/UefiLib.h>

#include <Protocol/SaPolicy.h>
#include <Protocol/PciEnumerationComplete.h>

//
// The boot script private data.
//
typedef struct {
  UINT8      *TableBase;
  UINT32     TableLength;           // Record the actual memory length
  UINT16     TableMemoryPageNumber; // Record the page number Allocated for the table
  BOOLEAN    InSmm;                 // Record if this library is in SMM.
  BOOLEAN    AtRuntime;             // Record if current state is after SmmExitBootServices or SmmLegacyBoot.
  UINT32     BootTimeScriptLength;  // Maintain boot time script length in LockBox after SmmReadyToLock in SMM.
  BOOLEAN    SmmLocked;             // Record if current state is after SmmReadyToLock
  BOOLEAN    BackFromS3;            // Indicate that the system is back from S3.
} SCRIPT_TABLE_PRIVATE_DATA;

SCRIPT_TABLE_PRIVATE_DATA  *mS3BootScriptTablePtr;
VOID
EFIAPI
S3SmmLockedCallback (
  IN EFI_EVENT Event,
  IN VOID      *Context
  );

EFI_STATUS
EFIAPI
CreateSaDxeConfigBlocks (
  IN OUT  VOID      **SaPolicy
  );

EFI_STATUS
EFIAPI
SaInstallPolicyProtocol (
  IN  EFI_HANDLE                  ImageHandle,
  IN  VOID                        *SaPolicy
  );

/**
  Initialize SA DXE Policy

  @param[in] ImageHandle          Image handle of this driver.

  @retval EFI_SUCCESS             Initialization complete.
  @retval EFI_UNSUPPORTED         The chipset is unsupported by this driver.
  @retval EFI_OUT_OF_RESOURCES    Do not have enough resources to initialize the driver.
  @retval EFI_DEVICE_ERROR        Device error, driver exits abnormally.
**/
EFI_STATUS
EFIAPI
SaPolicyInitDxe (
  IN EFI_HANDLE           ImageHandle
  )
{
  EFI_STATUS               Status;
  SA_POLICY_PROTOCOL       *SaPolicy;
  VOID                     *Registration;

  //
  // Call CreateSaDxeConfigBlocks to create & initialize platform policy structure
  // and get all Intel default policy settings.
  //
  Status = CreateSaDxeConfigBlocks ((VOID **) &SaPolicy);
  DEBUG ((DEBUG_INFO, "SaPolicy->TableHeader.NumberOfBlocks = 0x%x\n ", SaPolicy->TableHeader.NumberOfBlocks));
  ASSERT_EFI_ERROR (Status);

  ///
  /// Create S3 SmmLocked callback to fix assert
  ///
  EfiCreateProtocolNotifyEvent (
    &gEfiPciEnumerationCompleteProtocolGuid,
    TPL_CALLBACK,
    S3SmmLockedCallback,
    NULL,
    &Registration
    );
  
  //
  // Install SaInstallPolicyProtocol.
  // While installed, RC assumes the Policy is ready and finalized. So please
  // update and override any setting before calling this function.
  //
  Status = SaInstallPolicyProtocol (ImageHandle, SaPolicy);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  This function gets registered as a callback to Enable S3 SmmLocked before EndOfDxe

  @param[in] Event     - A pointer to the Event that triggered the callback.
  @param[in] Context   - A pointer to private data registered with the callback function.
**/
VOID
EFIAPI
S3SmmLockedCallback (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS          Status;
  VOID                *ProtocolPointer;
  SCRIPT_TABLE_PRIVATE_DATA  *S3TablePtr;

  DEBUG ((DEBUG_INFO, "S3SmmLockedCallback Start\n"));

  Status = gBS->LocateProtocol (&gEfiPciEnumerationCompleteProtocolGuid, NULL, (VOID **) &ProtocolPointer);
  if (EFI_SUCCESS != Status) {
    return;
  }
  
  gBS->CloseEvent (Event);

  S3TablePtr = (SCRIPT_TABLE_PRIVATE_DATA *)(UINTN)PcdGet64 (PcdS3BootScriptTablePrivateDataPtr);
  mS3BootScriptTablePtr = S3TablePtr;
  mS3BootScriptTablePtr->SmmLocked = TRUE;

  DEBUG ((DEBUG_INFO, "S3SmmLockedCallback End\n"));
  return;
}
