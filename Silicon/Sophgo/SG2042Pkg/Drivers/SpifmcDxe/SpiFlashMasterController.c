/** @file
 *  SPI Flash Master Controller (SPIFMC)
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/BaseLib.h>
#include "SpiFlashMasterController.h"
#include <Library/TimerLib.h>

SPI_MASTER                  *mSpiMasterInstance;
STATIC EFI_EVENT            mNorFlashVirtualAddrChangeEvent;
STATIC SPI_NOR              *mNorFlashInstance;
STATIC UINT8                mSpiControllerNum;

STATIC
EFI_STATUS
SpifmcWaitInt (
  IN UINTN   SpiBase,
  IN UINT8   IntType
  )
{
  UINT32  Stat;

  while (1) {
    Stat = MmioRead32 ((UINTN)(SpiBase + SPIFMC_INT_STS));
    if (Stat & IntType) {
      return EFI_SUCCESS;
    }
  }

  return EFI_TIMEOUT;
}

STATIC
UINT32
SpifmcInitReg (
  IN UINTN   SpiBase
  )
{
  UINT32 Register;

  Register = MmioRead32 ((UINTN)(SpiBase + SPIFMC_TRAN_CSR));
  Register &= ~(SPIFMC_TRAN_CSR_TRAN_MODE_MASK
           | SPIFMC_TRAN_CSR_CNTNS_READ
           | SPIFMC_TRAN_CSR_FAST_MODE
           | SPIFMC_TRAN_CSR_BUS_WIDTH_2_BIT
           | SPIFMC_TRAN_CSR_BUS_WIDTH_4_BIT
           | SPIFMC_TRAN_CSR_DMA_EN
           | SPIFMC_TRAN_CSR_ADDR_BYTES_MASK
           | SPIFMC_TRAN_CSR_WITH_CMD
           | SPIFMC_TRAN_CSR_FIFO_TRG_LVL_MASK);

  return Register;
}

/**
  SpifmcReadRegister is a workaround function:
  AHB bus could only do 32-bit access to SPIFMC fifo,
  so cmd without 3-byte addr will leave 3-byte data in fifo.
  Set TX to mark that these 3-byte data would be sent out.
**/
EFI_STATUS
EFIAPI
SpifmcReadRegister (
  IN  SPI_NOR *Nor,
  IN  UINT8   Opcode,
  IN  UINTN   Length,
  OUT UINT8   *Buffer
  )
{
  INT32      Index;
  UINTN      SpiBase;
  UINT32     Register;
  EFI_STATUS Status;

  SpiBase = Nor->SpiBase;
  Register = SpifmcInitReg (SpiBase);
  Register |= SPIFMC_TRAN_CSR_BUS_WIDTH_1_BIT;
  Register |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
  Register |= SPIFMC_TRAN_CSR_WITH_CMD;
  Register |= SPIFMC_TRAN_CSR_TRAN_MODE_RX | SPIFMC_TRAN_CSR_TRAN_MODE_TX;

  //
  // OPT bit[1]: Disable no address cmd fifo flush
  //
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_OPT), 2);
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);
  MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), Opcode);

  for (Index = 0; Index < Length; Index++) {
    MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), 0);
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_INT_STS), 0);
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_NUM), Length);
  Register |= SPIFMC_TRAN_CSR_GO_BUSY;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_CSR), Register);

  Status = SpifmcWaitInt (SpiBase, SPIFMC_INT_TRAN_DONE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wait Transfer Done %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  while (Length--) {
    *Buffer++ = MmioRead8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT));
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpifmcWriteRegister (
  IN SPI_NOR      *Nor,
  IN UINT8        Opcode,
  IN CONST UINT8 *Buffer,
  IN UINTN        Length
  )
{
  INT32      Index;
  UINTN      SpiBase;
  UINT32     Register;
  EFI_STATUS Status;

  SpiBase = Nor->SpiBase;

  Register = SpifmcInitReg (SpiBase);
  Register |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
  Register |= SPIFMC_TRAN_CSR_WITH_CMD;

  //
  // If write values to the Status Register,
  // configure TRAN_CSR register as the same as SpifmcReadReg.
  //
  if (Opcode == SPINOR_OP_WRSR) {
    Register |= SPIFMC_TRAN_CSR_TRAN_MODE_RX | SPIFMC_TRAN_CSR_TRAN_MODE_TX;
    MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_NUM), Length);
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);
  MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), Opcode);

  for (Index = 0; Index < Length; Index++) {
    MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), Buffer[Index]);
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_INT_STS), 0);
  Register |= SPIFMC_TRAN_CSR_GO_BUSY;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_CSR), Register);

  Status = SpifmcWaitInt (SpiBase, SPIFMC_INT_TRAN_DONE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wait Transfer Done %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpifmcRead (
  IN  SPI_NOR *Nor,
  IN  UINTN   From,
  IN  UINTN   Length,
  OUT UINT8   *Buffer
  )
{
  INT32      XferSize;
  INT32      Offset;
  INT32      Index;
  UINTN      SpiBase;
  UINT32     Register;
  EFI_STATUS Status;

  SpiBase = Nor->SpiBase;
  Offset = 0;

  Register = SpifmcInitReg (SpiBase);
  Register |= (Nor->AddrNbytes) << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
  Register |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
  Register |= SPIFMC_TRAN_CSR_WITH_CMD;
  Register |= SPIFMC_TRAN_CSR_TRAN_MODE_RX;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);
  MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), Nor->ReadOpcode);

  //
  // This is a workaround.
  // For Length<=SPIFMC_MAX_FIFO_DEPTH, we have to modify the Length manually.
  // Although we set the TRAN_CSR_FIFO_TRG_LVL_1_BYTE, not
  // TRAN_CSR_FIFO_TRG_LVL_8_BYTE, we cannot wait for the INT_RD_FIFO to be set
  // when the Length equals one byte.
  //
  Length = MAX (SPIFMC_MAX_FIFO_DEPTH, Length);

  for (Index = Nor->AddrNbytes - 1; Index >= 0; Index --) {
    MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), (From >> Index * 8) & 0xff);
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_INT_STS), 0);
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_NUM), Length);
  Register |= SPIFMC_TRAN_CSR_GO_BUSY;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_CSR), Register);

  Status = SpifmcWaitInt (SpiBase, SPIFMC_INT_RD_FIFO);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wait Read FIFO Done %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  while (Offset < Length) {
    XferSize = MIN (SPIFMC_MAX_FIFO_DEPTH, Length - Offset);

    while ((MmioRead32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT)) & 0xf) != XferSize)
      ;

    for (Index = 0; Index < XferSize; Index++) {
      Buffer[Index + Offset] = MmioRead8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT));
    }

    Offset += XferSize;
  }

  Status = SpifmcWaitInt (SpiBase, SPIFMC_INT_TRAN_DONE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wait Transfer Done %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpifmcWrite (
  IN  SPI_NOR     *Nor,
  IN  UINTN       To,
  IN  UINTN       Length,
  IN  CONST UINT8 *Buffer
  )
{
  INT32      Index;
  UINTN      SpiBase;
  INT32      Offset;
  INT32      XferSize;
  UINT32     Register;
  UINT32     WaitTime;
  EFI_STATUS Status;

  SpiBase     = Nor->SpiBase;
  Offset      = 0;
  WaitTime    = 0;

  Register = SpifmcInitReg (SpiBase);
  Register |= Nor->AddrNbytes << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
  Register |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
  Register |= SPIFMC_TRAN_CSR_WITH_CMD;
  Register |= SPIFMC_TRAN_CSR_TRAN_MODE_TX;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);
  MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), Nor->ProgramOpcode);

  for (Index = Nor->AddrNbytes - 1; Index >= 0; Index--) {
    MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), (To >> Index * 8) & 0xff);
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_INT_STS), 0);
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_NUM), Length);
  Register |= SPIFMC_TRAN_CSR_GO_BUSY;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_CSR), Register);

  while ((MmioRead32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT)) & 0xf) != 0)
    ;

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);

  while (Offset < Length) {
    XferSize = MIN (SPIFMC_MAX_FIFO_DEPTH, Length - Offset);

    while ((MmioRead32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT)) & 0xf) != 0) {
      WaitTime ++;
      MicroSecondDelay (10);
      if (WaitTime > 30000) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Wait FIFO empty timeout!\n",
          __func__
          ));
        return EFI_TIMEOUT;
      }
    }

    for (Index = 0; Index < XferSize; Index++) {
      MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), Buffer[Index + Offset]);
    }

    Offset += XferSize;
  }

  Status = SpifmcWaitInt (SpiBase, SPIFMC_INT_TRAN_DONE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wait Transfer Done %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpifmcErase (
  IN  SPI_NOR *Nor,
  IN  UINTN   Offs
  )
{
  INT32      Index;
  UINTN      SpiBase;
  UINT32     Register;
  EFI_STATUS Status;

  SpiBase = Nor->SpiBase;

  Register = SpifmcInitReg (SpiBase);
  Register |= Nor->AddrNbytes << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
  Register |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
  Register |= SPIFMC_TRAN_CSR_WITH_CMD;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);
  MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), Nor->EraseOpcode);

  for (Index = Nor->AddrNbytes - 1; Index >= 0; Index--) {
    MmioWrite8 ((UINTN)(SpiBase + SPIFMC_FIFO_PORT), (Offs >> Index * 8) & 0xff);
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_INT_STS), 0);
  Register |= SPIFMC_TRAN_CSR_GO_BUSY;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_CSR), Register);

  Status = SpifmcWaitInt (SpiBase, SPIFMC_INT_TRAN_DONE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wait Transfer Done %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_FIFO_PT), 0);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpifmcInit (
  IN  SPI_NOR *Nor
  )
{
  UINTN      SpiBase;
  UINT32     Register;

  SpiBase = Nor->SpiBase;

  //
  // disable DMMR (Direct Memory Mapping Read)
  //
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_DMMR), 0);

  //
  // Soft reset
  //
  Register = MmioRead32 ((UINTN)(SpiBase + SPIFMC_CTRL));
  Register &= ~SPIFMC_CTRL_SCK_DIV_SHIFT_MASK;
  Register |= SPIFMC_CTRL_SRST;
  // SCK frequency = HCLK frequency / (2 * (SckDiv + 1))
  Register |= 0x3;
  MmioWrite32 (SpiBase + SPIFMC_CTRL, Register);

  //
  // Hardware CE contrl, soft reset cannot change the register
  //
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_CE_CTRL), 0);

  Register = Nor->AddrNbytes << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
  Register |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_4_BYTE;
  Register |= SPIFMC_TRAN_CSR_WITH_CMD;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_TRAN_CSR), Register);

  return EFI_SUCCESS;
}

