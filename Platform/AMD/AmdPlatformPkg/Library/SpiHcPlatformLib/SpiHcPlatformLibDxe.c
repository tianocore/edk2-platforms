/** @file

  Implementation of SpiHcPlatformLib for DXE

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SpiHc.h>
#include <Library/PciSegmentLib.h>
#include <Library/SpiHcPlatformLib.h>
#include "SpiHcInternal.h"
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <FchRegistersCommon.h>

#define SPI_HC_MAXIMUM_TRANSFER_BYTES  64

// Global variables to manage the platform-dependent SPI host controller
EFI_PHYSICAL_ADDRESS  mHcAddress;

/**
  This function reports the details of the SPI Host Controller to the SpiHc driver.

  @param[out]     Attributes              The suported attributes of the SPI host controller
  @param[out]     FrameSizeSupportMask    The suported FrameSizeSupportMask of the SPI host controller
  @param[out]     MaximumTransferBytes    The suported MaximumTransferBytes of the SPI host controller

  @retval EFI_SUCCESS             SPI_HOST_CONTROLLER_INSTANCE was allocated properly
  @retval EFI_OUT_OF_RESOURCES    The SPI_HOST_CONTROLLER_INSTANCE could not be allocated
**/
EFI_STATUS
EFIAPI
GetPlatformSpiHcDetails (
  OUT     UINT32  *Attributes,
  OUT     UINT32  *FrameSizeSupportMask,
  OUT     UINT32  *MaximumTransferBytes
  )
{
  // Fill in the SPI Host Controller Protocol
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
  return EFI_SUCCESS;
}
