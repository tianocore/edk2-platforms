/** @file
  Traditional MM Variable Write Lib

  This library provides phase agnostic access to the UEFI Variable Services.
  This is done by implementing a wrapper on top of the phase specific mechanism
  for reading from UEFI variables.

  This is the traditional SMM specific LibraryClass constructor.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/SmmVariable.h>

#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>

extern EFI_SMM_VARIABLE_PROTOCOL  *mVariableWriteLibSmmVariable;
extern BOOLEAN                    mEfiAtRuntime;

/**
  Callback for ExitBootService, which is registered at the constructor.
  This callback sets a global variable mEfiAtRuntime to indicate whether
  it is after ExitBootService.

  @param[in] Protocol        Protocol unique ID.
  @param[in] Interface       Interface instance.
  @param[in] Handle          The handle on which the interface is installed.
**/
EFI_STATUS
EFIAPI
VarLibExitBootServicesCallback (
  IN      CONST EFI_GUID   *Protocol,
  IN      VOID             *Interface,
  IN      EFI_HANDLE        Handle
  )
{
  mEfiAtRuntime = TRUE;
  return EFI_SUCCESS;
}

/**
  The constructor function acquires the EFI SMM Variable Services

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The constructor always returns RETURN_SUCCESS.
  @retval EFI_NOT_FOUND   gEfiSmmVariableProtocolGuid Protocol interface not
                          found, which technically should not be possible since
                          this protocol is in the LibraryClass DEPEX

**/
EFI_STATUS
EFIAPI
TraditionalMmVariableWriteLibConstructor (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;
  VOID          *Registration = NULL;

  //
  // Locate SmmVariableProtocol.
  //
  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **) &mVariableWriteLibSmmVariable);
  ASSERT_EFI_ERROR (Status);

  //
  // Register VarLibExitBootServicesCallback for gEdkiiSmmExitBootServicesProtocolGuid.
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEdkiiSmmExitBootServicesProtocolGuid,
                    VarLibExitBootServicesCallback,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
