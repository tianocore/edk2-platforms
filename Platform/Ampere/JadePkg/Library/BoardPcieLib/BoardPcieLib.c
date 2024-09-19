/** @file
  Pcie board specific driver to handle asserting PERST signal to Endpoint
  card. PERST asserting is via group of GPIO pins to CPLD as Platform Specification.

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

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

#define PCIE_PERST_DELAY  (100 * 1000)               // 100ms

VOID
BoardPcieReleaseAllPerst (
  IN UINT8  SocketId
  )
{
  UINT32  GpioIndex, GpioPin;

  // Write 1 to all GPIO[16..21] to release all PERST
  GpioPin = AC01_GPIO_PINS_PER_SOCKET * SocketId + 16;
  for (GpioIndex = 0; GpioIndex < 6; GpioIndex++) {
    GpioModeConfig (GpioPin + GpioIndex, GpioConfigOutHigh);
  }

  MicroSecondDelay (PCIE_PERST_DELAY);
}

EFI_STATUS
GetGpioGroup (
  IN  UINT8   RootComplexId,
  IN  UINT8   PcieIndex,
  OUT UINT32  *GpioGroupVal
  )
{
  /* Ampere Altra Max RootComplex->ID: 4:7 */
  if (PcieIndex < 2) {
    switch (RootComplexId) {
      case 4:
        *GpioGroupVal = 34 - (PcieIndex * 2);
        break;
      case 5:
        *GpioGroupVal = 38 - (PcieIndex * 2);
        break;
      case 6:
        *GpioGroupVal = 30 - (PcieIndex * 2);
        break;
      case 7:
        *GpioGroupVal = 26 - (PcieIndex * 2);
        break;
      default:
        return EFI_INVALID_PARAMETER;
    }
  } else {
    /* Ampere Altra Max RootComplex->ID: 4:7 */
    switch (RootComplexId) {
      case 4:
        *GpioGroupVal = 46 - ((PcieIndex - 2) * 2);
        break;
      case 5:
        *GpioGroupVal = 42 - ((PcieIndex - 2) * 2);
        break;
      case 6:
        *GpioGroupVal = 18 - ((PcieIndex - 2) * 2);
        break;
      case 7:
        *GpioGroupVal = 22 - ((PcieIndex - 2) * 2);
        break;
      default:
        return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
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
  UINT32      GpioGroupVal;
  UINT32      Val;
  UINT32      GpioIndex;
  UINT32      GpioPin;
  EFI_STATUS  Status;

  if (!IsPullToHigh) {
    if (RootComplex->Type == RootComplexTypeA) {
      if (RootComplex->ID < MaxPcieControllerOfRootComplexA) {
        /* Ampere Altra: 4 */
        //
        // RootComplexTypeA: RootComplex->ID: 0->3 ; PcieIndex: 0->3
        //
        GpioGroupVal = RCA_MAX_PERST_GROUPVAL - PcieIndex
                       - RootComplex->ID * MaxPcieControllerOfRootComplexA;
      } else {
        Status = GetGpioGroup (RootComplex->ID, PcieIndex, &GpioGroupVal);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Invalid Root Complex ID %d\n", RootComplex->ID));
          return Status;
        }
      }
    } else {
      //
      // RootComplexTypeB: RootComplex->ID: 4->7 ; PcieIndex: 0->7
      //
      GpioGroupVal = RCB_MAX_PERST_GROUPVAL - PcieIndex
                     - (RootComplex->ID - MaxRootComplexA) * MaxPcieControllerOfRootComplexB;
    }

    // Update the value of GPIO[16..21]. Corresponding PERST line will be decoded by CPLD.
    GpioPin = AC01_GPIO_PINS_PER_SOCKET * RootComplex->Socket + 16;
    for (GpioIndex = 0; GpioIndex < 6; GpioIndex++) {
      Val = (GpioGroupVal & 0x3F) & (1 << GpioIndex);
      if (Val == 0) {
        GpioModeConfig (GpioPin + GpioIndex, GpioConfigOutLow);
      } else {
        GpioModeConfig (GpioPin + GpioIndex, GpioConfigOutHigh);
      }
    }

    // Keep reset as low as 100 ms as specification
    MicroSecondDelay (PCIE_PERST_DELAY);
  } else {
    BoardPcieReleaseAllPerst (RootComplex->Socket);
  }

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
