/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/MmCommunication2.h>

#include "SystemFirmwareUpdateLibCommon.h"

static EFI_MM_COMMUNICATION2_PROTOCOL  *mMmCommunicationProtocol = NULL;

/**
  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  event. It converts a pointer to a new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context
**/
VOID
EFIAPI
SystemFirmwareUpdateLibAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gRT->ConvertPointer (0x0, (VOID **)&mMmCommunicationProtocol);
}

/**
  Constructor function of the RuntimeNVParamLib.

  @param ImageHandle        The image handle.
  @param SystemTable        The system table.

  @retval  EFI_SUCCESS      Operation succeeded.
  @retval  Others           An error has occurred
**/
EFI_STATUS
EFIAPI
FirmwareUpdateConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT   VirtualAddressChangeEvent = NULL;
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEfiMmCommunication2ProtocolGuid,
                  NULL,
                  (VOID **)&mMmCommunicationProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can't locate gEfiMmCommunication2ProtocolGuid\n", __func__));
    return Status;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_CALLBACK,
                  SystemFirmwareUpdateLibAddressChangeEvent,
                  NULL,
                  &VirtualAddressChangeEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can't create event VirtualAddressChangeEvent\n", __func__));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Provides an interface to access the Firmware update services via MM interface.

  @param[in]  Request             Pointer to the request buffer
  @param[in]  RequestDataSize     Size of the request buffer.
  @param[out] Response            Pointer to the response buffer
  @param[in]  ResponseDataSize    Size of the response buffer.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_INVALID_PARAMETER   An invalid data parameter or an invalid
                                  combination of data parameters.
  @retval EFI_INVALID_PARAMETER   An invalid data parameter or an invalid
                                  combination of data parameters.
  @retval EFI_OUT_OF_RESOURCES    Internal resources are not available
  @retval Others                  An error has occurred.
**/
EFI_STATUS
FirmwareUpdateMmCommunicate (
  IN  VOID    *Request,
  IN  UINT32  RequestDataSize,
  OUT VOID    *Response,
  IN  UINT32  ResponseDataSize OPTIONAL
  )
{
  EFI_MM_COMMUNICATE_FWU_REQUEST  CommBuffer;
  EFI_STATUS                      Status;

  if (  (Request == NULL) || (RequestDataSize == 0)
     || (RequestDataSize > EFI_MM_MAX_PAYLOAD_SIZE)
     || ((ResponseDataSize != 0) && (Response == NULL))
     || (ResponseDataSize > EFI_MM_MAX_PAYLOAD_SIZE))
  {
    return EFI_INVALID_PARAMETER;
  }

  CopyGuid (&CommBuffer.HeaderGuid, &gFwUpdateMmGuid);
  CommBuffer.MessageLength = RequestDataSize;
  CopyMem (CommBuffer.Data, Request, RequestDataSize);

  if (mMmCommunicationProtocol == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = mMmCommunicationProtocol->Communicate (
                                       mMmCommunicationProtocol,
                                       &CommBuffer,
                                       &CommBuffer,
                                       NULL
                                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ResponseDataSize > 0) {
    CopyMem (Response, CommBuffer.Data, ResponseDataSize);
  }

  return EFI_SUCCESS;
}
