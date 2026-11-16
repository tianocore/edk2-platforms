// SPDX-License-Identifier: BSD-2-Clause-Patent
// https://spdx.org/licenses
//
// Copyright (C) 2025-2026 Marvell
//
// SFP SSDT ACPI Table
// TableID must start with 'sfp'
//
// SFP devices for EBB102 and Ebb103 boards

 DefinitionBlock ("", "SSDT", 2, "MRVL", "sfp1023e", 0x00000001)
 {
     External (\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO, DeviceObj) // GPIO defined in Ssdt-ebb103.asl
     External (\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO, DeviceObj) // GPIO defined in Ssdt-ebb103.asl

     Scope (\_SB) {
      //SFP Slot 0, connected to Device(CH03)
      Device (SFS0) {
        Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
        Name (_STR, Unicode ("MARVELL SFP0 SLOT"))
        Name (_DSD, Package () {
          ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
          Package () {
            Package () { "compatible", "sff,sfp" },
            Package () { "i2c-bus", \_SB.PCI0.P2P0.TWI1.MUX0.CH03 },
            Package () { "maximum-power-milliwatt", 1000 },
            Package () { "los-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO, 1, 0, 0 }},       //Index 1, Pin 0
            Package () { "mod-def0-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO, 0, 0, 1 }},  //Index 0, Pin 0
            Package () { "tx-fault-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO, 3, 0, 0 }},  //Index 3, Pin 0
            Package () { "tx-disable-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO, 2, 0, 0 }},//Index 2, Pin 0
          }
        })
        Method (_STA, 0, NotSerialized) {
          Return (0x0F)
        }
       }  //SFS0

      //SFP Slot 1, connected to Device(CH04)
      Device (SFS1) {
        Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
        Name (_STR, Unicode ("MARVELL SFP1 SLOT"))
        Name (_DSD, Package () {
          ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
          Package () {
            Package () { "compatible", "sff,sfp" },
            Package () { "i2c-bus", \_SB.PCI0.P2P0.TWI1.MUX0.CH04 },
            Package () { "maximum-power-milliwatt", 1000 },
            Package () { "los-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO, 1, 0, 0 }},         //Index 1, Pin 0
            Package () { "mod-def0-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO, 0, 0, 1 }},    //Index 0, Pin 0
            Package () { "tx-fault-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO, 3, 0, 0 }},    //Index 3, Pin 0
            Package () { "tx-disable-gpios", Package () {\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO, 2, 0, 0 }},  //Index 2, Pin 0
          }
        })
        Method (_STA, 0, NotSerialized) {
          Return (0x0F)
        }
       }  //SFS1

      //Platform device to manage SFP devices using Linux ethtool
      Device (SFPX) {
        Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
        Name (_STR, Unicode ("MARVELL SFP MGMT"))
        Name (_DSD, Package () {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
              Package () { "compatible", "marvell,sfp-mgmt-proxy" },
            }
        })
        Method (_STA, 0, NotSerialized) {
         Return (0x0F)
        }

        //Device indicating SFP0 is on RPM0/LMAC0
        Device (SFP0) {
          Name (_ADR, 0)
          Name (_DSD, Package () {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
              Package () { "rpm", 0 },
              Package () { "lmac", 0 },
              Package () { "sfp", \_SB.SFS0 },
            }
          })
        } //SFP0

        //Device indicating SFP1 is on RPM1/LMAC0
        Device (SFP1) {
          Name (_ADR, 1)
          Name (_DSD, Package () {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
              Package () { "rpm", 1 },
              Package () { "lmac", 0 },
              Package () { "sfp", \_SB.SFS1 },
            }
          })
        } //SFP1
       }  //SFPX
     } // \_SB
 }
