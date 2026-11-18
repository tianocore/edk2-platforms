/** @file

  Implementation of SpiHcPlatformLib for PEI

  This provides the PEI-phase instance of SpiHcPlatformLib. It shares the
  hardware transaction logic in SpiHcPlatformLib.c and SpiHcInternal.c with the
  DXE and SMM instances; only the platform-detail discovery
  (GetPlatformSpiHcDetails) and the mHcAddress global live here, mirroring
  SpiHcPlatformLibDxe.c. The FCH SPI controller base address is discovered via
  a phase-agnostic PCI segment read, so no boot-services dependency is required.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Protocol/SpiHc.h>
#include <Library/PciSegmentLib.h>
#include <Library/SpiHcPlatformLib.h>
#include "SpiHcInternal.h"
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <FchRegistersCommon.h>

#define SPI_HC_MAXIMUM_TRANSFER_BYTES  64

// Global variable to manage the platform-dependent SPI host controller
EFI_PHYSICAL_ADDRESS  mHcAddress;

/**
  This function reports the details of the SPI Host Controller to the SpiHc driver.

  @param[out]     Attributes              The supported attributes of the SPI host controller
  @param[out]     FrameSizeSupportMask    The supported FrameSizeSupportMask of the SPI host controller
  @param[out]     MaximumTransferBytes    The supported MaximumTransferBytes of the SPI host controller

  @retval EFI_SUCCESS             SPI host controller details were reported properly
**/
EFI_STATUS
EFIAPI
GetPlatformSpiHcDetails (
  OUT     UINT32  *Attributes,
  OUT     UINT32  *FrameSizeSupportMask,
  OUT     UINT32  *MaximumTransferBytes
  )
{
  // Fill in the SPI Host Controller details
  *Attributes = HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS |
                HC_SUPPORTS_READ_ONLY_OPERATIONS |
                HC_SUPPORTS_WRITE_ONLY_OPERATIONS;
  *FrameSizeSupportMask = FCH_SPI_FRAME_SIZE_SUPPORT_MASK;
  *MaximumTransferBytes = SPI_HC_MAXIMUM_TRANSFER_BYTES;

  // fill in Platform specific global variables
  mHcAddress = (
                PciSegmentRead32 (
                  PCI_SEGMENT_LIB_ADDRESS (0x00, FCH_LPC_BUS, FCH_LPC_DEV, FCH_LPC_FUNC, FCH_LPC_REGA0)
                  )
                ) & 0xFFFFFF00;
  if ((mHcAddress == 0) || (mHcAddress == 0xFFFFFF00)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SPI HC base address 0x%08X\n", __func__, mHcAddress));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
