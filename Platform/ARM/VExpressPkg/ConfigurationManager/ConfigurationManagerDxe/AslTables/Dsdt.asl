/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2014-2020, Arm Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock("DsdtTable.aml", "DSDT", 1, "ARMLTD", "ARM-VEXP", 1) {
  Scope(_SB) {
    //
    // Processor
    //
    Device(CPU0) {
      Name(_HID, "ACPI0007")
      Name(_UID, Zero)
    }
    Device(CPU1) {
      Name(_HID, "ACPI0007")
      Name(_UID, One)
    }
    Device(CPU2) {
      Name(_HID, "ACPI0007")
      Name(_UID, 2)
    }
    Device(CPU3) {
      Name(_HID, "ACPI0007")
      Name(_UID, 3)
    }
    Device(CPU4) {
      Name(_HID, "ACPI0007")
      Name(_UID, 4)
    }
    Device(CPU5) {
      Name(_HID, "ACPI0007")
      Name(_UID, 5)
    }
    Device(CPU6) {
      Name(_HID, "ACPI0007")
      Name(_UID, 6)
    }
    Device(CPU7) {
      Name(_HID, "ACPI0007")
      Name(_UID, 7)
    }
  } // Scope(_SB)
}
