/** @file
  BMC Event Log functions.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BmcElog.h"

//
// Define module globals used to register for notification of when
// the ELOG REDIR protocol has been produced.
//
EFI_EVENT                   mEfiBmcTransEvent;
EFI_BMC_ELOG_INSTANCE_DATA  *mRedirProtoPrivate;

/**
  WaitTillErased.

  @param BmcElogPrivateData  - Bmc event log instance
  @param ResvId              - Reserved ID

  @retval EFI_SUCCESS
  @retval EFI_NO_RESPONSE

**/
EFI_STATUS
WaitTillErased (
  EFI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData,
  UINT8                       *ResvId
  )
{
  EFI_STATUS  Status;

  Status = WaitTillClearSel (ResvId);
  return Status;
}

/**
  Efi Set Bmc Elog Data.

  @param This        - Protocol pointer
  @param ElogData    - Buffer for log storage
  @param DataType    - Event Log type
  @param AlertEvent  - If it is an alert event
  @param Size        - Log data size
  @param RecordId    - Indicate which recorder it is

  @retval EFI_STATUS

**/
EFI_STATUS
EfiSetBmcElogData (
  IN  EFI_SM_ELOG_REDIR_PROTOCOL  *This,
  IN  UINT8                       *ElogData,
  IN  EFI_SM_ELOG_TYPE            DataType,
  IN  BOOLEAN                     AlertEvent,
  IN  UINTN                       Size,
  OUT UINT64                      *RecordId
  )
{
  EFI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                  Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_SM_ELOG_REDIR_THIS (This);

  if (DataType >= EfiSmElogMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (Size > SEL_RECORD_SIZE) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (BmcElogPrivateData->DataType == DataType) {
    Status = SetBmcElogRecord (ElogData, DataType, AlertEvent, Size, RecordId);
  }

  return Status;
}

/**
  Efi Get Bmc Elog Data.

  @param This        - Protocol pointer
  @param ElogData    - Buffer for log data store
  @param DataType    - Event log type
  @param Size        - Size of log data
  @param RecordId    - indicate which recorder it is

  @retval EFI_STATUS

**/
EFI_STATUS
EfiGetBmcElogData (
  IN EFI_SM_ELOG_REDIR_PROTOCOL  *This,
  IN OUT UINT8                   *ElogData,
  IN EFI_SM_ELOG_TYPE            DataType,
  IN OUT UINTN                   *Size,
  IN OUT UINT64                  *RecordId
  )
{
  EFI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                  Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_SM_ELOG_REDIR_THIS (This);

  if (DataType >= EfiSmElogMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmcElogPrivateData->DataType == DataType) {
    Status = GetBmcElogRecord (ElogData, DataType, Size, RecordId);
  }

  return Status;
}

/**
  Efi Erase Bmc Elog Data.

  @param This        - Protocol pointer
  @param DataType    - Event log type
  @param RecordId    - return which recorder it is

  @retval EFI_STATUS

**/
EFI_STATUS
EfiEraseBmcElogData (
  IN EFI_SM_ELOG_REDIR_PROTOCOL  *This,
  IN EFI_SM_ELOG_TYPE            DataType,
  IN OUT UINT64                  *RecordId
  )
{
  EFI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                  Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_SM_ELOG_REDIR_THIS (This);

  if (DataType >= EfiSmElogMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmcElogPrivateData->DataType == DataType) {
    Status = EraseBmcElogRecord (DataType, RecordId);

    if (Status == EFI_SUCCESS) {
      if (RecordId != NULL) {
        *RecordId = (UINT16)(*((UINT16 *)&BmcElogPrivateData->TempData[0]));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Efi Activate Bmc Elog.

  @param This        - Protocol pointer
  @param DataType    - indicate event log type
  @param EnableElog  - Enable/Disable event log
  @param ElogStatus  - return log status

  @retval EFI_STATUS

**/
EFI_STATUS
EfiActivateBmcElog (
  IN EFI_SM_ELOG_REDIR_PROTOCOL  *This,
  IN EFI_SM_ELOG_TYPE            DataType,
  IN BOOLEAN                     *EnableElog,
  OUT BOOLEAN                    *ElogStatus
  )
{
  EFI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                  Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_SM_ELOG_REDIR_THIS (This);

  if (DataType >= EfiSmElogMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmcElogPrivateData->DataType == DataType) {
    Status = ActivateBmcElog (DataType, EnableElog, ElogStatus);
  }

  return Status;
}

/**
  Set Elog Redir Install.

  @retval EFI_SUCCESS

**/
EFI_STATUS
SetElogRedirInstall (
  VOID
  )
{
  EFI_HANDLE  NewHandle;
  EFI_STATUS  Status;
  BOOLEAN     EnableElog;
  BOOLEAN     ElogStatus;
  UINT16      Instance;

  Status     = EFI_SUCCESS;
  EnableElog = TRUE;
  ElogStatus = TRUE;
  Instance   = 0;

  mRedirProtoPrivate->Signature                 = SM_ELOG_REDIR_SIGNATURE;
  mRedirProtoPrivate->DataType                  = EfiElogSmIPMI;
  mRedirProtoPrivate->BmcElog.ActivateEventLog  = (EFI_ACTIVATE_ELOG)EfiActivateBmcElog;
  mRedirProtoPrivate->BmcElog.EraseEventlogData = (EFI_ERASE_ELOG_DATA)EfiEraseBmcElogData;
  mRedirProtoPrivate->BmcElog.GetEventLogData   = (EFI_GET_ELOG_DATA)EfiGetBmcElogData;
  mRedirProtoPrivate->BmcElog.SetEventLogData   = (EFI_SET_ELOG_DATA)EfiSetBmcElogData;
  mRedirProtoPrivate->Instance                  = Instance;
  //
  // Now install the Protocol
  //
  NewHandle = NULL;
  Status    = gMmst->MmInstallProtocolInterface (
                       &NewHandle,
                       &gSmmRedirElogProtocolGuid,
                       EFI_NATIVE_INTERFACE,
                       &mRedirProtoPrivate->BmcElog
                       );

  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  //
  // Activate the Event Log (This should depend upon Setup).
  //
  EfiActivateBmcElog (&mRedirProtoPrivate->BmcElog, EfiElogSmIPMI, &EnableElog, &ElogStatus);

  return EFI_SUCCESS;
}

/**
  InitializeSmBmcElogLayer.

  @retval EFI_STATUS

**/
EFI_STATUS
InitializeSmBmcElogLayer (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  mRedirProtoPrivate = AllocatePool (sizeof (EFI_BMC_ELOG_INSTANCE_DATA));
  ASSERT (mRedirProtoPrivate != NULL);
  if (mRedirProtoPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetElogRedirInstall ();
  //
  // Check if the BMC System Event Log (SEL) is full and whether to report the error.
  //

  CheckIfSelIsFull ();

  return Status;
}
