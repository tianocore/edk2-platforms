/** @file

  Common code for RDE Response Table in BIOS Directory.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include "SpiFvbRdeResponse.h"

EFI_LBA  mFvbStartingLba;
UINTN    mFvbStartingLbaOffset;
EFI_LBA  mFvbEndingLba;
UINTN    mFvbEndingLbaOffset;

/**
  Initial FVB parameters

  @retval EFI_SUCCESS       Driver initialization succeeded
  @retval all others        Driver initialization failed

**/
EFI_STATUS
InitFvbRdeResponse (
  )
{
  EFI_STATUS  Status;
  UINT64      InMediaOffset;
  UINT32      Imagesize;
  UINT64      MediaLogicalOffset;

  DEBUG ((DEBUG_INFO, "%a - Entry.\n", __func__));

  //
  // *Imagesize must be 0 for size query; otherwise AmdRdeDirectoryGetImage compares
  // caller size to directory entry size and may return EFI_INVALID_PARAMETER.
  //
  Imagesize = 0;
  Status    = AmdRdeDirectoryGetImage (RDE_RESPONSE_TABLE, &InMediaOffset, &Imagesize, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can not locate SoC RDE Response table.\n", __func__));
    return Status;
  }

  MediaLogicalOffset    = InMediaOffset & ROM_ADDRESS_MASK;
  mFvbStartingLba       = MediaLogicalOffset / BLOCK_SIZE;
  mFvbStartingLbaOffset = MediaLogicalOffset % BLOCK_SIZE;
  mFvbEndingLba         = (MediaLogicalOffset + Imagesize) / BLOCK_SIZE;
  mFvbEndingLbaOffset   = (MediaLogicalOffset + Imagesize) % BLOCK_SIZE;

  mNvStorageBase      = (EFI_PHYSICAL_ADDRESS)InMediaOffset;
  mNvStorageSize      = (UINT64)Imagesize;
  mNvStorageLbaOffset = mFvbStartingLba;
  mSpiFlashOffset     = GetSpiFlashOffset (mSpiNorFlashProtocol->FlashSize);

  DEBUG ((DEBUG_INFO, "%a - Flash area base address of RDE Response Table = %X\n", __func__, FixedPcdGet32 (PcdFlashAreaBaseAddress)));
  DEBUG ((DEBUG_INFO, "%a - mNvStorageBase of RDE Response Table = %X\n", __func__, mNvStorageBase));
  DEBUG ((DEBUG_INFO, "%a - mNvStorageSize of RDE Response Table = %X\n", __func__, mNvStorageSize));
  DEBUG ((DEBUG_INFO, "%a - mNvStorageLbaOffset of RDE Response Table = %X\n", __func__, mNvStorageLbaOffset));
  DEBUG ((DEBUG_INFO, "%a - mSpiFlashOffset of RDE Response Table = %X\n", __func__, mSpiFlashOffset));
  DEBUG ((DEBUG_INFO, "%a - mFvbStartingLba of RDE Response Table = %X\n", __func__, mFvbStartingLba));
  DEBUG ((DEBUG_INFO, "%a - mFvbStartingLbaOffset of RDE Response Table = %X\n", __func__, mFvbStartingLbaOffset));
  DEBUG ((DEBUG_INFO, "%a - mFvbEndingLba of RDE Response Table = %X\n", __func__, mFvbEndingLba));
  DEBUG ((DEBUG_INFO, "%a - mFvbEndingLbaOffset ofRDE Response Table = %X\n", __func__, mFvbEndingLbaOffset));
  return EFI_SUCCESS;
}
