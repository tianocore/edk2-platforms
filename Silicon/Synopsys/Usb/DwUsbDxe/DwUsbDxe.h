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

#ifndef __DW_USB_DXE_H__
#define __DW_USB_DXE_H__

#define DW_USB_BASE FixedPcdGet32 (PcdDwUsbBaseAddress)
#define USB_PHY_BASE FixedPcdGet32 (PcdDwUsbSysCtrlBaseAddress)

// This stuff is a verbatim copy of the linux phy-hi6220-usb.c driver
// Its also in the linaro hikey edk2 git tree.
// No real explanation of what these do beyond that file.
/*#define SC_PERIPH_CTRL4			0x00c

#define CTRL4_PICO_SIDDQ		BIT6
#define CTRL4_PICO_OGDISABLE		BIT8
#define CTRL4_PICO_VBUSVLDEXT		BIT10
#define CTRL4_PICO_VBUSVLDEXTSEL	BIT11
#define CTRL4_OTG_PHY_SEL		BIT21

#define SC_PERIPH_CTRL5			0x010

#define CTRL5_USBOTG_RES_SEL		BIT3
#define CTRL5_PICOPHY_ACAENB		BIT4
#define CTRL5_PICOPHY_BC_MODE		BIT5
#define CTRL5_PICOPHY_CHRGSEL		BIT6
#define CTRL5_PICOPHY_VDATSRCEND	BIT7
#define CTRL5_PICOPHY_VDATDETENB	BIT8
#define CTRL5_PICOPHY_DCDENB		BIT9
#define CTRL5_PICOPHY_IDDIG		BIT10

#define SC_PERIPH_CTRL8			0x018
#define SC_PERIPH_RSTEN0		0x300
#define SC_PERIPH_RSTDIS0		0x304

#define RST0_USBOTG_BUS			BIT4
#define RST0_POR_PICOPHY		BIT5
#define RST0_USBOTG			BIT6
#define RST0_USBOTG_32K			BIT7

#define EYE_PATTERN_PARA		0x7053348c*/

// GPIO pins to enable hub and turn on host mode
#define GPIODATA_3              0x0020
#define GPIODATA_7              0x0200
#define GPIODIR                 0x0400
#define GPIOAFSEL               0x0420

#define USB_SEL_GPIO0_3          3     // GPIO 0_3
#define USB_5V_HUB_EN            7     // GPIO 0_7
#define USB_ID_DET_GPIO2_5       21    // GPIO 2_5
#define USB_VBUS_DET_GPIO2_6     22    // GPIO 2_6
 

#define PHY_READ_REG32(Offset) MmioRead32 (USB_PHY_BASE + Offset)
#define PHY_WRITE_REG32(Offset, Val)  MmioWrite32 (USB_PHY_BASE + Offset, Val)
#define GPIO_READ_REG32(Offset) MmioRead32(0xf8011000+Offset)
#define GPIO_WRITE_REG32(Offset, Val) MmioWrite32(0xf8011000+Offset,Val)

#define READ_REG64(Offset) MmioRead64 (DW_USB_BASE + Offset)
#define READ_REG32(Offset) MmioRead32 (DW_USB_BASE + Offset)
#define READ_REG16(Offset) (UINT16) READ_REG32 (Offset)
#define WRITE_REG64(Offset, Val)  MmioWrite64 (DW_USB_BASE + Offset, Val)
#define WRITE_REG32(Offset, Val)  MmioWrite32 (DW_USB_BASE + Offset, Val)
#define WRITE_REG16(Offset, Val)  MmioWrite32 (DW_USB_BASE + Offset, (UINT32) Val)
#define WRITE_REG8(Offset, Val)   MmioWrite32 (DW_USB_BASE + Offset, (UINT32) Val)

// Max packet size in bytes (For Full Speed USB 64 is the only valid value)
#define MAX_PACKET_SIZE_CONTROL     64

#define MAX_PACKET_SIZE_BULK        512

// 8 Endpoints, in and out. Don't count the Endpoint 0 setup buffer
#define DW_NUM_ENDPOINTS               16

// Endpoint Indexes
#define DW_EP0SETUP                    0x20
#define DW_EP0RX                       0x00
#define DW_EP0TX                       0x01
#define DW_EP1RX                       0x02
#define DW_EP1TX                       0x03

