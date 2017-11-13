/** @file

  Copyright (c) 2015-2016, Linaro Limited. All rights reserved.
  Copyright (c) 2017, Jeremy Linton

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _DWUSBHOSTDXE_H_
#define _DWUSBHOSTDXE_H_

#include <Uefi.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Usb.h>

#include <Protocol/PciIo.h>
#include <Protocol/Usb2HostController.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include "DwcHw.h"

#define MAX_DEVICE                      16
#define MAX_ENDPOINT                    16

typedef struct _DWUSB_OTGHC_DEV DWUSB_OTGHC_DEV;

#define DWUSB_OTGHC_DEV_SIGNATURE	SIGNATURE_32 ('d', 'w', 'h', 'c')
#define DWHC_FROM_THIS(a)		CR(a, DWUSB_OTGHC_DEV, DwUsbOtgHc, DWUSB_OTGHC_DEV_SIGNATURE)

extern EFI_DRIVER_BINDING_PROTOCOL   gDwUsbDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gDwUsbComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gDwUsbComponentName2;

//
// The RequestType in EFI_USB_DEVICE_REQUEST is composed of
// three fields: One bit direction, 2 bit type, and 5 bit
// target.
//
#define USB_REQUEST_TYPE(Dir, Type, Target) \
          ((UINT8)((((Dir) == EfiUsbDataIn ? 0x01 : 0) << 7) | (Type) | (Target)))

typedef struct {
	ACPI_HID_DEVICE_PATH		AcpiDevicePath;
	PCI_DEVICE_PATH			    PciDevicePath;
	EFI_DEVICE_PATH_PROTOCOL	EndDevicePath;
} EFI_USB_PCIIO_DEVICE_PATH;

typedef struct _DWUSB_INTERRUPT_QUEUE DWUSB_INTERRUPT_QUEUE;
struct _DWUSB_INTERRUPT_QUEUE {
	DWUSB_INTERRUPT_QUEUE                *Next;
	EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction;
    VOID                                 *Data;
	UINTN                                 DataLength;
	VOID                                 *Context;
    UINT32                                TransferResult;
    UINT8                                 DeviceAddress; //used for matching
    UINT8                                 EndPointAddress;
	UINT8                                 Cancel;
//  Now everything needed to queue another interrupt transfer
	EFI_USB2_HC_PROTOCOL                  *This;
	UINT8                                 DeviceSpeed;
    UINTN                                 MaximumPacketLength;
    EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator;
	UINT8                                 DataToggle;
	UINT32                                TicksBeforeActive;
	INT32                                 Ticks;
};

struct _DWUSB_OTGHC_DEV {
	UINTN				Signature;
	EFI_HANDLE			DeviceHandle;

	EFI_USB2_HC_PROTOCOL		DwUsbOtgHc;

	EFI_USB_HC_STATE		DwHcState;

	EFI_USB_PCIIO_DEVICE_PATH	DevicePath;

	EFI_EVENT			ExitBootServiceEvent;

	UINT64				DwUsbBase;

	UINT16				PortStatus;
	UINT16				PortChangeStatus;

	UINT32				RhDevNum;

	EFI_EVENT           PollTimer; //interrupt transfers require an async response model, use a timer to send the query/callback
	DWUSB_INTERRUPT_QUEUE *InterruptQueue; //the queue of interrupt transfer results
	EFI_PCI_IO_PROTOCOL  *PciIo;
	EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
	UINT32              BulkActive;
	BOOLEAN             AtFullSpeed;
	BOOLEAN             ActiveChannel[DWC2_MAX_CHANNELS];
};

#endif //_DWUSBHOSTDXE_H_
