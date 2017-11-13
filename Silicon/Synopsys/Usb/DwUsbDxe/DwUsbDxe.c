/** @file

  Copyright (c) 2015, Linaro Limited. All rights reserved.
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.
  Copyright (c) 2017, Jeremy Linton. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Usb.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Protocol/UsbDevice.h>
#include <Protocol/UsbHostController.h>
#include <Protocol/Usb2HostController.h>
#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci.h>

#include "DwUsbDxe.h"
#include "Hi6220.h"

// Blah, the existing hikey driver only works in OTG mode (aka its an endpoint device for fastboot)
// out of the box this driver configures a made up protocol which
// setups the port to act as a fastboot programming taget. This sucks!
#ifdef NOTACTIVE

STATIC dwc_otg_dev_dma_desc_t *g_dma_desc,*g_dma_desc_ep0,*g_dma_desc_in;
STATIC USB_DEVICE_REQUEST *p_ctrlreq;
STATIC VOID *rx_buf;
STATIC UINT32 rx_desc_bytes = 0;
STATIC UINTN mNumDataBytes;

#define USB_BLOCK_HIGH_SPEED_SIZE    512
#define DATA_SIZE 32768
#define CMD_SIZE 512
#define MATCH_CMD_LITERAL(Cmd, Buf) !AsciiStrnCmp (Cmd, Buf, sizeof (Cmd) - 1)



STATIC USB_DEVICE_DESCRIPTOR    *mDeviceDescriptor;

// The config descriptor, interface descriptor, and endpoint descriptors in a
// buffer (in that order)
STATIC VOID                     *mDescriptors;
// Convenience pointers to those descriptors inside the buffer:
STATIC USB_INTERFACE_DESCRIPTOR *mInterfaceDescriptor;
STATIC USB_CONFIG_DESCRIPTOR    *mConfigDescriptor;
STATIC USB_ENDPOINT_DESCRIPTOR  *mEndpointDescriptors;

STATIC USB_DEVICE_RX_CALLBACK   mDataReceivedCallback;
STATIC USB_DEVICE_TX_CALLBACK   mDataSentCallback;

STATIC EFI_USB_STRING_DESCRIPTOR mLangStringDescriptor = {
  4,
  USB_DESC_TYPE_STRING,
  {0x409}
};


// The time between interrupt polls, in units of 100 nanoseconds
// 10 Microseconds
#define DW_INTERRUPT_POLL_PERIOD 10000
STATIC int usb_drv_port_speed(void) /*To detect which mode was run, high speed or full speed*/
{
    /*
    * 2'b00: High speed (PHY clock is running at 30 or 60 MHz)
    */
    UINT32 val = READ_REG32(DSTS) & 2;
    return (!val);
}



STATIC VOID reset_endpoints(void)
{
  /* EP0 IN ACTIVE NEXT=1 */
  WRITE_REG32(DIEPCTL0, 0x8800);

  /* EP0 OUT ACTIVE */
  WRITE_REG32(DOEPCTL0, 0x8000);

  /* Clear any pending OTG Interrupts */
  WRITE_REG32(GOTGINT, 0xFFFFFFFF);

  /* Clear any pending interrupts */
  WRITE_REG32(GINTSTS, 0xFFFFFFFF);
  WRITE_REG32(DIEPINT0, 0xFFFFFFFF);
  WRITE_REG32(DOEPINT0, 0xFFFFFFFF);
  WRITE_REG32(DIEPINT1, 0xFFFFFFFF);
  WRITE_REG32(DOEPINT1, 0xFFFFFFFF);

  /* IN EP interrupt mask */
  WRITE_REG32(DIEPMSK, 0x0D);
  /* OUT EP interrupt mask */
  WRITE_REG32(DOEPMSK, 0x0D);
  /* Enable interrupts on Ep0 */
  WRITE_REG32(DAINTMSK, 0x00010001);

  /* EP0 OUT Transfer Size:64 Bytes, 1 Packet, 3 Setup Packet, Read to receive setup packet*/
  WRITE_REG32(DOEPTSIZ0, 0x60080040);

  //notes that:the compulsive conversion is expectable.
  g_dma_desc_ep0->status.b.bs = 0x3;
  g_dma_desc_ep0->status.b.mtrf = 0;
  g_dma_desc_ep0->status.b.sr = 0;
  g_dma_desc_ep0->status.b.l = 1;
  g_dma_desc_ep0->status.b.ioc = 1;
  g_dma_desc_ep0->status.b.sp = 0;
  g_dma_desc_ep0->status.b.bytes = 64;
  g_dma_desc_ep0->buf = (UINT32)(UINTN)(p_ctrlreq);
  g_dma_desc_ep0->status.b.sts = 0;
  g_dma_desc_ep0->status.b.bs = 0x0;
  WRITE_REG32(DOEPDMA0, (unsigned long)(g_dma_desc_ep0));
  /* EP0 OUT ENABLE CLEARNAK */
  WRITE_REG32(DOEPCTL0, (READ_REG32(DOEPCTL0) | 0x84000000));
}

STATIC VOID ep_tx(IN UINT8 ep, CONST VOID *ptr, UINT32 len)
{
    UINT32 blocksize;
    UINT32 packets;

    /* EPx OUT ACTIVE */
    WRITE_REG32(DIEPCTL(ep), (READ_REG32(DIEPCTL(ep))) | 0x8000);
    if(!ep) {
        blocksize = 64;
    } else {
        blocksize = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;
    }
    packets = (len + blocksize - 1) / blocksize;

    if (!len) { //send a null packet
        /* one empty packet */
        g_dma_desc_in->status.b.bs = 0x3;
        g_dma_desc_in->status.b.l = 1;
        g_dma_desc_in->status.b.ioc = 1;
        g_dma_desc_in->status.b.sp = 1;
        g_dma_desc_in->status.b.bytes = 0;
        g_dma_desc_in->buf = 0;
        g_dma_desc_in->status.b.sts = 0;
        g_dma_desc_in->status.b.bs = 0x0;

        WRITE_REG32(DIEPDMA(ep), (unsigned long)(g_dma_desc_in));             // DMA Address (DMAAddr) is zero
    } else { //prepare to send a packet
        /*WRITE_REG32((len | (packets << 19)), DIEPTSIZ(ep));*/  // packets+transfer size
	WRITE_REG32(DIEPTSIZ(ep), len | (packets << 19));

	//flush cache
	WriteBackDataCacheRange ((void*)ptr, len);

        g_dma_desc_in->status.b.bs = 0x3;
        g_dma_desc_in->status.b.l = 1;
        g_dma_desc_in->status.b.ioc = 1;
        g_dma_desc_in->status.b.sp = 1;
        g_dma_desc_in->status.b.bytes = len;
        g_dma_desc_in->buf = (UINT32)((UINTN)ptr);
        g_dma_desc_in->status.b.sts = 0;
        g_dma_desc_in->status.b.bs = 0x0;
        WRITE_REG32(DIEPDMA(ep), (unsigned long)(g_dma_desc_in));         // ptr is DMA address
    }
    asm("dsb  sy");
    asm("isb  sy");
    /* epena & cnak*/
    WRITE_REG32(DIEPCTL(ep), READ_REG32(DIEPCTL(ep)) | 0x84000800);
    return;
}

STATIC VOID ep_rx(unsigned ep, UINT32 len)
{
    /* EPx UNSTALL */
    WRITE_REG32(DOEPCTL(ep), ((READ_REG32(DOEPCTL(ep))) & (~0x00200000)));
    /* EPx OUT ACTIVE */
    WRITE_REG32(DOEPCTL(ep), (READ_REG32(DOEPCTL(ep)) | 0x8000));

    if (len >= DATA_SIZE)
	    rx_desc_bytes = DATA_SIZE;
    else
	    rx_desc_bytes = len;

    rx_buf = AllocatePool (DATA_SIZE);
    ASSERT (rx_buf != NULL);

    InvalidateDataCacheRange (rx_buf, len);

    g_dma_desc->status.b.bs = 0x3;
    g_dma_desc->status.b.mtrf = 0;
    g_dma_desc->status.b.sr = 0;
    g_dma_desc->status.b.l = 1;
    g_dma_desc->status.b.ioc = 1;
    g_dma_desc->status.b.sp = 0;
    g_dma_desc->status.b.bytes = rx_desc_bytes;
    g_dma_desc->buf = (UINT32)((UINTN)rx_buf);
    g_dma_desc->status.b.sts = 0;
    g_dma_desc->status.b.bs = 0x0;

    asm("dsb  sy");
    asm("isb  sy");
    WRITE_REG32(DOEPDMA(ep), (UINT32)((UINTN)g_dma_desc));
    /* EPx OUT ENABLE CLEARNAK */
    WRITE_REG32(DOEPCTL(ep), (READ_REG32(DOEPCTL(ep)) | 0x84000000));
}

