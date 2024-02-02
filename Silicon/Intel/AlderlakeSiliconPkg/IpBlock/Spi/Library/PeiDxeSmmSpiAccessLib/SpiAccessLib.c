/** @file
  SPI library for abstraction of SPI HW registers accesses

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Pci22.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/SpiAccessLib.h>
#include <Library/PchPciBdfLib.h>
#include <Register/SpiRegs.h>
#include <Register/FlashRegs.h>
#include <Register/PchRegs.h>
#include <Register/PchDmiRegs.h>


/**
  Checks if PCH SPI Controler is present and available

  @retval TRUE    PCH SPI controller is avaialable
  @retval FALSE   PCH SPI controller is not available
**/
BOOLEAN
SpiIsControllerAvailable (
  VOID
  )
{
  //
  // Checks for SPI controller
  //
  return (PciSegmentRead16 (SpiPciCfgBase () + PCI_VENDOR_ID_OFFSET) != 0xFFFF);
}

/**
  Returns PCH SPI BAR0 value

  @retval  UINT32  PCH SPI BAR0 value
**/
UINT32
SpiGetBar0 (
  VOID
  )
{
  UINT32  SpiBar0;

  ASSERT (SpiIsControllerAvailable ());
  SpiBar0 = PciSegmentRead32 (SpiPciCfgBase () + R_SPI_CFG_BAR0) & ~B_SPI_CFG_BAR0_MASK;
  ASSERT (SpiBar0 != 0);

  return SpiBar0;
}

/**
  Checks if device Attached Flash (DAF) mode is active

  @retval TRUE    SAF mode is active
  @retval FALSE   SAF mode is not active
**/
BOOLEAN
SpiIsSafModeActive (
  VOID
  )
{
  UINT32 SpiBar0;
  SpiBar0 = SpiGetBar0 ();

  return !!(MmioRead32 (SpiBar0 + R_SPI_MEM_HSFSC) & B_SPI_MEM_HSFSC_SAF_MODE_ACTIVE);
}