// DcInterrupt bits
#define DW_DC_INTERRUPT_BRESET         BIT0
#define DW_DC_INTERRUPT_SOF            BIT1
#define DW_DC_INTERRUPT_PSOF           BIT2
#define DW_DC_INTERRUPT_SUSP           BIT3
#define DW_DC_INTERRUPT_RESUME         BIT4
#define DW_DC_INTERRUPT_HS_STAT        BIT5
#define DW_DC_INTERRUPT_DMA            BIT6
#define DW_DC_INTERRUPT_VBUS           BIT7
#define DW_DC_INTERRUPT_EP0SETUP       BIT8
#define DW_DC_INTERRUPT_EP0RX          BIT10
#define DW_DC_INTERRUPT_EP0TX          BIT11
#define DW_DC_INTERRUPT_EP1RX          BIT12
#define DW_DC_INTERRUPT_EP1TX          BIT13
// All valid peripheral controller interrupts
#define DW_DC_INTERRUPT_MASK           0x003FFFDFF

#define DW_ADDRESS                     0x200
#define DW_ADDRESS_DEVEN               BIT7

#define DW_MODE                        0x20C
#define DW_MODE_DATA_BUS_WIDTH         BIT8
#define DW_MODE_CLKAON                 BIT7
#define DW_MODE_SFRESET                BIT4
#define DW_MODE_WKUPCS                 BIT2

#define DW_ENDPOINT_MAX_PACKET_SIZE    0x204

#define DW_ENDPOINT_TYPE               0x208
#define DW_ENDPOINT_TYPE_NOEMPKT       BIT4
#define DW_ENDPOINT_TYPE_ENABLE        BIT3

#define DW_INTERRUPT_CONFIG            0x210
// Interrupt config value to only interrupt on ACK of IN and OUT tokens
#define DW_INTERRUPT_CONFIG_ACK_ONLY   BIT2 | BIT5 | BIT6

#define DW_DC_INTERRUPT                0x218
#define DW_DC_INTERRUPT_ENABLE         0x214

#define DW_CTRL_FUNCTION               0x228
#define DW_CTRL_FUNCTION_VENDP         BIT3
#define DW_CTRL_FUNCTION_DSEN          BIT2
#define DW_CTRL_FUNCTION_STATUS        BIT1

#define DW_DEVICE_UNLOCK               0x27C
#define DW_DEVICE_UNLOCK_MAGIC         0xAA37

#define DW_SW_RESET_REG                0x30C
#define DW_SW_RESET_ALL                BIT0

#define DW_DEVICE_ID                   0x370

#define DW_OTG_CTRL_SET                0x374
#define DW_OTG_CTRL_CLR                OTG_CTRL_SET + 2
#define DW_OTG_CTRL_OTG_DISABLE        BIT10
#define DW_OTG_CTRL_VBUS_CHRG          BIT6
#define DW_OTG_CTRL_VBUS_DISCHRG       BIT5
#define DW_OTG_CTRL_DM_PULLDOWN        BIT2
#define DW_OTG_CTRL_DP_PULLDOWN        BIT1
#define DW_OTG_CTRL_DP_PULLUP          BIT0

#define DW_OTG_STATUS                  0x378
#define DW_OTG_STATUS_B_SESS_END       BIT7
#define DW_OTG_STATUS_A_B_SESS_VLD     BIT1

#define DW_OTG_INTERRUPT_LATCH_SET     0x37C
#define DW_OTG_INTERRUPT_LATCH_CLR     0x37E
#define DW_OTG_INTERRUPT_ENABLE_RISE   0x384

#define DW_DMA_ENDPOINT_INDEX          0x258

#define DW_ENDPOINT_INDEX              0x22c
#define DW_DATA_PORT                   0x220
#define DW_BUFFER_LENGTH               0x21c

// Device ID Values
#define PHILLIPS_VENDOR_ID_VAL 0x04cc
#define DW_PRODUCT_ID_VAL 0x1761
#define DW_DEVICE_ID_VAL ((ISP1761_PRODUCT_ID_VAL << 16) |\
                               PHILLIPS_VENDOR_ID_VAL)

#define DWC_OTG_BASE		       DW_USB_BASE

#define USB_NUM_ENDPOINTS          2
#define MAX_EPS_CHANNELS 		   16

#define BULK_OUT_EP 			   1
#define BULK_IN_EP				   1