STATIC
EFI_STATUS
HandleGetDescriptor (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  UINT8       DescriptorType;
  UINTN       ResponseSize;
  VOID       *ResponseData;
//  CHAR16      SerialNo[16];
//  UINTN       SerialNoLen;
//  EFI_STATUS  Status;

  ResponseSize = 0;
  ResponseData = NULL;

  // Pretty confused if bmRequestType is anything but this:
  ASSERT (Request->RequestType == USB_DEV_GET_DESCRIPTOR_REQ_TYPE);

  // Choose the response
  DescriptorType = Request->Value >> 8;
  switch (DescriptorType) {
  case USB_DESC_TYPE_DEVICE:
    DEBUG ((EFI_D_ERROR, "USB: Got a request for device descriptor\n"));
    ResponseSize = sizeof (USB_DEVICE_DESCRIPTOR);
    ResponseData = mDeviceDescriptor;
    break;
  case USB_DESC_TYPE_CONFIG:
    DEBUG ((EFI_D_ERROR, "USB: Got a request for config descriptor\n"));
    ResponseSize = mConfigDescriptor->TotalLength;
    ResponseData = mDescriptors;
    break;
  case USB_DESC_TYPE_STRING:
    DEBUG ((EFI_D_ERROR, "USB: Got a request for String descriptor %d\n", Request->Value & 0xFF));
    switch (Request->Value & 0xff) {
    case 0:
      ResponseSize = mLangStringDescriptor.Length;
      ResponseData = &mLangStringDescriptor;
      break;
    case 1:
      ResponseSize = mManufacturerStringDescriptor.Hdr.Length;
      ResponseData = &mManufacturerStringDescriptor;
      break;
    case 2:
      ResponseSize = mProductStringDescriptor.Hdr.Length;
      ResponseData = &mProductStringDescriptor;
      break;
    case 3:
/*      Status = gRT->GetVariable (
                      (CHAR16*)L"SerialNo",
                      &gArmGlobalVariableGuid,
                      NULL,
                      &SerialNoLen,
                      SerialNo
                      );
      if (EFI_ERROR (Status) == 0) {
        CopyMem (mSerialStringDescriptor.String, SerialNo, SerialNoLen);
      }
      ResponseSize = mSerialStringDescriptor.Length;
      ResponseData = &mSerialStringDescriptor;*/
      CopyMem (mSerialStringDescriptor.Hdr.String, "12345", 6);
      ResponseSize = 6;
      ResponseData = &mSerialStringDescriptor;

      break;
    }
    break;
  default:
    DEBUG ((EFI_D_ERROR, "USB: Didn't understand request for descriptor 0x%04x\n", Request->Value));
    break;
  }

  // Send the response
  if (ResponseData) {
    ASSERT (ResponseSize != 0);

    if (Request->Length < ResponseSize) {
      // Truncate response
      ResponseSize = Request->Length;
    } else if (Request->Length > ResponseSize) {
      DEBUG ((EFI_D_ERROR, "USB: Info: ResponseSize < wLength\n"));
    }

    ep_tx(0, ResponseData, ResponseSize);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
HandleSetAddress (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  // Pretty confused if bmRequestType is anything but this:
  ASSERT (Request->RequestType == USB_DEV_SET_ADDRESS_REQ_TYPE);
  DEBUG ((EFI_D_ERROR, "USB: Setting address to %d\n", Request->Value));
  reset_endpoints();

  WRITE_REG32(DCFG, (READ_REG32(DCFG) & ~0x7F0) | (Request->Value << 4));
  ep_tx(0, 0, 0);

  return EFI_SUCCESS;
}

int usb_drv_request_endpoint(unsigned int type, int dir)
{
  unsigned int ep = 1;    /*FIXME*/
  int ret;
  unsigned long newbits;
  
  ret = (int)ep | dir;
  newbits = (type << 18) | 0x10000000;
  
  /*
   * (type << 18):Endpoint Type (EPType)
   * 0x10000000:Endpoint Enable (EPEna)
   * 0x000C000:Endpoint Type (EPType);Hardcoded to 00 for control.
   * (ep<<22):TxFIFO Number (TxFNum)
   * 0x20000:NAK Status (NAKSts);The core is transmitting NAK handshakes on this endpoint.
   */
  if (dir) {  // IN: to host
  	WRITE_REG32(DIEPCTL(ep), ((READ_REG32(DIEPCTL(ep)))& ~0x000C0000) | newbits | (ep<<22)|0x20000);
  } else {    // OUT: to device
  	WRITE_REG32(DOEPCTL(ep), ((READ_REG32(DOEPCTL(ep))) & ~0x000C0000) | newbits);
  }
  
  return ret;
}
STATIC
EFI_STATUS
HandleSetConfiguration (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  ASSERT (Request->RequestType == USB_DEV_SET_CONFIGURATION_REQ_TYPE);

  // Cancel all transfers
  reset_endpoints();

  usb_drv_request_endpoint(2, 0);
  usb_drv_request_endpoint(2, 0x80);

  WRITE_REG32(DIEPCTL1, (READ_REG32(DIEPCTL1)) | 0x10088800);

  /* Enable interrupts on all endpoints */
  WRITE_REG32(DAINTMSK, 0xFFFFFFFF);

  ep_rx(1, CMD_SIZE);
  ep_tx(0, 0, 0);
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
HandleDeviceRequest (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  EFI_STATUS  Status;

  switch (Request->Request) {
  case USB_DEV_GET_DESCRIPTOR:
    Status = HandleGetDescriptor (Request);
    break;
  case USB_DEV_SET_ADDRESS:
    Status = HandleSetAddress (Request);
    break;
  case USB_DEV_SET_CONFIGURATION:
    Status = HandleSetConfiguration (Request);
    break;
  default:
    DEBUG ((EFI_D_ERROR,
      "Didn't understand RequestType 0x%x Request 0x%x\n",
      Request->RequestType, Request->Request));
      Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}


// Instead of actually registering interrupt handlers, we poll the controller's
//  interrupt source register in this function.
STATIC
VOID
CheckInterrupts (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
  UINT32 ints = READ_REG32(GINTSTS);    // interrupt register
  UINT32 epints;

  /*
   * bus reset
   * The core sets this bit to indicate that a reset is detected on the USB.
   */
  if (ints & 0x1000) {
	  WRITE_REG32(DCFG, 0x800004);
	  reset_endpoints();
  }

  /*
   * enumeration done, we now know the speed
   * The core sets this bit to indicate that speed enumeration is complete. The
   * application must read the Device Status (DSTS) register to obtain the
   * enumerated speed.
   */
  if (ints & 0x2000) {
	  /* Set up the maximum packet sizes accordingly */
	  unsigned long maxpacket = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;
	  //Set Maximum In Packet Size (MPS)
	  WRITE_REG32(DIEPCTL1, ((READ_REG32(DIEPCTL1)) & ~0x000003FF) | maxpacket);
	  //Set Maximum Out Packet Size (MPS)
	  WRITE_REG32(DOEPCTL1, ((READ_REG32(DOEPCTL1)) & ~0x000003FF) | maxpacket);
  }

  /*
   * IN EP event
   * The core sets this bit to indicate that an interrupt is pending on one of the IN
   * endpoints of the core (in Device mode). The application must read the
   * Device All Endpoints Interrupt (DAINT) register to determine the exact
   * number of the IN endpoint on which the interrupt occurred, and then read
   * the corresponding Device IN Endpoint-n Interrupt (DIEPINTn) register to
   * determine the exact cause of the interrupt. The application must clear the
   * appropriate status bit in the corresponding DIEPINTn register to clear this bit.
   */
  if (ints & 0x40000) {
	  epints = READ_REG32(DIEPINT0);
	  WRITE_REG32(DIEPINT0, epints);
	  if (epints & 0x1) /* Transfer Completed Interrupt (XferCompl) */
		  DEBUG ((EFI_D_ERROR, "INT: IN TX completed.DIEPTSIZ(0) = 0x%x.\n", READ_REG32(DIEPTSIZ0)));

	  epints = READ_REG32(DIEPINT1);
	  WRITE_REG32(DIEPINT1, epints);
	  if (epints & 0x1)
		  DEBUG ((EFI_D_ERROR, "ep1: IN TX completed\n"));
  }

  /*
   * OUT EP event
   * The core sets this bit to indicate that an interrupt is pending on one of the
   * OUT endpoints of the core (in Device mode). The application must read the
   * Device All Endpoints Interrupt (DAINT) register to determine the exact
   * number of the OUT endpoint on which the interrupt occurred, and then read
   * the corresponding Device OUT Endpoint-n Interrupt (DOEPINTn) register
   * to determine the exact cause of the interrupt. The application must clear the
   * appropriate status bit in the corresponding DOEPINTn register to clear this bit.
   */
  if (ints & 0x80000) {
	  /* indicates the status of an endpoint
	   * with respect to USB- and AHB-related events. */
	  epints = READ_REG32(DOEPINT0);
	  if(epints) {
		  WRITE_REG32(DOEPINT0, epints);
		  if (epints & 0x1)
			  DEBUG ((EFI_D_ERROR,"INT: EP0 RX completed. DOEPTSIZ(0) = 0x%x.\n", READ_REG32(DOEPTSIZ0)));
		  /*
		   *
		   IN Token Received When TxFIFO is Empty (INTknTXFEmp)
		   * Indicates that an IN token was received when the associated TxFIFO (periodic/nonperiodic)
		   * was empty. This interrupt is asserted on the endpoint for which the IN token
		   * was received.
		   */
		  if (epints & 0x8) { /* SETUP phase done */
			  // PRINT_DEBUG("Setup phase \n");
			  WRITE_REG32(DIEPCTL0, READ_REG32(DIEPCTL0) | 0x08000000);
			  WRITE_REG32(DOEPCTL0, READ_REG32(DOEPCTL0) | 0x08000000);
			  /*clear IN EP intr*/
			  WRITE_REG32(DIEPINT0, 0xffffffff);
			  HandleDeviceRequest((USB_DEVICE_REQUEST *)p_ctrlreq);
		  }

		  /* Make sure EP0 OUT is set up to accept the next request */
		  /* memset(p_ctrlreq, 0, NUM_ENDPOINTS*8); */
		  WRITE_REG32(DOEPTSIZ0, 0x60080040);
		  /*
		   * IN Token Received When TxFIFO is Empty (INTknTXFEmp)
		   * Indicates that an IN token was received when the associated TxFIFO (periodic/nonperiodic)
		   * was empty. This interrupt is asserted on the endpoint for which the IN token
		   * was received.
		   */
		  g_dma_desc_ep0->status.b.bs = 0x3;
		  g_dma_desc_ep0->status.b.mtrf = 0;
		  g_dma_desc_ep0->status.b.sr = 0;
		  g_dma_desc_ep0->status.b.l = 1;
		  g_dma_desc_ep0->status.b.ioc = 1;
		  g_dma_desc_ep0->status.b.sp = 0;
		  g_dma_desc_ep0->status.b.bytes = 64;
		  g_dma_desc_ep0->buf = (UINT32)(UINTN)(p_ctrlreq);
		  g_dma_desc_ep0->status.b.sts = 0;
		  g_dma_desc_ep0->status.b.bs = 0x0;
		  WRITE_REG32(DOEPDMA0, (unsigned long)(g_dma_desc_ep0));
		  // endpoint enable; clear NAK
		  WRITE_REG32(DOEPCTL0, 0x84000000);
	  }

	  epints = (READ_REG32(DOEPINT1));
	  if(epints) {
		  WRITE_REG32(DOEPINT1, epints);
		  /* Transfer Completed Interrupt (XferCompl);Transfer completed */
		  if (epints & 0x1) {
			  asm("dsb  sy");
			  asm("isb  sy");

			  UINT32 bytes = rx_desc_bytes - g_dma_desc->status.b.bytes;
			  UINT32 len = 0;

			  if (MATCH_CMD_LITERAL ("download", rx_buf)) {
				  mNumDataBytes = AsciiStrHexToUint64 (rx_buf + sizeof ("download"));
			  } else {
				if (mNumDataBytes != 0)
					mNumDataBytes -= bytes;
			  }

			  mDataReceivedCallback (bytes, rx_buf);

			  if (mNumDataBytes == 0)
				  len = CMD_SIZE;
			  else if (mNumDataBytes > DATA_SIZE)
				  len = DATA_SIZE;
			  else
				  len = mNumDataBytes;

			  ep_rx(1, len);
		  }
	  }
  }

  //WRITE_REG32 clear ints
  WRITE_REG32(GINTSTS, ints);
}

EFI_STATUS
DwUsbSend (
  IN        UINT8  EndpointIndex,
  IN        UINTN  Size,
  IN  CONST VOID  *Buffer
  )
{
    ep_tx(EndpointIndex, Buffer, Size);
    return 0;
}


STATIC VOID usb_init()
{
  VOID* buf;

  buf = UncachedAllocatePages (1);
  g_dma_desc = buf;
  g_dma_desc_ep0 = g_dma_desc + sizeof(struct dwc_otg_dev_dma_desc);
  g_dma_desc_in = g_dma_desc_ep0 + sizeof(struct dwc_otg_dev_dma_desc);
  p_ctrlreq = (USB_DEVICE_REQUEST *)g_dma_desc_in + sizeof(struct dwc_otg_dev_dma_desc);

  SetMem(g_dma_desc, sizeof(struct dwc_otg_dev_dma_desc), 0);
  SetMem(g_dma_desc_ep0, sizeof(struct dwc_otg_dev_dma_desc), 0);
  SetMem(g_dma_desc_in, sizeof(struct dwc_otg_dev_dma_desc), 0);

  /*Reset usb controller.*/
  /* Wait for AHB master idle */
  while (!((READ_REG32(GRSTCTL)) & 0x80000000));

  /* OTG: Assert Software Reset */
  WRITE_REG32(GRSTCTL, 1);

  /* Wait for OTG to ack reset */
  while ((READ_REG32(GRSTCTL)) & 1);

  /* Wait for OTG AHB master idle */
  while (!((READ_REG32(GRSTCTL)) & 0x80000000));

  WRITE_REG32(GDFIFOCFG, DATA_FIFO_CONFIG);
  WRITE_REG32(GRXFSIZ, RX_SIZE);
  WRITE_REG32(GNPTXFSIZ, ENDPOINT_TX_SIZE);
  WRITE_REG32(DIEPTXF1, DATA_IN_ENDPOINT_TX_FIFO1);

  /*
   * set Periodic TxFIFO Empty Level,
   * Non-Periodic TxFIFO Empty Level,
   * Enable DMA, Unmask Global Intr
   */
  WRITE_REG32(GAHBCFG, 0x1a1);

  /*select 8bit UTMI+, ULPI Inerface*/
  WRITE_REG32(GUSBCFG, 0x2400);

  /* Detect usb work mode,host or device? */
  while ((READ_REG32(GINTSTS)) & 1);
  MicroSecondDelay(1);

  /*Init global and device mode csr register.*/
  /*set Non-Zero-Length status out handshake */
  WRITE_REG32(DCFG, 0x800004);

  /* Interrupt unmask: IN event, OUT event, bus reset */
  WRITE_REG32(GINTMSK, 0xC3C08);

  while ((READ_REG32(GINTSTS)) & 0x2000);

  /* Clear any pending OTG Interrupts */
  WRITE_REG32(GOTGINT, 0xFFFFFFFF);
  /* Clear any pending interrupts */
  WRITE_REG32(GINTSTS, 0xFFFFFFFF);
  WRITE_REG32(GINTMSK, 0xFFFFFFFF);
  WRITE_REG32(GOTGINT, READ_REG32(GOTGINT) & (~0x3000));
  /*endpoint settings cfg*/
  reset_endpoints();

  /*init finish. and ready to transfer data*/

  /* Soft Disconnect */
  WRITE_REG32(DCTL, 0x802);
  MicroSecondDelay(1);

  /* Soft Reconnect */
  WRITE_REG32(DCTL, 0x800);
}



EFI_STATUS
EFIAPI
DwUsbStart (
  IN USB_DEVICE_DESCRIPTOR   *DeviceDescriptor,
  IN VOID                   **Descriptors,
  IN USB_DEVICE_RX_CALLBACK   RxCallback,
  IN USB_DEVICE_TX_CALLBACK   TxCallback
  )
{
  UINT8                    *Ptr;
  EFI_STATUS                Status;
  EFI_EVENT                 TimerEvent;

  ASSERT (DeviceDescriptor != NULL);
  ASSERT (Descriptors[0] != NULL);
  ASSERT (RxCallback != NULL);
  ASSERT (TxCallback != NULL);

  usb_init();

  mDeviceDescriptor = DeviceDescriptor;
  mDescriptors = Descriptors[0];

  // Right now we just support one configuration
  ASSERT (mDeviceDescriptor->NumConfigurations == 1);
  // ... and one interface
  mConfigDescriptor = (USB_CONFIG_DESCRIPTOR *)mDescriptors;
  ASSERT (mConfigDescriptor->NumInterfaces == 1);

  Ptr = ((UINT8 *) mDescriptors) + sizeof (USB_CONFIG_DESCRIPTOR);
  mInterfaceDescriptor = (USB_INTERFACE_DESCRIPTOR *) Ptr;
  Ptr += sizeof (USB_INTERFACE_DESCRIPTOR);

  mEndpointDescriptors = (USB_ENDPOINT_DESCRIPTOR *) Ptr;

  mDataReceivedCallback = RxCallback;
  mDataSentCallback = TxCallback;

  // Register a timer event so CheckInterupts gets called periodically
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckInterrupts,
                  NULL,
                  &TimerEvent
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  TimerEvent,
                  TimerPeriodic,
                  DW_INTERRUPT_POLL_PERIOD
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

USB_DEVICE_PROTOCOL mUsbDevice = {
  DwUsbStart,
  DwUsbSend
};




#endif


STATIC VOID phy_init(USB_OHCI_HC_DEV *Ehc)
{
  UINT32 val;

  DEBUG ((EFI_D_ERROR, "phy_init() sequence\n"));
	// setup direction of hub
	val = GPIO_READ_REG32(GPIODIR);
  DEBUG ((EFI_D_ERROR, "GPIO0_3=%X DIR=%X CTL=%X\n",	GPIO_READ_REG32(GPIODATA_3),val,GPIO_READ_REG32(GPIOAFSEL)));
	val |= 0x84;
	GPIO_WRITE_REG32(GPIODIR,val);


	asm("dsb  sy");
	asm("isb  sy");
	MicroSecondDelay (1000); 
	GPIO_WRITE_REG32(GPIODATA_3,1<<3);
	GPIO_WRITE_REG32(GPIODATA_7,1<<7);

	asm("dsb  sy");
	asm("isb  sy");
	MicroSecondDelay (1000); 
  DEBUG ((EFI_D_ERROR, "GPIO0_3=%X DIR=%X CTL=%X\n",	GPIO_READ_REG32(GPIODATA_3),val,GPIO_READ_REG32(GPIOAFSEL)));
  DEBUG ((EFI_D_ERROR, "GPIO0=%X\n",	GPIO_READ_REG32(0x3FC)));
//  DEBUG ((EFI_D_ERROR, "GPIO0_7=%X\n",	GPIO_READ_REG32(GPIODATA_7))); //turn on Hub port
/*
+STATIC
+VOID
+HiKeyDetectUsbModeInit (
+  IN VOID
+  )
+{
+  EFI_STATUS     Status;
+
+   //set pullup on both GPIO2_5 & GPIO2_6. It's required for inupt. 
+  MmioWrite32 (0xf8001864, 1);
+  MmioWrite32 (0xf8001868, 1);
+
+  Status = gBS->LocateProtocol (&gEmbeddedGpioProtocolGuid, NULL, (VOID **)&mGpio);
+  ASSERT_EFI_ERROR (Status);
+  Status = mGpio->Set (mGpio, USB_SEL_GPIO0_3, GPIO_MODE_OUTPUT_0);
+  ASSERT_EFI_ERROR (Status);
+  Status = mGpio->Set (mGpio, USB_5V_HUB_EN, GPIO_MODE_OUTPUT_0);
+  ASSERT_EFI_ERROR (Status);
+  MicroSecondDelay (1000);
+
+  Status = mGpio->Set (mGpio, USB_ID_DET_GPIO2_5, GPIO_MODE_INPUT);
+  ASSERT_EFI_ERROR (Status);
+  Status = mGpio->Set (mGpio, USB_VBUS_DET_GPIO2_6, GPIO_MODE_INPUT);
+  ASSERT_EFI_ERROR (Status);
+} */

  //setup clock
  val = PHY_READ_REG32(SC_PERIPH_CLKEN0);
  val |= BIT4;
  PHY_WRITE_REG32(SC_PERIPH_CLKEN0, val);

	do {
     val = PHY_READ_REG32(SC_PERIPH_CLKSTAT0);
	} while ((val & BIT4) == 0);

  // setup phy (this is just hi6220_phy_init() minus the "reset enable"?
	// AKA looks like its just pulling the phy out of reset
  val = PHY_READ_REG32(SC_PERIPH_RSTDIS0);
  val |= RST0_USBOTG_BUS | RST0_POR_PICOPHY |  RST0_USBOTG | RST0_USBOTG_32K;
  PHY_WRITE_REG32(SC_PERIPH_RSTDIS0, val);

	// this is the "on" mode from linux hi6220_phy_setup()
  val = PHY_READ_REG32(SC_PERIPH_CTRL5);
  val &= ~CTRL5_PICOPHY_BC_MODE;
  val |= CTRL5_USBOTG_RES_SEL | CTRL5_PICOPHY_ACAENB; //newer HIsi edk has this | CTRL5_PICOPHY_VDATDETENB | CTRL5_PICOPHY_DCDENB; ;
  PHY_WRITE_REG32(SC_PERIPH_CTRL5, val);

  val = PHY_READ_REG32(SC_PERIPH_CTRL4);
  val &=  ~(CTRL4_PICO_SIDDQ | CTRL4_PICO_OGDISABLE);
  val |=  CTRL4_PICO_VBUSVLDEXT | CTRL4_PICO_VBUSVLDEXTSEL |  CTRL4_OTG_PHY_SEL;
  PHY_WRITE_REG32(SC_PERIPH_CTRL4, val);

  PHY_WRITE_REG32(SC_PERIPH_CTRL8, EYE_PATTERN_PARA);
}

// Try to get the controller inited, the appropriate linux function
// is probably dwc2_core_host_init()
STATIC VOID DwHostInit(USB_OHCI_HC_DEV *Ehc)
{
	UINT32 reg;

		// rpi needs the USB controller powered?
		// do we?
	  // play with PCGCCTL 
/*    status = dwc_power_on();
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
				}*/

	DEBUG ((EFI_D_ERROR, "DwHostInit reset\n"));
  /*Reset usb controller.*/
	/* Wait for AHB master idle */
	while (!((READ_REG32(GRSTCTL)) & GRSTCTL_AHBIDLE));
  /* Assert Software Reset */
  WRITE_REG32(GRSTCTL, GRSTCTL_CSFTRST);
  /* Wait for ack reset */
  while ((READ_REG32(GRSTCTL)) & GRSTCTL_CSFTRST);
	/* Wait for AHB master idle */
	while (!((READ_REG32(GRSTCTL)) & GRSTCTL_AHBIDLE));

	// version
	reg=READ_REG32(GSNPSID); //global rx fifo size
	DEBUG ((EFI_D_ERROR, "DwHostInit version %X\n",reg));

	DEBUG ((EFI_D_ERROR, "DwHostInit setup dma\n"));
	reg=READ_REG32(GRXFSIZ); //global rx fifo size
	DEBUG ((EFI_D_ERROR, "DwHostInit global rx fifo size 0x%X\n",reg));
	reg=READ_REG32(GNPTXFSIZ); //global np tx fifo size
	DEBUG ((EFI_D_ERROR, "DwHostInit global np tx fifo size 0x%X\n",reg));
	reg=READ_REG32(HPTXFSIZ); //global tx fifo size
	DEBUG ((EFI_D_ERROR, "DwHostInit global tx fifo size 0x%X\n",reg));
	reg=READ_REG32(GNPTXSTS); //global np tx fifo status
	DEBUG ((EFI_D_ERROR, "DwHostInit global np tx fifo status 0x%X\n",reg));

	reg=READ_REG32(GAHBCFG);
	DEBUG ((EFI_D_ERROR, "DwHostInit global AHB config 0x%X\n",reg));
	reg |= GAHBCFG_DMA_EN;
  WRITE_REG32(GAHBCFG, reg);
	
//    dwc_soft_reset();
//    dwc_setup_dma_mode();
//    dwc_setup_interrupts();
//    status = dwc_start_xfer_scheduler();
/*    if (status != USB_STATUS_SUCCESS)
    {
        dwc_power_off();
				}*/
}

// Compare with dwc2_hc_init()
// Can we get away with just using a single channel?
// after all, we only want sync request/response type 
// behavior....
STATIC EFI_STATUS DwChannelInit(USB_OHCI_HC_DEV *Ehc, int DevAddr, int EpNum, EFI_USB_DATA_DIRECTION EpDir, BOOLEAN Slow, UINT8 Type, UINT8 MaxPacket)
{
  UINT32 hcchar;
	int channel = 0;
	hcchar  = DevAddr   << HCCHAR_DEVADDR_SHIFT & HCCHAR_DEVADDR_MASK;
	hcchar |= EpNum     << HCCHAR_EPNUM_SHIFT   & HCCHAR_EPNUM_MASK;
	hcchar |= Type      << HCCHAR_EPTYPE_SHIFT  & HCCHAR_EPTYPE_MASK;
	hcchar |= MaxPacket << HCCHAR_MPS_SHIFT     & HCCHAR_MPS_MASK;

	if (EpDir == EfiUsbDataIn)	{
		hcchar |= HCCHAR_EPDIR;
	}
	if (Slow)	{
		hcchar |= HCCHAR_LSPDDEV;
	}
	hcchar |= (READ_REG32(HFNUM) +1) & 1; //set odd frame???
	hcchar |= HCCHAR_CHENA; // enable channel, disable is already cleared

	WRITE_REG32(HCINT(channel), 0);             //clear existing channel status
	WRITE_REG32(HCINTMSK(channel), 0xFFFFFFFF); //enable all status flags
  WRITE_REG32(HCCHAR(channel),  hcchar);

	return EFI_SUCCESS;
}



/**
  Provides software reset for the USB host controller.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPOURTED      The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.

**/
EFI_STATUS
EFIAPI
OhciReset (
  IN EFI_USB_HC_PROTOCOL  *This,
  IN UINT16               Attributes
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciReset\n"));
  return EFI_SUCCESS;
}

/**
  Retrieve the current state of the USB host controller.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  State                 Variable to return the current host controller
                                state.

  @retval EFI_SUCCESS           Host controller state was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to
                                retrieve the host controller's current state.

**/

EFI_STATUS
EFIAPI
OhciGetState (
  IN  EFI_USB_HC_PROTOCOL  *This,
  OUT EFI_USB_HC_STATE     *State
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciGetState\n"));
	return EFI_SUCCESS;
}

/**
  Sets the USB host controller to a specific state.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  State                 The state of the host controller that will be set.

  @retval EFI_SUCCESS           The USB host controller was successfully placed
                                in the state specified by State.
  @retval EFI_INVALID_PARAMETER State is invalid.
  @retval EFI_DEVICE_ERROR      Failed to set the state due to device error.

**/

EFI_STATUS
EFIAPI
OhciSetState(
  IN EFI_USB_HC_PROTOCOL  *This,
  IN EFI_USB_HC_STATE     State
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciSetState\n"));
	return EFI_SUCCESS;
}

/**

  Submits control transfer to a target USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxPaketLength        Indicates the maximum packet size that the
                                default control transfer endpoint is capable of
                                sending or receiving.
  @param  Request               A pointer to the USB device request that will be sent
                                to the USB device.
  @param  TransferDirection     Specifies the data direction for the transfer.
                                There are three values available, DataIn, DataOut
                                and NoData.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            Indicates the size, in bytes, of the data buffer
                                specified by Data.
  @param  TimeOut               Indicates the maximum time, in microseconds,
                                which the transfer is allowed to complete.
  @param  TransferResult        A pointer to the detailed result information generated
                                by this control transfer.

  @retval EFI_SUCCESS           The control transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The control transfer could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The control transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The control transfer failed due to host controller or device error.
                                Caller should check TranferResult for detailed error information.

--*/

void DumpRequest(EFI_USB_DEVICE_REQUEST *Request)
{
	if (Request->RequestType & 0x80) {
		DEBUG ((EFI_D_ERROR, "USB: RequestType %X -> Host\n",Request->RequestType));
	}
	else {
		DEBUG ((EFI_D_ERROR, "USB: RequestType %X -> Device\n",Request->RequestType));
	}
	if (Request->RequestType & 0x60)	{
		DEBUG ((EFI_D_ERROR, "USB:                -> CLS/VEND\n"));
	}
	else	{
  	DEBUG ((EFI_D_ERROR, "USB:                -> STD\n"));
	}
	switch (Request->RequestType & 0x03)	{
 	  case USB_TARGET_DEVICE:
  	  DEBUG ((EFI_D_ERROR, "USB:                -> DEVICE\n"));
			break;
 	  case USB_TARGET_INTERFACE:
  	  DEBUG ((EFI_D_ERROR, "USB:                -> ITERFACE\n"));
			break;
 	  case USB_TARGET_ENDPOINT:
  	  DEBUG ((EFI_D_ERROR, "USB:                -> ENDPOINT\n"));
			break;
 	  case USB_TARGET_OTHER:
  	  DEBUG ((EFI_D_ERROR, "USB:                -> OTHER\n"));
			break;
	}
	DEBUG ((EFI_D_ERROR, "USB: RequestType 0x%X\n",Request->RequestType));
  switch (Request->Request)	{
		case USB_DEV_GET_STATUS:
			DEBUG ((EFI_D_ERROR, "USB:             GET STATUS\n"));
			break;
		case USB_DEV_CLEAR_FEATURE:
			DEBUG ((EFI_D_ERROR, "USB:             CLEAR FEATURE\n"));
			break;
		case USB_DEV_SET_FEATURE:
			DEBUG ((EFI_D_ERROR, "USB:             SET FEATURE\n"));
			break;
		case USB_DEV_SET_ADDRESS:
			DEBUG ((EFI_D_ERROR, "USB:             SET ADDRESS\n"));
			break;
		case USB_DEV_GET_DESCRIPTOR:
			DEBUG ((EFI_D_ERROR, "USB:             GET DESCRIPTOR\n"));
			break;
		case USB_DEV_SET_DESCRIPTOR:
			DEBUG ((EFI_D_ERROR, "USB:             SET DESCRIPTOR\n"));
			break;
		default:
			break;
	}
	DEBUG ((EFI_D_ERROR, "USB: Request     %d\n",Request->Request));
	DEBUG ((EFI_D_ERROR, "USB: Value       0x%X\n",Request->Value));
	DEBUG ((EFI_D_ERROR, "USB: Index       0x%X\n",Request->Index));
	DEBUG ((EFI_D_ERROR, "USB: Length      %d\n",Request->Length));
}

STATIC USB_DEVICE_DESCRIPTOR RootDevDescriptor = 
{
		.Length = sizeof(USB_DEVICE_DESCRIPTOR),
		.DescriptorType = USB_DESC_TYPE_DEVICE,
		.BcdUSB = 0x200,
		.DeviceClass = 0x09, //TYPE HUB
		.DeviceSubClass = 0,
		.DeviceProtocol = 1,
		.MaxPacketSize0 = 18,
		.IdVendor = 0,
		.IdProduct = 2,
		.BcdDevice = 0,
		.StrManufacturer = 0,
		.StrProduct = 1,
		.StrSerialNumber = 2,
		.NumConfigurations = 1
};

STATIC struct ROOT_HUB_CFG 
{
		USB_CONFIG_DESCRIPTOR RootCfgDescriptor;
		USB_INTERFACE_DESCRIPTOR RootIntDescriptor;
		USB_ENDPOINT_DESCRIPTOR RootEndDescriptor;
} RootCfg =
{
	.RootCfgDescriptor =
	{
		.Length = sizeof(USB_CONFIG_DESCRIPTOR),
		.DescriptorType = USB_DESC_TYPE_CONFIG,
		.TotalLength = sizeof(struct ROOT_HUB_CFG),
		.NumInterfaces = 1,
		.ConfigurationValue = 1,
		.Configuration = 0 ,
		.Attributes = 0xC0,
		.MaxPower = 0
	},
	.RootIntDescriptor = 
	{
		.Length = sizeof(USB_INTERFACE_DESCRIPTOR),
		.DescriptorType = USB_DESC_TYPE_INTERFACE,
		.InterfaceNumber = 0,
		.AlternateSetting = 0,
		.NumEndpoints = 1,
		.InterfaceClass = 0x09, //hub
		.InterfaceSubClass = 0,
		.InterfaceProtocol = 0,
		.Interface = 0
	},
	.RootEndDescriptor = 
	{
		.Length = sizeof(USB_ENDPOINT_DESCRIPTOR),
		.DescriptorType = USB_DESC_TYPE_ENDPOINT,
		.EndpointAddress = 0x81,
		.Attributes = 0x03,
		.MaxPacketSize = 64,
		.Interval = 0xFF
	},
};

typedef struct {
  EFI_USB_STRING_DESCRIPTOR Hdr;
  CHAR16 buf[16];
} DEF_EFI_USB_STRING_DESCRIPTOR;

STATIC DEF_EFI_USB_STRING_DESCRIPTOR mManufacturerStringDescriptor = {
  { 
	18,
    USB_DESC_TYPE_STRING,
    {'9'}
  }, 
  {'6', 'B', 'o', 'a', 'r', 'd', 's'}
};

STATIC DEF_EFI_USB_STRING_DESCRIPTOR mProductStringDescriptor = {
  {
    12,
    USB_DESC_TYPE_STRING,
	{'H'}
  }, 
  {'i', 'K', 'e', 'y'}
};

STATIC DEF_EFI_USB_STRING_DESCRIPTOR mSerialStringDescriptor = {
  {
    34,
    USB_DESC_TYPE_STRING,
    {'0'}
  },
  {'1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}
};




EFI_STATUS
EFIAPI
OhciControlTransfer (
  IN     EFI_USB_HC_PROTOCOL     *This,
  IN     UINT8                   DeviceAddress,
  IN     BOOLEAN                 IsSlowDevice,
  IN     UINT8                   MaxPacketLength,
  IN     EFI_USB_DEVICE_REQUEST  *Request,
  IN     EFI_USB_DATA_DIRECTION  TransferDirection,
  IN OUT VOID                    *Data                 OPTIONAL,
  IN OUT UINTN                   *DataLength           OPTIONAL,
  IN     UINTN                   TimeOut,
  OUT    UINT32                  *TransferResult
  )
{
  int handled = 0;
  static int first = 1;
#define ENABLE_EMULATION 20
  DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer (addr:%d) (direction:%d) Len:%d\n",DeviceAddress,TransferDirection,*DataLength));

  DumpRequest(Request);
  if ((DeviceAddress == 0+ENABLE_EMULATION) && (first))
  {
		  // ignore first set addr
		  first = 0;
		  handled = 1;
		  DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer, ignore first set addr for root port\n"));
  }
  if (DeviceAddress == 1+ENABLE_EMULATION) { //root port fake response
	  if (Request->RequestType == 0x80) { //device to host
		  if (Request->Request == USB_DEV_GET_DESCRIPTOR)	{
				DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer get descriptor\n"));
			  if ((Request->Value & 0xFF) == 0)
			  {
				  switch (Request->Value >> 8) {
					  case USB_DESC_TYPE_DEVICE:
						CopyMem (Data, &RootDevDescriptor, *DataLength);
						  handled = 1;
						  break;
					  case USB_DESC_TYPE_CONFIG:
						  CopyMem (Data, &RootCfg, *DataLength);
						  handled = 1;
						  break;
					  case USB_DESC_TYPE_STRING:
						  switch (Request->Index)
						  {
						  case 0:
							  CopyMem (Data, &mManufacturerStringDescriptor, *DataLength);
						    handled = 1;
							  break;
						  case 1:
							  CopyMem (Data, &mProductStringDescriptor, *DataLength);
								handled = 1;
							  break;
						  case 2:
							  CopyMem (Data, &mSerialStringDescriptor, *DataLength);
								handled = 1;
							  break;

						  }
						  break;
				  }
			  }
		  }
	  }
		if (Request->RequestType == 0) //host to device
		{
			if (Request->Request == USB_DEV_SET_CONFIGURATION)
			{
					DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Set configuration\n"));
					handled = 1;
			}
		}
	}

  if (!handled) {
		USB_OHCI_HC_DEV *Ehc;
		UINT64 addr; 
		UINT32 transfer;

		EFI_PHYSICAL_ADDRESS  BusPhysAddr;
		UINTN                 BusLength;
		VOID                  *Mapping;
		EFI_STATUS            Status;


		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer forward to (slow=%d) (MaxTransfer=%d) bus\n",IsSlowDevice,MaxPacketLength));
		Ehc = USB_OHCI_HC_DEV_FROM_THIS (This);

		BusLength = sizeof(EFI_USB_DEVICE_REQUEST);
		Status = Ehc->PciIo->Map(Ehc->PciIo, EfiPciIoOperationBusMasterRead, Request, &BusLength, &BusPhysAddr, &Mapping);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Usb: Map DMA failed\n"));
    }

		addr = BusPhysAddr;
		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer 1 DMAADDR=%X BUSADDR=%X\n",Request,addr));
		// Ek gad, really? Split each control transfer into its phases? (see dwc2_hc_init_xfer)
		// SETUP->DATA->STATUS
//		ASSERT(Request<32bit);
		WRITE_REG32(HCDMA(0), (UINT32)addr);
		transfer = TSIZ_SC_MC_PID_SETUP<<TSIZ_SC_MC_PID_SHIFT;
		transfer |= sizeof(EFI_USB_DEVICE_REQUEST)<<TSIZ_XFERSIZE_SHIFT;
		transfer |= 1 << TSIZ_PKTCNT_SHIFT;
		WRITE_REG32(HCTSIZ(0),transfer);



		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer 2\n"));
//		DwChannelInit(Ehc, DeviceAddress, 0 , EfiUsbDataIn, IsSlowDevice, USB_ENDPOINT_XFER_CONTROL, MaxPacketLength);
		DwChannelInit(Ehc, DeviceAddress, 0 , EfiUsbNoData, IsSlowDevice, USB_ENDPOINT_XFER_CONTROL, MaxPacketLength);
		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer 3\n"));
		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Status = %X\n",READ_REG32(HCINT(0))));
		while (READ_REG32(HCINT(0))==0); 
		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Status = %X pkt_remaining = %d \n",READ_REG32(HCINT(0)), READ_REG32(HCTSIZ(0))));
		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer 4\n"));

    Status = Ehc->PciIo->Unmap (Ehc->PciIo, Mapping);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Usb: failed to Unmap DMA\n"));
    }


		// word aligned buffer can go directly to HCDMA
		// packet count = 1, send 'Request'
		// wait for complete packet
		if (*DataLength)
		{
				// do the data phase.
				BusLength = *DataLength;
				Mapping = NULL;
				Status = Ehc->PciIo->Map(Ehc->PciIo, EfiPciIoOperationBusMasterCommonBuffer , Data, &BusLength, &BusPhysAddr, &Mapping);
				if (EFI_ERROR (Status)) {
						DEBUG ((EFI_D_ERROR, "Usb: Map DMA failed\n"));
				}
				
				addr = BusPhysAddr;

//				addr=(UINT64)Data;
				DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer 6 datap=%X\n",addr));
				WRITE_REG32(HCDMA(0), (UINT32)addr);
				transfer = TSIZ_SC_MC_PID_DATA0<<TSIZ_SC_MC_PID_SHIFT;
				transfer |= *DataLength<<TSIZ_XFERSIZE_SHIFT;
				transfer |= 1 << TSIZ_PKTCNT_SHIFT;
				WRITE_REG32(HCTSIZ(0),transfer);
		

				DwChannelInit(Ehc, DeviceAddress, 0 , TransferDirection, IsSlowDevice, USB_ENDPOINT_XFER_CONTROL, MaxPacketLength);
				DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Status = %X\n",READ_REG32(HCINT(0))));
				while (READ_REG32(HCINT(0))==0); 
				DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Status = %X pkt_remaining = %d \n",READ_REG32(HCINT(0)), READ_REG32(HCTSIZ(0))));
				Status = Ehc->PciIo->Unmap (Ehc->PciIo, Mapping);
				if (EFI_ERROR (Status)) {
						DEBUG ((EFI_D_ERROR, "Usb: failed to Unmap DMA\n"));
				}

		}

		// do the status phase
		// status flows in the oposite direction of data
		if (TransferDirection == EfiUsbDataIn) {
				TransferDirection = EfiUsbDataOut;
		}
		else	{
				TransferDirection = EfiUsbDataIn;
		}

		*TransferResult=0;

		BusLength = 4;
		Status = Ehc->PciIo->Map(Ehc->PciIo, EfiPciIoOperationBusMasterWrite , TransferResult , &BusLength, &BusPhysAddr, &Mapping);
		if (EFI_ERROR (Status)) {
				DEBUG ((EFI_D_ERROR, "Usb: Map DMA failed\n"));
		}
		
		addr = BusPhysAddr;

//		addr=(UINT64)TransferResult;
		WRITE_REG32(HCDMA(0), (UINT32)addr);
		transfer = TSIZ_SC_MC_PID_DATA1<<TSIZ_SC_MC_PID_SHIFT;
		transfer |= 0<<TSIZ_XFERSIZE_SHIFT;
		transfer |= 1 << TSIZ_PKTCNT_SHIFT;
		WRITE_REG32(HCTSIZ(0),transfer);
		DwChannelInit(Ehc, DeviceAddress, 0 , TransferDirection, IsSlowDevice, USB_ENDPOINT_XFER_CONTROL, MaxPacketLength);
		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Status = %X\n",READ_REG32(HCINT(0))));
		while (READ_REG32(HCINT(0))==0); 

		Status = Ehc->PciIo->Unmap (Ehc->PciIo, Mapping);
		if (EFI_ERROR (Status)) {
				DEBUG ((EFI_D_ERROR, "Usb: failed to Unmap DMA\n"));
		}

		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Status = %X pkt_remaining = %d \n",READ_REG32(HCINT(0)), READ_REG32(HCTSIZ(0))));
		DEBUG ((EFI_D_ERROR, "USB: OhciControlTransfer Status = %X tdata =%X\n",READ_REG32(HCINT(0)),*TransferResult ));
		// packet count = DataLength/Maxpacket (+1) send/recv 'Data'
		// wait from complete
		// (other direction for status)
		//DwChannelInit(Ehc, DeviceAddress, 0 , TransferDirection, IsSlowDevice, USB_ENDPOINT_XFER_CONTROL, MaxPacketLength);
  }

  return EFI_SUCCESS;
}


/**

  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an
                                endpoint direction of the target USB device.
                                Each endpoint address supports data transfer in
                                one direction except the control endpoint
                                (whose default endpoint address is 0).
                                It is the caller's responsibility to make sure that
                                the EndPointAddress represents a bulk endpoint.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            When input, indicates the size, in bytes, of the data buffer
                                specified by Data. When output, indicates the actually
                                transferred data size.
  @param  DataToggle            A pointer to the data toggle value. On input, it indicates
                                the initial data toggle value the bulk transfer should adopt;
                                on output, it is updated to indicate the data toggle value
                                of the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in microseconds, which the
                                transfer is allowed to complete.
  TransferResult                A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The bulk transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The bulk transfer could not be submitted due to lack of resource.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The bulk transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The bulk transfer failed due to host controller or device error.
                                Caller should check TranferResult for detailed error information.

**/


EFI_STATUS
EFIAPI
OhciBulkTransfer(
  IN     EFI_USB_HC_PROTOCOL  *This,
  IN     UINT8                DeviceAddress,
  IN     UINT8                EndPointAddress,
  IN     UINT8                MaxPacketLength,
  IN OUT VOID                 *Data,
  IN OUT UINTN                *DataLength,
  IN OUT UINT8                *DataToggle,
  IN     UINTN                TimeOut,
  OUT    UINT32               *TransferResult
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciBulkTransfer\n"));
	return EFI_SUCCESS;
}


/**

  Submits an interrupt transfer to an interrupt endpoint of a USB device.

  @param  Ohc                   Device private data
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device. Each endpoint address
                                supports data transfer in one direction except the
                                control endpoint (whose default endpoint address is 0).
                                It is the caller's responsibility to make sure that
                                the EndPointAddress represents an interrupt endpoint.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxPacketLength       Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  IsNewTransfer         If TRUE, an asynchronous interrupt pipe is built between
                                the host and the target interrupt endpoint.
                                If FALSE, the specified asynchronous interrupt pipe
                                is canceled.
  @param  DataToggle            A pointer to the data toggle value.  On input, it is valid
                                when IsNewTransfer is TRUE, and it indicates the initial
                                data toggle value the asynchronous interrupt transfer
                                should adopt.
                                On output, it is valid when IsNewTransfer is FALSE,
                                and it is updated to indicate the data toggle value of
                                the subsequent asynchronous interrupt transfer.
  @param  PollingInterval       Indicates the interval, in milliseconds, that the
                                asynchronous interrupt transfer is polled.
                                This parameter is required when IsNewTransfer is TRUE.
  @param  UCBuffer              Uncacheable buffer
  @param  DataLength            Indicates the length of data to be received at the
                                rate specified by PollingInterval from the target
                                asynchronous interrupt endpoint.  This parameter
                                is only required when IsNewTransfer is TRUE.
  @param  CallBackFunction      The Callback function.This function is called at the
                                rate specified by PollingInterval.This parameter is
                                only required when IsNewTransfer is TRUE.
  @param  Context               The context that is passed to the CallBackFunction.
                                This is an optional parameter and may be NULL.
  @param  IsPeriodic            Periodic interrupt or not
  @param  OutputED              The correspoding ED carried out
  @param  OutputTD              The correspoding TD carried out


  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/

EFI_STATUS
OhciInterruptTransfer (
  IN     USB_OHCI_HC_DEV                  *Ohc,
  IN     UINT8                            DeviceAddress,
  IN     UINT8                            EndPointAddress,
  IN     BOOLEAN                          IsSlowDevice,
  IN     UINT8                            MaxPacketLength,
  IN     BOOLEAN                          IsNewTransfer,
  IN OUT UINT8                            *DataToggle        OPTIONAL,
  IN     UINTN                            PollingInterval    OPTIONAL,
  IN     VOID                             *UCBuffer          OPTIONAL,
  IN     UINTN                            DataLength         OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK  CallBackFunction   OPTIONAL,
  IN     VOID                             *Context           OPTIONAL,
  IN     BOOLEAN                          IsPeriodic         OPTIONAL,
  OUT    VOID                             **OutputED         OPTIONAL,
  OUT    VOID                             **OutputTD         OPTIONAL
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciInterruptTransfer\n"));
	return EFI_SUCCESS;
}

/**

  Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device. Each endpoint address
                                supports data transfer in one direction except the
                                control endpoint (whose default endpoint address is 0).
                                It is the caller's responsibility to make sure that
                                the EndPointAddress represents an interrupt endpoint.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxiumPacketLength    Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  IsNewTransfer         If TRUE, an asynchronous interrupt pipe is built between
                                the host and the target interrupt endpoint.
                                If FALSE, the specified asynchronous interrupt pipe
                                is canceled.
  @param  DataToggle            A pointer to the data toggle value.  On input, it is valid
                                when IsNewTransfer is TRUE, and it indicates the initial
                                data toggle value the asynchronous interrupt transfer
                                should adopt.
                                On output, it is valid when IsNewTransfer is FALSE,
                                and it is updated to indicate the data toggle value of
                                the subsequent asynchronous interrupt transfer.
  @param  PollingInterval       Indicates the interval, in milliseconds, that the
                                asynchronous interrupt transfer is polled.
                                This parameter is required when IsNewTransfer is TRUE.
  @param  DataLength            Indicates the length of data to be received at the
                                rate specified by PollingInterval from the target
                                asynchronous interrupt endpoint.  This parameter
                                is only required when IsNewTransfer is TRUE.
  @param  CallBackFunction      The Callback function.This function is called at the
                                rate specified by PollingInterval.This parameter is
                                only required when IsNewTransfer is TRUE.
  @param  Context               The context that is passed to the CallBackFunction.
                                This is an optional parameter and may be NULL.

  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/


EFI_STATUS
EFIAPI
OhciAsyncInterruptTransfer (
  IN     EFI_USB_HC_PROTOCOL              *This,
  IN     UINT8                            DeviceAddress,
  IN     UINT8                            EndPointAddress,
  IN     BOOLEAN                          IsSlowDevice,
  IN     UINT8                            MaxPacketLength,
  IN     BOOLEAN                          IsNewTransfer,
  IN OUT UINT8                            *DataToggle        OPTIONAL,
  IN     UINTN                            PollingInterval    OPTIONAL,
  IN     UINTN                            DataLength         OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK  CallBackFunction   OPTIONAL,
  IN     VOID                             *Context           OPTIONAL
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciAsyncInterruptTransfer\n"));
	return EFI_SUCCESS;
}

/**

  Submits synchronous interrupt transfer to an interrupt endpoint
  of a USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device. Each endpoint
                                address supports data transfer in one direction
                                except the control endpoint (whose default
                                endpoint address is 0). It is the caller's responsibility
                                to make sure that the EndPointAddress represents
                                an interrupt endpoint.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxPacketLength       Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            On input, the size, in bytes, of the data buffer specified
                                by Data. On output, the number of bytes transferred.
  @param  DataToggle            A pointer to the data toggle value. On input, it indicates
                                the initial data toggle value the synchronous interrupt
                                transfer should adopt;
                                on output, it is updated to indicate the data toggle value
                                of the subsequent synchronous interrupt transfer.
  @param  TimeOut               Indicates the maximum time, in microseconds, which the
                                transfer is allowed to complete.
  @param  TransferResult        A pointer to the detailed result information from
                                the synchronous interrupt transfer.

  @retval EFI_UNSUPPORTED       This interface not available.
  @retval EFI_INVALID_PARAMETER Parameters not follow spec

**/


EFI_STATUS
EFIAPI
OhciSyncInterruptTransfer (
  IN     EFI_USB_HC_PROTOCOL  *This,
  IN     UINT8                DeviceAddress,
  IN     UINT8                EndPointAddress,
  IN     BOOLEAN              IsSlowDevice,
  IN     UINT8                MaxPacketLength,
  IN OUT VOID                 *Data,
  IN OUT UINTN                *DataLength,
  IN OUT UINT8                *DataToggle,
  IN     UINTN                TimeOut,
  OUT    UINT32               *TransferResult
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciSyncInterruptTransfer\n"));
	return EFI_SUCCESS;
}

/**

  Submits isochronous transfer to a target USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       End point address
  @param  MaximumPacketLength   Indicates the maximum packet size that the
                                default control transfer endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            Indicates the size, in bytes, of the data buffer
                                specified by Data.
  @param  TransferResult        A pointer to the detailed result information generated
                                by this control transfer.

  @retval EFI_UNSUPPORTED       This interface not available
  @retval EFI_INVALID_PARAMETER Data is NULL or DataLength is 0 or TransferResult is NULL

**/


EFI_STATUS
EFIAPI
OhciIsochronousTransfer (
  IN     EFI_USB_HC_PROTOCOL  *This,
  IN     UINT8                DeviceAddress,
  IN     UINT8                EndPointAddress,
  IN     UINT8                MaximumPacketLength,
  IN OUT VOID                 *Data,
  IN OUT UINTN                DataLength,
  OUT    UINT32               *TransferResult
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciIsochronousTransfer\n"));
  if (Data == NULL || DataLength == 0 || TransferResult == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_UNSUPPORTED;
}


/**

  Submits Async isochronous transfer to a target USB device.

  @param  his                   A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       End point address
  @param  MaximumPacketLength   Indicates the maximum packet size that the
                                default control transfer endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  IsochronousCallBack   When the transfer complete, the call back function will be called
  @param  Context               Pass to the call back function as parameter

  @retval EFI_UNSUPPORTED       This interface not available
  @retval EFI_INVALID_PARAMETER Data is NULL or Datalength is 0

**/

EFI_STATUS
EFIAPI
OhciAsyncIsochronousTransfer (
  IN     EFI_USB_HC_PROTOCOL                *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              MaximumPacketLength,
  IN OUT VOID                               *Data,
  IN OUT UINTN                              DataLength,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    IsochronousCallBack,
  IN     VOID                               *Context OPTIONAL
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciAsyncIsochronousTransfer\n"));
  if (Data == NULL || DataLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_UNSUPPORTED;
}

/**

  Retrieves the number of root hub ports.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  NumOfPorts            A pointer to the number of the root hub ports.

  @retval EFI_SUCCESS           The port number was retrieved successfully.
**/
EFI_STATUS
EFIAPI
OhciGetRootHubNumOfPorts (
  IN  EFI_USB_HC_PROTOCOL  *This,
  OUT UINT8                *NumOfPorts
  )
{
  DEBUG ((EFI_D_ERROR, "USB: OhciGetRootHubNumOfPorts\n"));
  //TODO, so apparently this devices root port can't be probed?
  // default to one port, and simulate it?
  *NumOfPorts = 1;
  return EFI_SUCCESS;
}

#define MAXPORTS 4;

/**

  Retrieves the current status of a USB root hub port.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL.
  @param  PortNumber            Specifies the root hub port from which the status
                                is to be retrieved.  This value is zero-based. For example,
                                if a root hub has two ports, then the first port is numbered 0,
                                and the second port is numbered 1.
  @param  PortStatus            A pointer to the current port status bits and
                                port status change bits.

  @retval EFI_SUCCESS           The status of the USB root hub port specified by PortNumber
                                was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER Port number not valid
**/


EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
  IN  EFI_USB_HC_PROTOCOL  *This,
  IN  UINT8                PortNumber,
  OUT EFI_USB_PORT_STATUS  *PortStatus
  )
{
  USB_OHCI_HC_DEV           *Ehc;

//	DEBUG ((EFI_D_ERROR, "USB: OhciGetRootHubPortStatus %d\n",PortNumber));
  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Ehc = USB_OHCI_HC_DEV_FROM_THIS (This);

	//TODO more faked status?
	// The HPRT register contains the host status
	// indicating speed/power etc
	PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;
	PortStatus->PortStatus       |= USB_PORT_STAT_LOW_SPEED;
	PortStatus->PortStatus       |= USB_PORT_STAT_ENABLE;
	PortStatus->PortStatus       |= USB_PORT_STAT_POWER;
	PortStatus->PortStatus       |= USB_PORT_STAT_CONNECTION;

	// on first status (reset/whatever) describe a new connection
  if (Ehc->PortStatus) {
		DEBUG ((EFI_D_ERROR, "USB: OhciGetRootHubPortStatus %d change to connected\n",PortNumber));
		DEBUG ((EFI_D_ERROR, "USB: OhciGetRootHubPortStatus actual hport status %X\n",READ_REG32(HPRT)));
		Ehc->PortStatus = 0;
		PortStatus->PortChangeStatus |= USB_PORT_STAT_CONNECTION;
	}

  return EFI_SUCCESS;
}

/**

  Sets a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be set.
  @param  PortFeature           Indicates the feature selector associated
                                with the feature set request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set for the
                                USB root hub port specified by PortNumber.
  @retval EFI_DEVICE_ERROR      Set feature failed because of hardware issue
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
**/
EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
  IN EFI_USB_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
	DEBUG ((EFI_D_ERROR, "USB: OhciSetRootHubPortFeature port(%d) -> %d\n",PortNumber,PortFeature));
  switch (PortFeature)	{
			case EfiUsbPortReset:
					DEBUG ((EFI_D_ERROR, "USB: OhciSetRootHubPortFeature reset\n"));
					break;
			case EfiUsbPortEnable:
					DEBUG ((EFI_D_ERROR, "USB: OhciSetRootHubPortFeature enable\n"));
					break;
			default:
					DEBUG ((EFI_D_ERROR, "USB: OhciSetRootHubPortFeature (other)\n"));
	}
	return EFI_SUCCESS;
}


/**

  Clears a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared for the
                                USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Some error happened when clearing feature
**/
EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
  IN EFI_USB_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
	DEBUG ((EFI_D_ERROR, "USB: OhciClearRootHubPortFeature port(%d) -> %d\n",PortNumber,PortFeature));
	return EFI_SUCCESS;
}

USB_OHCI_HC_DEV *
OhciAllocateDev (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  USB_OHCI_HC_DEV         *Ohc;
//  EFI_STATUS              Status;
//  VOID                    *Buf;
//  EFI_PHYSICAL_ADDRESS    PhyAddr;
//  VOID                    *Map;
  UINTN                   Pages;
  UINTN                   Bytes;

  Ohc = AllocateZeroPool (sizeof (USB_OHCI_HC_DEV));
  if (Ohc == NULL) {
    return NULL;
  }

  Ohc->Signature                      = USB_DW_HC_DEV_SIGNATURE;
  Ohc->PciIo                          = PciIo;

  Ohc->UsbHc.Reset                    = OhciReset;
  Ohc->UsbHc.GetState                 = OhciGetState;
  Ohc->UsbHc.SetState                 = OhciSetState;
  Ohc->UsbHc.ControlTransfer          = OhciControlTransfer;
  Ohc->UsbHc.BulkTransfer             = OhciBulkTransfer;
  Ohc->UsbHc.AsyncInterruptTransfer   = OhciAsyncInterruptTransfer;
  Ohc->UsbHc.SyncInterruptTransfer    = OhciSyncInterruptTransfer;
  Ohc->UsbHc.IsochronousTransfer      = OhciIsochronousTransfer;
  Ohc->UsbHc.AsyncIsochronousTransfer = OhciAsyncIsochronousTransfer;
  Ohc->UsbHc.GetRootHubPortNumber     = OhciGetRootHubNumOfPorts;
  Ohc->UsbHc.GetRootHubPortStatus     = OhciGetRootHubPortStatus;
  Ohc->UsbHc.SetRootHubPortFeature    = OhciSetRootHubPortFeature;
  Ohc->UsbHc.ClearRootHubPortFeature  = OhciClearRootHubPortFeature;
  Ohc->UsbHc.MajorRevision            = 0x1;
  Ohc->UsbHc.MinorRevision            = 0x1;

//  Ohc->HccaMemoryBlock = NULL;
  Ohc->HccaMemoryMapping   = NULL;
  Ohc->HccaMemoryBuf = NULL;
  Ohc->HccaMemoryPages = 0;
//  Ohc->InterruptContextList = NULL;
  Ohc->ControllerNameTable = NULL;
  Ohc->HouseKeeperTimer = NULL;

/*  Ohc->MemPool = UsbHcInitMemPool(PciIo, TRUE, 0);
  if(Ohc->MemPool == NULL) {
    goto FREE_DEV_BUFFER;
	}*/

  Bytes = 4096;
  Pages = EFI_SIZE_TO_PAGES (Bytes);

/*  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    Pages,
                    &Buf,
                    0
                    );

  if (EFI_ERROR (Status)) {
    goto FREE_MEM_POOL;
  }

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buf,
                    &Bytes,
                    &PhyAddr,
                    &Map
                    );

  if (EFI_ERROR (Status) || (Bytes != 4096)) {
    goto FREE_MEM_PAGE;
  }*/

//  Ohc->HccaMemoryBlock = (HCCA_MEMORY_BLOCK *)(UINTN)PhyAddr;
//  Ohc->HccaMemoryMapping = Map;
//  Ohc->HccaMemoryBuf = (VOID *)(UINTN)Buf;
  Ohc->HccaMemoryPages = Pages;
  Ohc->PortStatus = 1;

  return Ohc;

//FREE_MEM_PAGE:
//  PciIo->FreeBuffer (PciIo, Pages, Buf);
//FREE_MEM_POOL:
//  UsbHcFreeMemPool (Ohc->MemPool);
//FREE_DEV_BUFFER:
//  FreePool(Ohc);

  return NULL;
}

/**

  One notified function to stop the Host Controller when gBS->ExitBootServices() called.

  @param  Event                 Pointer to this event
  @param  Context               Event handler private data
**/
VOID
EFIAPI
DwHcExitBootService (
  EFI_EVENT                      Event,
  VOID                           *Context
  )
{
  DEBUG ((EFI_D_ERROR, "DwUsbExitBootService!\n"));
}


#pragma pack(1)
typedef struct {
  UINT8               ProgInterface;
  UINT8               SubClassCode;
  UINT8               BaseCode;
} USB_CLASSC;
#pragma pack()
//typedef struct _USB2_HC_DEV  USB2_HC_DEV;

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has Usb2HcProtocol installed will
  be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
DwUsbDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS              Status;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  USB_CLASSC              UsbClassCReg;

//  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingSupported\n"));
  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (USB_CLASSC) / sizeof (UINT8),
                        &UsbClassCReg
                        );

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to Usb class type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) || (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB)) {
    Status = EFI_UNSUPPORTED;
  }
  else  {
	  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingSupported found board!\n"));
  }

ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Starting the Usb EHCI Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          supports this device.
  @return EFI_UNSUPPORTED      do not support this device.
  @return EFI_DEVICE_ERROR     cannot be started due to device Error.
  @return EFI_OUT_OF_RESOURCES cannot allocate resources.

