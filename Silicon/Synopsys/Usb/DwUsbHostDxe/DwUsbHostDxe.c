/** @file

    Copyright (c) 2015, Linaro Limited. All rights reserved.
    Copyright (c) 2015, Hisilicon Limited. All rights reserved.
	Copyright (c) 2017, Jeremy Linton

    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


// Some notes about this driver:
//
//  First as it stands all transfers are routed through the "nonperiodic" queues
//  including interrupt transfers. This means we emulate interrupt transfers using
//  the UEFI timer and a queue. All the fancy hardware scheduling is unused.
//
//  This also means that there is absolutely no prioritization, bandwidth reservations
//  or anything else that are generally required of interrupt/isochronous transfers.
//
//  We no round robin the channels, this in theory allows us to have multiple commands
//  outstanding, but due to the fact that we are either using a timer and mostly
//  blocking thta timer during the command submission, its really unlikely that we 
//  have more than a single command outstanding.
//
//  At the moment this is host only driver, and the phy config, which should be attached
//  to the DW i2c channel likely isn't. 
//
//  This driver defaults to High speed, but if we detect split transaction problems with
//  low/full speed devices then we reset the root port and everything attached, and restart
//  at a lower speed. This allows us to transparently support high/full/low speed devices. 
//  That said, it seems low speed devices don't really work worth a darn on this controller. 
//  There are assorted knobs that can be tiwiddled to tweak the low speed bitstream, but
//  my USB analyzer is truly just a protocol analyzer and is unable to tell me how far out
//  of spec the physical singaling is, and I'm currently to lazy to try an analyze it 
//  with my scope/logic analyzer.
//

#include "DwUsbHostDxe.h"
#include "DwcHw.h"

// Enabling this, disables attempting split transactions, and 
// sets the initial root port to full speed (rather than high)
// This option is less useful now that we detect full/low speed
// devices having split transaction problems and automatically 
// reset to full speed
#define DW_AT_FULLSPEED 0

// Forcing host mode may allow HS/FS/LS devices? Nah, doesn't 
// appear that way on the HiSi, but do it anyway for the time.
#define DW_FORCE_HOST   1 

// This timer controls how frequently we wake up to service
// interrupt transfers (in milliseconds). 
#define DW_USB_POLL_TIMER 50

// timeout for interrupt transfers
#define DW_USB_POLL_INTERRUPT   100 //100 ms
// We delay ops for a few poll intervals following bulk transfers
// in a lame (non USB standard) way to avoid burning bandwidth
// on interrupt transfers if bulk/setup is active. This also
// avoids colliding interrupts with bulk transfers although this
// seems to work fine now.
#define POLL_DELAY 10


VOID DwHcInit (IN DWUSB_OTGHC_DEV *DwHc);
VOID DwCoreInit (IN DWUSB_OTGHC_DEV *DwHc);


#define HI6220_PERI_BASE         0xF7030000
#define PERI_SC_PERIPH_CLK0_EN	 0x200
#define PERI_SC_PERIPH_CLK0_DIS	 0x204
#define PERI_SC_PERIPH_CLK0_STAT 0x208
/* this routine is the only really HiKey specific routine
   in the module, replace it with appropriate calls elsewhere */
VOID
ConfigureUsbPhy (
    VOID
    )
{
    UINT32 Data;
    DEBUG ((EFI_D_VERBOSE, "ConfigureUsbPhy \n",__func__));
    /*Enable USB clock*/
    Data = MmioRead32 (HI6220_PERI_BASE + PERI_SC_PERIPH_CLK0_EN);
    Data |= 0x10;
    MmioWrite32 (HI6220_PERI_BASE + PERI_SC_PERIPH_CLK0_EN, 0x10);

    do {
        Data = MmioRead32 (HI6220_PERI_BASE + PERI_SC_PERIPH_CLK0_STAT);
    } while ((Data & 0x10) == 0);

    /*Take USB IPs out of reset*/
    MmioWrite32 (0xF7030000 + 0x304, 0xF0);

    do {
        Data = MmioRead32 (HI6220_PERI_BASE + 0x308);
        Data &= 0xF0;
    } while (Data);

    /*CTRL5*/
    Data = MmioRead32 (HI6220_PERI_BASE + 0x010);
    Data &= ~0x20;
    Data |= 0x318;
    MmioWrite32 (HI6220_PERI_BASE + 0x010, Data);

    /*CTRL4*/
    /*Configure USB PHY*/
    Data = MmioRead32 (HI6220_PERI_BASE + 0x00C);

    /*make PHY out of low power mode*/
    Data &= ~0x40;
    Data &= ~0x100;
    Data |= 0xC00;
    Data &= ~0x200000;
    MmioWrite32 (HI6220_PERI_BASE + 0x00C, Data);

    MmioWrite32 (HI6220_PERI_BASE + 0x018, 0x70533483); //EYE_PATTERN

    MicroSecondDelay (5000);
}

UINT32
Wait4Bit (
    IN UINT32     Reg,
    IN UINT32     Mask,
    IN BOOLEAN    Set
    )
{
    UINT32  Timeout = DW_USB_POLL_INTERRUPT;
    UINT32  Value;
    DEBUG ((EFI_D_VERBOSE, "Wait4Bit \n",__func__));

    while (--Timeout) {
        Value = MmioRead32 (Reg);
        if (!Set)
            Value = ~Value;

        if ((Value & Mask) == Mask) {
            DEBUG ((EFI_D_VERBOSE, "Wait4Bit val=%X return\n",Value));
            return 0;
        }

        MicroSecondDelay (1);
    }

//    DEBUG ((EFI_D_ERROR, "Wait4Bit: Timeout (Reg:0x%x, mask:0x%x, wait_set:%d, val:0x%X)\n", Reg, Mask, Set, Value));

    return 1;
}


UINT32
Wait4AnyBit (
    IN UINT32     Reg,
    IN UINT32     Mask,
	IN UINT32     MaskFail,
	IN UINT32     Timeout // in milliseconds
    )
{
    UINT32  Value;
	Timeout *= 1000; // we want this in microseconds
    DEBUG ((EFI_D_VERBOSE, "Wait4AnyBit \n"));

    while (--Timeout) {
        Value = MmioRead32 (Reg);

        if (Value & Mask) {
            DEBUG ((EFI_D_VERBOSE, "Wait4AnyBit val=%X return\n",Value));
            return 0;
        }
		// fail early
		if ((Timeout < DW_USB_POLL_INTERRUPT/4) & (Value & MaskFail)) {
			DEBUG ((EFI_D_ERROR, "Wait4AnyBit val=%X early fail return\n",Value));
			return 0;
		}

        MicroSecondDelay (1);
    }

    DEBUG ((EFI_D_VERBOSE, "Wait4AnyBit: Timeout (Reg:0x%x, mask:0x%x, val:0x%X)\n", Reg, Mask, Value));

    return 1;
}


// The Num parameter corresponds to the FIFO being flushed
//  0000 Non periodic TX FIFO in host mode
//  0001 Periodic FIFO host mode
//  xxx0 Device mode FIFO
// 10000 Flush all transmit FIFOs
VOID
DwFlushTxFifo (
    IN DWUSB_OTGHC_DEV *DwHc,
    IN INT32 Num
    )
{
    UINT32 Status;
    DEBUG ((EFI_D_VERBOSE, "DwFlushTxFifo \n",__func__));
    MmioWrite32 (DwHc->DwUsbBase + GRSTCTL, DWC2_GRSTCTL_TXFFLSH | (Num << DWC2_GRSTCTL_TXFNUM_OFFSET));

    Status = Wait4Bit (DwHc->DwUsbBase + GRSTCTL, DWC2_GRSTCTL_TXFFLSH, 0);
    if (Status)
        DEBUG ((EFI_D_ERROR, "DwFlushTxFifo: Timeout!\n"));

    MicroSecondDelay (1);
}

VOID
DwFlushRxFifo (
    IN DWUSB_OTGHC_DEV *DwHc
    )
{
    UINT32 Status;
    DEBUG ((EFI_D_VERBOSE, "DwFlushRxFifo \n",__func__));
    MmioWrite32 (DwHc->DwUsbBase + GRSTCTL, DWC2_GRSTCTL_RXFFLSH);

    Status = Wait4Bit (DwHc->DwUsbBase + GRSTCTL, DWC2_GRSTCTL_RXFFLSH, 0);
    if (Status)
        DEBUG ((EFI_D_ERROR, "DwFlushRxFifo: Timeout!\n"));

    MicroSecondDelay (1);
}

