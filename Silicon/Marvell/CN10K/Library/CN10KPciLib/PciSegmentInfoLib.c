/** @file
  PCI Segment Information Library for Marvell CN10K SoCs

  Returns the ECAM configuration space base addresses of all PCIe root
  complexes present on the CN10K. Every segment exposes a 1 GB ECAM window
  covering buses 0 - 0xFF.

  Copyright (c) 2026, Marvell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/PciSegmentInfoLib.h>

STATIC PCI_SEGMENT_INFO  mPciSegmentInfo[] = {
  /* Segment 0-2: ECAM root complexes (on-chip PCIe controllers) */
  { 0, 0x878000000000, 0, 0xFF },
  { 1, 0x878010000000, 0, 0xFF },
  { 2, 0x878020000000, 0, 0xFF },
  /* Segment 3-8: PEM controllers (external PCIe slots) */
  { 3, 0x878030000000, 0, 0xFF },
  { 4, 0x878040000000, 0, 0xFF },
  { 5, 0x878050000000, 0, 0xFF },
  { 6, 0x878060000000, 0, 0xFF },
  { 7, 0x878070000000, 0, 0xFF },
  { 8, 0x878080000000, 0, 0xFF },
};

/**
  Return the CN10K PCI segment table.

  The table is static and owned by this library. It contains one entry per
  root complex, covering segments 0-2 (ECAM) and 3-8 (PEM), each with a 1 GB
  ECAM window and buses 0-0xFF.

  @param[out]  Count  Receives the number of segments. Must not be NULL.

  @retval  NULL  Count is NULL.
  @return        The static PCI segment information array.
**/
PCI_SEGMENT_INFO *
EFIAPI
GetPciSegmentInfo (
  OUT UINTN  *Count
  )
{
  if (Count == NULL) {
    return NULL;
  }

  *Count = ARRAY_SIZE (mPciSegmentInfo);

  return mPciSegmentInfo;
}
