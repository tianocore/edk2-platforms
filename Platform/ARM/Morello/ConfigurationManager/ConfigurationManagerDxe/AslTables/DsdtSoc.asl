/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2021 - 2022, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for Arm Components  1.0, Platform Design Document

**/

#include "ConfigurationManager.h"

DefinitionBlock("Dsdt.aml", "DSDT", 2, "ARMLTD", "MORELLO", CFG_MGR_OEM_REVISION) {
  Scope(_SB) {
    Device(CP00) { // Cluster 0, Cpu 0
      Name(_HID, "ACPI0007")
      Name(_UID, 0)
      Name(_STA, 0xF)
    }

    Device(CP01) { // Cluster 0, Cpu 1
      Name(_HID, "ACPI0007")
      Name(_UID, 1)
      Name(_STA, 0xF)
    }

    Device(CP02) { // Cluster 1, Cpu 0
      Name(_HID, "ACPI0007")
      Name(_UID, 2)
      Name(_STA, 0xF)
    }

    Device(CP03) { // Cluster 1, Cpu 1
      Name(_HID, "ACPI0007")
      Name(_UID, 3)
      Name(_STA, 0xF)
    }
  } // Scope(_SB)
}