void DwUsbAttemptClear(
	IN DWUSB_OTGHC_DEV  *DwHc,
	IN UINT32   Channel
	)
{

	MmioWrite32 (DwHc->DwUsbBase + HCINT(Channel), 0x3FFF);		
	// disable channel
	MmioAndThenOr32 (DwHc->DwUsbBase + HCCHAR(Channel),
					 ~(DWC2_HCCHAR_CHEN | DWC2_HCCHAR_EPDIR),
					 DWC2_HCCHAR_CHDIS);

    DwFlushTxFifo (DwHc, 0x00); //flush non periodic queue
    DwFlushRxFifo (DwHc);

    MicroSecondDelay (50000);	

	MmioWrite32 (DwHc->DwUsbBase + HCINT(Channel), 0x3FFF);		

	MmioAndThenOr32 (DwHc->DwUsbBase + HCCHAR(Channel),
					 ~(DWC2_HCCHAR_EPDIR|DWC2_HCCHAR_CHDIS),
					 DWC2_HCCHAR_CHEN|DWC2_HCCHAR_CHDIS);
	Wait4Bit (DwHc->DwUsbBase + HCCHAR(Channel), DWC2_HCCHAR_CHEN, 0);
}


UINT32
Wait4Chhltd (
    IN DWUSB_OTGHC_DEV  *DwHc,
	IN UINT32   Channel,
    IN UINT32   *Sub,
    IN UINT32   *Toggle,
    IN BOOLEAN   IgnoreAck,
	IN BOOLEAN   IgnoreComplete,
	IN UINT32    Timeout
    )
{
    UINT32  HcintCompHltAck = DWC2_HCINT_XFERCOMP | DWC2_HCINT_CHHLTD;
    INT32 Ret;
    UINT32  Hcint, Hctsiz;
    DEBUG ((EFI_D_VERBOSE, "Wait4Chhltd \n",__func__));

    MicroSecondDelay (100);
//    Ret = Wait4Bit (DwHc->DwUsbBase + HCINT(Channel), DWC2_HCINT_CHHLTD, 1); //wait only for channel halt, cosider waiting for NAK/STALL too....

//    Ret = Wait4AnyBit (DwHc->DwUsbBase + HCINT(Channel), DWC2_HCINT_CHHLTD|DWC2_HCINT_XFERCOMP, DWC2_HCINT_NAK|DWC2_HCINT_STALL); //Ok really the sequence should be wait for complete
	// then halt the channel and wait for channel halt
	Ret = Wait4AnyBit (DwHc->DwUsbBase + HCINT(Channel), DWC2_HCINT_CHHLTD, DWC2_HCINT_NAK|DWC2_HCINT_STALL, Timeout); //for whatever reason just waiting for halt tends to work

    MicroSecondDelay (100);
    Hcint = MmioRead32 (DwHc->DwUsbBase + HCINT(Channel));
    if (Hcint & DWC2_HCINT_STALL) {
        DEBUG ((EFI_D_ERROR, "Wait4Chhltd: STALL Hcint=%X\n", Hcint));
		DwUsbAttemptClear(DwHc, Channel);
        return EFI_USB_ERR_STALL;
    }
    if (Hcint & DWC2_HCINT_BBLERR)  {
        DEBUG ((EFI_D_ERROR, "Wait4Chhltd: BABBLE Hcint=%X\n", Hcint));
        return EFI_USB_ERR_BABBLE;
    }   
    if (Hcint & DWC2_HCINT_FRMOVRUN) {
        DEBUG ((EFI_D_VERBOSE, "Wait4Chhltd: Overrun Hcint=%X\n", Hcint));
        return EFI_USB_ERR_SYSTEM+1;
    }

    if (Hcint & DWC2_HCINT_XACTERR) {
        DEBUG ((EFI_D_ERROR, "Wait4Chhltd: CRC/bitstuff/etc error Hcint=%X\n", Hcint));
        return EFI_USB_ERR_CRC;
    }

    if (Hcint & DWC2_HCINT_NAK) {
        //NAKs are part of the normal interrupt protocol
        DEBUG ((EFI_D_VERBOSE, "Wait4Chhltd: NAK Hcint=%X\n", Hcint)); 
        return EFI_USB_ERR_NAK;
    }

    if (Hcint & DWC2_HCINT_DATATGLERR) {
        DEBUG ((EFI_D_ERROR, "Wait4Chhltd: DATA toggle Hcint=%X\n", Hcint));
        return EFI_USB_ERR_SYSTEM; //what is the correct return? 
    }

    // Moved this down here so that NAK & other error conditions are 
    // handled before timeout
    if (Ret) {
        DEBUG ((EFI_D_VERBOSE, "Wait4Chhltd ret\n"));
		DwUsbAttemptClear(DwHc, Channel);
        return EFI_USB_ERR_TIMEOUT;
    }

	// A happy transfer is halted/complete/acked
	// an ignore ack transaction is just complete/halted
    if (IgnoreAck) {
        Hcint &= ~DWC2_HCINT_ACK;
    }
    else {
        HcintCompHltAck |= DWC2_HCINT_ACK;
    }
	if (IgnoreComplete)	{
		Hcint |= DWC2_HCINT_XFERCOMP;
	}

    if (Hcint != HcintCompHltAck) {
        DEBUG ((EFI_D_ERROR, "Wait4Chhltd: HCINT Error 0x%x\n", Hcint));
        return EFI_USB_ERR_STALL; //JL take this out to unstick device?
    }

    Hctsiz = MmioRead32 (DwHc->DwUsbBase + HCTSIZ(Channel));
    *Sub = (Hctsiz & DWC2_HCTSIZ_XFERSIZE_MASK) >> DWC2_HCTSIZ_XFERSIZE_OFFSET;
    *Toggle = (Hctsiz & DWC2_HCTSIZ_PID_MASK) >> DWC2_HCTSIZ_PID_OFFSET;

    return EFI_USB_NOERROR;
}

VOID
DwOtgHcInit (
    IN DWUSB_OTGHC_DEV  *DwHc,
    IN UINT8    Channel,
    IN UINT8    DevAddr,
    IN UINT8    Endpoint,
    IN UINT8    EpDir,
    IN UINT8    EpType,
    IN UINT16   MaxPacket,
	IN UINT8    DeviceSpeed,
	IN UINT8    TtPort,
	IN UINT8    TtHub,
	IN UINT8    DoComplete
    )
{
	UINT32 Split = 0;
    UINT32  Hcchar = (DevAddr << DWC2_HCCHAR_DEVADDR_OFFSET) |
        (Endpoint << DWC2_HCCHAR_EPNUM_OFFSET) |
        (EpDir << DWC2_HCCHAR_EPDIR_OFFSET) |
        (EpType << DWC2_HCCHAR_EPTYPE_OFFSET) |
        (MaxPacket << DWC2_HCCHAR_MPS_OFFSET);

	if ((DeviceSpeed != EFI_USB_SPEED_HIGH) && (!DwHc->AtFullSpeed))
	{
		DEBUG ((EFI_D_ERROR, "Split Message to %d speed device (%d,%d)  \n", DeviceSpeed, TtPort, TtHub));
		Split = TtPort | (TtHub << DWC2_HCSPLT_HUBADDR_OFFSET);
		Split |= DWC2_HCSPLT_XACTPOS_ALL << DWC2_HCSPLT_XACTPOS_OFFSET; //do all packets? Or just the data payload?
//		Split |= DWC2_HCSPLT_XACTPOS_BEGIN << DWC2_HCSPLT_XACTPOS_OFFSET;
		Split |= DoComplete?DWC2_HCSPLT_COMPSPLT:0;
		//Split |= DWC2_HCSPLT_COMPSPLT; //do a complete split transaction? (docs aren't 100% clear on what this means, although maybe its because i'm in DMA mode (hint that splits may not work with DMA)
		Split |= DWC2_HCSPLT_SPLTENA;
		// Do we need to set lowspeed? I'm guessing this is only when the root port is full/low
		// because the "speed" of a split transaction is still high to the TT..
		Hcchar |= DWC2_HCCHAR_LSPDDEV;
	} 
	else if (DeviceSpeed == EFI_USB_SPEED_LOW)
	{
		Hcchar |= DWC2_HCCHAR_LSPDDEV;
	}

    DEBUG ((EFI_D_VERBOSE, "DwOtgHcInit \n",__func__));

    MmioWrite32 (DwHc->DwUsbBase + HCINT(Channel), 0x3FFF);

    MmioWrite32 (DwHc->DwUsbBase + HCCHAR(Channel), Hcchar);

    MmioWrite32 (DwHc->DwUsbBase + HCSPLT(Channel), Split); 
}

VOID
DwCoreReset (
    IN DWUSB_OTGHC_DEV *DwHc
    )
{
    UINT32  Status;
    DEBUG ((EFI_D_VERBOSE, "DwCoreReset \n",__func__));

    Status = Wait4Bit (DwHc->DwUsbBase + GRSTCTL, DWC2_GRSTCTL_AHBIDLE, 1);
    if (Status)
        DEBUG ((EFI_D_ERROR, "DwCoreReset: Timeout!\n"));

    MmioWrite32 (DwHc->DwUsbBase + GRSTCTL, DWC2_GRSTCTL_CSFTRST);

    Status = Wait4Bit (DwHc->DwUsbBase + GRSTCTL, DWC2_GRSTCTL_CSFTRST, 0);
    if (Status)
        DEBUG ((EFI_D_ERROR, "DwCoreReset: Timeout!\n"));

    MicroSecondDelay (100000);
}

