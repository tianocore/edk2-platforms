/** @file
  BoardInitLib library internal implementation for DXE phase.

Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "DxeBoardInitLibInternal.h"

/**
  Reserve Legacy VGA IO space.

  @retval  EFI_SUCCESS  MMIO at Legacy VGA region has been allocated.
  @retval  !EFI_SUCCESS Error allocating the legacy VGA region.

**/
EFI_STATUS
EFIAPI
ReserveLegacyVgaIoSpace (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  VgaMemAddress;

  VgaMemAddress = (EFI_PHYSICAL_ADDRESS)VGA_MEM_BASE;
  Status        = gBS->AllocatePages (
                         AllocateAddress,
                         EfiMemoryMappedIO,
                         EFI_SIZE_TO_PAGES (VGA_MEM_SIZE),
                         &VgaMemAddress
                         );
  return Status;
}

/**
  Helper function to get size of MMIO region required for the Bus Range
  configured.

  @param[in]    BusRange      Chipset representation of Bus Range

  @retval                     Size of MMIO required for bus range
**/
UINT64
DecodeMmioBusRange (
  UINT64  BusRange
  )
{
  // Minimum MMIO region required is 1MB (1 Segment - 1 Bus).
  // Set Mmio Size to 1MB.
  UINT64  MmioSize;

  MmioSize = 0x100000;

  if (BusRange > 0x0E) {
    MmioSize = SIZE_32GB;
  } else {
    MmioSize = (MmioSize << BusRange);
  }

  return MmioSize;
}

/**
  Reserve PCIe Extended Config Space MMIO in the GCD and mark it runtime

  @param[in]  ImageHandle  ImageHandle of the loaded driver.
  @param[in]  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS  One or more of the drivers returned a success code.
  @retval  !EFI_SUCCESS  Error initializing the Legacy PIC.

**/
EFI_STATUS
EFIAPI
ReservePcieExtendedConfigSpace (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  AMD_MMIO_CFG_MSR  MmioCfgMsr;
  UINT64            MmioCfgBase;
  UINT64            MmioCfgSize;

  Status = EFI_SUCCESS;
  //
  // Reserve MMIO for PCI-Config space
  //
  MmioCfgMsr.AsUint64 = AsmReadMsr64 (AMD_MMIO_CFG_MSR_ADDR);
  MmioCfgBase         = MmioCfgMsr.AsUint64 & AMD_MMIO_CFG_ADDR_MASK;
  MmioCfgSize         = DecodeMmioBusRange (MmioCfgMsr.AsBits.BusRange);
  DEBUG ((DEBUG_INFO, "\nMMIO_CFG MSR = 0x%08lX\n", MmioCfgMsr.AsUint64));
  DEBUG ((DEBUG_INFO, "  Enable = %d\n", MmioCfgMsr.AsBits.Enable));
  DEBUG ((DEBUG_INFO, "  BusRange = %d\n", MmioCfgMsr.AsBits.BusRange));
  DEBUG ((DEBUG_INFO, "  MmioCfgBase = 0x%08lX\n", MmioCfgBase));
  DEBUG ((DEBUG_INFO, "  MmioCfgSize = 0x%08lX\n", MmioCfgSize));

  if (MmioCfgMsr.AsBits.Enable) {
    // Free Memory if it is allocated (call will likely return Not Found)
    Status = gDS->FreeMemorySpace (
                    MmioCfgBase,
                    MmioCfgSize
                    );
    // Remove Memory Space from GCD map (could return Not Found)
    Status = gDS->RemoveMemorySpace (
                    MmioCfgBase,
                    MmioCfgSize
                    );
    // Make sure Adding memory space succeeds or assert
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeReserved,
                    MmioCfgBase,
                    MmioCfgSize,
                    EFI_MEMORY_RUNTIME | EFI_MEMORY_UC
                    );
    ASSERT_EFI_ERROR (Status);
    // Make sure Allocating memory space succeed or assert
    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    EfiGcdMemoryTypeReserved,
                    0,
                    MmioCfgSize,
                    &MmioCfgBase,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    DEBUG ((
      DEBUG_INFO,
      "\nReserved PciCfg MMIO: Base = 0x%lX, Size = 0x%lX\n",
      MmioCfgBase,
      MmioCfgSize
      ));
  }

  return Status;
}
