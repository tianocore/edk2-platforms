/** @file
 *
 *  [DSDT] SD controller/card definition (SDHC)
 *
 *  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

Scope(_SB)
{
  Device (MMC0)
  {
    Name (_HID, "SGEM0000")
    Name (_UID, 0x00)           // _UID: Unique ID
    Name (_CCA, 0x01)           // _CCA: Cache Coherency Attribute
    Method (_STA)
    {
      Return(0xf)
    }

    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory ( // 64-bit memory
        ResourceConsumer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                       // Granularity
        0x704002A000,              // Min Base Address
        0x704002AFFF,              // Max Base Address
        0x00000000,                // Translate
        0x00001000                 // Length
      )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 134 }
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
            Package () { "non-removable", 0x1 },
            Package () { "bus-width", 4 },
            Package () { "no-sd", 0x1 },
            Package () { "no-sdio", 0x1  },
      }
    })
  }

  Device (SDC0)
  {
    Name (_HID, "SGSD0000")
    Name (_UID, 0x1)
    Name (_CCA, 0x01)           // _CCA: Cache Coherency Attribute
    Method (_STA)
    {
      Return(0xf)
    }
    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory ( // 64-bit memory
        ResourceConsumer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                       // Granularity
        0x704002B000,              // Min Base Address
        0x704002BFFF,              // Max Base Address
        0x00000000,                // Translate
        0x00001000                 // Length
      )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 136 }
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
            Package () { "bus-width", 4 },
            Package () { "no-mmc", 0x1 },
            Package () { "no-sdio", 0x1  },
      }
    })
  }
}
