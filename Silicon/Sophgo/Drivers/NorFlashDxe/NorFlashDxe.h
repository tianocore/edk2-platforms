/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __NOR_FLASH_DXE_H__
#define __NOR_FLASH_DXE_H__

#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Include/Spifmc.h>
#include <Include/SpiNorFlash.h>

#define NOR_FLASH_SIGNATURE             SIGNATURE_32 ('F', 'S', 'P', 'I')

///
/// Global ID for the SPI NOR Flash Protocol
///
#define SOPHGO_NOR_FLASH_PROTOCOL_GUID  \
  { 0xE9A39038, 0x1965, 0x4404,          \
    { 0xA5, 0x2A, 0xB9, 0xA3, 0xA1, 0xAE, 0xC2, 0xE4 } }

typedef struct {
  /* disk partition table magic number */
  UINT32  Magic;
  CHAR8   Name[32];
  UINT32  Offset;
  UINT32  Size;
  CHAR8   Reserve[4];
  /* load memory address*/
  UINTN   Lma;
} FLASH_PARTITION_INFO;

enum {
  DPT_MAGIC = 0x55AA55AA,
};

EFI_STATUS
EFIAPI
SpiNorGetFlashId (
  IN SPI_NOR     *Nor,
  IN BOOLEAN     UseInRuntime
  );

EFI_STATUS
EFIAPI
SpiNorReadData (
  IN  SPI_NOR    *Nor,
  IN  UINTN      FlashOffset,
  IN  UINTN      Length,
  OUT UINT8      *Buffer
  );

EFI_STATUS
EFIAPI
SpiNorReadStatus (
  IN  SPI_NOR     *Nor,
  OUT UINT8       *Sr
  );

EFI_STATUS
EFIAPI
SpiNorWriteStatus (
  IN SPI_NOR     *Nor,
  IN UINT8       *Sr,
  IN UINTN        Length
  );

EFI_STATUS
EFIAPI
SpiNorWriteData (
  IN SPI_NOR     *Nor,
  IN UINTN       FlashOffset,
  IN UINTN       Length,
  IN UINT8       *Buffer
  );

EFI_STATUS
EFIAPI
SpiNorErase (
  IN SPI_NOR     *Nor,
  IN UINTN       FlashOffset,
  IN UINTN       Length
  );

EFI_STATUS
EFIAPI
SpiNorGetFlashVariableOffset (
  IN SPI_NOR     *Nor
  );

EFI_STATUS
EFIAPI
SpiNorSoftReset (
  IN SPI_NOR     *Nor
  );

EFI_STATUS
EFIAPI
SpiNorInit (
  IN SOPHGO_NOR_FLASH_PROTOCOL *This,
  IN SPI_NOR                   *Nor
  );

EFI_STATUS
EFIAPI
SpiNorSetProtectAll (
  IN SPI_NOR     *Nor,
  IN BOOLEAN     IsProtectAll
  );

#endif // __NOR_FLASH_DXE_H__
