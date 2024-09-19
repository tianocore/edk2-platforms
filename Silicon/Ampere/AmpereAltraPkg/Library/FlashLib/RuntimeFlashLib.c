/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/MmCommunication2.h>

#include "FlashLibCommon.h"

STATIC EFI_MM_COMMUNICATION2_PROTOCOL  *mMmCommunicationProtocol = NULL;

/**
  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  event. It converts a pointer to a new virtual address.

  @param[in] Event        Event whose notification function is being invoked.
  @param[in] Context      Pointer to the notification function's context
**/
VOID
EFIAPI
FlashLibAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gRT->ConvertPointer (0x0, (VOID **)&gFlashLibVirtualBuffer);
  gRT->ConvertPointer (0x0, (VOID **)&mMmCommunicationProtocol);
}

/**
  Constructor function of the RuntimeFlashLib.

  @param[in] ImageHandle   The image handle.
  @param[in] SystemTable   The system table.

  @retval EFI_SUCCESS      Operation succeeded.
  @retval Others           An error has occurred.
**/
EFI_STATUS
EFIAPI
FlashLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT   VirtualAddressChangeEvent = NULL;
  EFI_STATUS  Status;

  gFlashLibPhysicalBuffer = AllocateRuntimeZeroPool (EFI_MM_MAX_TMP_BUF_SIZE);
  gFlashLibVirtualBuffer  = gFlashLibPhysicalBuffer;
  ASSERT (gFlashLibPhysicalBuffer != NULL);

  Status = gBS->LocateProtocol (
                  &gEfiMmCommunication2ProtocolGuid,
                  NULL,
                  (VOID **)&mMmCommunicationProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_CALLBACK,
                  FlashLibAddressChangeEvent,
                  NULL,
                  &VirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Provides an interface to access the Flash services via MM interface.

  @param[in]  Request             Pointer to the request buffer
  @param[in]  RequestDataSize     Size of the request buffer.
  @param[out] Response            Pointer to the response buffer
  @param[in]  ResponseDataSize    Size of the response buffer.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_INVALID_PARAMETER   An invalid data parameter or an invalid
                                  combination of data parameters.
  @retval Others                  An error has occurred.
**/
EFI_STATUS
FlashMmCommunicate (
  IN  VOID    *Request,
  IN  UINT32  RequestDataSize,
  OUT VOID    *Response,
  IN  UINT32  ResponseDataSize
  )
{
  EFI_MM_COMMUNICATE_REQUEST  CommBuffer;
  EFI_STATUS                  Status;

  if (  (Request == NULL) || (RequestDataSize == 0)
     || (RequestDataSize > EFI_MM_MAX_PAYLOAD_SIZE)
     || ((ResponseDataSize == 0) && (Response == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  CopyGuid (&CommBuffer.HeaderGuid, &gSpiNorMmGuid);
  CommBuffer.MessageLength = RequestDataSize;
  CopyMem (CommBuffer.Data, Request, RequestDataSize);

  if (mMmCommunicationProtocol == NULL) {
    return EFI_INVALID_PARAMETER;
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
