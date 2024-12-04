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
*   - ACPI 6.5, Chapter 8, Section 8.4.3, Lower Power Idle States
*   - Arm Functional Fixed Hardware Specification v1.2, Chapter 3, Section 3.1,
*     Idle management and Low Power Idle states
*   - ACPI 6.5, Chapter 8, Section 8.4.6, Collaborative Processor Performance
*     Control
*   - Arm Functional Fixed Hardware Specification v1.2, Chapter 3, Section 3.2,
*     Performance management and Collaborative Processor Performance Control
*
**/

#include "SgiPlatform.h"
#include "SgiAcpiHeader.h"

DefinitionBlock ("DsdtTable.aml", "DSDT", 2, "ARMLTD", "ARMSGI",
                 EFI_ACPI_ARM_OEM_REVISION) {
  Scope (_SB) {
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

          If (And (CAP0, OSC_CAP_CPPC_SUPPORT)) {
            /* CPPC revision 1 and below not supported */
            And (CAP0, Not (OSC_CAP_CPPC_SUPPORT), CAP0)
            Or (STS0, OSC_STS_CAPABILITY_MASKED, STS0)
          }

          If (And (CAP0, OSC_CAP_CPPC2_SUPPORT)) {
            if (LEqual (FixedPcdGet32 (PcdOscCppcEnable), Zero)) {
              And (CAP0, Not (OSC_CAP_CPPC2_SUPPORT), CAP0)
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

    Name (PLPI, Package () {  /* LPI for Processor, support 2 LPI states */
      0,                      // Version
      0,                      // Level Index
      2,                      // Count
      Package () {            // WFI for CPU
        1,                    // Min residency (uS)
        1,                    // Wake latency (uS)
        1,                    // Flags
        0,                    // Arch Context lost Flags (no loss)
        0,                    // Residency Counter Frequency
        0,                    // No parent state
        ResourceTemplate () { // Register Entry method
          Register (FFixedHW,
            32,               // Bit Width
            0,                // Bit Offset
            0xFFFFFFFF,       // Address
            3,                // Access Size
          )
        },
        ResourceTemplate () { // Null Residency Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        ResourceTemplate () { // Null Usage Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        "LPI1-Core"
      },
      Package () {            // Power Gating state for CPU
        150,                  // Min residency (uS)
        350,                  // Wake latency (uS)
        1,                    // Flags
        1,                    // Arch Context lost Flags (Core context lost)
        0,                    // Residency Counter Frequency
        0,                    // No parent state
        ResourceTemplate () { // Register Entry method
          Register (FFixedHW,
            32,               // Bit Width
            0,                // Bit Offset
            0x40000002,       // Address (PwrLvl:core, StateTyp:PwrDn)
            3,                // Access Size
          )
        },
        ResourceTemplate () { // Null Residency Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        ResourceTemplate () { // Null Usage Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        "LPI3-Core"
      },
    })

    Device (CL00) {   // Cluster 0
      Name (_HID, "ACPI0010")
      Name (_UID, 0)

      Device (CP00) { // Neoverse-V3 core 0
        Name (_HID, "ACPI0007")
        Name (_UID, 0)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160073000, 0x160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (0)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL01) {   // Cluster 1
      Name (_HID, "ACPI0010")
      Name (_UID, 1)

      Device (CP01) { // Neoverse-V3 core 1
        Name (_HID, "ACPI0007")
        Name (_UID, 1)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160073000, 0x160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (1)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL02) {   // Cluster 2
      Name (_HID, "ACPI0010")
      Name (_UID, 2)

      Device (CP02) { // Neoverse-V3 core 2
        Name (_HID, "ACPI0007")
        Name (_UID, 2)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160073000, 0x160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (2)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL03) {   // Cluster 3
      Name (_HID, "ACPI0010")
      Name (_UID, 3)

      Device (CP03) { // Neoverse-V3 core 3
        Name (_HID, "ACPI0007")
        Name (_UID, 3)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160073000, 0x160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (3)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL04) {   // Cluster 4
      Name (_HID, "ACPI0010")
      Name (_UID, 4)

      Device (CP04) { // Neoverse-V3 core 4
        Name (_HID, "ACPI0007")
        Name (_UID, 4)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160273000, 0x160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (4)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL05) {   // Cluster 5
      Name (_HID, "ACPI0010")
      Name (_UID, 5)

      Device (CP05) { // Neoverse-V3 core 5
        Name (_HID, "ACPI0007")
        Name (_UID, 5)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160273000, 0x160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (5)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL06) {   // Cluster 6
      Name (_HID, "ACPI0010")
      Name (_UID, 6)

      Device (CP06) { // Neoverse-V3 core 6
        Name (_HID, "ACPI0007")
        Name (_UID, 6)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160273000, 0x160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (6)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL07) {   // Cluster 7
      Name (_HID, "ACPI0010")
      Name (_UID, 7)

      Device (CP07) { // Neoverse-V3 core 7
        Name (_HID, "ACPI0007")
        Name (_UID, 7)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x160273000, 0x160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (7)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL08) {   // Cluster 8
      Name (_HID, "ACPI0010")
      Name (_UID, 8)

      Device (CP08) { // Neoverse-V3 core 8
        Name (_HID, "ACPI0007")
        Name (_UID, 8)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160073000, 0x1160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (8)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL09) {   // Cluster 9
      Name (_HID, "ACPI0010")
      Name (_UID, 9)

      Device (CP09) { // Neoverse-V3 core 9
        Name (_HID, "ACPI0007")
        Name (_UID, 9)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160073000, 0x1160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (9)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL10) {   // Cluster 10
      Name (_HID, "ACPI0010")
      Name (_UID, 10)

      Device (CP10) { // Neoverse-V3 core 10
        Name (_HID, "ACPI0007")
        Name (_UID, 10)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160073000, 0x1160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (10)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL11) {   // Cluster 11
      Name (_HID, "ACPI0010")
      Name (_UID, 11)

      Device (CP11) { // Neoverse-V3 core 11
        Name (_HID, "ACPI0007")
        Name (_UID, 11)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160073000, 0x1160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (11)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL12) {   // Cluster 12
      Name (_HID, "ACPI0010")
      Name (_UID, 12)

      Device (CP12) { // Neoverse-V3 core 12
        Name (_HID, "ACPI0007")
        Name (_UID, 12)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160273000, 0x1160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (12)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL13) {   // Cluster 13
      Name (_HID, "ACPI0010")
      Name (_UID, 13)

      Device (CP13) { // Neoverse-V3 core 13
        Name (_HID, "ACPI0007")
        Name (_UID, 13)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160273000, 0x1160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (13)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL14) {   // Cluster 14
      Name (_HID, "ACPI0010")
      Name (_UID, 14)

      Device (CP14) { // Neoverse-V3 core 14
        Name (_HID, "ACPI0007")
        Name (_UID, 14)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160273000, 0x1160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (14)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL15) {   // Cluster 15
      Name (_HID, "ACPI0010")
      Name (_UID, 15)

      Device (CP15) { // Neoverse-V3 core 15
        Name (_HID, "ACPI0007")
        Name (_UID, 15)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x1160273000, 0x1160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (15)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL16) {   // Cluster 16
      Name (_HID, "ACPI0010")
      Name (_UID, 16)

      Device (CP16) { // Neoverse-V3 Core 16
        Name (_HID, "ACPI0007")
        Name (_UID, 16)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160073000, 0x2160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (16)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL17) {   // Cluster 17
      Name (_HID, "ACPI0010")
      Name (_UID, 17)

      Device (CP17) { // Neoverse-V3 Core 17
        Name (_HID, "ACPI0007")
        Name (_UID, 17)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160073000, 0x2160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (17)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL18) {   // Cluster 18
      Name (_HID, "ACPI0010")
      Name (_UID, 18)

      Device (CP18) { // Neoverse-V3 Core 18
        Name (_HID, "ACPI0007")
        Name (_UID, 18)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160073000, 0x2160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (18)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL19) {   // Cluster 19
      Name (_HID, "ACPI0010")
      Name (_UID, 19)

      Device (CP19) { // Neoverse-V3 Core 19
        Name (_HID, "ACPI0007")
        Name (_UID, 19)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160073000, 0x2160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (19)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL20) {   // Cluster 20
      Name (_HID, "ACPI0010")
      Name (_UID, 20)

      Device (CP20) { // Neoverse-V3 Core 20
        Name (_HID, "ACPI0007")
        Name (_UID, 20)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160273000, 0x2160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (20)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL21) {   // Cluster 21
      Name (_HID, "ACPI0010")
      Name (_UID, 21)

      Device (CP21) { // Neoverse-V3 Core 21
        Name (_HID, "ACPI0007")
        Name (_UID, 21)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160273000, 0x2160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (21)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL22) {   // Cluster 22
      Name (_HID, "ACPI0010")
      Name (_UID, 22)

      Device (CP22) { // Neoverse-V3 Core 22
        Name (_HID, "ACPI0007")
        Name (_UID, 22)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160273000, 0x2160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (22)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL23) {   // Cluster 23
      Name (_HID, "ACPI0010")
      Name (_UID, 23)

      Device (CP23) { // Neoverse-V3 Core 23
        Name (_HID, "ACPI0007")
        Name (_UID, 23)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x2160273000, 0x2160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (23)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL24) {   // Cluster 24
      Name (_HID, "ACPI0010")
      Name (_UID, 24)

      Device (CP24) { // Neoverse-V3 Core 24
        Name (_HID, "ACPI0007")
        Name (_UID, 24)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160073000, 0x3160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (24)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL25) {   // Cluster 25
      Name (_HID, "ACPI0010")
      Name (_UID, 25)

      Device (CP25) { // Neoverse-V3 Core 25
        Name (_HID, "ACPI0007")
        Name (_UID, 25)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160073000, 0x3160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (25)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL26) {   // Cluster 26
      Name (_HID, "ACPI0010")
      Name (_UID, 26)

      Device (CP26) { // Neoverse-V3 Core 26
        Name (_HID, "ACPI0007")
        Name (_UID, 26)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160073000, 0x3160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (26)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL27) {   // Cluster 27
      Name (_HID, "ACPI0010")
      Name (_UID, 27)

      Device (CP27) { // Neoverse-V3 Core 27
        Name (_HID, "ACPI0007")
        Name (_UID, 27)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160073000, 0x3160073004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (27)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL28) {   // Cluster 28
      Name (_HID, "ACPI0010")
      Name (_UID, 28)

      Device (CP28) { // Neoverse-V3 Core 28
        Name (_HID, "ACPI0007")
        Name (_UID, 28)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160273000, 0x3160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (28)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL29) {   // Cluster 29
      Name (_HID, "ACPI0010")
      Name (_UID, 29)

      Device (CP29) { // Neoverse-V3 Core 29
        Name (_HID, "ACPI0007")
        Name (_UID, 29)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160273000, 0x3160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (29)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL30) {   // Cluster 30
      Name (_HID, "ACPI0010")
      Name (_UID, 30)

      Device (CP30) { // Neoverse-V3 Core 30
        Name (_HID, "ACPI0007")
        Name (_UID, 30)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160273000, 0x3160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (30)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CL31) {   // Cluster 31
      Name (_HID, "ACPI0010")
      Name (_UID, 31)

      Device (CP31) { // Neoverse-V3 Core 31
        Name (_HID, "ACPI0007")
        Name (_UID, 31)
        Name (_STA, 0xF)

        Name (_CPC, Package()
          CPPC_PACKAGE_INIT (0x3160273000, 0x3160273004, 20, 160, 160, 85, 85, 5)
        )

        Name (_PSD, Package () {
          Package ()
            PSD_INIT (31)
        })

        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }
  } // Scope(_SB)
}
