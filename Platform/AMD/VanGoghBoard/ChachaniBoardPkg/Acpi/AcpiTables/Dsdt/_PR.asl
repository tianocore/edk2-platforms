// /** @file
// Acpi _PR.asl
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/
//
//  Processor Objects
//
Scope(\_SB) {
  Device(PLTF) {
    Name(_HID, "ACPI0010")
    Name(_CID, EISAID("PNP0A05"))
    Name(_UID, 1)

    Device(C000) {        Name(_HID, "ACPI0007")        Name(_UID, 0x00)     }
    Device(C001) {        Name(_HID, "ACPI0007")        Name(_UID, 0x01)     }
    Device(C002) {        Name(_HID, "ACPI0007")        Name(_UID, 0x02)     }
    Device(C003) {        Name(_HID, "ACPI0007")        Name(_UID, 0x03)     }
    Device(C004) {        Name(_HID, "ACPI0007")        Name(_UID, 0x04)     }
    Device(C005) {        Name(_HID, "ACPI0007")        Name(_UID, 0x05)     }
    Device(C006) {        Name(_HID, "ACPI0007")        Name(_UID, 0x06)     }
    Device(C007) {        Name(_HID, "ACPI0007")        Name(_UID, 0x07)     }
    Device(C008) {        Name(_HID, "ACPI0007")        Name(_UID, 0x08)     }
    Device(C009) {        Name(_HID, "ACPI0007")        Name(_UID, 0x09)     }
    Device(C00A) {        Name(_HID, "ACPI0007")        Name(_UID, 0x0A)     }
    Device(C00B) {        Name(_HID, "ACPI0007")        Name(_UID, 0x0B)     }
    Device(C00C) {        Name(_HID, "ACPI0007")        Name(_UID, 0x0C)     }
    Device(C00D) {        Name(_HID, "ACPI0007")        Name(_UID, 0x0D)     }
    Device(C00E) {        Name(_HID, "ACPI0007")        Name(_UID, 0x0E)     }
    Device(C00F) {        Name(_HID, "ACPI0007")        Name(_UID, 0x0F)     }
 }
}