**/
EFI_STATUS
EFIAPI
DwUsbDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
//  USB2_HC_DEV               *Ehc;
  USB_OHCI_HC_DEV           *Ehc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath;

  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart \n"));
  //
  // Open the PciIo Protocol, then enable the USB host controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart2 \n"));
  //
  // Open Device Path Protocol for on USB host controller
  //
  HcDevicePath = NULL;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &HcDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart3 \n"));
  //
  // Create then install USB2_HC_PROTOCOL
  //
//  Ehc = EhcCreateUsb2Hc (PciIo, HcDevicePath, OriginalPciAttributes);
//  Ehc = EhcCreateUsb2Hc (PciIo, HcDevicePath, 0);
  Ehc = OhciAllocateDev(PciIo);
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart4 \n"));
  if (Ehc == NULL) {
    DEBUG ((EFI_D_ERROR, "EhcDriverBindingStart: failed to create USB2_HC\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_PCIIO;
  }
  Status = gBS->InstallProtocolInterface (
                  &Controller,
//                  &gEfiUsb2HcProtocolGuid,
                  &gEfiUsbHcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Ehc->UsbHc
                  );
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart5 \n"));
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcDriverBindingStart: failed to install USB2_HC Protocol\n"));
    goto FREE_POOL;
  }
  //
  // Start the asynchronous interrupt monitor
  //
/*  Status = gBS->SetTimer (Ehc->PollTimer, TimerPeriodic, EHC_ASYNC_POLL_INTERVAL);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcDriverBindingStart: failed to start async interrupt monitor\n"));

    EhcHaltHC (Ehc, EHC_GENERIC_TIMEOUT);
    goto UNINSTALL_USBHC;
  }
*/
  //
  // Create event to stop the HC when exit boot service.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DwHcExitBootService,
                  Ehc,
                  &gEfiEventExitBootServicesGuid,
                  &Ehc->ExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_USBHC;
  }

  //
  // Install the component name protocol, don't fail the start
  // because of something for display.
  //
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart6 \n"));
  AddUnicodeString2 (
    "eng",
    gOhciComponentName.SupportedLanguages,
    &Ehc->ControllerNameTable,
    L"Designware Host Controller (USB 2.0)",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gOhciComponentName2.SupportedLanguages,
    &Ehc->ControllerNameTable,
    L"Designware Host Controller (USB 2.0)",
    FALSE
    );
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart7 \n"));
  phy_init(Ehc); //TODO these need the pci protocol so they talk to the right device...
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart8 \n"));
	DwHostInit(Ehc); 

