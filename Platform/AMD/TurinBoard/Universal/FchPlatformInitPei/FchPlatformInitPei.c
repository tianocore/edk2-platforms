/** @file


  FCH initialization hook PEI.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include "FchPlatformInit.h"
#include <AMD.h>

#define PMCONTROL_REG       0x504

/**
  Enable LPC IO Port for IPMI KCS interface.

**/
VOID
EnableLpcWideIoPort2 (
  IN VOID
  )
{
  if (FixedPcdGet8 (PcdIpmiInterfaceType) == IPMIDeviceInfoInterfaceTypeKCS) {
    DEBUG ((DEBUG_INFO, "Enabling wide io port 2.\n"));
    //
    // Pleaser refer AMD PPR Vol 3 for respective Family/Model SoC
    // for detail information.
    //

    //
    // Offset 0x090 (FCH::ITF::LPC::WIDE_IO_2)
    // IO_Base_Address_2. 16-bit PCI I/O base address for
    // wide generic port range.
    //
    PciWrite16 (
      PCI_SEGMENT_LIB_ADDRESS (
        0,
        FCH_LPC_BUS,
        FCH_LPC_DEV,
        FCH_LPC_FUNC,
        0x90
        ),
      FixedPcdGet16 (PcdIpmiKcsIoBaseAddress)
      );

    //
    // Offset 0x048 (FCH::ITF::LPC::IO_MEM_PORT_DECODE_ENABLE)
    // Enables Wide IO port 2 (defined in registers 90/91h) enable.
    //
    PciWrite8 (
      PCI_SEGMENT_LIB_ADDRESS (
        0,
        FCH_LPC_BUS,
        FCH_LPC_DEV,
        FCH_LPC_FUNC,
        (FCH_LPC_REG48 + 3)
        ),
      0x2
      );

    //
    // Offset 0x074 (FCH::ITF::LPC::ALTERNATIVE_WIDE_IO_RANGE_ENABLE)
    // Alternative_Wide_Io_2_Range_Enable to I/O address defined in
    // reg0x90 and reg0x91.
    //
    PciWrite8 (
      PCI_SEGMENT_LIB_ADDRESS (
        0,
        FCH_LPC_BUS,
        FCH_LPC_DEV,
        FCH_LPC_FUNC,
        FCH_LPC_REG74
        ),
      0x8
      );              // Enable BIT3 for Alternative_Wide_Io_2_Range_Enable
  }
}

/**
  Enable SPI TPM

**/
VOID
EnableSpiTpm (
  IN VOID
  )
{
  // Set TPM Decode
  PciAndThenOr8 (
    PCI_SEGMENT_LIB_ADDRESS (
      0,
      FCH_LPC_BUS,
      FCH_LPC_DEV,
      FCH_LPC_FUNC,
      FCH_LPC_REG7C
      ),
    0xFF,
    0x85
    );

  // Set RouteTpm2Spi
  PciAndThenOr8 (
    PCI_SEGMENT_LIB_ADDRESS (
      0,
      FCH_LPC_BUS,
      FCH_LPC_DEV,
      FCH_LPC_FUNC,
      FCH_LPC_REGA0
      ),
    0xFF,
    0x08
    );

  // Set AGPIO76 As SPI_TPM_CS_L
  MmioWrite8 (
    ACPI_MMIO_BASE + IOMUX_BASE + 0x4C,
    ((MmioRead8 (ACPI_MMIO_BASE + IOMUX_BASE + 0x4C) & 0xFF) | 0x01)
    );
}

/**
  Clear SCI_EN bit in PMCONTROL register

**/
VOID
ClearSciEn (
  IN VOID
  )
{
  // Clear SCI_EN bit in PMCONTROL register
  MmioWrite32 (
    ACPI_MMIO_BASE + PMIO_BASE + PMCONTROL_REG,
    (MmioRead32 (ACPI_MMIO_BASE + PMIO_BASE + PMCONTROL_REG) & (~(UINT32) BIT0))
    );
}

/**
  Entry point for FCH intialization PEIM

  @param   FileHandle Pointer to the FFS file header.
  @param   PeiServices Pointer to the PEI services table.

  @retval  EFI_STATUS EFI_SUCCESS
           EFI_STATUS respective failure status.
**/
EFI_STATUS
EFIAPI
FchInitEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  DEBUG ((DEBUG_INFO, "Entered %a Platform FCH initialization.\n", __func__));

  EnableLpcWideIoPort2 ();
  EnableSpiTpm ();
  ClearSciEn ();
  return EFI_SUCCESS;
}