#define RX_REQ_LEN 				   512
#define MAX_PACKET_LEN 			   512

#define DATA_FIFO_CONFIG 		   0x0F801000
/* RX FIFO: 2048 bytes */
#define RX_SIZE          		   0x00000200
/* Non-periodic TX FIFO: 128 bytes. start address: 0x200 * 4. */
#define ENDPOINT_TX_SIZE 		   0x01000200

/* EP1  TX FIFO: 1024 bytes. start address: 0x300 * 4. */
/* EP2  TX FIFO: 1024 bytes. start address: 0x400 * 4. */
/* EP3  TX FIFO: 1024 bytes. start address: 0x500 * 4. */
/* EP4  TX FIFO: 1024 bytes. start address: 0x600 * 4. */
/* EP5  TX FIFO: 1024 bytes. start address: 0x700 * 4. */
/* EP6  TX FIFO: 1024 bytes. start address: 0x800 * 4. */
/* EP7  TX FIFO: 1024 bytes. start address: 0x900 * 4. */
/* EP8  TX FIFO: 1024 bytes. start address: 0xA00 * 4. */
/* EP9  TX FIFO: 1024 bytes. start address: 0xB00 * 4. */
/* EP10 TX FIFO: 1024 bytes. start address: 0xC00 * 4. */
/* EP11 TX FIFO: 512  bytes. start address: 0xD00 * 4. */
/* EP12 TX FIFO: 512  bytes. start address: 0xD80 * 4. */
/* EP13 TX FIFO: 512  bytes. start address: 0xE00 * 4. */
/* EP14 TX FIFO: 512  bytes. start address: 0xE80 * 4. */
/* EP15 TX FIFO: 512  bytes. start address: 0xF00 * 4. */

#define DATA_IN_ENDPOINT_TX_FIFO1  0x01000300
#define DATA_IN_ENDPOINT_TX_FIFO2  0x01000400
#define DATA_IN_ENDPOINT_TX_FIFO3  0x01000500
#define DATA_IN_ENDPOINT_TX_FIFO4  0x01000600
#define DATA_IN_ENDPOINT_TX_FIFO5  0x01000700
#define DATA_IN_ENDPOINT_TX_FIFO6  0x01000800
#define DATA_IN_ENDPOINT_TX_FIFO7  0x01000900
#define DATA_IN_ENDPOINT_TX_FIFO8  0x01000A00
#define DATA_IN_ENDPOINT_TX_FIFO9  0x01000B00
#define DATA_IN_ENDPOINT_TX_FIFO10 0x01000C00
#define DATA_IN_ENDPOINT_TX_FIFO11 0x00800D00
#define DATA_IN_ENDPOINT_TX_FIFO12 0x00800D80
#define DATA_IN_ENDPOINT_TX_FIFO13 0x00800E00
#define DATA_IN_ENDPOINT_TX_FIFO14 0x00800E80
#define DATA_IN_ENDPOINT_TX_FIFO15 0x00800F00

/*DWC_OTG regsiter descriptor*/
/*Device mode CSR MAP*/
#define DEVICE_CSR_BASE			(0x800)
/*Device mode CSR MAP*/
#define DEVICE_INEP_BASE		(0x900)
/*Device mode CSR MAP*/
#define DEVICE_OUTEP_BASE		(0xB00)

/*** OTG LINK CORE REGISTERS ***/
/* Core Global Registers */
#define GOTGCTL     			(0x000) 
#define GOTGINT     			(0x004)
#define GAHBCFG     			(0x008) //Global AHB Config
#define GAHBCFG_AHB_SINGLE              (1 << 23)
#define GAHBCFG_NOTI_ALL_DMA_WRIT       (1 << 22)
#define GAHBCFG_REM_MEM_SUPP            (1 << 21)
#define GAHBCFG_P_TXF_EMP_LVL           (1 << 8)
#define GAHBCFG_NP_TXF_EMP_LVL          (1 << 7)
#define GAHBCFG_DMA_EN                  (1 << 5)
#define GAHBCFG_HBSTLEN_MASK            (0xf << 1)
#define GAHBCFG_HBSTLEN_SHIFT           1
#define GAHBCFG_HBSTLEN_SINGLE          0
#define GAHBCFG_HBSTLEN_INCR            1
#define GAHBCFG_HBSTLEN_INCR4           3
#define GAHBCFG_HBSTLEN_INCR8           5
#define GAHBCFG_HBSTLEN_INCR16          7
#define GAHBCFG_GLBL_INTR_EN            (1 << 0)
#define GAHBCFG_CTRL_MASK               (GAHBCFG_P_TXF_EMP_LVL | \
                                         GAHBCFG_NP_TXF_EMP_LVL | \
                                         GAHBCFG_DMA_EN | \
                                         GAHBCFG_GLBL_INTR_EN)
