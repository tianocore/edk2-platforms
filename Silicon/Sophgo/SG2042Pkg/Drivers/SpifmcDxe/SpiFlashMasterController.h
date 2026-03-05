/** @file
 *
 *  SPI Flash Master Controller (SPIFMC) registers.
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/
#ifndef __SPI_DXE_H__
#define __SPI_DXE_H__

#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Include/Spifmc.h>
#include <Include/SpiNorFlash.h>

//
// SPIFMC registers
//
#define SPIFMC_CTRL                        0x00
#define SPIFMC_CTRL_CPHA                      BIT12
#define SPIFMC_CTRL_CPOL                      BIT13
#define SPIFMC_CTRL_HOLD_OL                   BIT14
#define SPIFMC_CTRL_WP_OL                     BIT15
#define SPIFMC_CTRL_LSBF                      BIT20
#define SPIFMC_CTRL_SRST                      BIT21
#define SPIFMC_CTRL_SCK_DIV_SHIFT             0
#define SPIFMC_CTRL_SCK_DIV_SHIFT_MASK        0x7FF
#define SPIFMC_CTRL_FRAME_LEN_SHIFT           16

#define SPIFMC_CE_CTRL                     0x04
#define SPIFMC_CE_CTRL_CEMANUAL               BIT0
#define SPIFMC_CE_CTRL_CEMANUAL_EN            BIT1

#define SPIFMC_DLY_CTRL                    0x08
#define SPIFMC_CTRL_FM_INTVL_MASK             0x000f
#define SPIFMC_CTRL_FM_INTVL                  BIT0
#define SPIFMC_CTRL_CET_MASK                  0x0f00
#define SPIFMC_CTRL_CET                       BIT8

#define SPIFMC_DMMR                        0x0c

#define SPIFMC_TRAN_CSR                    0x10
#define SPIFMC_TRAN_CSR_TRAN_MODE_MASK        0x0003
#define SPIFMC_TRAN_CSR_TRAN_MODE_RX          BIT0
#define SPIFMC_TRAN_CSR_TRAN_MODE_TX          BIT1
#define SPIFMC_TRAN_CSR_CNTNS_READ            BIT2
#define SPIFMC_TRAN_CSR_FAST_MODE             BIT3
#define SPIFMC_TRAN_CSR_BUS_WIDTH_1_BIT       (0x00 << 4)
#define SPIFMC_TRAN_CSR_BUS_WIDTH_2_BIT       (0x01 << 4)
#define SPIFMC_TRAN_CSR_BUS_WIDTH_4_BIT       (0x02 << 4)
#define SPIFMC_TRAN_CSR_DMA_EN                BIT6
#define SPIFMC_TRAN_CSR_MISO_LEVEL            BIT7
#define SPIFMC_TRAN_CSR_ADDR_BYTES_MASK       0x0700
#define SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT      8
#define SPIFMC_TRAN_CSR_WITH_CMD              BIT11
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_MASK     0x3000
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE   (0x00 << 12)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_2_BYTE   (0x01 << 12)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_4_BYTE   (0x02 << 12)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE   (0x03 << 12)
#define SPIFMC_TRAN_CSR_GO_BUSY               BIT15

#define SPIFMC_TRAN_NUM                    0x14
#define SPIFMC_FIFO_PORT                   0x18
#define SPIFMC_FIFO_PT                     0x20

#define SPIFMC_INT_STS                     0x28
#define SPIFMC_INT_TRAN_DONE                  BIT0
#define SPIFMC_INT_RD_FIFO                    BIT2
#define SPIFMC_INT_WR_FIFO                    BIT3
#define SPIFMC_INT_RX_FRAME                   BIT4
#define SPIFMC_INT_TX_FRAME                   BIT5

#define SPIFMC_INT_EN                     0x2c
#define SPIFMC_INT_TRAN_DONE_EN               BIT0
#define SPIFMC_INT_RD_FIFO_EN                 BIT2
#define SPIFMC_INT_WR_FIFO_EN                 BIT3
#define SPIFMC_INT_RX_FRAME_EN                BIT4
#define SPIFMC_INT_TX_FRAME_EN                BIT5

#define SPIFMC_OPT                        0x30

#define SPIFMC_MAX_FIFO_DEPTH             8

#define SPI_MASTER_SIGNATURE                      SIGNATURE_32 ('M', 'S', 'P', 'I')
#define SPI_MASTER_FROM_SPI_MASTER_PROTOCOL(a)    CR (a, SPI_MASTER, SpiMasterProtocol, SPI_MASTER_SIGNATURE)

///
/// Global ID for the SPI NOR Flash Protocol
///
#define SOPHGO_SPI_MASTER_PROTOCOL_GUID  \
  { 0xB67F29A5, 0x7E7D, 0x48C6,          \
    { 0xA0, 0x00, 0xE8, 0xB5, 0x1D, 0x6D, 0x3A, 0xA8 } }

typedef struct {
  SOPHGO_SPI_MASTER_PROTOCOL SpiMasterProtocol;
  UINTN                      Signature;
  EFI_HANDLE                 Handle;
  EFI_LOCK                   Lock;
} SPI_MASTER;

EFI_STATUS
EFIAPI
SpifmcReadRegister (
  IN  SPI_NOR *Nor,
  IN  UINT8   Opcode,
  IN  UINTN   Length,
  OUT UINT8   *Buffer
  );

EFI_STATUS
EFIAPI
SpifmcWriteRegister (
  IN SPI_NOR      *Nor,
  IN UINT8        Opcode,
  IN CONST UINT8 *Buffer,
  IN UINTN        Length
  );

EFI_STATUS
EFIAPI
SpifmcRead (
  IN  SPI_NOR *Nor,
  IN  UINTN   From,
  IN  UINTN   Length,
  OUT UINT8   *Buffer
  );

EFI_STATUS
EFIAPI
SpifmcWrite (
  IN SPI_NOR     *Nor,
  IN UINTN       To,
  IN UINTN       Length,
  IN CONST UINT8 *Buffer
  );

EFI_STATUS
EFIAPI
SpifmcErase (
  IN SPI_NOR *Nor,
  IN UINTN   Offs
  );

SPI_NOR *
EFIAPI
SpiMasterSetupSlave (
  IN SOPHGO_SPI_MASTER_PROTOCOL *This,
  IN UINT8                      SelectedFlashNumber
  );

EFI_STATUS
EFIAPI
SpifmcInit (
  IN SPI_NOR *Nor
  );

EFI_STATUS
EFIAPI
SpiMasterEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

#endif //__SPI_DXE_H__
