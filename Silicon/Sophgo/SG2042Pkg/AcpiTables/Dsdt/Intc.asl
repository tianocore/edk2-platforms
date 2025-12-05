/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>

**/

Scope(_SB)
{
  Device (INTC) {        // Top intc
    Name(_HID, "SG200002")
    Name(_CRS, ResourceTemplate() {
      QWordMemory (
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0x70300102E0,                // Min Base Address
        0x70300102E3,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      QWordMemory (
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0x7030010300,                // Min Base Address
        0x7030010303,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      QWordMemory (
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0x7030010304,                // Min Base Address
        0x7030010307,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      Interrupt (ResourceConsumer, Edge, ActiveHigh, Exclusive,,,) {
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
        76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
        88, 89, 90, 91, 92, 93, 94, 95,
      }
    })

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package ()
      {
        Package () { "top-intc-id", 0 },
        Package () { "for-msi", 1 },
        Package () { "reg-bitwidth", 32 },
      }
    })
  }
}