#define GUSBCFG     			(0x00C)
#define GRSTCTL     			(0x010) //Global reset control
#define GRSTCTL_AHBIDLE                 (1 << 31)
#define GRSTCTL_DMAREQ                  (1 << 30)
#define GRSTCTL_TXFNUM_MASK             (0x1f << 6)
#define GRSTCTL_TXFNUM_SHIFT            6
#define GRSTCTL_TXFNUM_LIMIT            0x1f
#define GRSTCTL_TXFNUM(_x)              ((_x) << 6)
#define GRSTCTL_TXFFLSH                 (1 << 5)
#define GRSTCTL_RXFFLSH                 (1 << 4)
#define GRSTCTL_IN_TKNQ_FLSH            (1 << 3)
#define GRSTCTL_FRMCNTRRST              (1 << 2)
#define GRSTCTL_HSFTRST                 (1 << 1)
#define GRSTCTL_CSFTRST                 (1 << 0)

#define GINTSTS				    (0x014)
#define GINTMSK     			(0x018)
#define GRXSTSR     			(0x01C)
#define GRXSTSP     			(0x020)
#define GRXFSIZ     			(0x024) //Global RX FIFO Size
#define GNPTXFSIZ   			(0x028) //Global non periodic TX FIFO Size
#define GNPTXSTS    			(0x02C) //Global non periodic TX FIFO Status
#define GSNPSID                 (0x040) //looks like a chip version?

#define GHWCFG1     			(0x044)
#define GHWCFG2     			(0x048)
#define GHWCFG3     			(0x04c)
#define GHWCFG4     			(0x050)
#define GLPMCFG     			(0x054)

#define GDFIFOCFG     			(0x05c)

#define HPTXFSIZ    			(0x100)
#define DIEPTXF(x) 			(0x100 + 4 * (x))
#define DIEPTXF1   			(0x104)
#define DIEPTXF2   			(0x108)
#define DIEPTXF3   			(0x10C)
#define DIEPTXF4   			(0x110)
#define DIEPTXF5   			(0x114)
#define DIEPTXF6   			(0x118)
#define DIEPTXF7   			(0x11C)
#define DIEPTXF8   			(0x120)
#define DIEPTXF9   			(0x124)
#define DIEPTXF10  			(0x128)
#define DIEPTXF11  			(0x12C)
#define DIEPTXF12  			(0x130)
#define DIEPTXF13  			(0x134)
#define DIEPTXF14  			(0x138)
#define DIEPTXF15  			(0x13C)

/*** HOST MODE REGISTERS ***/
/* Host Global Registers */
#define HCFG       			(0x400) // host config
#define HFIR       			(0x404) // host frame interval
#define HFNUM      			(0x408) // host frame number
#define HFNUM_FRNUM_MASK                (0xffff << 0) 
#define HPTXSTS    			(0x410) // host tx fifo stats
#define HAINT      			(0x414) // host all channels interrupt flags
#define HAINTMSK   			(0x418) // host all channels interrupt mask

/* Host Port Control and Status Registers */
#define HPRT        			(0x440)
#define HPRT0_SPD_MASK                  (0x3 << 17)
#define HPRT0_SPD_SHIFT                 17
#define HPRT0_SPD_HIGH_SPEED            0
#define HPRT0_SPD_FULL_SPEED            1
#define HPRT0_SPD_LOW_SPEED             2
#define HPRT0_TSTCTL_MASK               (0xf << 13)
#define HPRT0_TSTCTL_SHIFT              13
#define HPRT0_PWR                       (1 << 12)
#define HPRT0_LNSTS_MASK                (0x3 << 10)
#define HPRT0_LNSTS_SHIFT               10
#define HPRT0_RST                       (1 << 8)
#define HPRT0_SUSP                      (1 << 7)
#define HPRT0_RES                       (1 << 6)
#define HPRT0_OVRCURRCHG                (1 << 5)
#define HPRT0_OVRCURRACT                (1 << 4)
#define HPRT0_ENACHG                    (1 << 3)
#define HPRT0_ENA                       (1 << 2)
#define HPRT0_CONNDET                   (1 << 1)
#define HPRT0_CONNSTS                   (1 << 0)

