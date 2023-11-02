/** @file
  Source code to lock password variables.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/VariablePolicy.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/VariablePolicyHelperLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/UserAuthentication.h>

#include "UserAuthenticationVariable.h"

/**
  Lock password variables for security concern.

  @retval EFI_SUCCESS           Succeed to lock variable.
  @retval EFI_NOT_FOUND         Variable Lock protocol is not found.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.

**/
EFI_STATUS
LockPasswordVariable (
  VOID
  )
{
  EFI_STATUS                      Status;
  CHAR16                          PasswordHistoryName[sizeof (USER_AUTHENTICATION_VAR_NAME)/sizeof (CHAR16) + 5];
  UINTN                           Index;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (!EFI_ERROR (Status)) {
    Status = RegisterBasicVariablePolicy (
               VariablePolicy,
               &gUserAuthenticationGuid,
               USER_AUTHENTICATION_VAR_NAME,
               VARIABLE_POLICY_NO_MIN_SIZE,
               VARIABLE_POLICY_NO_MAX_SIZE,
               VARIABLE_POLICY_NO_MUST_ATTR,
               VARIABLE_POLICY_NO_CANT_ATTR,
               VARIABLE_POLICY_TYPE_LOCK_NOW
               );
    ASSERT_EFI_ERROR (Status);
    for (Index = 1; Index <= PASSWORD_HISTORY_CHECK_COUNT; Index++) {
      UnicodeSPrint (PasswordHistoryName, sizeof (PasswordHistoryName), L"%s%04x", USER_AUTHENTICATION_VAR_NAME, Index);
      Status = RegisterBasicVariablePolicy (
                 VariablePolicy,
                 &gUserAuthenticationGuid,
                 PasswordHistoryName,
                 VARIABLE_POLICY_NO_MIN_SIZE,
                 VARIABLE_POLICY_NO_MAX_SIZE,
                 VARIABLE_POLICY_NO_MUST_ATTR,
                 VARIABLE_POLICY_NO_CANT_ATTR,
                 VARIABLE_POLICY_TYPE_LOCK_NOW
                 );
      ASSERT_EFI_ERROR (Status);
    }

    Status = RegisterBasicVariablePolicy (
               VariablePolicy,
               &gUserAuthenticationGuid,
               USER_AUTHENTICATION_HISTORY_LAST_VAR_NAME,
               VARIABLE_POLICY_NO_MIN_SIZE,
               VARIABLE_POLICY_NO_MAX_SIZE,
               VARIABLE_POLICY_NO_MUST_ATTR,
               VARIABLE_POLICY_NO_CANT_ATTR,
               VARIABLE_POLICY_TYPE_LOCK_NOW
               );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
