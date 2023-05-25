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

    Name (CLPI, Package () {  /* LPI for Cluster, support 1 LPI state */
      0,                      // Version
      0,                      // Level Index
      1,                      // Count
      Package () {            // Power Gating state for Cluster
        2500,                 // Min residency (uS)
        1150,                 // Wake latency (uS)
        1,                    // Flags
        1,                    // Arch Context Flags
        0,                    // Residency Counter Frequency
        0,                    // No Parent State
        0x00000020,           // Integer Entry method
        ResourceTemplate () { // Null Residency Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        ResourceTemplate () { // Null Usage Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        "LPI2-Cluster"
      },
    })

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
        1,                    // Parent node can be in any shallower state
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

    Device (CLU0) {   // Cluster 0
      Name (_HID, "ACPI0010")
      Name (_UID, 0)
      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.CLPI)
      }

      Device (CP00) { // Cluster 0, Cpu 0
        Name (_HID, "ACPI0007")
        Name (_UID, 0)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }

      Device (CP01) { // Cluster 0, Cpu 1
        Name (_HID, "ACPI0007")
        Name (_UID, 1)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
    }

    Device (CLU1) {   // Cluster 1
      Name (_HID, "ACPI0010")
      Name (_UID, 1)
      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.CLPI)
      }

      Device (CP02) { // Cluster 1, Cpu 0
        Name (_HID, "ACPI0007")
        Name (_UID, 2)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }

      Device (CP03) { // Cluster 1, Cpu 1
        Name (_HID, "ACPI0007")
        Name (_UID, 3)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
      }
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
        Memory32Fixed(ReadWrite, 0x1C180000, 0x00000200)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 134 }
      })
    }

    // VIRTIO RANDOM
    Device(VR02) {
      Name(_HID, "LNRO0005")
      Name(_UID, 2)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, 0x1C190000, 0x00000200)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 133 }
      })
    }

    // VIRTIO P9 Device
    Device(VR03) {
      Name(_HID, "LNRO0005")
      Name(_UID, 3)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, 0x1C1A0000, 0x00000200)
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