/* Host Channel-Specific Registers */
#define HCCHAR(x)   			(0x500 + 0x20 * (x)) //channel characteristics
#define HCCHAR_CHENA                    (1 << 31) // channel enable
#define HCCHAR_CHDIS                    (1 << 30) // channel disable
#define HCCHAR_ODDFRM                   (1 << 29)
#define HCCHAR_DEVADDR_MASK             (0x7f << 22)
#define HCCHAR_DEVADDR_SHIFT            22
#define HCCHAR_MULTICNT_MASK            (0x3 << 20)
#define HCCHAR_MULTICNT_SHIFT           20
#define HCCHAR_EPTYPE_MASK              (0x3 << 18)
#define HCCHAR_EPTYPE_SHIFT             18
#define HCCHAR_LSPDDEV                  (1 << 17)
#define HCCHAR_EPDIR                    (1 << 15)
#define HCCHAR_EPNUM_MASK               (0xf << 11)
#define HCCHAR_EPNUM_SHIFT              11
#define HCCHAR_MPS_MASK                 (0x7ff << 0)
#define HCCHAR_MPS_SHIFT                0
#define HCSPLT(x)   			(0x504 + 0x20 * (x))
#define HCINT(x)    			(0x508 + 0x20 * (x))
#define HCINTMSK(x) 			(0x50C + 0x20 * (x))
#define HCINTMSK_RESERVED14_31          (0x3ffff << 14)
#define HCINTMSK_FRM_LIST_ROLL          (1 << 13)
#define HCINTMSK_XCS_XACT               (1 << 12)
#define HCINTMSK_BNA                    (1 << 11)
#define HCINTMSK_DATATGLERR             (1 << 10)
#define HCINTMSK_FRMOVRUN               (1 << 9)
#define HCINTMSK_BBLERR                 (1 << 8)
#define HCINTMSK_XACTERR                (1 << 7)
#define HCINTMSK_NYET                   (1 << 6)
#define HCINTMSK_ACK                    (1 << 5)
#define HCINTMSK_NAK                    (1 << 4)
#define HCINTMSK_STALL                  (1 << 3)
#define HCINTMSK_AHBERR                 (1 << 2)
#define HCINTMSK_CHHLTD                 (1 << 1) //channel halted
#define HCINTMSK_XFERCOMPL              (1 << 0)
#define HCTSIZ(x)   			(0x510 + 0x20 * (x)) //transfer size/packet reg
#define TSIZ_DOPNG                      (1 << 31) //do ping before transfer
#define TSIZ_SC_MC_PID_MASK             (0x3 << 29) //USB packet id
#define TSIZ_SC_MC_PID_SHIFT            29
#define TSIZ_SC_MC_PID_DATA0            0
#define TSIZ_SC_MC_PID_DATA2            1
#define TSIZ_SC_MC_PID_DATA1            2
#define TSIZ_SC_MC_PID_MDATA            3
#define TSIZ_SC_MC_PID_SETUP            3
#define TSIZ_PKTCNT_MASK                (0x3ff << 19)
#define TSIZ_PKTCNT_SHIFT               19
#define TSIZ_NTD_MASK                   (0xff << 8)
#define TSIZ_NTD_SHIFT                  8
#define TSIZ_SCHINFO_MASK               (0xff << 0)
#define TSIZ_SCHINFO_SHIFT              0
#define TSIZ_XFERSIZE_MASK              (0x7ffff << 0)
#define TSIZ_XFERSIZE_SHIFT             0
#define HCDMA(x)    			(0x514 + 0x20 * (x))

/*** DEVICE MODE REGISTERS ***/
/* Device Global Registers */
#define DCFG        			(0x800)
#define DCTL        			(0x804)
#define DSTS        			(0x808)
#define DIEPMSK     			(0x810)
#define DOEPMSK     			(0x814)
#define DAINT       			(0x818)
#define DAINTMSK    			(0x81C)
#define DTKNQR1     			(0x820)
#define DTKNQR2     			(0x824)
#define DVBUSDIS    			(0x828)
#define DVBUSPULSE  			(0x82C)
#define DTHRCTL     			(0x830)

