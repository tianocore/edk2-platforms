/** @file
  SMM Library instance of SPI Flash Common Library Class

  Copyright (c) 2021 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MmServicesTableLib.h>
#include <Protocol/Spi2.h>
#include <Library/DebugLib.h>

extern PCH_SPI2_PROTOCOL   *mSpi2Protocol;

extern UINTN mBiosAreaBaseAddress;
extern UINTN mBiosSize;
extern UINTN mBiosOffset;

/**
  The library constructor.

  The function does the necessary initialization work for this library
  instance.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       The function always return EFI_SUCCESS for now.
                                It will ASSERT on error for debug version.
  @retval     EFI_ERROR         Please reference LocateProtocol for error code details.
**/
EFI_STATUS
EFIAPI
SmmSpiFlashCommonLibConstructor (
  VOID
  )
{
  EFI_STATUS Status;
  UINT32     BaseAddr;
  UINT32     RegionSize;

  mBiosAreaBaseAddress = (UINTN)PcdGet32 (PcdBiosAreaBaseAddress);
  mBiosSize            = (UINTN)PcdGet32 (PcdBiosSize);

  //
  // Locate the SMM SPI2 protocol.
  //
  Status = gMmst->MmLocateProtocol (
                    &gPchSmmSpi2ProtocolGuid,
                    NULL,
                    (VOID **) &mSpi2Protocol
                    );
  ASSERT_EFI_ERROR (Status);

  mSpi2Protocol->GetRegionAddress (mSpi2Protocol, &gFlashRegionBiosGuid, &BaseAddr, &RegionSize);
  mBiosOffset = BaseAddr;
  return Status;
}
