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

EFI_STATUS
NotifyPeiBmcElogCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gPeiIpmiTransportPpiGuid,
    NotifyPeiBmcElogCallback
  }
};

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
EFIAPI
EfiSetBmcElogData (
  IN  EFI_SM_ELOG_REDIR_PPI  *This,
  IN  UINT8                  *ElogData,
  IN  EFI_SM_ELOG_TYPE       DataType,
  IN  BOOLEAN                AlertEvent,
  IN  UINTN                  Size,
  OUT UINT64                 *RecordId
  )
{
  EFI_PEI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                      Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_PEI_ELOG_REDIR_THIS (This);

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
EFIAPI
EfiGetBmcElogData (
  IN EFI_SM_ELOG_REDIR_PPI  *This,
  IN OUT UINT8              *ElogData,
  IN EFI_SM_ELOG_TYPE       DataType,
  IN OUT UINTN              *Size,
  IN OUT UINT64             *RecordId
  )
{
  EFI_PEI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                      Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_PEI_ELOG_REDIR_THIS (This);

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
EFIAPI
EfiEraseBmcElogData (
  IN EFI_SM_ELOG_REDIR_PPI  *This,
  IN EFI_SM_ELOG_TYPE       DataType,
  IN OUT UINT64             *RecordId
  )
{
  EFI_PEI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                      Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_PEI_ELOG_REDIR_THIS (This);

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
EFIAPI
EfiActivateBmcElog (
  IN EFI_SM_ELOG_REDIR_PPI  *This,
  IN EFI_SM_ELOG_TYPE       DataType,
  IN BOOLEAN                *EnableElog,
  OUT BOOLEAN               *ElogStatus
  )
{
  EFI_PEI_BMC_ELOG_INSTANCE_DATA  *BmcElogPrivateData;
  EFI_STATUS                      Status;

  Status             = EFI_SUCCESS;
  BmcElogPrivateData = INSTANCE_FROM_EFI_PEI_ELOG_REDIR_THIS (This);

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
  EFI_PEI_BMC_ELOG_INSTANCE_DATA  *mRedirPeiProtoPrivate
  )
{
  EFI_STATUS  Status;
  BOOLEAN     EnableElog;
  BOOLEAN     ElogStatus;
  UINT16      Instance;

  Status     = EFI_SUCCESS;
  EnableElog = TRUE;
  ElogStatus = TRUE;
  Instance   = 0;

  mRedirPeiProtoPrivate->Signature                    = EFI_PEI_ELOG_REDIR_SIGNATURE;
  mRedirPeiProtoPrivate->DataType                     = EfiElogSmIPMI;
  mRedirPeiProtoPrivate->BmcElogPpi.ActivateEventLog  = EfiActivateBmcElog;
  mRedirPeiProtoPrivate->BmcElogPpi.EraseEventlogData = EfiEraseBmcElogData;
  mRedirPeiProtoPrivate->BmcElogPpi.GetEventLogData   = EfiGetBmcElogData;
  mRedirPeiProtoPrivate->BmcElogPpi.SetEventLogData   = EfiSetBmcElogData;

  mRedirPeiProtoPrivate->BmcElog.Guid  = &gPeiRedirElogPpiGuid;
  mRedirPeiProtoPrivate->BmcElog.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  mRedirPeiProtoPrivate->BmcElog.Ppi   = (VOID *)&mRedirPeiProtoPrivate->BmcElogPpi;
  mRedirPeiProtoPrivate->Instance      = Instance;
  //
  // Now install the Protocol
  //

  Status = PeiServicesInstallPpi (&mRedirPeiProtoPrivate->BmcElog);
  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  //
  // Activate the Event Log (This should depend upon Setup).
  //
  EfiActivateBmcElog (&mRedirPeiProtoPrivate->BmcElogPpi, EfiElogSmIPMI, &EnableElog, &ElogStatus);

  return EFI_SUCCESS;
}

/**
  InitializeBmcElogLayer.

  @param ImageHandle - ImageHandle of the loaded driver
  @param SystemTable - Pointer to the System Table

  @retval EFI_STATUS

**/
EFI_STATUS
InitializeBmcElogLayer (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  Status = PeiServicesNotifyPpi (&mNotifyList[0]);
  return Status;
}

/**
  NotifyPeiBmcElogCallback   This notification function is invoked when an instance of the
  IPMI Transport layer at PEI is produced.

  @param PeiServices      - Pointer to the PEI Services table
  @param NotifyDescriptor - Pointer to the NotifyPpi Descriptor
  @param Ppi              - Pointer to the PPI passed during Notify

  @retval EFI_STATUS

**/
EFI_STATUS
NotifyPeiBmcElogCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_BMC_ELOG_INSTANCE_DATA  *mRedirPeiProtoPrivate;

  Status = EFI_SUCCESS;

  mRedirPeiProtoPrivate = AllocateZeroPool (sizeof (EFI_PEI_BMC_ELOG_INSTANCE_DATA));

  if (mRedirPeiProtoPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "IPMI BMCELog Peim:EFI_OUT_OF_RESOURCES of memory allocation\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  SetElogRedirInstall (mRedirPeiProtoPrivate);

  CheckIfSelIsFull ();

  return Status;
}