/* Device Logical IN Endpoint-Specific Registers */
#define DIEPCTL(x)  			(0x900 + 0x20 * (x))
#define DIEPINT(x)  			(0x908 + 0x20 * (x))
#define DIEPTSIZ(x) 			(0x910 + 0x20 * (x))
#define DIEPDMA(x)  			(0x914 + 0x20 * (x))
#define DTXFSTS(x)  			(0x918 + 0x20 * (x))

/* Device Logical OUT Endpoint-Specific Registers */
#define DOEPCTL(x)  			(0xB00 + 0x20 * (x))
#define DOEPINT(x)  			(0xB08 + 0x20 * (x))
#define DOEPTSIZ(x) 			(0xB10 + 0x20 * (x))
#define DOEPDMA(x)  			(0xB14 + 0x20 * (x))

/* Power and Clock Gating Register */
#define PCGCCTL				(0xE00)

#define EP0FIFO				(0x1000)

#define USB_ENDPOINT_XFER_CONTROL       0
#define USB_ENDPOINT_XFER_ISOC          1
#define USB_ENDPOINT_XFER_BULK          2
#define USB_ENDPOINT_XFER_INT           3

/**
 * This union represents the bit fields in the DMA Descriptor
 * status quadlet. Read the quadlet into the <i>d32</i> member then
 * set/clear the bits using the <i>b</i>it, <i>b_iso_out</i> and
 * <i>b_iso_in</i> elements.
 */
typedef union dev_dma_desc_sts {
		/** raw register data */
	unsigned int d32;
		/** quadlet bits */
	struct {
		/** Received number of bytes */
		unsigned bytes:16;
		/** NAK bit - only for OUT EPs */
		unsigned nak:1;
		unsigned reserved17_22:6;
		/** Multiple Transfer - only for OUT EPs */
		unsigned mtrf:1;
		/** Setup Packet received - only for OUT EPs */
		unsigned sr:1;
		/** Interrupt On Complete */
		unsigned ioc:1;
		/** Short Packet */
		unsigned sp:1;
		/** Last */
		unsigned l:1;
		/** Receive Status */
		unsigned sts:2;
		/** Buffer Status */
		unsigned bs:2;
	} b;
} dev_dma_desc_sts_t;

/**
 * DMA Descriptor structure
 *
 * DMA Descriptor structure contains two quadlets:
 * Status quadlet and Data buffer pointer.
 */
typedef struct dwc_otg_dev_dma_desc {
	/** DMA Descriptor status quadlet */
	dev_dma_desc_sts_t status;
	/** DMA Descriptor data buffer pointer */
	UINT32 buf;
} dwc_otg_dev_dma_desc_t;














typedef struct _USB_OHCI_HC_DEV USB_OHCI_HC_DEV;

struct _USB_OHCI_HC_DEV {
  UINTN                     Signature;
  EFI_USB_HC_PROTOCOL       UsbHc;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT64                    OriginalPciAttributes;

//  HCCA_MEMORY_BLOCK         *HccaMemoryBlock;
  VOID                      *HccaMemoryBuf;
  VOID                      *HccaMemoryMapping;
  UINTN                     HccaMemoryPages;

//  ED_DESCRIPTOR             *IntervalList[6][32];
//  INTERRUPT_CONTEXT_ENTRY   *InterruptContextList;
  VOID                      *MemPool;

  UINT32                    ToggleFlag;

  EFI_EVENT                 HouseKeeperTimer;
  //
  // ExitBootServicesEvent is used to stop the OHC DMA operation
  // after exit boot service.
  //
  EFI_EVENT                  ExitBootServiceEvent;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;

  UINT32                    PortStatus;
};

#define USB_DW_HC_DEV_SIGNATURE     SIGNATURE_32('d','u','s','b')
extern EFI_DRIVER_BINDING_PROTOCOL   gOhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gOhciComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gOhciComponentName2;

#define USB_OHCI_HC_DEV_FROM_THIS(a)    CR(a, USB_OHCI_HC_DEV, UsbHc, USB_DW_HC_DEV_SIGNATURE)
#define USB2_OHCI_HC_DEV_FROM_THIS(a)    CR(a, USB_OHCI_HC_DEV, Usb2Hc, USB_DW_HC_DEV_SIGNATURE)





#endif //ifndef __DW_USB_DXE_H__