EFI_STATUS
DwHcTransfer (
    IN     DWUSB_OTGHC_DEV  *DwHc,
    IN     UINT8      DeviceAddress,
    IN     UINTN      MaximumPacketLength,
    IN OUT UINT32     *Pid,
    IN     UINT32     TransferDirection,
    IN OUT VOID       *Data,
    IN OUT UINTN      *DataLength,
    IN     UINT32     EpAddress,
    IN     UINT32     EpType,
    OUT    UINT32     *TransferResult,
    IN     BOOLEAN    IgnoreAck,
    IN     BOOLEAN    DoPing,
	IN     UINT8      DeviceSpeed,
	IN     UINT8      TtPort,
	IN     UINT8      TtHub,
	IN     UINT8      IgnoreComplete,
	IN     UINT32     Timeout
    )
{
    UINT32                          TxferLen;
    UINTN                           BufferLen;
    UINT32                          Done = 0;
    UINT32                          NumPackets;
    UINT32                          Sub;
    UINT32                          StopTransfer = 0;
    EFI_STATUS                      Status = EFI_SUCCESS;
	UINT32                          SplitDone = 0;
	EFI_PHYSICAL_ADDRESS            BusPhysAddr;
	VOID                            *Mapping = NULL;
	// picking a channel based on EP works in this driver but doesn't seem to afford andy
	// advantages (at the moment).
//	UINT32                          Channel = (EpAddress & 0x7F) % DWC2_MAX_CHANNELS;
	EFI_TPL                         OriginalTPL;				
//	UINT32                          Channel = DWC2_HC_CHANNEL; //just use channel 0 for everything
//	UINT32                          Channel = 0;

	static UINT32                   StaticChannel = 0;
	UINT32                          Channel = (StaticChannel++) % DWC2_MAX_CHANNELS;
	UINT32                          ChannelRetries = DWC2_MAX_CHANNELS;

	OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);    
	for (ChannelRetries = 0 ; ChannelRetries < DWC2_MAX_CHANNELS; ChannelRetries++)
	{
		if (DwHc->ActiveChannel[Channel])
		{
			Channel++;
			DEBUG ((EFI_D_ERROR, "DwHcTransfer: Channel Collision\n"));
		}
		else
		{
			DwHc->ActiveChannel[Channel] = TRUE;
			break;
		}
	}
	gBS->RestoreTPL (OriginalTPL);

	if (ChannelRetries == DWC2_MAX_CHANNELS)
	{
		DEBUG ((EFI_D_ERROR, "DwHcTransfer: Out of channels\n"));
	}
 	DEBUG ((EFI_D_VERBOSE, "DwHcTransfer channel=%d\n",Channel));


	OriginalTPL = gBS->RaiseTPL(TPL_NOTIFY);

    do {
        DwOtgHcInit (DwHc, Channel, DeviceAddress, EpAddress, TransferDirection, EpType, MaximumPacketLength, DeviceSpeed, TtPort, TtHub, SplitDone);

        TxferLen = *DataLength - Done;
        if (TxferLen > DWC2_MAX_TRANSFER_SIZE)
            TxferLen = DWC2_MAX_TRANSFER_SIZE - MaximumPacketLength + 1;
        if (TxferLen > DWC2_DATA_BUF_SIZE)
            TxferLen = DWC2_DATA_BUF_SIZE - MaximumPacketLength + 1;

        if (TxferLen > 0) {
            NumPackets = (TxferLen + MaximumPacketLength - 1) / MaximumPacketLength;
            if (NumPackets > DWC2_MAX_PACKET_COUNT) {
                NumPackets = DWC2_MAX_PACKET_COUNT;
                TxferLen = NumPackets * MaximumPacketLength;
            }
        } else {
            NumPackets = 1;
        }

        if (TransferDirection)
            TxferLen = NumPackets * MaximumPacketLength;

        MmioWrite32 (DwHc->DwUsbBase + HCTSIZ(Channel), (TxferLen << DWC2_HCTSIZ_XFERSIZE_OFFSET) |
                     (NumPackets << DWC2_HCTSIZ_PKTCNT_OFFSET) | (DoPing << DWC2_HCTSIZ_DOPNG_OFFSET) |
                     (*Pid << DWC2_HCTSIZ_PID_OFFSET));

		DEBUG ((EFI_D_VERBOSE, "Usb: dma (%d) size %d %d\n",TransferDirection, *DataLength, TxferLen));

		if (TransferDirection) // direction=1 IN from USB
		{
			if (*DataLength < TxferLen)
			{
				BufferLen = *DataLength;
			}
			else
			{
				BufferLen = TxferLen;
			}
			Status = DwHc->PciIo->Map (DwHc->PciIo, EfiPciIoOperationBusMasterWrite , Data+Done , &BufferLen, &BusPhysAddr, &Mapping);
		}
		else 
		{
			BufferLen = TxferLen;
			Status = DwHc->PciIo->Map (DwHc->PciIo, EfiPciIoOperationBusMasterRead , Data+Done , &BufferLen, &BusPhysAddr, &Mapping);
		}

		// something is horribly wrong with the way i'm binding the PciIo if this stabilizes 
		// the transfers (and it removes nearly all the remaining stalls)
		asm("dsb sy");
		asm("isb sy");

		if (EFI_ERROR (Status)) {
				DEBUG ((EFI_D_ERROR, "Usb: Map DMA failed\n"));
		}

        MmioWrite32 (DwHc->DwUsbBase + HCDMA(Channel), (UINTN)BusPhysAddr);

        MmioAndThenOr32 (DwHc->DwUsbBase + HCCHAR(Channel), ~(DWC2_HCCHAR_MULTICNT_MASK |
															  DWC2_HCCHAR_CHEN |
															  DWC2_HCCHAR_CHDIS),
                         ((1 << DWC2_HCCHAR_MULTICNT_OFFSET) |
                          DWC2_HCCHAR_CHEN));

//		ArmDataSynchronizationBarrier();

        *TransferResult = Wait4Chhltd (DwHc, Channel, &Sub, Pid, IgnoreAck, IgnoreComplete, Timeout);
		if ((DeviceSpeed != EFI_USB_SPEED_HIGH) && (!SplitDone) && (!DwHc->AtFullSpeed))
		{
			SplitDone++;
			MmioWrite32 (DwHc->DwUsbBase + HCINT(Channel), 0x3FFF);
			MmioAndThenOr32 (DwHc->DwUsbBase + HCCHAR(Channel), ~(DWC2_HCCHAR_MULTICNT_MASK | DWC2_HCCHAR_CHEN | DWC2_HCCHAR_CHDIS),
							 ((1 << DWC2_HCCHAR_MULTICNT_OFFSET) | DWC2_HCCHAR_CHEN | DWC2_HCSPLT_COMPSPLT));
			*TransferResult = Wait4Chhltd (DwHc, Channel, &Sub, Pid, IgnoreAck, IgnoreComplete, Timeout);
//						continue;
		}

		DwHc->PciIo->Flush(DwHc->PciIo);

		if (Mapping)
		{
			DwHc->PciIo->Unmap (DwHc->PciIo, Mapping);
			Mapping = NULL;
		}

        if (*TransferResult) {
            DEBUG ((EFI_D_VERBOSE, "DwHcTransfer: failed\n"));
            Status = EFI_DEVICE_ERROR;
            break;
        }

        if (TransferDirection) {
            TxferLen -= Sub;
            if (Sub)
                StopTransfer = 1;
		}


        Done += TxferLen;
    } while (Done < *DataLength && !StopTransfer);

    MmioWrite32 (DwHc->DwUsbBase + HCINTMSK(Channel), 0);
    MmioWrite32 (DwHc->DwUsbBase + HCINT(Channel), 0xFFFFFFFF);

    *DataLength = Done;

	gBS->RestoreTPL (OriginalTPL);

	OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);    
	DwHc->ActiveChannel[Channel] = FALSE;
	gBS->RestoreTPL (OriginalTPL);

    return Status;
}

/**
   EFI_USB2_HC_PROTOCOL APIs
**/

