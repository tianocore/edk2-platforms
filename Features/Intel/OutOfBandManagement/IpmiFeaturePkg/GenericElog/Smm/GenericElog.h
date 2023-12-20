/** @file
  Generic Event Log functions.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_GENELOG_H_
#define _SMM_GENELOG_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/SmmLib.h>
#include <Library/DebugLib.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "ServerManagement.h"
#include <Protocol/IpmiTransportProtocol.h>
#include <Protocol/GenericElog.h>

#define EFI_ELOG_PHYSICAL     0
#define EFI_ELOG_VIRTUAL      1
#define MAX_REDIR_DESCRIPTOR  10

///
/// A pointer to a function in IPF points to a plabel.
///
typedef struct {
  UINT64    EntryPoint;
  UINT64    GP;
} EFI_PLABEL;

typedef struct {
  EFI_PLABEL    *Function;
  EFI_PLABEL    Plabel;
} FUNCTION_PTR;

typedef struct {
  EFI_SM_ELOG_REDIR_PROTOCOL    *This;
  FUNCTION_PTR                  SetEventLogData;
  FUNCTION_PTR                  GetEventLogData;
  FUNCTION_PTR                  EraseEventLogData;
  FUNCTION_PTR                  ActivateEventLog;
} REDIR_MODULE_PROC;

typedef struct {
  BOOLEAN              Valid;
  REDIR_MODULE_PROC    Command[2];
} REDIR_MODULES;

typedef struct {
  REDIR_MODULES    Redir[MAX_REDIR_DESCRIPTOR];
  UINTN            MaxDescriptors;
} ELOG_MODULE_GLOBAL;

/**
  Efi Convert Function.

  @param Function

  @retval EFI_SUCCESS
**/
EFI_STATUS
EfiConvertFunction (
  IN  FUNCTION_PTR  *Function
  );

/**
  Set the function entry.

  @param FunctionPointer - The destination function pointer
  @param Function        - The source function pointer

  @retval EFI_SUCCESS - Set the function pointer successfully

**/
EFI_STATUS
EfiSetFunctionEntry (
  IN  FUNCTION_PTR  *FunctionPointer,
  IN  VOID          *Function
  );

/**
  Sm Redir Address Change Event.

  @param Event
  @param Context

**/
VOID
SmRedirAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Sends the Event-Log data to the destination.

  @param ElogData              - Pointer to the Event-Log data that needs to be recorded.
  @param DataType              - Type of Elog Data that is being recorded.
  @param AlertEvent            - This is an indication that the input data type is an Alert.
  @param DataSize              - Data Size.
  @param RecordId              - Record ID sent by the target.
  @param Global                - The module global variable pointer.
  @param Virtual               - If this function is called in virtual mode or physical mode

  @retval EFI_SUCCESS            - Event-Log was recorded successfully.
  @retval EFI_OUT_OF_RESOURCES   - Not enough resources to record data.
  @retval EFI_UNSUPPORTED        - The Data Type is unsupported.

**/
EFI_STATUS
EfiLibSetElogData (
  IN  UINT8             *ElogData,
  IN  EFI_SM_ELOG_TYPE  DataType,
  IN  BOOLEAN           AlertEvent,
  IN  UINTN             DataSize,
  OUT UINT64            *RecordId,
  ELOG_MODULE_GLOBAL    *Global,
  BOOLEAN               Virtual
  );

/**
  Gets the Event-Log data from the destination.

  @param ElogData              - Pointer to the Event-Log data buffer that will contain the data to be retrieved.
  @param DataType              - Type of Elog Data that is being recorded.
  @param DataSize              - Data Size.
  @param RecordId              - This is the RecordId of the next record. If ElogData is NULL,
                                this gives the RecordId of the first record available in the database with the correct DataSize.
                                A value of 0 on return indicates the last record if the EFI_STATUS indicates a success
  @param Global                - The module global variable pointer.
  @param Virtual               - If this function is called in virtual mode or physical mode

  @retval EFI_SUCCESS           - Event-Log was retrieved successfully.
  @retval EFI_NOT_FOUND         - Event-Log target not found.
  @retval EFI_BUFFER_TOO_SMALL  - Target buffer is too small to retrieve the data.
  @retval EFI_UNSUPPORTED       - The Data Type is unsupported

**/
EFI_STATUS
EfiLibGetElogData (
  IN UINT8             *ElogData,
  IN EFI_SM_ELOG_TYPE  DataType,
  IN OUT UINTN         *DataSize,
  IN OUT UINT64        *RecordId,
  ELOG_MODULE_GLOBAL   *Global,
  BOOLEAN              Virtual
  );

/**
  Erases the Event-Log data from the destination.

  @param DataType              - Type of Elog Data that is being Erased.
  @param RecordId              - This is the RecordId of the data to be erased. If RecordId is NULL, all
                                 the records on the database are erased if permitted by the target.
                                 Contains the deleted RecordId on return
  @param Global                - The module global variable pointer.
  @param Virtual               - If this function is called in virtual mode or physical mode

  @retval EFI_SUCCESS           - Event-Log was erased successfully
  @retval EFI_UNSUPPORTED       - The Data Type is unsupported
  @retval EFI_NOT_FOUND         - Event-Log target not found

**/
EFI_STATUS
EfiLibEraseElogData (
  IN EFI_SM_ELOG_TYPE  DataType,
  IN OUT UINT64        *RecordId,
  ELOG_MODULE_GLOBAL   *Global,
  BOOLEAN              Virtual
  );

/**
  This API enables/Disables Event Log.

  @param DataType              - Type of Elog Data that is being Activated.
  @param EnableElog            - Enables (TRUE) / Disables (FALSE) Event Log. If NULL just returns the
                                 Current ElogStatus.
  @param ElogStatus            - Current (New) Status of Event Log. Enabled (TRUE), Disabled (FALSE).
  @param Global                - The module global variable pointer.
  @param Virtual               - If this function is called in virtual mode or physical mode

  @retval EFI_SUCCESS           - Event-Log was recorded successfully
  @retval EFI_UNSUPPORTED       - The Data Type is unsupported

**/
EFI_STATUS
EfiLibActivateElog (
  IN  EFI_SM_ELOG_TYPE  DataType,
  IN BOOLEAN            *EnableElog,
  OUT BOOLEAN           *ElogStatus,
  ELOG_MODULE_GLOBAL    *Global,
  BOOLEAN               Virtual
  );

/**
  Initialize the generic ELog driver of server management.

  @retval EFI_SUCCESS - The driver initialized successfully

**/
EFI_STATUS
InitializeSmElogLayer (
  VOID
  );

#endif //_SMM_GENELOG_H_
