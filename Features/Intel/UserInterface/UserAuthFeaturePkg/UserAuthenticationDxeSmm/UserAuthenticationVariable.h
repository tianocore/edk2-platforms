/** @file
  Header file for definition of User Authentication Variable.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef USER_AUTHENTICATION_VARIABLE_H_
#define USER_AUTHENTICATION_VARIABLE_H_

#define PASSWORD_MAX_TRY_COUNT        3
#define PASSWORD_HISTORY_CHECK_COUNT  5

//
// Name of the variable
//
#define USER_AUTHENTICATION_VAR_NAME               L"Password"
#define USER_AUTHENTICATION_HISTORY_LAST_VAR_NAME  L"PasswordLast"

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
  );

#endif
