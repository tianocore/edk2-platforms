/** @file
  Silicon post-mem policy PEIM.

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/SiliconPolicyInitLib.h>
#include <Library/SiliconPolicyUpdateLib.h>
#include <Library/SmmControlLib.h>


/**
  Silicon Policy Init after memory PEI module entry point

  @param[in]  FileHandle           Not used.
  @param[in]  PeiServices          General purpose services available to every PEIM.

  @retval     EFI_SUCCESS          The function completes successfully
**/
EFI_STATUS
EFIAPI
SiliconPolicyPeiPostMemEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  VOID  *Policy;
  EFI_STATUS Status;

  Policy = SiliconPolicyInitPostMem (NULL);
  SiliconPolicyUpdatePostMem (Policy);
  SiliconPolicyDonePostMem (Policy);
  Status = PeiInstallSmmControlPpi ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}