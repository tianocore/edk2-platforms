/** @file
  AMD SMM core hook library

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmCorePlatformHookLib.h>
#include <Library/AmdSmmCorePlatformHookLib.h>

LIST_ENTRY  *RegisteredHookPtr = NULL;
LIST_ENTRY  RegisteredHook;

/**
  Register a SMM dispatcher hook.

  @param[in] SmmDispatchHookBefore  Function hook to SMM Dispatch before.
  @param[in] SmmDispatchHookAfter   Function hook to SMM Dispatch after.
  @param[in] Priority               The priority to execute the hook.

  @retval EFI_SUCCESS       The hook is registered successfully.

**/
EFI_STATUS
RegisterSmmDispatcherHook (
  IN  AMD_SMM_DISPATCH_HOOK_BEFORE    SmmDispatchHookBefore OPTIONAL,
  IN  AMD_SMM_DISPATCH_HOOK_AFTER     SmmDispatchHookAfter OPTIONAL,
  IN  AMD_SMM_DISPATCH_HOOK_PRIORITY  Priority
  )
{
  AMD_SMM_DISPATCH_HOOK  *ThisHook;

  if ((SmmDispatchHookBefore == NULL) && (SmmDispatchHookAfter == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Both SmmDispatchHookBefore and SmmDispatchHookAfter are NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (RegisteredHookPtr == NULL) {
    InitializeListHead (&RegisteredHook);
    RegisteredHookPtr = &RegisteredHook;
  }

  ThisHook = (AMD_SMM_DISPATCH_HOOK *)AllocateZeroPool (sizeof (AMD_SMM_DISPATCH_HOOK));
  if (ThisHook == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Insufficient memory for AMD_SMM_DISPATCH_HOOK.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&ThisHook->NextList);
  ThisHook->AmdSmmDispatchHookBefore = SmmDispatchHookBefore;
  ThisHook->AmdSmmDispatchHookAfter  = SmmDispatchHookAfter;
  ThisHook->Priority                 = Priority;
  InsertHeadList (&RegisteredHook, &ThisHook->NextList);
  DEBUG ((DEBUG_VERBOSE, "%a: New AMD_SMM_DISPATCH_HOOK is inserted.\n", __func__));
  return EFI_SUCCESS;
}

/**
  Performs platform specific tasks before invoking registered SMI handlers.

  This function performs platform specific tasks before invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookBeforeSmmDispatch (
  VOID
  )
{
  AMD_SMM_DISPATCH_HOOK  *ThisHook;

  if (IsListEmpty (&RegisteredHook)) {
    return EFI_NOT_FOUND;
  }

  ThisHook = (AMD_SMM_DISPATCH_HOOK *)GetFirstNode (&RegisteredHook);
  while (TRUE) {
    //
    // We can handle priority in the future when needed.
    //
    if (ThisHook->AmdSmmDispatchHookBefore != NULL) {
      ThisHook->AmdSmmDispatchHookBefore ();
    }

    if (IsNodeAtEnd (&RegisteredHook, &ThisHook->NextList)) {
      break;
    }

    ThisHook = (AMD_SMM_DISPATCH_HOOK *)GetNextNode (&RegisteredHook, &ThisHook->NextList);
  }

  return EFI_SUCCESS;
}

/**
  Performs platform specific tasks after invoking registered SMI handlers.

  This function performs platform specific tasks after invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookAfterSmmDispatch (
  VOID
  )
{
  AMD_SMM_DISPATCH_HOOK  *ThisHook;

  if (IsListEmpty (&RegisteredHook)) {
    return EFI_NOT_FOUND;
  }

  ThisHook = (AMD_SMM_DISPATCH_HOOK *)GetFirstNode (&RegisteredHook);
  while (TRUE) {
    //
    // We can handle priority in the future when needed.
    //
    if (ThisHook->AmdSmmDispatchHookAfter != NULL) {
      ThisHook->AmdSmmDispatchHookAfter ();
    }

    if (IsNodeAtEnd (&RegisteredHook, &ThisHook->NextList)) {
      break;
    }

    ThisHook = (AMD_SMM_DISPATCH_HOOK *)GetNextNode (&RegisteredHook, &ThisHook->NextList);
  }

  return EFI_SUCCESS;
}

/**
  Constructor for SmmLockBox library.
  This is used to set SmmLockBox context, which will be used in PEI phase in S3 boot path later.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCCESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
SmmCorePlatformHookConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (RegisteredHookPtr == NULL) {
    InitializeListHead (&RegisteredHook);
    RegisteredHookPtr = &RegisteredHook;
  }

  return EFI_SUCCESS;
}
