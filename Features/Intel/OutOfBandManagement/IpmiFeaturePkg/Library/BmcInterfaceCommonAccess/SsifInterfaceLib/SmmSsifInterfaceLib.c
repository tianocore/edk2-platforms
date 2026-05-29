/** @file SmmSsifInterfaceLib.c
  SSIF Transport SMM phase Implementation library functions.

  @copyright
  Copyright 2016 - 2021 Intel Corporation. <BR>
  Copyright (c) 1985 - 2023, American Megatrends International LLC. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BmcCommonInterfaceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SsifInterfaceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/IpmiTransport2Protocol.h>
#include <Protocol/SmbusHc.h>

/*++

Routine Description:
  Send IPMI command through SMBUS instance.

Arguments:
  Interface     - Pointer to System interface.
  SlaveAddress  - The SMBUS hardware address.
  Command       - This command is transmitted by the SMBus host controller to the SMBus slave device..
  Operation     - Operation to be performed.
  PecCheck      - Defines if Packet Error Code (PEC) checking is required for this operation.
  Length        - Signifies the number of bytes that this operation will do.
  Buffer        - Contains the value of data to execute to
                  the SMBus slave device. The length of this buffer is identified by Length.

Returns:
  EFI_NOT_FOUND - SMBUS instance is not found.
  Others        - Return status of the SMBUS Execute operation.
--*/
EFI_STATUS
IpmiSmbusSendCommand (
  IN     IPMI_SYSTEM_INTERFACE     *Interface,
  IN     EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN     EFI_SMBUS_DEVICE_COMMAND  Command,
  IN     EFI_SMBUS_OPERATION       Operation,
  IN     BOOLEAN                   PecCheck,
  IN OUT UINTN                     *Length,
  IN OUT VOID                      *Buffer
  )
{
  EFI_STATUS             Status;
  EFI_SMBUS_HC_PROTOCOL  *EfiSmbusHcProtocol;

  Status             = EFI_NOT_FOUND;
  EfiSmbusHcProtocol = (EFI_SMBUS_HC_PROTOCOL *)Interface->Ssif.SsifInterfaceApiPtr;

  if (EfiSmbusHcProtocol != NULL) {
    Status = EfiSmbusHcProtocol->Execute (
                                   EfiSmbusHcProtocol,
                                   SlaveAddress,
                                   Command,
                                   Operation,
                                   PecCheck,
                                   Length,
                                   Buffer
                                   );
  }

  DEBUG ((DEBUG_INFO, "%a EfiSmbusHcProtocol->Execute Status = %r\n", __func__, Status));

  return Status;
}

/*++

Routine Description:
  Locate Smbus instance and initialize interface pointer.

Arguments:
  IpmiTransport2     - Pointer to IPMI transport2 instance.

Returns:
  Status - Status returned while locating SMBUS instance.
--*/
EFI_STATUS
IpmiGetSmbusApiPtr (
  IN OUT IPMI_TRANSPORT2  *IpmiTransport2
  )
{
  EFI_STATUS             Status;
  EFI_SMBUS_HC_PROTOCOL  *EfiSmbusHcProtocol;
  UINTN                  HandleCount;
  EFI_HANDLE             *HandleBuffer;
  UINTN                  Index;
  BMC_INTERFACE_STATUS   BmcStatus;

  IpmiTransport2->Interface.Ssif.SsifInterfaceApiGuid = gEfiSmbusHcProtocolGuid;

  HandleBuffer = NULL;
  HandleCount  = 0;

  Status = gMmst->MmLocateHandle (
                    ByProtocol,
                    &gEfiSmbusHcProtocolGuid,
                    NULL,
                    &HandleCount,
                    HandleBuffer
                    );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_BUFFER_TOO_SMALL) {
      // Allocate memory for Handle buffer
      HandleBuffer = AllocateZeroPool (HandleCount);
    }

    if (HandleBuffer == NULL) {
      return EFI_NOT_FOUND;
    }

    Status = gMmst->MmLocateHandle (
                      ByProtocol,
                      &gEfiSmbusHcProtocolGuid,
                      NULL,
                      &HandleCount,
                      HandleBuffer
                      );
    if (EFI_ERROR (Status)) {
      // Free HandleBuffer memory
      FreePool (HandleBuffer);
      return EFI_NOT_FOUND;
    }
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gMmst->MmHandleProtocol (
                      HandleBuffer[Index],
                      &gEfiSmbusHcProtocolGuid,
                      (VOID **)&EfiSmbusHcProtocol
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    IpmiTransport2->Interface.Ssif.InterfaceState      = IpmiInterfaceInitialized;
    IpmiTransport2->Interface.Ssif.SsifInterfaceApiPtr = (UINTN)EfiSmbusHcProtocol;

    Status = CheckSelfTestByInterfaceType (
               IpmiTransport2,
               &BmcStatus,
               SysInterfaceSsif
               );
    if (EFI_ERROR (Status) || (BmcStatus == BmcStatusHardFail)) {
      IpmiTransport2->Interface.Ssif.InterfaceState = IpmiInterfaceInitError;
      continue;
    }

    GetSystemInterfaceCapability (IpmiTransport2);
    GetGlobalEnables (IpmiTransport2);
    break;
  }

  FreePool (HandleBuffer);
  return Status;
}
