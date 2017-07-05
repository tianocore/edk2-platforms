/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2014-2017, ARM Ltd. All rights reserved.<BR>
    This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

DefinitionBlock("DsdtTable.aml", "DSDT", 1, "ARMLTD", "ARM-VEXPRESS", 1) {
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

    // UART PL011
    Device(COM2) {
      Name(_HID, "ARMH0011")
      Name(_CID, "PL011")
      Name(_UID, Zero)

      Method(_STA) {
        Return(0xF)
      }

      Method(_CRS, 0x0, NotSerialized) {
        Name(RBUF, ResourceTemplate() {
          // UART1
          Memory32Fixed(ReadWrite, 0x1c090000, 0x1000)
          Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x25 }
          // UART3
          // Memory32Fixed(ReadWrite, 0x1c0B0000, 0x1000)
          // Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x27 }
        })
        Return (RBUF)
      }
    }

  } // Scope(_SB)
}
