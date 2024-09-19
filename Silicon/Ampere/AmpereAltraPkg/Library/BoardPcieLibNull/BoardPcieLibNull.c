/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BoardPcieLib.h>

/**
  Assert PERST of input PCIe controller

  @param[in]  RootComplex           RootComplex instance.
  @param[in]  PcieIndex             PCIe controller index of input Root Complex.
  @param[in]  Bifurcation           Bifurcation mode of input Root Complex.
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
  return 0x0F;
}
