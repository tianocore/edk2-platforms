/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

Scope(_SB)
{
  Method (_OSC,4) {
    // Create DWord-adressable for Arg3 First DWORD.
    CreateDWordField(Arg3,0,CDW1)
    CreateDWordField(Arg3,4,CDW2)

    // Check for proper UUID
    If (LEqual(Arg0,ToUUID("0811B06E-4A27-44F9-8D60-3CBBC22E7B48"))) {

      If (LNotEqual(Arg1,One)) {// Unknown revision
        Or (CDW1,0x0A,CDW1)
      }
      Else {
        And (CDW2,0xC0,CDW2)
      }

      Return (Arg3)
    }
    Else {
      Or (CDW1,0x6,CDW1) // Unrecognized UUID
      Return (Arg3)
    }
  } // End _OSC

  Name (CLPI, Package () {  /* LPI for Cluster, support 1 LPI state */
    0,                      // Version
    0,                      // Level Index
    1,                      // Count
    // LPI3
    Package () LPI_PACKAGE_INIT(3500, 100, 1, 0, 100, 1, 0x1000000080000000, "RISC-V NONRET_DEFAULT")
  })

  Name (PLPI, Package () {  /* LPI for Processor, support 3 LPI states */
    0,                      // Version
    0,                      // Level Index
    3,                      // Count
    // LPI1
    Package () LPI_PACKAGE_INIT(1, 1, 1, 0, 100, 0, 0x0000000000000000, "RISC-V WFI"),

    // LPI2
    Package () LPI_PACKAGE_INIT(10, 10, 1, 0, 100, 1, 0x1000000000000000, "RISC-V RET_DEFAULT"),

    // LPI3
    Package () LPI_PACKAGE_INIT(3500, 100, 1, 0, 100, 1, 0x1000000080000000, "RISC-V NONRET_DEFAULT")
  })

  //
  // SG2042 Processor declaration
  //
  Device (CL00) {   // Cluster 0
    Name (_HID, "ACPI0010")
    Name (_UID, 0)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP00) { // SG2042 Cluster 0, core 0
      Name (_HID, "ACPI0007")
      Name (_UID, 0)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Hart ID */
        0x00, 0x00, 0x00, 0x00,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x00,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP01) { // SG2042 Cluster 0, core 1
      Name (_HID, "ACPI0007")
      Name (_UID, 1)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* Hart ID */
        0x00, 0x00, 0x00, 0x01,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x01,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP02) { // SG2042 Cluster 0, core 2
      Name (_HID, "ACPI0007")
      Name (_UID, 2)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* Hart ID */
        0x00, 0x00, 0x00, 0x02,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x02,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP03) { // SG2042 Cluster 0, core 3
      Name (_HID, "ACPI0007")
      Name (_UID, 3)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, /* Hart ID */
        0x00, 0x00, 0x00, 0x03,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x03,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP04) { // SG2042 Cluster 1, core 4
      Name (_HID, "ACPI0007")
      Name (_UID, 4)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, /* Hart ID */
        0x00, 0x00, 0x00, 0x04,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x04,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP05) { // SG2042 Cluster 1, core 5
      Name (_HID, "ACPI0007")
      Name (_UID, 5)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, /* Hart ID */
        0x00, 0x00, 0x00, 0x05,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x05,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP06) { // SG2042 Cluster 1, core 6
      Name (_HID, "ACPI0007")
      Name (_UID, 6)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, /* Hart ID */
        0x00, 0x00, 0x00, 0x06,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x06,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP07) { // SG2042 Cluster 1, core 7
      Name (_HID, "ACPI0007")
      Name (_UID, 7)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, /* Hart ID */
        0x00, 0x00, 0x00, 0x07,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x07,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL02) {   // Cluster 2
    Name (_HID, "ACPI0010")
    Name (_UID, 2)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP08) { // SG2042 Cluster 2, core 8
      Name (_HID, "ACPI0007")
      Name (_UID, 8)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, /* Hart ID */
        0x00, 0x00, 0x00, 0x08,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x08,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP09) { // SG2042 Cluster 2, core 9
      Name (_HID, "ACPI0007")
      Name (_UID, 9)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, /* Hart ID */
        0x00, 0x00, 0x00, 0x09,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x09,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP10) { // SG2042 Cluster 2, core 10
      Name (_HID, "ACPI0007")
      Name (_UID, 10)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, /* Hart ID */
        0x00, 0x00, 0x00, 0x0A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0A,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP11) { // SG2042 Cluster 2, core 11
      Name (_HID, "ACPI0007")
      Name (_UID, 11)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, /* Hart ID */
        0x00, 0x00, 0x00, 0x0B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL03) {   // Cluster 3
    Name (_HID, "ACPI0010")
    Name (_UID, 3)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP12) { // SG2042 Cluster 3, core 12
      Name (_HID, "ACPI0007")
      Name (_UID, 12)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, /* Hart ID */
        0x00, 0x00, 0x00, 0x0C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0C,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP13) { // SG2042 Cluster 3, core 13
      Name (_HID, "ACPI0007")
      Name (_UID, 13)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, /* Hart ID */
        0x00, 0x00, 0x00, 0x0D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP14) { // SG2042 Cluster 3, core 14
      Name (_HID, "ACPI0007")
      Name (_UID, 14)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, /* Hart ID */
        0x00, 0x00, 0x00, 0x0E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0E,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP15) { // SG2042 Cluster 3, core 15
      Name (_HID, "ACPI0007")
      Name (_UID, 15)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, /* Hart ID */
        0x00, 0x00, 0x00, 0x0F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL04) {   // Cluster 4
    Name (_HID, "ACPI0010")
    Name (_UID, 4)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP16) { // SG2042 Cluster 4, core 16
      Name (_HID, "ACPI0007")
      Name (_UID, 16)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* Hart ID */
        0x00, 0x00, 0x00, 0x10,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x10,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP17) { // SG2042 Cluster 4, core 17
      Name (_HID, "ACPI0007")
      Name (_UID, 17)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, /* Hart ID */
        0x00, 0x00, 0x00, 0x11,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x11,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP18) { // SG2042 Cluster 4, core 18
      Name (_HID, "ACPI0007")
      Name (_UID, 18)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, /* Hart ID */
        0x00, 0x00, 0x00, 0x12,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x12,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP19) { // SG2042 Cluster 4, core 19
      Name (_HID, "ACPI0007")
      Name (_UID, 19)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, /* Hart ID */
        0x00, 0x00, 0x00, 0x13,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x13,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL05) {   // Cluster 5
    Name (_HID, "ACPI0010")
    Name (_UID, 5)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP20) { // SG2042 Cluster 5, core 20
      Name (_HID, "ACPI0007")
      Name (_UID, 20)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, /* Hart ID */
        0x00, 0x00, 0x00, 0x14,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x14,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP21) { // SG2042 Cluster 5, core 21
      Name (_HID, "ACPI0007")
      Name (_UID, 21)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, /* Hart ID */
        0x00, 0x00, 0x00, 0x15,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x15,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP22) { // SG2042 Cluster 5, core 22
      Name (_HID, "ACPI0007")
      Name (_UID, 22)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, /* Hart ID */
        0x00, 0x00, 0x00, 0x16,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x16,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP23) { // SG2042 Cluster 5, core 23
      Name (_HID, "ACPI0007")
      Name (_UID, 23)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, /* Hart ID */
        0x00, 0x00, 0x00, 0x17,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x17,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL06) {   // Cluster 6
    Name (_HID, "ACPI0010")
    Name (_UID, 6)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP24) { // SG2042 Cluster 6, core 24
      Name (_HID, "ACPI0007")
      Name (_UID, 24)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, /* Hart ID */
        0x00, 0x00, 0x00, 0x18,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x18,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP25) { // SG2042 Cluster 6, core 25
      Name (_HID, "ACPI0007")
      Name (_UID, 25)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, /* Hart ID */
        0x00, 0x00, 0x00, 0x19,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x19,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP26) { // SG2042 Cluster 6, core 26
      Name (_HID, "ACPI0007")
      Name (_UID, 26)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, /* Hart ID */
        0x00, 0x00, 0x00, 0x1A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1A,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP27) { // SG2042 Cluster 6, core 27
      Name (_HID, "ACPI0007")
      Name (_UID, 27)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, /* Hart ID */
        0x00, 0x00, 0x00, 0x1B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL07) {   // Cluster 7
    Name (_HID, "ACPI0010")
    Name (_UID, 7)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP28) { // SG2042 Cluster 7, core 28
      Name (_HID, "ACPI0007")
      Name (_UID, 28)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, /* Hart ID */
        0x00, 0x00, 0x00, 0x1C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1C,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP29) { // SG2042 Cluster 7, core 29
      Name (_HID, "ACPI0007")
      Name (_UID, 29)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, /* Hart ID */
        0x00, 0x00, 0x00, 0x1D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP30) { // SG2042 Cluster 7, core 30
      Name (_HID, "ACPI0007")
      Name (_UID, 30)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, /* Hart ID */
        0x00, 0x00, 0x00, 0x1E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1E,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP31) { // SG2042 Cluster 7, core 31
      Name (_HID, "ACPI0007")
      Name (_UID, 31)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, /* Hart ID */
        0x00, 0x00, 0x00, 0x1F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL08) {   // Cluster 8
    Name (_HID, "ACPI0010")
    Name (_UID, 8)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP32) { // SG2042 Cluster 8, core 32
      Name (_HID, "ACPI0007")
      Name (_UID, 32)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, /* Hart ID */
        0x00, 0x00, 0x00, 0x20,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x20,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP33) { // SG2042 Cluster 8, core 33
      Name (_HID, "ACPI0007")
      Name (_UID, 33)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, /* Hart ID */
        0x00, 0x00, 0x00, 0x21,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x21,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP34) { // SG2042 Cluster 8, core 34
      Name (_HID, "ACPI0007")
      Name (_UID, 34)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, /* Hart ID */
        0x00, 0x00, 0x00, 0x22,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x22,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP35) { // SG2042 Cluster 8, core 35
      Name (_HID, "ACPI0007")
      Name (_UID, 35)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, /* Hart ID */
        0x00, 0x00, 0x00, 0x23,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x23,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL09) {   // Cluster 9
    Name (_HID, "ACPI0010")
    Name (_UID, 9)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP36) { // SG2042 Cluster 9, core 36
      Name (_HID, "ACPI0007")
      Name (_UID, 36)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, /* Hart ID */
        0x00, 0x00, 0x00, 0x24,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x24,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP37) { // SG2042 Cluster 9, core 37
      Name (_HID, "ACPI0007")
      Name (_UID, 37)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, /* Hart ID */
        0x00, 0x00, 0x00, 0x25,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x25,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP38) { // SG2042 Cluster 9, core 38
      Name (_HID, "ACPI0007")
      Name (_UID, 38)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, /* Hart ID */
        0x00, 0x00, 0x00, 0x26,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x26,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP39) { // SG2042 Cluster 9, core 39
      Name (_HID, "ACPI0007")
      Name (_UID, 39)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, /* Hart ID */
        0x00, 0x00, 0x00, 0x27,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x27,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL10) {   // Cluster 10
    Name (_HID, "ACPI0010")
    Name (_UID, 10)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP40) { // SG2042 Cluster 10, core 40
      Name (_HID, "ACPI0007")
      Name (_UID, 40)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, /* Hart ID */
        0x00, 0x00, 0x00, 0x28,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x28,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP41) { // SG2042 Cluster 10, core 41
      Name (_HID, "ACPI0007")
      Name (_UID, 41)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, /* Hart ID */
        0x00, 0x00, 0x00, 0x29,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x29,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP42) { // SG2042 Cluster 10, core 42
      Name (_HID, "ACPI0007")
      Name (_UID, 42)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, /* Hart ID */
        0x00, 0x00, 0x00, 0x2A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2A,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP43) { // SG2042 Cluster 10, core 43
      Name (_HID, "ACPI0007")
      Name (_UID, 43)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, /* Hart ID */
        0x00, 0x00, 0x00, 0x2B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL11) {   // Cluster 11
    Name (_HID, "ACPI0010")
    Name (_UID, 11)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP44) { // SG2042 Cluster 11, core 44
      Name (_HID, "ACPI0007")
      Name (_UID, 44)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2C, /* Hart ID */
        0x00, 0x00, 0x00, 0x2C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2C,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP45) { // SG2042 Cluster 11, core 45
      Name (_HID, "ACPI0007")
      Name (_UID, 45)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, /* Hart ID */
        0x00, 0x00, 0x00, 0x2D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP46) { // SG2042 Cluster 11, core 46
      Name (_HID, "ACPI0007")
      Name (_UID, 46)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, /* Hart ID */
        0x00, 0x00, 0x00, 0x2E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2E,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP47) { // SG2042 Cluster 0, core 47
      Name (_HID, "ACPI0007")
      Name (_UID, 47)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, /* Hart ID */
        0x00, 0x00, 0x00, 0x2F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL12) {   // Cluster 12
    Name (_HID, "ACPI0010")
    Name (_UID, 12)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP48) { // SG2042 Cluster 12, core 48
      Name (_HID, "ACPI0007")
      Name (_UID, 48)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, /* Hart ID */
        0x00, 0x00, 0x00, 0x30,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x30,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP49) { // SG2042 Cluster 12, core 49
      Name (_HID, "ACPI0007")
      Name (_UID, 49)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, /* Hart ID */
        0x00, 0x00, 0x00, 0x31,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x31,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP50) { // SG2042 Cluster 12, core 50
      Name (_HID, "ACPI0007")
      Name (_UID, 50)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, /* Hart ID */
        0x00, 0x00, 0x00, 0x32,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x32,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP51) { // SG2042 Cluster 12, core 51
      Name (_HID, "ACPI0007")
      Name (_UID, 51)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, /* Hart ID */
        0x00, 0x00, 0x00, 0x33,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x33,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL13) {   // Cluster 13
    Name (_HID, "ACPI0010")
    Name (_UID, 13)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP52) { // SG2042 Cluster 13, core 52
      Name (_HID, "ACPI0007")
      Name (_UID, 52)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, /* Hart ID */
        0x00, 0x00, 0x00, 0x34,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x34,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP53) { // SG2042 Cluster 13, core 53
      Name (_HID, "ACPI0007")
      Name (_UID, 53)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, /* Hart ID */
        0x00, 0x00, 0x00, 0x35,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x35,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP54) { // SG2042 Cluster 13, core 54
      Name (_HID, "ACPI0007")
      Name (_UID, 54)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, /* Hart ID */
        0x00, 0x00, 0x00, 0x36,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x36,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP55) { // SG2042 Cluster 13, core 55
      Name (_HID, "ACPI0007")
      Name (_UID, 55)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, /* Hart ID */
        0x00, 0x00, 0x00, 0x37,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x37,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL14) {   // Cluster 14
    Name (_HID, "ACPI0010")
    Name (_UID, 14)

    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP56) { // SG2042 Cluster 14, core 56
      Name (_HID, "ACPI0007")
      Name (_UID, 56)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, /* Hart ID */
        0x00, 0x00, 0x00, 0x38,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x38,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP57) { // SG2042 Cluster 14, core 57
      Name (_HID, "ACPI0007")
      Name (_UID, 57)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, /* Hart ID */
        0x00, 0x00, 0x00, 0x39,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x39,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP58) { // SG2042 Cluster 14, core 58
      Name (_HID, "ACPI0007")
      Name (_UID, 58)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, /* Hart ID */
        0x00, 0x00, 0x00, 0x3A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3A,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP59) { // SG2042 Cluster 14, core 59
      Name (_HID, "ACPI0007")
      Name (_UID, 59)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, /* Hart ID */
        0x00, 0x00, 0x00, 0x3B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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

  Device (CL15) {   // Cluster 15
    Name (_HID, "ACPI0010")
    Name (_UID, 15)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (CP60) { // SG2042 Cluster 15, core 60
      Name (_HID, "ACPI0007")
      Name (_UID, 60)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, /* Hart ID */
        0x00, 0x00, 0x00, 0x3C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3C,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP61) { // SG2042 Cluster 15, core 61
      Name (_HID, "ACPI0007")
      Name (_UID, 61)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3D, /* Hart ID */
        0x00, 0x00, 0x00, 0x3D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP62) { // SG2042 Cluster 15, core 62
      Name (_HID, "ACPI0007")
      Name (_UID, 62)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, /* Hart ID */
        0x00, 0x00, 0x00, 0x3E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3E,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (CP63) { // SG2042 Cluster 15, core 63
      Name (_HID, "ACPI0007")
      Name (_UID, 63)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, /* Hart ID */
        0x00, 0x00, 0x00, 0x3F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
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
}
