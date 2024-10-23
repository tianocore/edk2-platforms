/** @file
  Pcie board specific driver to handle asserting PERST signal to Endpoint
  card. PERST asserting is via group of GPIO pins to CPLD as Platform Specification.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/RootComplexInfoHob.h>
#include <Library/DebugLib.h>
#include <Library/GpioLib.h>
#include <Library/TimerLib.h>
#include <Platform/Ac01.h>

#define RCA_MAX_PERST_GROUPVAL  62
#define RCB_MAX_PERST_GROUPVAL  46
#define DEFAULT_SEGMENT_NUMBER  0x0F

VOID
BoardPcieReleaseAllPerst (
  IN UINT8  SocketId
  )
{
  // No Perst support fro COM-HPC-ALT
  return;
}

/**
  Assert PERST of PCIe controller

  @param[in]  RootComplex           Root Complex instance.
  @param[in]  PcieIndex             PCIe controller index of input Root Complex.
  @param[in]  IsPullToHigh          Target status for the PERST.

  @retval RETURN_SUCCESS            The operation is successful.
  @retval Others                    An error occurred.
**/
RETURN_STATUS
EFIAPI
BoardPcieAssertPerst (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN BOOLEAN            IsPullToHigh
  )
{
  // No Perst support fro COM-HPC-ALT
  return RETURN_SUCCESS;
}

/**
  Override the segment number for a root complex with a board specific number.

  @param[in]  RootComplex           Root Complex instance with properties.

  @retval Segment number corresponding to the input root complex.
          Default segment number is 0x0F.
**/
UINT16
BoardPcieGetSegmentNumber (
  IN  AC01_ROOT_COMPLEX  *RootComplex
  )
{
  UINT8  Ac01BoardSegment[PLATFORM_CPU_MAX_SOCKET][AC01_PCIE_MAX_ROOT_COMPLEX] =
  {
    { 0x0C, 0x0D, 0x01, 0x00, 0x02, 0x03, 0x04, 0x05 },
    { 0x10, 0x11, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B }
  };

  if (  (RootComplex->Socket < PLATFORM_CPU_MAX_SOCKET)
     && (RootComplex->ID < AC01_PCIE_MAX_ROOT_COMPLEX))
  {
    return Ac01BoardSegment[RootComplex->Socket][RootComplex->ID];
  }

  return (RootComplex->ID - 2);
}
