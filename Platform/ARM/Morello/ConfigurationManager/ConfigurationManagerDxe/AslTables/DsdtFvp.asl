/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2021-2023, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "ConfigurationManager.h"
#include "MorelloPlatform.h"

DefinitionBlock("Dsdt.aml", "DSDT", 2, "ARMLTD", "MORELLO", CFG_MGR_OEM_REVISION) {
  Scope(_SB) {
    /* _OSC: Operating System Capabilities */
    Method (_OSC, 4, Serialized) {
      CreateDWordField (Arg3, 0x00, STS0)
      CreateDWordField (Arg3, 0x04, CAP0)

      /* Platform-wide Capabilities */
      If (LEqual (Arg0, ToUUID("0811b06e-4a27-44f9-8d60-3cbbc22e7b48"))) {
        /* OSC rev 1 supported, for other version, return failure */
        If (LEqual (Arg1, One)) {
          And (STS0, Not (OSC_STS_MASK), STS0)

          If (And (CAP0, OSC_CAP_OS_INITIATED_LPI)) {
            /* OS initiated LPI not supported */
            And (CAP0, Not (OSC_CAP_OS_INITIATED_LPI), CAP0)
            Or (STS0, OSC_STS_CAPABILITY_MASKED, STS0)
          }

          If (And (CAP0, OSC_CAP_PLAT_COORDINATED_LPI)) {
            if (LEqual (FixedPcdGet32 (PcdOscLpiEnable), Zero)) {
              And (CAP0, Not (OSC_CAP_PLAT_COORDINATED_LPI), CAP0)
              Or (STS0, OSC_STS_CAPABILITY_MASKED, STS0)
            }
          }

        } Else {
          And (STS0, Not (OSC_STS_MASK), STS0)
          Or (STS0, Or (OSC_STS_FAILURE, OSC_STS_UNRECOGNIZED_REV), STS0)
        }
      } Else {
        And (STS0, Not (OSC_STS_MASK), STS0)
        Or (STS0, Or (OSC_STS_FAILURE, OSC_STS_UNRECOGNIZED_UUID), STS0)
      }

      Return (Arg3)
    }

    // VIRTIO DISK
    Device(VR00) {
      Name(_HID, "LNRO0005")
      Name(_UID, 0)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(
          ReadWrite,
          FixedPcdGet32 (PcdVirtioBlkBaseAddress),
          FixedPcdGet32 (PcdVirtioBlkSize)
        )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) {
          FixedPcdGet32 (PcdVirtioBlkInterrupt)
        }
      })
    }

    // VIRTIO NET
    Device(VR01) {
      Name(_HID, "LNRO0005")
      Name(_UID, 1)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, 0x1C180000, 0x00010000)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 134 }
      })
    }

    // VIRTIO RANDOM
    Device(VR02) {
      Name(_HID, "LNRO0005")
      Name(_UID, 2)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, 0x1C190000, 0x00010000)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 133 }
      })
    }

    // VIRTIO P9 Device
    Device(VR03) {
      Name(_HID, "LNRO0005")
      Name(_UID, 3)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, 0x1C1A0000, 0x00010000)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 135 }
      })
    }

    // SMC91X
    Device(NET0) {
      Name(_HID, "LNRO0003")
      Name(_UID, 0)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, 0x1D100000, 0x00001000)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 130 }
      })
    }
  } // Scope(_SB)
}
