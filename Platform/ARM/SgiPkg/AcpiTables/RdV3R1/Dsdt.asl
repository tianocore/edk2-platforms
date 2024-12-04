/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2025, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
* @par Specification Reference:
*   - ACPI 6.5, Chapter 5, Section 5.2.11.1, Differentiated System Description
*     Table (DSDT)
*
**/

#include "SgiPlatform.h"
#include "SgiAcpiHeader.h"

DefinitionBlock ("DsdtTable.aml", "DSDT", 2, "ARMLTD", "ARMSGI",
                 EFI_ACPI_ARM_OEM_REVISION) {
  Scope (_SB) {
    Device (CL00) {   // Cluster 0
      Name (_HID, "ACPI0010")
      Name (_UID, 0)

      Device (CP00) { // Neoverse-V3 Core 0
        Name (_HID, "ACPI0007")
        Name (_UID, 0)
        Name (_STA, 0xF)
      }
    }

    Device (CL01) {   // Cluster 1
      Name (_HID, "ACPI0010")
      Name (_UID, 1)

      Device (CP01) { // Neoverse-V3 Core 1
        Name (_HID, "ACPI0007")
        Name (_UID, 1)
        Name (_STA, 0xF)
      }
    }

    Device (CL02) {   // Cluster 2
      Name (_HID, "ACPI0010")
      Name (_UID, 2)

      Device (CP02) { // Neoverse-V3 Core 2
        Name (_HID, "ACPI0007")
        Name (_UID, 2)
        Name (_STA, 0xF)
      }
    }

    Device (CL03) {   // Cluster 3
      Name (_HID, "ACPI0010")
      Name (_UID, 3)

      Device (CP03) { // Neoverse-V3 Core 3
        Name (_HID, "ACPI0007")
        Name (_UID, 3)
        Name (_STA, 0xF)
      }
    }

    Device (CL04) {   // Cluster 4
      Name (_HID, "ACPI0010")
      Name (_UID, 4)

      Device (CP04) { // Neoverse-V3 Core 4
        Name (_HID, "ACPI0007")
        Name (_UID, 4)
        Name (_STA, 0xF)
      }
    }

    Device (CL05) {   // Cluster 5
      Name (_HID, "ACPI0010")
      Name (_UID, 5)

      Device (CP05) { // Neoverse-V3 Core 5
        Name (_HID, "ACPI0007")
        Name (_UID, 5)
        Name (_STA, 0xF)
      }
    }

    Device (CL06) {   // Cluster 6
      Name (_HID, "ACPI0010")
      Name (_UID, 6)

      Device (CP06) { // Neoverse-V3 Core 6
        Name (_HID, "ACPI0007")
        Name (_UID, 6)
        Name (_STA, 0xF)
      }
    }

    Device (CL07) {   // Cluster 7
      Name (_HID, "ACPI0010")
      Name (_UID, 7)

      Device (CP07) { // Neoverse-V3 Core 7
        Name (_HID, "ACPI0007")
        Name (_UID, 7)
        Name (_STA, 0xF)
      }
    }

    Device (CL08) {   // Cluster 8
      Name (_HID, "ACPI0010")
      Name (_UID, 8)

      Device (CP08) { // Neoverse-V3 Core 8
        Name (_HID, "ACPI0007")
        Name (_UID, 8)
        Name (_STA, 0xF)
      }
    }

    Device (CL09) {   // Cluster 9
      Name (_HID, "ACPI0010")
      Name (_UID, 9)

      Device (CP09) { // Neoverse-V3 Core 9
        Name (_HID, "ACPI0007")
        Name (_UID, 9)
        Name (_STA, 0xF)
      }
    }

    Device (CL10) {   // Cluster 10
      Name (_HID, "ACPI0010")
      Name (_UID, 10)

      Device (CP10) { // Neoverse-V3 Core 10
        Name (_HID, "ACPI0007")
        Name (_UID, 10)
        Name (_STA, 0xF)
      }
    }

    Device (CL11) {   // Cluster 11
      Name (_HID, "ACPI0010")
      Name (_UID, 11)

      Device (CP11) { // Neoverse-V3 Core 11
        Name (_HID, "ACPI0007")
        Name (_UID, 11)
        Name (_STA, 0xF)
      }
    }

    Device (CL12) {   // Cluster 12
      Name (_HID, "ACPI0010")
      Name (_UID, 12)

      Device (CP12) { // Neoverse-V3 Core 12
        Name (_HID, "ACPI0007")
        Name (_UID, 12)
        Name (_STA, 0xF)
      }
    }

    Device (CL13) {   // Cluster 13
      Name (_HID, "ACPI0010")
      Name (_UID, 13)

      Device (CP13) { // Neoverse-V3 Core 13
        Name (_HID, "ACPI0007")
        Name (_UID, 13)
        Name (_STA, 0xF)
      }
    }

    Device (CL14) {   // Cluster 14
      Name (_HID, "ACPI0010")
      Name (_UID, 14)

      Device (CP14) { // Neoverse-V3 Core 14
        Name (_HID, "ACPI0007")
        Name (_UID, 14)
        Name (_STA, 0xF)
      }
    }

    Device (CL15) {   // Cluster 15
      Name (_HID, "ACPI0010")
      Name (_UID, 15)

      Device (CP15) { // Neoverse-V3 Core 15
        Name (_HID, "ACPI0007")
        Name (_UID, 15)
        Name (_STA, 0xF)
      }
    }

    Device (CL16) {   // Cluster 16
      Name (_HID, "ACPI0010")
      Name (_UID, 16)

      Device (CP16) { // Neoverse-V3 Core 16
        Name (_HID, "ACPI0007")
        Name (_UID, 16)
        Name (_STA, 0xF)
      }
    }

    Device (CL17) {   // Cluster 17
      Name (_HID, "ACPI0010")
      Name (_UID, 17)

      Device (CP17) { // Neoverse-V3 Core 17
        Name (_HID, "ACPI0007")
        Name (_UID, 17)
        Name (_STA, 0xF)

      }
    }

    Device (CL18) {   // Cluster 18
      Name (_HID, "ACPI0010")
      Name (_UID, 18)

      Device (CP18) { // Neoverse-V3 Core 18
        Name (_HID, "ACPI0007")
        Name (_UID, 18)
        Name (_STA, 0xF)
      }
    }

    Device (CL19) {   // Cluster 19
      Name (_HID, "ACPI0010")
      Name (_UID, 19)

      Device (CP19) { // Neoverse-V3 Core 19
        Name (_HID, "ACPI0007")
        Name (_UID, 19)
        Name (_STA, 0xF)
      }
    }

    Device (CL20) {   // Cluster 20
      Name (_HID, "ACPI0010")
      Name (_UID, 20)

      Device (CP20) { // Neoverse-V3 Core 20
        Name (_HID, "ACPI0007")
        Name (_UID, 20)
        Name (_STA, 0xF)
      }
    }

    Device (CL21) {   // Cluster 21
      Name (_HID, "ACPI0010")
      Name (_UID, 21)

      Device (CP21) { // Neoverse-V3 Core 21
        Name (_HID, "ACPI0007")
        Name (_UID, 21)
        Name (_STA, 0xF)
      }
    }

    Device (CL22) {   // Cluster 22
      Name (_HID, "ACPI0010")
      Name (_UID, 22)

      Device (CP22) { // Neoverse-V3 Core 22
        Name (_HID, "ACPI0007")
        Name (_UID, 22)
        Name (_STA, 0xF)
      }
    }

    Device (CL23) {   // Cluster 23
      Name (_HID, "ACPI0010")
      Name (_UID, 23)

      Device (CP23) { // Neoverse-V3 Core 23
        Name (_HID, "ACPI0007")
        Name (_UID, 23)
        Name (_STA, 0xF)
      }
    }

    Device (CL24) {   // Cluster 24
      Name (_HID, "ACPI0010")
      Name (_UID, 24)

      Device (CP24) { // Neoverse-V3 Core 24
        Name (_HID, "ACPI0007")
        Name (_UID, 24)
        Name (_STA, 0xF)
      }
    }

    Device (CL25) {   // Cluster 25
      Name (_HID, "ACPI0010")
      Name (_UID, 25)

      Device (CP25) { // Neoverse-V3 Core 25
        Name (_HID, "ACPI0007")
        Name (_UID, 25)
        Name (_STA, 0xF)
      }
    }

    Device (CL26) {   // Cluster 26
      Name (_HID, "ACPI0010")
      Name (_UID, 26)

      Device (CP26) { // Neoverse-V3 Core 26
        Name (_HID, "ACPI0007")
        Name (_UID, 26)
        Name (_STA, 0xF)
      }
    }

    Device (CL27) {   // Cluster 27
      Name (_HID, "ACPI0010")
      Name (_UID, 27)

      Device (CP27) { // Neoverse-V3 Core 27
        Name (_HID, "ACPI0007")
        Name (_UID, 27)
        Name (_STA, 0xF)
      }
    }
  } // Scope(_SB)
}
