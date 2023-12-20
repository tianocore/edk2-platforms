/** @file
  Lightweight lib to support EFI Server Management drivers.
  This source file provides Event Log support.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GenericElog.h>

#include <Library/ServerMgmtRtLib.h>

//
// Module Globals
//
EFI_SM_ELOG_PROTOCOL  *mGenericElogProtocol     = NULL;
VOID                  *mGenericElogRegistration = NULL;

/**
  This function is called whenever an instance of ELOG protocol is created.  When the function is notified
  it initializes the module global data.

  @param Event                  This is used only for EFI compatibility.
  @param Context                This is used only for EFI compatibility.

**/
VOID
EFIAPI
GenericElogNotificationFunction (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiGenericElogProtocolGuid, NULL, (VOID **)&mGenericElogProtocol);
  if (EFI_ERROR (Status)) {
    mGenericElogProtocol = NULL;
  }
}

/**
  The function will set up a notification on the ELOG protocol.  This function is required to be called prior
  to utilizing the ELOG protocol from within this library.

  @retval EFI_SUCCESS          after the notification has been setup.
**/
EFI_STATUS
EfiInitializeGenericElog (
  VOID
  )
{
  EFI_EVENT   Event;
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, GenericElogNotificationFunction, NULL, &Event);
  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (&gEfiGenericElogProtocolGuid, Event, &mGenericElogRegistration);
  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  gBS->SignalEvent (Event);

  return EFI_SUCCESS;
}

/**
  This function sends event log data to the destination such as LAN, ICMB, BMC etc.

  @param ElogData     is a pointer to the event log data that needs to be recorded
  @param DataType     type of Elog data that is being recorded.  The Elog is redirected based on this parameter.
  @param AlertEvent   is an indication that the input data type is an alert.  The underlying
                      drivers need to decide if they need to listen to the DataType and send it on
                      an appropriate channel as an alert use of the information.
  @param DataSize     is the size of the data to be logged
  @param RecordId     is the array of record IDs sent by the target.  This can be used to retrieve the
                      records or erase the records.
  @retval EFI_SUCCESS - if the data was logged.
  @retval EFI_INVALID_PARAMETER - if the DataType is >= EfiSmElogMax
  @retval EFI_NOT_FOUND - the event log target was not found
  @retval EFI_PROTOCOL_ERROR - there was a data formatting error

**/
EFI_STATUS
EfiSmSetEventLogData (
  IN  UINT8             *ElogData,
  IN  EFI_SM_ELOG_TYPE  DataType,
  IN  BOOLEAN           AlertEvent,
  IN  UINTN             DataSize,
  OUT UINT64            *RecordId
  )
{
  //
  // If the protocol is not found return EFI_NOT_FOUND
  //
  if (mGenericElogProtocol == NULL) {
    return EFI_NOT_FOUND;
  }

  return mGenericElogProtocol->SetEventLogData (
                                 mGenericElogProtocol,
                                 ElogData,
                                 DataType,
                                 AlertEvent,
                                 DataSize,
                                 RecordId
                                 );
}

/**
  This function gets event log data from the destination dependent on the DataType.  The destination
  can be a remote target such as LAN, ICMB, IPMI, or a FV.  The ELOG redir driver will resolve the
  destination.

  @param ElogData - pointer to the event log data buffer to contain the data to be retrieved.
  @param DataType - this is the type of Elog data to be gotten.  Elog is redirected based upon this
                    information.
  @param DataSize - this is the size of the data to be retrieved.
  @param RecordId - the RecordId of the next record.  If ElogData is NULL, this gives the RecordId of the first
                    record available in the database with the correct DataSize.  A value of 0 on return indicates
                    that it was last record if the Status is EFI_SUCCESS.

  @retval EFI_SUCCESS - if the event log was retrieved successfully.
  @retval EFI_NOT_FOUND - if the event log target was not found.
  @retval EFI_NO_RESPONSE - if the event log target is not responding.  This is done by the redir driver.
  @retval EFI_INVALID_PARAMETER - DataType or another parameter was invalid.
  @retval EFI_BUFFER_TOO_SMALL -the ElogData buffer is too small to be filled with the requested data.

**/
EFI_STATUS
EfiSmGetEventLogData (
  IN  UINT8             *ElogData,
  IN  EFI_SM_ELOG_TYPE  DataType,
  IN  OUT UINTN         *DataSize,
  IN OUT UINT64         *RecordId
  )
{
  //
  // If the protocol is not found return EFI_NOT_FOUND
  //
  if (mGenericElogProtocol == NULL) {
    return EFI_NOT_FOUND;
  }

  return mGenericElogProtocol->GetEventLogData (
                                 mGenericElogProtocol,
                                 ElogData,
                                 DataType,
                                 DataSize,
                                 RecordId
                                 );
}

/**
  This function erases the event log data defined by the DataType.  The redir driver associated with
  the DataType resolves the path to the record.

  @param DataType - the type of Elog data that is to be erased.
  @param RecordId - the RecordId of the data to be erased.  If RecordId is NULL, all records in the
                    database are erased if permitted by the target.  RecordId will contain the deleted
                    RecordId on return.

  @retval EFI_SUCCESS - the record or collection of records were erased.
  @retval EFI_NOT_FOUND - the event log target was not found.
  @retval EFI_NO_RESPONSE - the event log target was found but did not respond.
  @retval EFI_INVALID_PARAMETER - one of the parameters was invalid.

**/
EFI_STATUS
EfiSmEraseEventlogData (
  IN EFI_SM_ELOG_TYPE  DataType,
  IN OUT UINT64        *RecordId
  )
{
  //
  // If the protocol is not found return EFI_NOT_FOUND
  //
  if (mGenericElogProtocol == NULL) {
    return EFI_NOT_FOUND;
  }

  return mGenericElogProtocol->EraseEventlogData (
                                 mGenericElogProtocol,
                                 DataType,
                                 RecordId
                                 );
}

/**
  This function enables or disables the event log defined by the DataType.


  @param DataType   - the type of Elog data that is being activated.
  @param EnableElog - enables or disables the event log defined by the DataType.  If it is NULL
                      it returns the current status of the DataType log.
  @param ElogStatus - is the current status of the Event log defined by the DataType.  Enabled is
                      TRUE and Disabled is FALSE.

  @retval EFI_SUCCESS - if the event log was successfully enabled or disabled.
  @retval EFI_NOT_FOUND - the event log target was not found.
  @retval EFI_NO_RESPONSE - the event log target was found but did not respond.
  @retval EFI_INVALID_PARAMETER - one of the parameters was invalid.

**/
EFI_STATUS
EfiSmActivateEventLog (
  IN EFI_SM_ELOG_TYPE  DataType,
  IN BOOLEAN           *EnableElog,
  OUT BOOLEAN          *ElogStatus
  )
{
  //
  // If the protocol is not found return EFI_NOT_FOUND
  //
  if (mGenericElogProtocol == NULL) {
    return EFI_NOT_FOUND;
  }

  return mGenericElogProtocol->ActivateEventLog (
                                 mGenericElogProtocol,
                                 DataType,
                                 EnableElog,
                                 ElogStatus
                                 );
}
