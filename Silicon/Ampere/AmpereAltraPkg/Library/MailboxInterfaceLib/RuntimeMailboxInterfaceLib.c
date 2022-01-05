/** @file
  The library implements the hardware Mailbox (Doorbell) interface for communication
  between the Application Processor (ARMv8) and the System Control Processors (SMpro/PMpro).

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>
#include <Library/AmpereCpuLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MailboxInterfaceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Platform/Ac01.h>

extern UINTN  gDoorbellBaseAddress[PLATFORM_CPU_MAX_SOCKET][NUMBER_OF_DOORBELLS_PER_SOCKET];

STATIC EFI_EVENT  mVirtualAddressChangeEvent                                                          = NULL;
STATIC BOOLEAN    mDoorbellRuntimeSetupTable[PLATFORM_CPU_MAX_SOCKET][NUMBER_OF_DOORBELLS_PER_SOCKET] = {
  { FALSE }
};

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
MailboxVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  SocketId;
  UINTN  DoorbellId;

  for (SocketId = 0; SocketId < PLATFORM_CPU_MAX_SOCKET; SocketId++) {
    for (DoorbellId = 0; DoorbellId < NUMBER_OF_DOORBELLS_PER_SOCKET; DoorbellId++) {
      if (mDoorbellRuntimeSetupTable[SocketId][DoorbellId]) {
        EfiConvertPointer (0x0, (VOID **)&gDoorbellBaseAddress[SocketId][DoorbellId]);
      }
    }
  }
}

/**
  Setup Doorbell for running at runtime.

  @param[in]  Socket            Active socket index.
  @param[in]  Doorbell          Doorbell channel.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.

**/
EFI_STATUS
EFIAPI
MailboxRuntimeSetup (
  IN UINT8              Socket,
  IN DOORBELL_CHANNELS  Doorbell
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;
  UINTN                            DoorbellAddress;

  if (  (Socket >= GetNumberOfActiveSockets ())
     || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (mVirtualAddressChangeEvent == NULL) {
    //
    // Register for the virtual address change event
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    MailboxVirtualAddressChangeEvent,
                    NULL,
                    &gEfiEventVirtualAddressChangeGuid,
                    &mVirtualAddressChangeEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  DoorbellAddress = MailboxGetDoorbellAddress (Socket, Doorbell);
  ASSERT (DoorbellAddress != 0);
  Status = gDS->GetMemorySpaceDescriptor (
                  DoorbellAddress & ~(SIZE_64KB - 1),
                  &Descriptor
                  );
  ASSERT_EFI_ERROR (Status);
  Status = gDS->SetMemorySpaceAttributes (
                  DoorbellAddress & ~(SIZE_64KB - 1),
                  SIZE_64KB,
                  Descriptor.Attributes | EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  mDoorbellRuntimeSetupTable[Socket][Doorbell] = TRUE;

  return EFI_SUCCESS;
}
