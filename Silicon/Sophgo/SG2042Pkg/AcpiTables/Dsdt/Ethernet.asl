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
  Device (ETH0) {
    Name (_HID, "SG200007")
    Name (_UID, Zero)
    Name (_CCA, 1)
    Method (_STA)                                       // _STA: Device status
    {
      Return (0xF)
    }
    Name (_CRS, ResourceTemplate () {
      QWordMemory (
        ResourceConsumer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                       // Granularity
        0x7040026000,              // Min Base Address
        0x7040029FFF,              // Max Base Address
        0x0,                       // Translate
        0x0000004000               // Length
      )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 132 }
    })

    Name (_DSD, Package ()  // _DSD: Device-Specific Data
    {
      ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) { "compatible", Package () { "bitmain,ethernet" } },
        Package (2) { "interrupt-names", Package () { "macirq" } },
        Package (2) { "snps,multicast-filter-bins", 0 },
        Package (2) { "snps,perfect-filter-entries", 1 },
        Package (2) { "snps,txpbl", 32 },
        Package (2) { "snps,rxpbl", 32 },
        Package (2) { "snps,aal", 1 },
        Package (2) { "snps,axi-config", "AXIC" },
        Package (2) { "phy-mode", "rgmii-txid" },
        Package (2) { "phy-reset-gpios", Package () { \_SB.GPO0.PRTa, 27, 0 } },
      }
    })

    Name (AXIC, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "snps,wr_osr_lmt", 1},
        Package () { "snps,rd_osr_lmt", 2},
        Package () { "snps,blen", Package () { 4, 8, 16, 0, 0, 0, 0 } },
      }
    })

    Device (MDIO)
    {
      Name (_ADR, 0x0)
      Device (PHY0)
      {
        Name (_ADR, 0x0)
        Name (_DSD, Package () {
          ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
          Package () {
            Package () {"reg", 0},
            Package () {"device_type", "ethernet-phy"},
          }
        })
      }
    }
  }
}