UNINSTALL_USBHC:
CLOSE_PCIIO:
FREE_POOL:

  DEBUG ((EFI_D_INFO, "EhcDriverBindingStart: EHCI started for controller @ %p\n", Controller));
  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStart9 \n"));
  return EFI_SUCCESS;

}

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS          Success.
  @return EFI_DEVICE_ERROR     Fail.

**/
EFI_STATUS
EFIAPI
DwUsbDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  EFI_USB2_HC_PROTOCOL  *Usb2Hc;
//  EFI_PCI_IO_PROTOCOL   *PciIo;
  USB_OHCI_HC_DEV       *Ehc;

  DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStop \n"));
  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  Status = gBS->OpenProtocol (
                  Controller,
//                  &gEfiUsb2HcProtocolGuid,
                  &gEfiUsbHcProtocolGuid,
                  (VOID **) &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

//  Ehc   = EHC_FROM_THIS (Usb2Hc);
  Ehc = USB_OHCI_HC_DEV_FROM_THIS (This);
//  PciIo = Ehc->PciIo;

  Status = gBS->UninstallProtocolInterface (
                  Controller,
//                  &gEfiUsb2HcProtocolGuid,
                  &gEfiUsbHcProtocolGuid,
                  Usb2Hc
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Stop AsyncRequest Polling timer then stop the EHCI driver
  // and uninstall the EHCI protocl.
  //
/*  gBS->SetTimer (Ehc->PollTimer, TimerCancel, EHC_ASYNC_POLL_INTERVAL);
  EhcHaltHC (Ehc, EHC_GENERIC_TIMEOUT);

  if (Ehc->PollTimer != NULL) {
    gBS->CloseEvent (Ehc->PollTimer);
  }
*/
  if (Ehc->ExitBootServiceEvent != NULL) {
    gBS->CloseEvent (Ehc->ExitBootServiceEvent);
  }
/*
  EhcFreeSched (Ehc);

  if (Ehc->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (Ehc->ControllerNameTable);
  }
*/
  //
  // Disable routing of all ports to EHCI controller, so all ports are 
  // routed back to the UHCI or OHCI controller.
  //
  /*EhcClearOpRegBit (Ehc, EHC_CONFIG_FLAG_OFFSET, CONFIGFLAG_ROUTE_EHC);

  //
  // Restore original PCI attributes
  //
  PciIo->Attributes (
                  PciIo,
                  EfiPciIoAttributeOperationSet,
                  Ehc->OriginalPciAttributes,
                  NULL
                  );
  */
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (Ehc);

  return EFI_SUCCESS;
}



EFI_DRIVER_BINDING_PROTOCOL
gOhciDriverBinding = {
  DwUsbDriverBindingSupported,
  DwUsbDriverBindingStart,
  DwUsbDriverBindingStop,
  0x30,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
DwUsbEntryPoint (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  DEBUG ((EFI_D_ERROR, "DwUsbEntryPoint \n"));
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gOhciDriverBinding,
           ImageHandle,
           &gOhciComponentName,
           &gOhciComponentName2
           );
}