SPI_NOR *
EFIAPI
SpiMasterSetupSlave (
  IN SOPHGO_SPI_MASTER_PROTOCOL *This,
  IN UINT8                      SelectedFlashNumber
  )
{
  if (SelectedFlashNumber >= mSpiControllerNum) {
    return NULL;
  }

  return &(mNorFlashInstance[SelectedFlashNumber]);
}

STATIC
VOID
EFIAPI
SpiNorVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{

  EfiConvertPointer (0x0, (VOID**)&mSpiMasterInstance->SpiMasterProtocol.ReadRegister);
  EfiConvertPointer (0x0, (VOID**)&mSpiMasterInstance->SpiMasterProtocol.WriteRegister);
  EfiConvertPointer (0x0, (VOID**)&mSpiMasterInstance->SpiMasterProtocol.Read);
  EfiConvertPointer (0x0, (VOID**)&mSpiMasterInstance->SpiMasterProtocol.Write);
  EfiConvertPointer (0x0, (VOID**)&mSpiMasterInstance->SpiMasterProtocol.Erase);
  EfiConvertPointer (0x0, (VOID**)&mSpiMasterInstance);
  EfiConvertPointer (0x0, (VOID**)&mNorFlashInstance);

}

STATIC
EFI_STATUS
SetMemory (
  VOID
  )
{
  EFI_STATUS  Status;
  SPI_NOR     *Nor;
  INT32       Index;

  Nor = mNorFlashInstance;

  for (Index = 0; Index < mSpiControllerNum; ++Index, ++Nor) {
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    Nor->SpiBase,
                    SIZE_16MB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP | EFI_MEMORY_RUNTIME
                  );
    if (Status == EFI_ACCESS_DENIED) {
      goto init;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Add memory space failed: %r\n",
            __func__, __LINE__, Status));
      return Status;
    }

  init:
    Status = gDS->SetMemorySpaceAttributes (
                    Nor->SpiBase,
                    SIZE_16MB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP | EFI_MEMORY_RUNTIME
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to set memory attributes\n", __func__));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpifmcEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS  Status;
  INT32       Index, Loop, I;
  SPI_NOR     *Nor;
  UINTN       Base;

  mSpiMasterInstance = AllocateRuntimeZeroPool (sizeof (SPI_MASTER));
  if (mSpiMasterInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock (&mSpiMasterInstance->Lock, TPL_NOTIFY);

  mSpiMasterInstance->SpiMasterProtocol.ReadRegister   = SpifmcReadRegister;
  mSpiMasterInstance->SpiMasterProtocol.WriteRegister  = SpifmcWriteRegister;
  mSpiMasterInstance->SpiMasterProtocol.Read           = SpifmcRead;
  mSpiMasterInstance->SpiMasterProtocol.Write          = SpifmcWrite;
  mSpiMasterInstance->SpiMasterProtocol.Erase          = SpifmcErase;
  mSpiMasterInstance->SpiMasterProtocol.SetupDevice    = SpiMasterSetupSlave;

  mSpiMasterInstance->Signature = SPI_MASTER_SIGNATURE;

  Status = gBS->InstallMultipleProtocolInterfaces (
                    &(mSpiMasterInstance->Handle),
                    &gSophgoSpiMasterProtocolGuid,
                    &(mSpiMasterInstance->SpiMasterProtocol),
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    FreePool (mSpiMasterInstance);
    return EFI_DEVICE_ERROR;
  }

  mSpiControllerNum = 1;
  mNorFlashInstance = AllocateRuntimeZeroPool (mSpiControllerNum * sizeof(SPI_NOR));
  if (mNorFlashInstance == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  mNorFlashInstance[0].SpiBase = FixedPcdGet64 (PcdSPIFMC0Base);
  // mNorFlashInstance[1].SpiBase = FixedPcdGet64 (PcdSPIFMC1Base);

  Status = SetMemory ();
  //if (EFI_ERROR (Status)) {
  //  FreePool (mNorFlashInstance);
  //  return Status;
  //}

  Nor = mNorFlashInstance;
  for (Index = 0; Index < mSpiControllerNum; ++Index, ++Nor) {
    Nor->BounceBufSize = SIZE_4KB;
    Nor->BounceBuf = AllocateRuntimeZeroPool (Nor->BounceBufSize);
    if (Nor->BounceBuf == NULL) {
      for (Loop = 0; Loop < Index; ++Loop) {
        FreePool (mNorFlashInstance[Loop].BounceBuf);
      }
      FreePool (mNorFlashInstance);
      return EFI_OUT_OF_RESOURCES;
    }

    Base = mNorFlashInstance[Index].SpiBase;
    if (PcdGet32 (PcdCpuRiscVMmuMaxSatpMode) > 0UL) {
      for (I = 39; I < 64; I++) {
        if (Base & (1ULL << 38)) {
          Base |= (1ULL << I);
        } else {
          Base &= ~(1ULL << I);
        }
      }
    }
    mNorFlashInstance[Index].SpiBase = Base;

    SpifmcInit (Nor);
    DEBUG ((DEBUG_VERBOSE, "%a[%d] SPI Base Address = 0x%llx\n",
            __func__, __LINE__, Nor->SpiBase));
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SpiNorVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mNorFlashVirtualAddrChangeEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to register VA change event\n",
      __func__
      ));
    goto ErrorCreateEvent;
  }

  return EFI_SUCCESS;

ErrorCreateEvent:
  gBS->UninstallMultipleProtocolInterfaces (
        &mSpiMasterInstance->Handle,
        &gSophgoSpiMasterProtocolGuid,
        NULL
        );

  for (Index = 0; Index < mSpiControllerNum; ++Index) {
    FreePool (mNorFlashInstance[Index].BounceBuf);
  }
  FreePool (mNorFlashInstance);

  return Status;
}
