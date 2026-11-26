/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __FVB_DXE_H__
#define __FVB_DXE_H__

#include <Protocol/BlockIo.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Include/Spifmc.h>
#include <Include/SpiNorFlash.h>

#define FVB_FLASH_SIGNATURE                       SIGNATURE_32('S', 'n', 'o', 'r')
#define INSTANCE_FROM_FVB_THIS(a)                 CR(a, FVB_DEVICE, FvbProtocol, FVB_FLASH_SIGNATURE)
#define GET_DATA_OFFSET(BaseAddr, Lba, LbaSize)   ((BaseAddr) + (UINTN)((Lba) * (LbaSize)))

typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  EFI_DEVICE_PATH_PROTOCOL            End;
} FVB_DEVICE_PATH;

typedef struct {
  SPI_NOR                             *Nor;
  SOPHGO_NOR_FLASH_PROTOCOL           *NorFlashProtocol;
  SOPHGO_SPI_MASTER_PROTOCOL          *SpiMasterProtocol;
  EFI_HANDLE                          Handle;
  UINT32                              Signature;
  UINTN                               RegionBaseAddress;
  UINTN                               Size;
  UINTN                               FvbOffset;
  UINTN                               FvbSize;
  EFI_LBA                             StartLba;
  EFI_BLOCK_IO_MEDIA                  Media;
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL FvbProtocol;
  FVB_DEVICE_PATH                     DevicePath;
} FVB_DEVICE;

EFI_STATUS
EFIAPI
FvbGetAttributes (
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL*     This,
    OUT       EFI_FVB_ATTRIBUTES_2*                    Attributes
);

EFI_STATUS
EFIAPI
FvbSetAttributes (
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL*     This,
    IN OUT    EFI_FVB_ATTRIBUTES_2*                    Attributes
);

EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL*     This,
    OUT       EFI_PHYSICAL_ADDRESS*                    Address
);

EFI_STATUS
EFIAPI
FvbGetBlockSize (
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL*     This,
    IN        EFI_LBA                                  Lba,
    OUT       UINTN*                                   BlockSize,
    OUT       UINTN*                                   NumberOfBlocks
);

EFI_STATUS
EFIAPI
FvbRead (
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL*     This,
    IN        EFI_LBA                                  Lba,
    IN        UINTN                                    Offset,
    IN OUT    UINTN*                                   NumBytes,
    IN OUT    UINT8*                                   Buffer
);

EFI_STATUS
EFIAPI
FvbWrite (
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL*     This,
    IN        EFI_LBA                                  Lba,
    IN        UINTN                                    Offset,
    IN OUT    UINTN*                                   NumBytes,
    IN        UINT8*                                   Buffer
);

EFI_STATUS
EFIAPI
FvbEraseBlocks (
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL*     This,
    ...
);

#endif //__FVB_DXE_H__
