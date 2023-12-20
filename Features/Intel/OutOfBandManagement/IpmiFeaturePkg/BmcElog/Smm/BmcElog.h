/** @file
  BMC Event Log functions.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_BMCELOG_H_
#define _SMM_BMCELOG_H_

//
// Statements that include other files
//

#include <Library/MmServicesTableLib.h>
#include <Protocol/IpmiTransportProtocol.h>
#include <Protocol/GenericElog.h>
#include "BmcElogCommon.h"

//
// BMC Elog instance data
//
typedef struct {
  UINTN                         Signature;
  SM_COM_ADDRESS                ControllerAddress;
  SM_COM_ADDRESS                TargetAddress;
  UINT16                        Instance;
  EFI_SM_ELOG_TYPE              DataType;
  UINT8                         TempData[MAX_TEMP_DATA + 1];
  EFI_SM_ELOG_REDIR_PROTOCOL    BmcElog;
} EFI_BMC_ELOG_INSTANCE_DATA;

//
// BMC Elog Instance signature
//
#define SM_ELOG_REDIR_SIGNATURE  SIGNATURE_32 ('e', 'l', 'o', 'f')

#define INSTANCE_FROM_EFI_SM_ELOG_REDIR_THIS(a)  CR (a, EFI_BMC_ELOG_INSTANCE_DATA, BmcElog, SM_ELOG_REDIR_SIGNATURE)

/**
  InitializeSmBmcElogLayer.

  @retval EFI_STATUS

**/
EFI_STATUS
InitializeSmBmcElogLayer (
  VOID
  );

#endif //_SMM_BMCELOG_H_
