/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/NorFlashInfoLib.h>
#include "SpiFlashDxe.h"

SPI_MASTER_PROTOCOL  *SpiMasterProtocol;
SPI_FLASH_INSTANCE   *mSpiFlashInstance;

STATIC
EFI_STATUS
SpiFlashWriteEnableCmd (
  IN  SPI_DEVICE_PARAMS  *Slave
  )
{
  EFI_STATUS  Status;
  SPI_OP_PARAMS  Op = SPI_WRITE_EN_OP();

  /* Send write enable command */
  Status = SpiMasterProtocol->CmdWrite (SpiMasterProtocol, Slave, &Op);

  return Status;
}

STATIC
EFI_STATUS
SpiFlashWriteDisableCmd (
  IN  SPI_DEVICE_PARAMS  *Slave
  )
{
  EFI_STATUS  Status;
  SPI_OP_PARAMS  Op = SPI_WRITE_DIS_OP();

  /* Send write disable command */
  Status = SpiMasterProtocol->CmdWrite (SpiMasterProtocol, Slave, &Op);

  return Status;
}

STATIC
EFI_STATUS
SpiFlashPoll (
  IN SPI_DEVICE_PARAMS *Slave
)
{
  EFI_STATUS  Status;
  UINT16      State;
  UINT32      Counter     = 0xFFFFF;
  UINT8       ReadLength  = 2;

  SPI_OP_PARAMS  OpRdSts = SPI_OP (
                                  SPI_OP_CMD (SPI_CMD_READ_STATUS),
                                  SPI_OP_NO_ADDR,
                                  SPI_OP_NO_DUMMY,
                                  SPI_OP_DATA_IN (ReadLength, (VOID *)&State)
                                  );

  Status = SpiMasterProtocol->CmdRead (SpiMasterProtocol, Slave, &OpRdSts);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Spi error while reading status\n", __func__));
    return Status;
  }

  do {
    Status = SpiMasterProtocol->CmdRead (SpiMasterProtocol, Slave, &OpRdSts);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Spi error while reading status\n", __func__));
      return Status;
    }

    Counter--;
    if (!(State & STATUS_REG_POLL_WIP_MSK)) {
      break;
    }
  } while (Counter > 0);

  if (Counter == 0) {
    DEBUG ((DEBUG_ERROR, "%a(): Timeout while writing to spi flash\n", __func__));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SpiFlashErase (
  IN SPI_DEVICE_PARAMS  *Slave,
  IN UINT32             Offset,
  IN UINTN              Length
  )
{
  EFI_STATUS  Status;
  UINT32      EraseAddr;
  UINTN      EraseSize;
  UINT8       EraseCmd;

  if (Slave->Info->Flags & NOR_FLASH_ERASE_4K) {
    EraseCmd  = SPI_CMD_ERASE_4K;
    EraseSize = SIZE_4KB;
  } else if (Slave->Info->Flags & NOR_FLASH_ERASE_32K) {
    EraseCmd  = SPI_CMD_ERASE_32K;
    EraseSize = SIZE_32KB;
  } else {
    EraseCmd  = SPI_CMD_ERASE_64K;
    EraseSize = Slave->Info->SectorSize;
  }

  /* Verify input parameters */
  if (Offset % EraseSize || Length % EraseSize) {
    DEBUG (
           (DEBUG_ERROR, "%a(): Either erase offset or length "
                         "is not multiple of erase size\n, __func__")
           );
    return EFI_DEVICE_ERROR;
  }

  Status = SpiFlashWriteEnableCmd (Slave);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Error while setting write_enable\n", __func__));
    return Status;
  }

  while (Length) {
    EraseAddr = Offset;

    SPI_OP_PARAMS  OpErase = SPI_OP (
                                        SPI_OP_CMD (EraseCmd),
                                        SPI_OP_ADDR (3, EraseAddr),
                                        SPI_OP_NO_DUMMY,
                                        SPI_OP_NO_DATA
                                        );

    Status = SpiMasterProtocol->CmdWrite (SpiMasterProtocol, Slave, &OpErase);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Spi erase fail\n", __func__));
      return Status;
    }

    Status = SpiFlashPoll(Slave);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Offset += EraseSize;
    Length -= EraseSize;
  }

  Status = SpiFlashWriteDisableCmd (Slave);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Error while setting write_disable\n", __func__));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SpiFlashRead (
  IN SPI_DEVICE_PARAMS *Slave,
  IN UINT32 Offset,
  IN UINTN Length,
  OUT VOID *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN      ReadAddr, ReadLength, RemainLength;
  UINT32      FlashSize;

  FlashSize = Slave->Info->PageSize * Slave->Info->SectorSize;

  /* Current handling is only limit for single flash Bank */
  while (Length) {
    ReadAddr = Offset;

    RemainLength = (FlashSize - Offset);
    if (Length < RemainLength) {
      ReadLength = Length;
    } else {
      ReadLength = RemainLength;
    }

    /* Send read command */
    SPI_OP_PARAMS  OpRead = SPI_OP (
                                SPI_OP_CMD (SPI_CMD_READ_DATA),
                                SPI_OP_ADDR (Slave->AddrSize, ReadAddr),
                                SPI_OP_NO_DUMMY,
                                SPI_OP_DATA_IN (ReadLength, Buffer)
                                );

    Status = SpiMasterProtocol->DataRead (SpiMasterProtocol, Slave, &OpRead);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Spi error while reading data\n", __func__));
      return Status;
    }

    Offset += ReadLength;
    Length -= ReadLength;
    Buffer = (VOID *)((UINTN)Buffer + ReadLength);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SpiFlashWrite (
  IN SPI_DEVICE_PARAMS *Slave,
  IN UINT32 Offset,
  IN UINTN Length,
  IN VOID *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       ByteAddr, ChunkLength, ActualIndex, PageSize;
  UINT32      WriteAddr;

  PageSize = Slave->Info->PageSize;

  Status = SpiFlashWriteEnableCmd (Slave);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Error while setting write enable\n", __func__));
    return Status;
  }

  for (ActualIndex = 0; ActualIndex < Length; ActualIndex += ChunkLength) {
    WriteAddr = Offset;

    ByteAddr    = Offset % PageSize;
    ChunkLength = MIN (Length - ActualIndex, (UINT64)(PageSize - ByteAddr));

    SPI_OP_PARAMS  OpPgProg = SPI_OP (
                                      SPI_OP_CMD (SPI_CMD_PAGE_PROGRAM),
                                      SPI_OP_ADDR (3, WriteAddr),
                                      SPI_OP_NO_DUMMY,
                                      SPI_OP_DATA_OUT (ChunkLength, (VOID *)((UINTN)Buffer + ActualIndex))
                                      );

    Status = SpiMasterProtocol->DataWrite (SpiMasterProtocol, Slave, &OpPgProg);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Error while programming write address\n", __func__));
      return Status;
    }

    Status = SpiFlashPoll(Slave);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Offset += ChunkLength;
  }

  Status = SpiFlashWriteDisableCmd (Slave);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Error while setting write disable\n", __func__));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SpiFlashUpdateBlock (
  IN SPI_DEVICE_PARAMS *Slave,
  IN UINT32 Offset,
  IN UINTN ToUpdate,
  IN UINT8 *Buffer,
  IN UINT8 *TmpBuf,
  IN UINTN EraseSize
  )
{
  EFI_STATUS Status;

  /* Read backup */
  Status = SpiFlashRead (Slave, Offset, EraseSize, TmpBuf);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a(): Update: Error while reading old data\n", __func__));
      return Status;
    }

  /* Erase entire sector */
  Status = SpiFlashErase (Slave, Offset, EraseSize);
  if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a(): Update: Error while erasing block\n", __func__));
      return Status;
    }

  /* Write new data */
  SpiFlashWrite (Slave, Offset, ToUpdate, Buffer);
  if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a(): Update: Error while writing new data\n", __func__));
      return Status;
    }

  /* Write backup */
  if (ToUpdate != EraseSize) {
    Status = SpiFlashWrite (Slave, Offset + ToUpdate, EraseSize - ToUpdate,
      &TmpBuf[ToUpdate]);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a(): Update: Error while writing backup\n", __func__));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SpiFlashUpdate (
  IN SPI_DEVICE_PARAMS *Slave,
  IN UINT32 Offset,
  IN UINTN ByteCount,
  IN UINT8 *Buffer
  )
{
  EFI_STATUS Status;
  UINT64 SectorSize, ToUpdate, Scale = 1;
  UINT8 *TmpBuf, *End;

  SectorSize = Slave->Info->SectorSize;

  End = Buffer + ByteCount;

  TmpBuf = (UINT8 *)AllocateZeroPool (SectorSize);
  if (TmpBuf == NULL) {
    DEBUG((DEBUG_ERROR, "%a(): Cannot allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  if (End - Buffer >= 200)
    Scale = (End - Buffer) / 100;

  for (; Buffer < End; Buffer += ToUpdate, Offset += ToUpdate) {
    ToUpdate = MIN((UINT64)(End - Buffer), SectorSize);
    Print (L"   \rUpdating, %d%%", 100 - (End - Buffer) / Scale);
    Status = SpiFlashUpdateBlock (Slave, Offset, ToUpdate, Buffer, TmpBuf, SectorSize);

    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a(): Error while updating\n", __func__));
      return Status;
    }
  }

  Print(L"\n");
  FreePool (TmpBuf);

  return EFI_SUCCESS;
}

EFI_STATUS
SpiFlashUpdateWithProgress (
  IN SPI_DEVICE_PARAMS                             *Slave,
  IN UINT32                                         Offset,
  IN UINTN                                          ByteCount,
  IN UINT8                                         *Buffer,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,        OPTIONAL
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  EFI_STATUS Status;
  UINTN SectorSize;
  UINTN SectorNum;
  UINTN ToUpdate;
  UINTN Index;
  UINT8 *TmpBuf;

  SectorSize = Slave->Info->SectorSize;
  SectorNum = (ByteCount / SectorSize) + 1;
  ToUpdate = SectorSize;

  TmpBuf = (UINT8 *)AllocateZeroPool (SectorSize);
  if (TmpBuf == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < SectorNum; Index++) {
    if (Progress != NULL) {
      Progress (StartPercentage +
                ((Index * (EndPercentage - StartPercentage)) / SectorNum));
    }

    /* In the last chunk update only an actual number of remaining bytes */
    if (Index + 1 == SectorNum) {
      ToUpdate = ByteCount % SectorSize;
    }

    Status = SpiFlashUpdateBlock (Slave,
               Offset + Index * SectorSize,
               ToUpdate,
               Buffer + Index * SectorSize,
               TmpBuf,
               SectorSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Error while updating\n", __func__));
      return Status;
    }
  }
  FreePool (TmpBuf);

  if (Progress != NULL) {
    Progress (EndPercentage);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiFlashReadId (
  IN SPI_DEVICE_PARAMS  *Slave,
  IN BOOLEAN            UseInRuntime
  )
{
  EFI_STATUS  Status;
  UINT8       IdLen = 3;
  UINT8       Id[IdLen];

  SPI_OP_PARAMS  Op = SPI_READID_OP (Id, IdLen);

  Status = SpiMasterProtocol->CmdRead (SpiMasterProtocol, Slave, &Op);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Spi error while reading id\n", __func__));
    return Status;
  }

  Status = NorFlashGetInfo (Id, &Slave->Info, UseInRuntime);

  if (EFI_ERROR (Status)) {
    DEBUG (
           (DEBUG_ERROR,
            "%a: Unrecognized JEDEC Id bytes: 0x%02x%02x%02x\n",
            __func__,
            Id[0],
            Id[1],
            Id[2])
           );
    return Status;
  }

  NorFlashPrintInfo (Slave->Info);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiFlashInit (
  IN SPI_FLASH_PROTOCOL  *This,
  IN SPI_DEVICE_PARAMS   *Slave
  )
{
  EFI_STATUS  Status;
  UINT8       StatusRegister;

  Slave->AddrSize = (Slave->Info->Flags & NOR_FLASH_4B_ADDR) ? 4 : 3;

  Status = SpiFlashWriteEnableCmd (Slave);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Error while setting write enable\n", __func__));
    return Status;
  }

  if (Slave->AddrSize == 4) {

    /* Enable 4byte addressing */
    SPI_OP_PARAMS  Op4BAddEn = SPI_OP (
                                        SPI_OP_CMD (SPI_CMD_4B_ADDR_ENABLE),
                                        SPI_OP_NO_ADDR,
                                        SPI_OP_NO_DUMMY,
                                        SPI_OP_NO_DATA
                                        );
    Status = SpiMasterProtocol->CmdWrite (SpiMasterProtocol, Slave, &Op4BAddEn);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Error while setting 4B address\n", __func__));
      return Status;
    }
  }

  /* Initialize flash status register */
  StatusRegister = 0x0;
  SPI_OP_PARAMS  OpWrSts = SPI_OP (
                                    SPI_OP_CMD (SPI_CMD_WRITE_STATUS_REG),
                                    SPI_OP_NO_ADDR,
                                    SPI_OP_NO_DUMMY,
                                    SPI_OP_DATA_OUT (1, (VOID *)&StatusRegister)
                                    );

  Status = SpiMasterProtocol->CmdWrite (SpiMasterProtocol, Slave, &OpWrSts);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Error while writing status register\n", __func__));
    return Status;
  }

  Status = SpiFlashWriteDisableCmd (Slave);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Error while setting write disable\n", __func__));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SpiFlashInitProtocol (
  IN SPI_FLASH_PROTOCOL  *SpiFlashProtocol
  )
{
  SpiFlashProtocol->Read   = SpiFlashRead;
  SpiFlashProtocol->Write  = SpiFlashWrite;
  SpiFlashProtocol->Update = SpiFlashUpdate;
  SpiFlashProtocol->UpdateWithProgress = SpiFlashUpdateWithProgress;
  SpiFlashProtocol->Erase  = SpiFlashErase;
  SpiFlashProtocol->ReadId = SpiFlashReadId;
  SpiFlashProtocol->Init   = SpiFlashInit;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiFlashEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                                &gJH7110SpiMasterProtocolGuid,
                                NULL,
                                (VOID **)&SpiMasterProtocol
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot locate SPI Master protocol\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  mSpiFlashInstance = AllocateRuntimeZeroPool (sizeof (SPI_FLASH_INSTANCE));
  if (mSpiFlashInstance == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  SpiFlashInitProtocol (&mSpiFlashInstance->SpiFlashProtocol);

  mSpiFlashInstance->Signature = SPI_FLASH_SIGNATURE;

  Status = gBS->InstallMultipleProtocolInterfaces (
                                                   &(mSpiFlashInstance->Handle),
                                                   &gJH7110SpiFlashProtocolGuid,
                                                   &(mSpiFlashInstance->SpiFlashProtocol),
                                                   NULL
                                                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot install SPI flash protocol\n", __func__));
    goto ErrorInstallProto;
  }

  return EFI_SUCCESS;

ErrorInstallProto:
  FreePool (mSpiFlashInstance);

  return EFI_SUCCESS;
}