EFI_STATUS
EFIAPI
DwHcGetCapability (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
    )
{
    DWUSB_OTGHC_DEV *DwHc;
    DEBUG ((EFI_D_VERBOSE, "DwHcGetCapability \n",__func__));

    DwHc = DWHC_FROM_THIS (This);

    if ((MaxSpeed == NULL) || (PortNumber == NULL) || (Is64BitCapable == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

	if (DwHc->AtFullSpeed)
	{
		*MaxSpeed = EFI_USB_SPEED_FULL;
	}
	else
	{
		*MaxSpeed = EFI_USB_SPEED_HIGH;
	}
    *PortNumber = 1;
    *Is64BitCapable = 1;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwHcReset (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT16               Attributes
    )
{
    DWUSB_OTGHC_DEV *DwHc;
    DEBUG ((EFI_D_ERROR, "Designware HostController Reset\n"));

    DwHc = DWHC_FROM_THIS (This);

    ConfigureUsbPhy ();
    DwCoreInit(DwHc);
    DwHcInit(DwHc);

    MmioAndThenOr32 (DwHc->DwUsbBase + HPRT0,
                     ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                       DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG),
                     DWC2_HPRT0_PRTRST);

    MicroSecondDelay (50000);

    MmioAnd32 (DwHc->DwUsbBase + HPRT0, ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                                          DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG |
                                          DWC2_HPRT0_PRTRST));

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwHcGetState (
    IN   EFI_USB2_HC_PROTOCOL  *This,
    OUT  EFI_USB_HC_STATE      *State
    )
{
    DWUSB_OTGHC_DEV *DwHc;

    DEBUG ((EFI_D_VERBOSE, "GetState \n",__func__));

    DwHc = DWHC_FROM_THIS (This);

    *State = DwHc->DwHcState;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwHcSetState (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN EFI_USB_HC_STATE     State
    )
{
    DWUSB_OTGHC_DEV *DwHc;
    DEBUG ((EFI_D_VERBOSE, "DwHcSetState \n",__func__));

    DwHc = DWHC_FROM_THIS (This);

    DwHc->DwHcState = State;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwHcGetRootHubPortStatus (
    IN   EFI_USB2_HC_PROTOCOL  *This,
    IN   UINT8                 PortNumber,
    OUT  EFI_USB_PORT_STATUS   *PortStatus
    )
{
    DWUSB_OTGHC_DEV *DwHc;
    UINT32    Hprt0;
    static    int first = 1;
//  DEBUG ((EFI_D_ERROR, "%s \n",L __func__));
//    DEBUG ((EFI_D_ERROR, "GetPortStatus \n"));

    if (first)
    {
        first = 0 ;
		DEBUG ((EFI_D_ERROR, "DwHcReset first \n"));
        DwHcReset(This,0);
    }

    if (PortNumber > DWC2_HC_CHANNEL)
        return EFI_INVALID_PARAMETER;

    if (PortStatus == NULL)
        return EFI_INVALID_PARAMETER;

    DwHc = DWHC_FROM_THIS (This);

    PortStatus->PortStatus = 0;
    PortStatus->PortChangeStatus = 0;

    Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);

    if (Hprt0 & DWC2_HPRT0_PRTCONNSTS) {
        PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
    }

    if (Hprt0 & DWC2_HPRT0_PRTENA) {
        PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
    }

    if (Hprt0 & DWC2_HPRT0_PRTSUSP) {
        PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
    }

    if (Hprt0 & DWC2_HPRT0_PRTOVRCURRACT) {
        PortStatus->PortStatus |= USB_PORT_STAT_OVERCURRENT;
    }

    if (Hprt0 & DWC2_HPRT0_PRTRST) {
        PortStatus->PortStatus |= USB_PORT_STAT_RESET;
    }

    if (Hprt0 & DWC2_HPRT0_PRTPWR) {
        PortStatus->PortStatus |= USB_PORT_STAT_POWER;
    }

	if (!DwHc->AtFullSpeed)
	{
		PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
	}

    if (Hprt0 & DWC2_HPRT0_PRTENCHNG) {
//    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
    }

    if (Hprt0 & DWC2_HPRT0_PRTCONNDET) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
    }

    if (Hprt0 & DWC2_HPRT0_PRTOVRCURRCHNG) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_OVERCURRENT;
    }

//  DEBUG ((EFI_D_ERROR, "GetPortStatus %X %X\n",PortStatus->PortStatus,PortStatus->PortChangeStatus));

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwHcSetRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
    )
{
    DWUSB_OTGHC_DEV   *DwHc;
    UINT32      Hprt0;
    EFI_STATUS    Status = EFI_SUCCESS;

    DEBUG ((EFI_D_ERROR, "DwHcSetRootHubPortFeature \n"));

    if (PortNumber > DWC2_HC_CHANNEL) {
        Status = EFI_INVALID_PARAMETER;
        goto End;
    }

    DwHc = DWHC_FROM_THIS (This);

    switch (PortFeature) {
        case EfiUsbPortEnable:
            break;
        case EfiUsbPortSuspend:
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                       DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG);
            Hprt0 |= DWC2_HPRT0_PRTSUSP;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        case EfiUsbPortReset:
            MmioAndThenOr32 (DwHc->DwUsbBase + HPRT0,
                             ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                               DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG),
                             DWC2_HPRT0_PRTRST);
            MicroSecondDelay (50000);
            MmioAnd32 (DwHc->DwUsbBase + HPRT0, ~DWC2_HPRT0_PRTRST);
            break;
        case EfiUsbPortPower:
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                       DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG);
            Hprt0 |= DWC2_HPRT0_PRTPWR;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        case EfiUsbPortOwner:
            break;
        default:
            Status = EFI_INVALID_PARAMETER;
            break;
    }

  End:
    return Status;
}

EFI_STATUS
EFIAPI
DwHcClearRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
    )
{
    DWUSB_OTGHC_DEV   *DwHc;
    UINT32      Hprt0;
    EFI_STATUS    Status = EFI_SUCCESS;

    DEBUG ((EFI_D_ERROR, "DwHcClearRootHubPortFeature \n"));

    if (PortNumber > DWC2_HC_CHANNEL) {
        Status = EFI_INVALID_PARAMETER;
        goto End;
    }

    DwHc = DWHC_FROM_THIS (This);

    switch (PortFeature) {
        case EfiUsbPortEnable:
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                       DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG);
            Hprt0 |= DWC2_HPRT0_PRTENA;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        case EfiUsbPortReset:
            MmioAndThenOr32 (DwHc->DwUsbBase + HPRT0,
                             ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                               DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG),
                             DWC2_HPRT0_PRTRST);
            MicroSecondDelay (50000);
            MmioAnd32 (DwHc->DwUsbBase + HPRT0, ~DWC2_HPRT0_PRTRST);
            break;
        case EfiUsbPortSuspend:
            MmioWrite32 (DwHc->DwUsbBase + PCGCCTL, 0);
            MicroSecondDelay (40000);
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                       DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG);
            Hprt0 |= DWC2_HPRT0_PRTRES;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            Hprt0 &= ~DWC2_HPRT0_PRTSUSP;
            MicroSecondDelay (150000);
            Hprt0 &= ~DWC2_HPRT0_PRTRES;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        case EfiUsbPortPower:
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                       DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG);
            Hprt0 &= ~DWC2_HPRT0_PRTPWR;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        case EfiUsbPortOwner:
            break;
        case EfiUsbPortConnectChange:
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~DWC2_HPRT0_PRTCONNDET;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        case EfiUsbPortResetChange:
            break;
        case EfiUsbPortEnableChange:
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~DWC2_HPRT0_PRTENCHNG;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        case EfiUsbPortSuspendChange:
            break;
        case EfiUsbPortOverCurrentChange:
            Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
            Hprt0 &= ~DWC2_HPRT0_PRTOVRCURRCHNG;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
            break;
        default:
            Status = EFI_INVALID_PARAMETER;
            break;
    }

  End:
    return Status;
}

void DumpRequest(EFI_USB_DEVICE_REQUEST *Request)
{
	if (Request->RequestType & 0x80) {
		DEBUG ((EFI_D_VERBOSE, "USB: RequestType %X -> Host\n",Request->RequestType));
	}
	else {
		DEBUG ((EFI_D_VERBOSE, "USB: RequestType %X -> Device\n",Request->RequestType));
	}
	if (Request->RequestType & 0x60)  {
		DEBUG ((EFI_D_VERBOSE, "USB:                -> CLS/VEND\n"));
	}
	else  {
		DEBUG ((EFI_D_VERBOSE, "USB:                -> STD\n"));
	}
	switch (Request->RequestType & 0x03)  {
		case USB_TARGET_DEVICE:
			DEBUG ((EFI_D_VERBOSE, "USB:                -> DEVICE\n"));
			break;
		case USB_TARGET_INTERFACE:
			DEBUG ((EFI_D_VERBOSE, "USB:                -> ITERFACE\n"));
			break;
		case USB_TARGET_ENDPOINT:
			DEBUG ((EFI_D_VERBOSE, "USB:                -> ENDPOINT\n"));
			break;
		case USB_TARGET_OTHER:
			DEBUG ((EFI_D_VERBOSE, "USB:                -> OTHER\n"));
			break;
	}
	DEBUG ((EFI_D_VERBOSE, "USB: RequestType 0x%X\n",Request->RequestType));
	switch (Request->Request) {
		case USB_DEV_GET_STATUS:
			DEBUG ((EFI_D_VERBOSE, "USB:             GET STATUS\n"));
			break;
		case USB_DEV_CLEAR_FEATURE:
			DEBUG ((EFI_D_VERBOSE, "USB:             CLEAR FEATURE\n"));
			break;
		case USB_DEV_SET_FEATURE:
			DEBUG ((EFI_D_VERBOSE, "USB:             SET FEATURE\n"));
			break;
		case USB_DEV_SET_ADDRESS:
			DEBUG ((EFI_D_VERBOSE, "USB:             SET ADDRESS\n"));
			break;
		case USB_DEV_GET_DESCRIPTOR:
			DEBUG ((EFI_D_VERBOSE, "USB:             GET DESCRIPTOR\n"));
			break;
		case USB_DEV_SET_DESCRIPTOR:
			DEBUG ((EFI_D_VERBOSE, "USB:             SET DESCRIPTOR\n"));
			break;
		default:
			break;
	}
	DEBUG ((EFI_D_VERBOSE, "USB: Request     %d\n",Request->Request));
	DEBUG ((EFI_D_VERBOSE, "USB: Value       0x%X\n",Request->Value));
	DEBUG ((EFI_D_VERBOSE, "USB: Index       0x%X\n",Request->Index));
	DEBUG ((EFI_D_VERBOSE, "USB: Length      %d\n",Request->Length));
}


