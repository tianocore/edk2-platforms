/** @file
  This driver will register two callbacks to call fsp's notifies.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Library/HobLib.h>
#include <FspStatusCode.h>
#include "../Include/FspGlobalData.h"

extern EFI_GUID  gFspHobGuid;
extern EFI_GUID  gEfiResetArchProtocolGuid;
extern EFI_GUID  gAmdFspSetupTableInitDoneGuid;

EFI_EVENT  gAmdFspSetupTableInitDoneEvent;
EFI_EVENT  gResetDoneEvent;

/**
  Relocate this image under 4G memory.

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_SUCCESS  Image successfully relocated.
  @retval EFI_ABORTED  Failed to relocate image.

**/
EFI_STATUS
RelocateImageUnder4GIfNeeded (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
FspWrapperMultiPhaseHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  );

STATIC
VOID *
GetFspHobList (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gFspHobGuid);
  if (GuidHob != NULL) {
    return *(VOID **)GET_GUID_HOB_DATA (GuidHob);
  } else {
    return NULL;
  }
}

/**
  Callback function after runtime reset being ready immediately.

  @param[in] Event      Not used.
  @param[in] Context    Not used.

**/
VOID
EFIAPI
DoResetAfterRtImmediately (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  gBS->CloseEvent (Event);
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
}

/**
  Callback function after FSP finished applying setup table in DXE phase.

  The platform is considered to stop ANY critical services, and then do COLD RESET.

  @param[in] Event      Not used.
  @param[in] Context    Not used.

**/
VOID
EFIAPI
CheckAndRebootSystemAfterFspSetupTable (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  gBS->CloseEvent (Event);
  VOID  *Registration;

  DEBUG ((DEBUG_INFO, "FSP Setup table Done!\n"));
  DEBUG ((DEBUG_INFO, "Reset?%s\n", PcdGetBool (PcdAmdFspSetupTableInitNeedsReset) ? L"TRUE" : L"FALSE"));
  if (!PcdGetBool (PcdAmdFspSetupTableInitNeedsReset)) {
    return;
  }

  // DO RESET HERE!
  Status = gBS->LocateProtocol (&gEfiResetArchProtocolGuid, NULL, (VOID **)&Registration);
  if ( !EFI_ERROR (Status)) {
    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
    // Will not return here.
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DoResetAfterRtImmediately,
                  NULL,
                  &gResetDoneEvent
                  );
  if (!EFI_ERROR (Status)) {
    Registration = NULL;
    Status       = gBS->RegisterProtocolNotify (
                          &gEfiResetArchProtocolGuid,
                          gResetDoneEvent,
                          &Registration
                          );
  }
}

/**
  Main entry for the FSP DXE module.

  This routine registers two callbacks to call fsp's notifies.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FspsMultiPhaseSiInitDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  if (!PcdGet8 (PcdFspModeSelection)) {
    // Dispatch Mode
    DEBUG ((DEBUG_INFO, "Waiting for FSP Setup table...\n"));
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    CheckAndRebootSystemAfterFspSetupTable,
                    NULL,
                    &gAmdFspSetupTableInitDoneEvent
                    );
    if (!EFI_ERROR (Status)) {
      Registration = NULL;
      Status       = gBS->RegisterProtocolNotify (
                            &gAmdFspSetupTableInitDoneGuid,
                            gAmdFspSetupTableInitDoneEvent,
                            &Registration
                            );
    }

    return EFI_SUCCESS;
  }

  //
  // Load this driver's image to memory
  //
  Status = RelocateImageUnder4GIfNeeded (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot relocate into 4G- Mem!\n"));
    return EFI_UNSUPPORTED;
  }

  VOID  *FspHobList = (VOID *)((UINTN)GetFspHobList ()&0xFFFFFFFF);

  return FspWrapperMultiPhaseHandler (&FspHobList, FspMultiPhaseSiInitApiIndex);
}

VOID
EFIAPI
CallFspWrapperResetSystem (
  IN EFI_STATUS  FspStatusResetType
  )
{
  //
  // Perform reset according to the type.
  //
}
