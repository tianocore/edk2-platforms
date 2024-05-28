/** @file
  SMM core hook for AMD SPI Host Controller State

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/AmdSpiSmmHcState.h>

#include <Library/AmdSmmCorePlatformHookLib.h>
#include "SmmCoreAmdSpiHcHookLib.h"

VOID  *mSmmCorePlatformHookHcStateRegistration = NULL;

SMM_CORE_HOOK_AMD_SPI_HC_STATE_CONTEXT  mSmmCorePlatformHookContext;

/**
  Performs platform specific tasks before invoking registered SMI handlers.

  This function performs platform specific tasks before invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
SmmCoreSpiHcHookBeforeSmmDispatch (
  VOID
  )
{
  EFI_STATUS                     Status;
  SMM_EFI_SPI_HC_STATE_PROTOCOL  *SpiHcState;

  Status     = EFI_SUCCESS;
  SpiHcState = mSmmCorePlatformHookContext.SmmSpiHcStateInterface;
  if (SpiHcState != NULL) {
    Status = SpiHcState->SaveState (SpiHcState);
    // Open up SPI HC for SMM, Restore state will automatically return back to
    // state on SMM entry
    Status = SpiHcState->Unlock (SpiHcState);
    Status = SpiHcState->UnblockAllOpcodes (SpiHcState);
  }

  return Status;
}

/**
  Performs platform specific tasks after invoking registered SMI handlers.

  This function performs platform specific tasks after invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
SmmCoreSpiHcHookAfterSmmDispatch (
  VOID
  )
{
  EFI_STATUS                     Status;
  SMM_EFI_SPI_HC_STATE_PROTOCOL  *SpiHcState;

  Status     = EFI_SUCCESS;
  SpiHcState = mSmmCorePlatformHookContext.SmmSpiHcStateInterface;
  if (SpiHcState != NULL) {
    Status = SpiHcState->RestoreState (SpiHcState);
  }

  return Status;
}

/**
  Notification for SMM ReadyToLock protocol.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification runs successfully.
**/
EFI_STATUS
EFIAPI
SmmCorePlatformHookHcStateNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  mSmmCorePlatformHookContext.SmmSpiHcStateInterface = Interface;
  mSmmCorePlatformHookContext.SmmSpiHcStateHandle    = Handle;
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
SmmCoreAmdSpiHcHookConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  SetMem (
    &mSmmCorePlatformHookContext,
    sizeof (mSmmCorePlatformHookContext),
    0
    );
  //
  // Register gAmdSpiHcStateProtocolGuid notification.
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gAmdSpiHcStateProtocolGuid,
                    SmmCorePlatformHookHcStateNotify,
                    &mSmmCorePlatformHookHcStateRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register AMD SMM Dispatcher hook instance.
  //
  Status = RegisterSmmDispatcherHook (
             SmmCoreSpiHcHookBeforeSmmDispatch,
             SmmCoreSpiHcHookAfterSmmDispatch,
             0
             );
  return Status;
}