EFI_STATUS
EFIAPI
DwHcControlTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  EFI_USB_DEVICE_REQUEST              *Request,
    IN  EFI_USB_DATA_DIRECTION              TransferDirection,
    IN  OUT VOID                            *Data,
    IN  OUT UINTN                           *DataLength,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
    )
{
    DWUSB_OTGHC_DEV   *DwHc;
    EFI_STATUS    Status;
    UINT32      Pid;
    UINTN     Length;
    EFI_USB_DATA_DIRECTION  StatusDirection;
    UINT32      Direction;
	UINT8				*StatusBuffer;
	EFI_TPL   OriginalTPL;				

    DEBUG ((EFI_D_VERBOSE, "DwHcControlTranfer \n"));
    DumpRequest(Request);

    if ((Request == NULL) || (TransferResult == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((TransferDirection != EfiUsbDataIn) &&
        (TransferDirection != EfiUsbDataOut) &&
        (TransferDirection != EfiUsbNoData)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((TransferDirection == EfiUsbNoData) &&
        ((Data != NULL) || (*DataLength != 0))) {
        return EFI_INVALID_PARAMETER;
    }

    if ((TransferDirection != EfiUsbNoData) &&
        ((Data == NULL) || (*DataLength == 0))) {
        return EFI_INVALID_PARAMETER;
    }

    if ((MaximumPacketLength != 8) && (MaximumPacketLength != 16) &&
        (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
        return EFI_INVALID_PARAMETER;
    }

    DwHc  = DWHC_FROM_THIS(This);

	OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);
	DwHc->BulkActive=POLL_DELAY;
	gBS->RestoreTPL (OriginalTPL);

    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status    = EFI_DEVICE_ERROR;

    Pid = DWC2_HC_PID_SETUP;
    Length = 8;
    Status = DwHcTransfer (DwHc, DeviceAddress, MaximumPacketLength, &Pid, 0, Request, &Length,
                           0, DWC2_HCCHAR_EPTYPE_CONTROL, TransferResult, 1, 0, DeviceSpeed, 
						   Translator->TranslatorPortNumber, Translator->TranslatorHubAddress, 0, TimeOut);

    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "DwHcControlTransfer: Setup Stage Error\n"));
		if ((DeviceSpeed != EFI_USB_SPEED_HIGH) && (DwHc->AtFullSpeed == 0))
		{
			DEBUG ((EFI_D_ERROR, "DwHcControlTransfer: Split transaction failed, reducing bus speed\n"));
			DwHc->AtFullSpeed = 1;
			DwHcReset(This, 0);
		}
		Status = EFI_USB_ERR_SYSTEM;
        goto EXIT;
    }

    if (Data) {
        Pid = DWC2_HC_PID_DATA1;

        if (TransferDirection == EfiUsbDataIn)
            Direction = 1;
        else
            Direction = 0;

        Status = DwHcTransfer (DwHc, DeviceAddress, MaximumPacketLength, &Pid, Direction,
                               Data, DataLength, 0, DWC2_HCCHAR_EPTYPE_CONTROL, TransferResult, 0, 0, DeviceSpeed, 
							   Translator->TranslatorPortNumber, Translator->TranslatorHubAddress, 0, TimeOut);

        if (EFI_ERROR(Status)) {
            DEBUG ((EFI_D_ERROR, "DwHcControlTransfer: Data Stage Error\n"));
            goto EXIT;
        }
    }

    if ((TransferDirection == EfiUsbDataOut) || (TransferDirection == EfiUsbNoData))
        StatusDirection = 1;
    else
        StatusDirection = 0;

    Pid = DWC2_HC_PID_DATA1;
    Length = 0;

    StatusBuffer = UncachedAllocatePages (EFI_SIZE_TO_PAGES (DWC2_STATUS_BUF_SIZE));
    if (StatusBuffer == NULL) {
        DEBUG ((EFI_D_ERROR, "CreateDwUsbHc: No page availablefor StatusBuffer\n"));
		goto EXIT;
    }

    Status = DwHcTransfer (DwHc, DeviceAddress, MaximumPacketLength, &Pid, StatusDirection, StatusBuffer,
                           &Length, 0, DWC2_HCCHAR_EPTYPE_CONTROL, TransferResult, 0, 0, DeviceSpeed,
						   Translator->TranslatorPortNumber, Translator->TranslatorHubAddress, 0, TimeOut);

    UncachedFreePages (StatusBuffer, EFI_SIZE_TO_PAGES (DWC2_STATUS_BUF_SIZE));
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "DwHcControlTransfer: Status Stage Error\n"));
    }

  EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
DwHcBulkTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  UINT8                               DataBuffersNumber,
    IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
    IN  OUT UINTN                           *DataLength,
    IN  OUT UINT8                           *DataToggle,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
    )
{
    DWUSB_OTGHC_DEV   *DwHc;
    EFI_STATUS    Status;
    UINT8     TransferDirection;
    UINT8     EpAddress;
    UINT32      Pid;
    UINT8     Ping;
	EFI_TPL   OriginalTPL;				

    DEBUG ((EFI_D_VERBOSE, "DwHcBulkTransfer DEV=%X EP=%X LEN=%X \n",DeviceAddress,EndPointAddress,*DataLength));
    if ((Data == NULL) || (Data[0] == NULL) ||
        (DataLength == NULL) || (*DataLength == 0) ||
        (TransferResult == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((*DataToggle != 0) && (*DataToggle != 1))
        return EFI_INVALID_PARAMETER;

    if ((DeviceSpeed == EFI_USB_SPEED_LOW) || (DeviceSpeed == EFI_USB_SPEED_SUPER))
        return EFI_INVALID_PARAMETER;

    if (((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
        ((DeviceSpeed == EFI_USB_SPEED_HIGH) && (MaximumPacketLength > 512)))
        return EFI_INVALID_PARAMETER;

    DwHc = DWHC_FROM_THIS (This);

    *TransferResult   = EFI_USB_ERR_SYSTEM;
    Status      = EFI_DEVICE_ERROR;
    TransferDirection = (EndPointAddress >> 7) & 0x01; //IN = 1, OUT = 0
    EpAddress   = EndPointAddress & 0x0F;
    Pid     = (*DataToggle << 1);
    Ping    = 0;
//    Ping    = TransferDirection; //for write do ping first

	OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);
	DwHc->BulkActive=POLL_DELAY;
	gBS->RestoreTPL (OriginalTPL);

    Status = DwHcTransfer (DwHc, DeviceAddress, MaximumPacketLength, &Pid, TransferDirection, Data[0], DataLength,
                           EpAddress, DWC2_HCCHAR_EPTYPE_BULK, TransferResult, 1, Ping, DeviceSpeed,
						   Translator->TranslatorPortNumber, Translator->TranslatorHubAddress, 0, TimeOut);

    *DataToggle = (Pid >> 1);

//    MicroSecondDelay (5000);
//	DwHc->BulkActive=POLL_DELAY;

    return Status;
}

EFI_STATUS
EFIAPI
DwHcAsyncInterruptTransfer (
    IN  EFI_USB2_HC_PROTOCOL                  *This,
    IN  UINT8                                 DeviceAddress,
    IN  UINT8                                 EndPointAddress,
    IN  UINT8                                 DeviceSpeed,
    IN  UINTN                                 MaximumPacketLength,
    IN  BOOLEAN                               IsNewTransfer,
    IN  OUT UINT8                             *DataToggle,
    IN  UINTN                                 PollingInterval,
    IN  UINTN                                 DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
    IN  VOID                                  *Context OPTIONAL
    )
{
    DWUSB_OTGHC_DEV       *DwHc;
    EFI_STATUS            Status;
	EFI_TPL               OriginalTPL;				
	DWUSB_INTERRUPT_QUEUE *CompletionEntry;
    DEBUG ((EFI_D_VERBOSE, "DwHcAsyncInterruptTransfer Addr=%d, EP=%d len=%d new=%d Interval=%d(%d)\n",DeviceAddress, EndPointAddress, DataLength, IsNewTransfer, PollingInterval, PollingInterval/DW_USB_POLL_TIMER));

    if (!(EndPointAddress & USB_ENDPOINT_DIR_IN)) {
        DEBUG ((EFI_D_ERROR, "DwHcAsyncInterruptTransfer: endpoint direction not right\n"));
        return EFI_INVALID_PARAMETER;
    }

    DwHc = DWHC_FROM_THIS (This);

    if (IsNewTransfer) {
        if (DataLength == 0) {
            DEBUG ((EFI_D_ERROR, "DwHcAsyncInterruptTransfer: invalid len\n"));
            return EFI_INVALID_PARAMETER;
        }

        if ((*DataToggle != 1) && (*DataToggle != 0)) {
            DEBUG ((EFI_D_ERROR, "DwHcAsyncInterruptTransfer: invalid datatoggle\n"));
            return EFI_INVALID_PARAMETER;
        }

        if ((PollingInterval > 255) || (PollingInterval < 1)) {
            DEBUG ((EFI_D_ERROR, "DwHcAsyncInterruptTransfer: Invalid polling interval\n"));
            return EFI_INVALID_PARAMETER;
        }
    }
	else {
		// The async nature of this callback is bonkers with
		// relation to how it should work. So we do all the 
		// "scheduled" processing in the foreground and then
		// just queue the results..
		OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);
		// Which means we want to cancel any outstanding items here..
		CompletionEntry = DwHc->InterruptQueue;
		while (CompletionEntry) {
			if ((CompletionEntry->DeviceAddress == DeviceAddress) && (CompletionEntry->EndPointAddress == EndPointAddress))	{
				CompletionEntry->Cancel = 1;
			}
			CompletionEntry = CompletionEntry->Next;
		}
		gBS->RestoreTPL (OriginalTPL);
		return EFI_SUCCESS;
	}

    CompletionEntry = AllocateZeroPool (sizeof(DWUSB_INTERRUPT_QUEUE));
	if (CompletionEntry == NULL)	{
		DEBUG ((EFI_D_ERROR, "DwHcAsyncInterruptTransfer: failed to allocate buffer\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
	}

    CompletionEntry->Data = AllocateZeroPool (DataLength);
    if (CompletionEntry->Data == NULL) {
		FreePool(CompletionEntry);
		DEBUG ((EFI_D_ERROR, "DwHcAsyncInterruptTransfer: failed to allocate buffer\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;     
    }

	CompletionEntry->CallBackFunction = CallBackFunction;
	CompletionEntry->DataLength = DataLength;
	CompletionEntry->Context = Context;
	CompletionEntry->TransferResult = EFI_USB_NOERROR;
	CompletionEntry->DeviceAddress = DeviceAddress;
	CompletionEntry->EndPointAddress = EndPointAddress;
	CompletionEntry->Cancel = 0;
	CompletionEntry->This = This;
	CompletionEntry->DeviceSpeed = DeviceSpeed;
	CompletionEntry->MaximumPacketLength = MaximumPacketLength;
	CompletionEntry->Translator = Translator;
	if (PollingInterval > DW_USB_POLL_TIMER) {
		CompletionEntry->TicksBeforeActive = (PollingInterval/DW_USB_POLL_TIMER);
	} else {
		CompletionEntry->TicksBeforeActive = 1;
	}
	CompletionEntry->Ticks = -1;

    Status      = EFI_SUCCESS;

	CompletionEntry->DataToggle = *DataToggle;

	DEBUG ((EFI_D_VERBOSE, "DwHcAsyncInterruptTransfer: Save off results of interrupt transfer\n"));
	// queue the result to be reported on the timer
	OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);
	CompletionEntry->Next = DwHc->InterruptQueue;
	DwHc->InterruptQueue = CompletionEntry;
	gBS->RestoreTPL (OriginalTPL);



  EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
DwHcSyncInterruptTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  OUT VOID                            *Data,
    IN  OUT UINTN                           *DataLength,
    IN  OUT UINT8                           *DataToggle,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
    )
{
    DEBUG ((EFI_D_ERROR, "DwHcSyncInterruptTransfer Not Supported\n",__func__));
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
DwHcIsochronousTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  UINT8                               DataBuffersNumber,
    IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN  UINTN                               DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
    )
{
    DEBUG ((EFI_D_ERROR, "DwHcIsochronousTransfer Not Supported\n",__func__));
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
DwHcAsyncIsochronousTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  UINT8                               DataBuffersNumber,
    IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN  UINTN                               DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
    IN  VOID                                *Context
    )
{
    DEBUG ((EFI_D_ERROR, "DwHcAsyncIsochronousTransfer Not Supported\n",__func__));
    return EFI_UNSUPPORTED;
}

/**
   Supported Functions
**/

VOID
InitFslspClkSel (
    IN DWUSB_OTGHC_DEV *DwHc
    )
{
    UINT32  PhyClk;
    DEBUG ((EFI_D_VERBOSE, "InitFslspClkSel \n",__func__));
    PhyClk = DWC2_HCFG_FSLSPCLKSEL_30_60_MHZ;

    MmioAndThenOr32 (DwHc->DwUsbBase + HCFG,
                     ~DWC2_HCFG_FSLSPCLKSEL_MASK,
                     PhyClk << DWC2_HCFG_FSLSPCLKSEL_OFFSET);
	if (DwHc->AtFullSpeed)
	{
		MmioAndThenOr32 (DwHc->DwUsbBase + HCFG,
						 ~DWC2_HCFG_FSLSSUPP,
						 DWC2_HCFG_FSLSSUPP);
	}
}		


/**

   Timer to submit periodic interrupt transfer, and invoke callbacks

   @param  Event                 Event handle
   @param  Context               Device private data

**/

VOID
EFIAPI
DwHcTimerCallback (
	IN  EFI_EVENT           Event,
	IN  VOID                *Context
	)
{
	DWUSB_OTGHC_DEV *DwHc = (DWUSB_OTGHC_DEV *) Context;
	EFI_TPL                  OriginalTPL;
	DWUSB_INTERRUPT_QUEUE    *CompletionEntry;
	DWUSB_INTERRUPT_QUEUE    *CompletionNext;
	UINT32                   ret = 0;

	OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);
	if (DwHc->BulkActive>0) {
		// This is a gentle hack 
		// to avoid colliding requests
		// for now.
		DwHc->BulkActive--;
		ret = 1;
	}
	gBS->RestoreTPL (OriginalTPL);

	if (ret)
		return;

//	DEBUG ((EFI_D_ERROR, "DwHcTimerCallback TPL=%dn\n",OriginalTPL));

	// pick off all queued items.
	OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);
	CompletionNext = DwHc->InterruptQueue;
	//DwHc->InterruptQueue = NULL;
	gBS->RestoreTPL (OriginalTPL);

	// perform callbacks on queued interrupt status
	while (CompletionNext)	{
		DEBUG ((EFI_D_VERBOSE, "DwHcTimerCallbackn\n"));
		CompletionEntry = CompletionNext;
		CompletionNext = CompletionEntry->Next;
		CompletionEntry->Ticks++;
		if ((CompletionEntry->CallBackFunction) && ( CompletionEntry->Cancel == 0 ) && (CompletionEntry->Ticks > CompletionEntry->TicksBeforeActive))
		{
			UINT32        Pid;
			UINT8         TransferDirection;
			UINT8         EpAddress;
			EFI_STATUS    Status;
			EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator = CompletionEntry->Translator;
			UINTN         DataLength;
			UINT32        Timeout = DW_USB_POLL_INTERRUPT;
			
			if ( CompletionEntry->DeviceAddress == 1 )
			{
				// fast timeouts on first hub port
				Timeout = 10;
				DEBUG ((EFI_D_VERBOSE, "DwHcTimerCallbackn Reset timer for addr1\n"));
			}

			// ok execute thie
			EpAddress   = CompletionEntry->EndPointAddress & 0x0F;
			TransferDirection = (CompletionEntry->EndPointAddress >> 7) & 0x01;
								
			Pid         = CompletionEntry->DataToggle;
			DataLength  = CompletionEntry->DataLength;
				
			DEBUG ((EFI_D_VERBOSE, "DwHcTimerCallback message to  addr=%d, EP=%d Pid=%d\n",CompletionEntry->DeviceAddress, EpAddress, Pid));

			Status = DwHcTransfer (DwHc, CompletionEntry->DeviceAddress, CompletionEntry->MaximumPacketLength, &Pid, TransferDirection, 
								   CompletionEntry->Data, &DataLength,
								   EpAddress, DWC2_HCCHAR_EPTYPE_INTR, &CompletionEntry->TransferResult, 1, 0, CompletionEntry->DeviceSpeed,
								   Translator->TranslatorPortNumber, Translator->TranslatorHubAddress, 1, Timeout);

			DEBUG ((EFI_D_VERBOSE, "DwHcTimerCallback Pid=%d\n", Pid));
			if (EFI_ERROR(Status))
			{
				//DEBUG ((EFI_D_ERROR, "Consider canceling the transaction here?\n"));
				if (CompletionEntry->TransferResult == EFI_USB_ERR_NAK ) {
					// NAK's are still successful transmission
					// particularly for interrupt transfers
					Status = EFI_USB_NOERROR;
				}

				if (CompletionEntry->TransferResult == (EFI_USB_ERR_SYSTEM+1) ) {
					// overruns may still have data, lets pass it on
					Status = EFI_USB_NOERROR;
					//DataLength = CompletionEntry->DataLength;
					//CompletionEntry->TransferResult = EFI_USB_NOERROR;
				}
				if (CompletionEntry->TransferResult == EFI_USB_ERR_TIMEOUT)
				{
					// This actually appears to recovery everything correctly
					// but does it really need recovery, or just a retry?
					//DEBUG ((EFI_D_ERROR, "Doing full reset\n"));
					// doing full reset
					DwHcReset(CompletionEntry->This,0);
				}

			}
				
			CompletionEntry->DataToggle = Pid;
			
			// don't report NAK's and DATA Toggle errors, what the end user doesn't know won't kill them..
			if ((CompletionEntry->TransferResult != EFI_USB_ERR_SYSTEM) && (CompletionEntry->TransferResult != EFI_USB_ERR_NAK )) {
				CompletionEntry->CallBackFunction (CompletionEntry->Data, DataLength, CompletionEntry->Context, CompletionEntry->TransferResult);
			}
			CompletionEntry->Ticks = 0; //reset the ticks
		}
	}

	{
		// Nove canceled items to a cancel list
		// and then delete all the entries on the canceled list
		DWUSB_INTERRUPT_QUEUE *Canceled;
		// now remove any canceled entries
		OriginalTPL = gBS->RaiseTPL(TPL_HIGH_LEVEL);
		CompletionNext = DwHc->InterruptQueue;
		DwHc->InterruptQueue = NULL;
		Canceled = NULL;
		
		while (CompletionNext)	{
			
			CompletionEntry = CompletionNext;
			CompletionNext = CompletionEntry->Next;
			if (CompletionEntry->Cancel)
			{
				// toss it on the canceled queue
				CompletionEntry->Next = Canceled;
				Canceled = CompletionEntry->Next;
			}
			else
			{
				// toss it back on the pending queue
				CompletionEntry->Next = DwHc->InterruptQueue;
				DwHc->InterruptQueue = CompletionEntry;
			}
		}
		gBS->RestoreTPL (OriginalTPL);

		while (Canceled)
		{
			DEBUG ((EFI_D_VERBOSE, "DwHcTimerCallback cancel\n"));
			CompletionEntry = Canceled;
			Canceled = CompletionEntry->Next;
			FreePool(CompletionEntry->Data);
			FreePool(CompletionEntry);
		}
	}

}

VOID
DwHcInit (
    IN DWUSB_OTGHC_DEV *DwHc
    )
{
    UINT32 NpTxFifoSz = 0;
    UINT32 pTxFifoSz = 0;
    UINT32 Hprt0 = 0;
    INT32  i, Status, NumChannels;
    DEBUG ((EFI_D_VERBOSE, "HwHcInit \n",__func__));

    MmioWrite32 (DwHc->DwUsbBase + PCGCCTL, 0);

    InitFslspClkSel (DwHc);

    MmioWrite32 (DwHc->DwUsbBase + GRXFSIZ, DWC2_HOST_RX_FIFO_SIZE);

    NpTxFifoSz |= DWC2_HOST_NPERIO_TX_FIFO_SIZE << DWC2_FIFOSIZE_DEPTH_OFFSET;
    NpTxFifoSz |= DWC2_HOST_RX_FIFO_SIZE << DWC2_FIFOSIZE_STARTADDR_OFFSET;
    MmioWrite32 (DwHc->DwUsbBase + GNPTXFSIZ, NpTxFifoSz);

    pTxFifoSz |= DWC2_HOST_PERIO_TX_FIFO_SIZE << DWC2_FIFOSIZE_DEPTH_OFFSET;
    pTxFifoSz |= (DWC2_HOST_RX_FIFO_SIZE + DWC2_HOST_NPERIO_TX_FIFO_SIZE) <<
        DWC2_FIFOSIZE_STARTADDR_OFFSET;
    MmioWrite32 (DwHc->DwUsbBase + HPTXFSIZ, pTxFifoSz);

    MmioAnd32 (DwHc->DwUsbBase + GOTGCTL, ~(DWC2_GOTGCTL_HSTSETHNPEN));

    DwFlushTxFifo (DwHc, 0x10);
    DwFlushRxFifo (DwHc);

    NumChannels = MmioRead32 (DwHc->DwUsbBase + GHWCFG2);
    NumChannels &= DWC2_HWCFG2_NUM_HOST_CHAN_MASK;
    NumChannels >>= DWC2_HWCFG2_NUM_HOST_CHAN_OFFSET;
    NumChannels += 1;

    for (i=0; i<NumChannels; i++)
        MmioAndThenOr32 (DwHc->DwUsbBase + HCCHAR(i),
                         ~(DWC2_HCCHAR_CHEN | DWC2_HCCHAR_EPDIR),
                         DWC2_HCCHAR_CHDIS);

    for (i=0; i<NumChannels; i++) {
        MmioAndThenOr32 (DwHc->DwUsbBase + HCCHAR(i),
                         ~DWC2_HCCHAR_EPDIR,
                         (DWC2_HCCHAR_CHEN | DWC2_HCCHAR_CHDIS));
        Status = Wait4Bit (DwHc->DwUsbBase + HCCHAR(i), DWC2_HCCHAR_CHEN, 0);
        if (Status)
            DEBUG ((EFI_D_VERBOSE, "DwHcInit: Timeout!\n"));
    }

    if (MmioRead32 (DwHc->DwUsbBase + GINTSTS) & DWC2_GINTSTS_CURMODE_HOST) {
        Hprt0 = MmioRead32 (DwHc->DwUsbBase + HPRT0);
        Hprt0 &= ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET);
        Hprt0 &= ~(DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG);

        if (!(Hprt0 & DWC2_HPRT0_PRTPWR)) {
            Hprt0 |= DWC2_HPRT0_PRTPWR;
            MmioWrite32 (DwHc->DwUsbBase + HPRT0, Hprt0);
        }
    }
}

VOID
DwCoreInit (
    IN DWUSB_OTGHC_DEV *DwHc
    )
{
    UINT32    AhbCfg = 0;
    UINT32    UsbCfg = 0;
    DEBUG ((EFI_D_VERBOSE, "DwCoreInit \n"));

    UsbCfg = MmioRead32 (DwHc->DwUsbBase + GUSBCFG);

    UsbCfg |= DWC2_GUSBCFG_ULPI_EXT_VBUS_DRV;
    UsbCfg &= ~DWC2_GUSBCFG_TERM_SEL_DL_PULSE;

    MmioWrite32 (DwHc->DwUsbBase + GUSBCFG, UsbCfg);

    DwCoreReset (DwHc);

    UsbCfg &= ~(DWC2_GUSBCFG_ULPI_UTMI_SEL | DWC2_GUSBCFG_PHYIF);
    UsbCfg |= CONFIG_DWC2_PHY_TYPE << DWC2_GUSBCFG_ULPI_UTMI_SEL_OFFSET;

    UsbCfg &= ~DWC2_GUSBCFG_DDRSEL;

    MmioWrite32 (DwHc->DwUsbBase + GUSBCFG, UsbCfg);

    DwCoreReset (DwHc);

    UsbCfg = MmioRead32 (DwHc->DwUsbBase + GUSBCFG);
    UsbCfg &= ~(DWC2_GUSBCFG_ULPI_FSLS | DWC2_GUSBCFG_ULPI_CLK_SUS_M);
	if (DW_FORCE_HOST) {
		UsbCfg |= DWC2_GUSBCFG_FORCEHOSTMODE; 
	}
    UsbCfg &= ~(DWC2_GUSBCFG_HNPCAP | DWC2_GUSBCFG_SRPCAP); //disable host neg protocol and SRP
    MmioWrite32 (DwHc->DwUsbBase + GUSBCFG, UsbCfg);

    DEBUG ((EFI_D_ERROR, "DwCoreInit USB cfg %X\n",UsbCfg));

    AhbCfg |= DWC2_GAHBCFG_HBURSTLEN_INCR4;
    AhbCfg |= DWC2_GAHBCFG_DMAENABLE;

    MmioWrite32 (DwHc->DwUsbBase + GAHBCFG, AhbCfg);
//    MmioOr32 (DwHc->DwUsbBase + GUSBCFG, DWC2_GUSBCFG_HNPCAP | DWC2_GUSBCFG_SRPCAP); //enable host negotiation, and session request (not wanted for host mode)

}

DWUSB_OTGHC_DEV *
CreateDwUsbHc (
	IN EFI_PCI_IO_PROTOCOL  *PciIo
    )
{
    DWUSB_OTGHC_DEV *DwHc;
	UINT32  Channel;
    DEBUG ((EFI_D_VERBOSE, "CreateDwUsbHc \n",__func__));

    DwHc = AllocateZeroPool (sizeof(DWUSB_OTGHC_DEV));

    if (DwHc == NULL) {
        return NULL;
    }

    DwHc->Signature                                 = DWUSB_OTGHC_DEV_SIGNATURE;
    DwHc->DwUsbOtgHc.GetCapability                  = DwHcGetCapability;
    DwHc->DwUsbOtgHc.Reset                          = DwHcReset;
    DwHc->DwUsbOtgHc.GetState                       = DwHcGetState;
    DwHc->DwUsbOtgHc.SetState                       = DwHcSetState;
    DwHc->DwUsbOtgHc.ControlTransfer                = DwHcControlTransfer;
    DwHc->DwUsbOtgHc.BulkTransfer                   = DwHcBulkTransfer;
    DwHc->DwUsbOtgHc.AsyncInterruptTransfer         = DwHcAsyncInterruptTransfer;
    DwHc->DwUsbOtgHc.SyncInterruptTransfer          = DwHcSyncInterruptTransfer;
    DwHc->DwUsbOtgHc.IsochronousTransfer            = DwHcIsochronousTransfer;
    DwHc->DwUsbOtgHc.AsyncIsochronousTransfer       = DwHcAsyncIsochronousTransfer;
    DwHc->DwUsbOtgHc.GetRootHubPortStatus           = DwHcGetRootHubPortStatus;
    DwHc->DwUsbOtgHc.SetRootHubPortFeature          = DwHcSetRootHubPortFeature;
    DwHc->DwUsbOtgHc.ClearRootHubPortFeature        = DwHcClearRootHubPortFeature;
    DwHc->DwUsbOtgHc.MajorRevision                  = 0x02;
    DwHc->DwUsbOtgHc.MinorRevision                  = 0x00;
    DwHc->DwUsbBase         = FixedPcdGet32 (PcdDwUsbBaseAddress); //TODO remove this and convert to Pci->Mem.Read/Write() calls

	DwHc->InterruptQueue = NULL;
	DwHc->PciIo = PciIo;
	DwHc->BulkActive = 0;
	DwHc->AtFullSpeed = DW_AT_FULLSPEED;

	for (Channel = 0 ; Channel < DWC2_MAX_CHANNELS; Channel++)
	{
		DwHc->ActiveChannel[Channel] = FALSE;
	}


    return DwHc;
}

VOID
EFIAPI
DwUsbHcExitBootService (
    EFI_EVENT Event,
    VOID    *Context
    )
{
    DWUSB_OTGHC_DEV   *DwHc;
	
	DEBUG ((EFI_D_ERROR, "\n\n\nEXIT BOOT SERVICES!\n\n\n"));

    DwHc = (DWUSB_OTGHC_DEV *) Context;

    MmioAndThenOr32 (DwHc->DwUsbBase + HPRT0,
                     ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
                       DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG),
                     DWC2_HPRT0_PRTRST);

    MicroSecondDelay (50000);

    DwCoreReset (DwHc);
}

#pragma pack(1)
typedef struct {
	UINT8               ProgInterface;
	UINT8               SubClassCode;
	UINT8               BaseCode;
} USB_CLASSC;
#pragma pack()

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
		DEBUG ((EFI_D_VERBOSE, "DwUsbDriverBindingSupported found board!\n"));
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

EFI_STATUS
EFIAPI
DwUsbDriverBindingStart (
	IN EFI_DRIVER_BINDING_PROTOCOL *This,
	IN EFI_HANDLE                  Controller,
	IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
	)
{
    EFI_STATUS              Status;
    DWUSB_OTGHC_DEV         *DwHc;
	EFI_PCI_IO_PROTOCOL       *PciIo;
	EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath;

    Status = EFI_SUCCESS;

	//
	// Open the PciIo Protocol, then enable the USB host controller
	//
	Status = gBS->OpenProtocol ( Controller,
								 &gEfiPciIoProtocolGuid, 
								 (VOID **) &PciIo,
								 This->DriverBindingHandle,
								 Controller,
								 EFI_OPEN_PROTOCOL_BY_DRIVER );

	if (EFI_ERROR (Status)) {
		return Status;
	}

	DEBUG ((EFI_D_VERBOSE, "DwUsbDriverBindingStart2 \n"));
	//
	// Open Device Path Protocol for on USB host controller
	//
	HcDevicePath = NULL;
	Status = gBS->OpenProtocol ( Controller,
								 &gEfiDevicePathProtocolGuid,
								 (VOID **) &HcDevicePath,
								 This->DriverBindingHandle,
								 Controller,
								 EFI_OPEN_PROTOCOL_GET_PROTOCOL );
	DEBUG ((EFI_D_VERBOSE, "DwUsbDriverBindingStart3 \n"));

    if (EFI_ERROR (Status)) {
        return Status;
    }


    DwHc = CreateDwUsbHc (PciIo);

    if (DwHc == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

	DEBUG ((EFI_D_VERBOSE, "DwUsbDriverBindingStart4 \n"));
	Status = gBS->InstallProtocolInterface ( &Controller,
											 &gEfiUsb2HcProtocolGuid,
											 EFI_NATIVE_INTERFACE,
											 &DwHc->DwUsbOtgHc );

	DEBUG ((EFI_D_VERBOSE, "DwUsbDriverBindingStart5 \n"));
    if (EFI_ERROR (Status)) {
		goto UNINSTALL_PROTOCOL;
    }

	DEBUG ((EFI_D_VERBOSE, "DwUsbDriverBindingStart6 \n"));
	//
	// Start the asynchronous interrupt monitor
	//
	Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL,
							   TPL_CALLBACK,
							   DwHcTimerCallback,
							   DwHc,
							   &DwHc->PollTimer);
	if (EFI_ERROR (Status)) {
		DEBUG ((EFI_D_ERROR, "Failed to register DwUsbHostDxe Timer\n"));				
	}

	Status = gBS->SetTimer (DwHc->PollTimer, TimerPeriodic, EFI_TIMER_PERIOD_MILLISECONDS(DW_USB_POLL_TIMER));
	if (EFI_ERROR (Status)) {
		DEBUG ((EFI_D_ERROR, "Failed to enable DwUsbHostDxe Timer\n"));				
	}


    Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL,
								 TPL_NOTIFY,
								 DwUsbHcExitBootService,
								 DwHc,
								 &gEfiEventExitBootServicesGuid,
								 &DwHc->ExitBootServiceEvent );

    if (EFI_ERROR (Status)) {
        goto UNINSTALL_PROTOCOL;
    }

    return Status;

  UNINSTALL_PROTOCOL:
    FreePool (DwHc);
  EXIT:
    return Status;
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
	DWUSB_OTGHC_DEV       *DwHc;

	DEBUG ((EFI_D_ERROR, "DwUsbDriverBindingStop \n"));
	//
	// Test whether the Controller handler passed in is a valid
	// Usb controller handle that should be supported, if not,
	// return the error status directly
	//
	Status = gBS->OpenProtocol (
		Controller,
		&gEfiUsb2HcProtocolGuid,
		(VOID **) &Usb2Hc,
		This->DriverBindingHandle,
		Controller,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
		);

	if (EFI_ERROR (Status)) {
		return Status;
	}

	DwHc = DWHC_FROM_THIS (This);

	Status = gBS->UninstallProtocolInterface (
		Controller,
		&gEfiUsb2HcProtocolGuid,
		DwHc
		);

	if (EFI_ERROR (Status)) {
		return Status;
	}

	//
	// Stop AsyncRequest Polling timer then stop the EHCI driver
	// and uninstall the EHCI protocl.
	//

	// stop the interrupt handing
	Status = gBS->SetTimer (DwHc->PollTimer, TimerCancel, 0 );
	if (DwHc->PollTimer != NULL) {
		gBS->CloseEvent (DwHc->PollTimer);
	}
  
	if (DwHc->ExitBootServiceEvent != NULL) {
		gBS->CloseEvent (DwHc->ExitBootServiceEvent);
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

	FreePool (DwHc);

	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwUsbHostExitPoint (
    IN EFI_HANDLE ImageHandle
    )
{
    EFI_STATUS              Status;

    Status = EFI_SUCCESS;

    return Status;
}


EFI_DRIVER_BINDING_PROTOCOL
gDwUsbDriverBinding = {
	DwUsbDriverBindingSupported,
	DwUsbDriverBindingStart,
	DwUsbDriverBindingStop,
	0x30,
	NULL,
	NULL
};

EFI_STATUS
EFIAPI
DwUsbHostEntryPoint (
	IN EFI_HANDLE                            ImageHandle,
	IN EFI_SYSTEM_TABLE                      *SystemTable
	)
{
	DEBUG ((EFI_D_ERROR, "DwUsbEntryPoint \n"));
	return EfiLibInstallDriverBindingComponentName2 ( 
		ImageHandle,
		SystemTable,
		&gDwUsbDriverBinding,
		ImageHandle,
		&gDwUsbComponentName,
		&gDwUsbComponentName2
		);
}
