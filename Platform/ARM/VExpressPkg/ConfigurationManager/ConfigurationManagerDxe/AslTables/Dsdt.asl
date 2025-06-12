/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2014-2023, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2013, Al Stone <al.stone@linaro.org>
  All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock("DsdtTable.aml", "DSDT", 2, "ARMLTD", "ARM-VEXP", 1) {
  Scope(_SB) {
    //
    // Processor declaration
    //
    Method (_OSC, 4, Serialized)  { // _OSC: Operating System Capabilities
      CreateDWordField (Arg3, 0x00, STS0)
      CreateDWordField (Arg3, 0x04, CAP0)
      If ((Arg0 == ToUUID ("0811b06e-4a27-44f9-8d60-3cbbc22e7b48") /* Platform-wide Capabilities */)) {
        If (!(Arg1 == One)) {
          STS0 &= ~0x1F
          STS0 |= 0x0A
        } Else {
          If ((CAP0 & 0x100)) {
            CAP0 &= ~0x100 /* No support for OS Initiated LPI */
            STS0 &= ~0x1F
            STS0 |= 0x12
          }
        }
      } Else {
        STS0 &= ~0x1F
        STS0 |= 0x06
      }
      Return (Arg3)
    }

    // SMC91X
    Device (NET0) {
      Name (_HID, "LNRO0003")
      Name (_UID, 0)

      Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0x1a000000, 0x00010000)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) {0x2F}
      })
    }

    // VIRTIO block device
    Device (VIRT) {
      Name (_HID, "LNRO0005")
      Name (_UID, 0)

      Name (_CRS, ResourceTemplate() {
        Memory32Fixed (ReadWrite, 0x1c130000, 0x1000)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) {0x4A}
      })
    }
  } // Scope(_SB)
}
