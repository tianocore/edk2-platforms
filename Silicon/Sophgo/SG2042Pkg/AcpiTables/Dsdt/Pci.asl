/** @file
*
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

/*
  See ACPI 6.5 Spec, 6.2.11, PCI Firmware Spec 3.0, 4.5
*/
#define PCI_OSC_SUPPORT() \
  Name(SUPP, Zero) /* PCI _OSC Support Field value */ \
  Name(CTRL, Zero) /* PCI _OSC Control Field value */ \
  Method(_OSC,4) { \
    If(LEqual(Arg0,ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))) { \
      /* Create DWord-adressable fields from the Capabilities Buffer */ \
      CreateDWordField(Arg3,0,CDW1) \
      CreateDWordField(Arg3,4,CDW2) \
      CreateDWordField(Arg3,8,CDW3) \
      /* Save Capabilities DWord2 & 3 */ \
      Store(CDW2,SUPP) \
      Store(CDW3,CTRL) \
      /* Only allow native hot plug control if OS supports: */ \
      /* ASPM */ \
      /* Clock PM */ \
      /* MSI/MSI-X */ \
      If(LNotEqual(And(SUPP, 0x16), 0x16)) { \
        And(CTRL,0x1E,CTRL) \
      }\
      \
      /* Do not allow native PME, AER */ \
      /* Never allow SHPC (no SHPC controller in this system)*/ \
      And(CTRL,0x10,CTRL) \
      If(LNotEqual(Arg1,One)) { /* Unknown revision */ \
        Or(CDW1,0x08,CDW1) \
      } \
      \
      If(LNotEqual(CDW3,CTRL)) { /* Capabilities bits were masked */ \
        Or(CDW1,0x10,CDW1) \
      } \
      \
      /* Update DWORD3 in the buffer */ \
      Store(CTRL,CDW3) \
      Return(Arg3) \
    } Else { \
      Or(CDW1,4,CDW1) /* Unrecognized UUID */ \
      Return(Arg3) \
    } \
  } // End _OSC

Scope(_SB)
{
  // PCIe Root bus
  Device (PCI0)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 0)         // Segment of this Root complex
    Name (_BBN, 0)         // Base Bus Number
    Name (_CCA, 1)

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "cdns,max-outbound-regions", 16 },
        Package () { "cdns,no-bar-match-nbits", 48 },
        Package () { "vendor-id", 0x1E30 },
        Package () { "device-id", 0x2042 },
        Package () { "pcie-id", 0x0 },
        Package () { "link-id", 0x0 },
        Package () { "top-intc-used", 0 },
        Package () { "top-intc-id", 0 },
      }
    })

    // PCI Routing Table
    Name (_PRT, Package () {
      Package () { 0xFFFF, 0, 0, 122 },   // INTA
      Package () { 0xFFFF, 1, 0, 122 },   // INTB
      Package () { 0xFFFF, 2, 0, 122 },   // INTC
      Package () { 0xFFFF, 3, 0, 122 },   // INTD
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0,                  // AddressGranularity
        0x0,                // AddressMinimum - Minimum Bus Number
        0x3f,               // AddressMaximum - Maximum Bus Number
        0,                  // AddressTranslation - Set to 0
        0x40                // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                // Granularity
        0x0020000000,       // Min Base Address
        0x005FFFFFFF,       // Max Base Address
        0x4000000000,       // Translate
        0x0040000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                // Granularity
        0x4100000000,       // Min Base Address pci address
        0x43FFFFFFFF,       // Max Base Address
        0x0,                // Translate
        0x0300000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,                // Granularity
        0x0,                // Min Base Address
        0x3fffff,           // Max Base Address
        0x4010000000,       // Translate
        0x400000            // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (ResourceProducer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x0,          // MIN
        0x1effffffff, // MAX
        0x0,          // TRA
        0x1f00000000, // LEN
        ,
        ,
        )
    })

    Device (RES0)
    {
      Name (_HID, "SG200001" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x0)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
        0x0000000000,                       // Granularity
        0x7060000000,                       // Range Minimum
        0x7061FFFFFF,                       // Range Maximum
        0x0000000000,                       // Translation Offset
        0x0002000000,                       // Length
        , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI0)

  // PCIe Root bus
  Device (PCI1)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 1)         // Segment of this Root complex
    Name (_BBN, 0x40)      // Base Bus Number
    Name (_CCA, 1)

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "interrupt-parent" , Package() { \_SB.INTC }},
        Package () { "cdns,max-outbound-regions", 16 },
        Package () { "cdns,no-bar-match-nbits", 48 },
        Package () { "vendor-id", 0x1E30 },
        Package () { "device-id", 0x2042 },
        Package () { "pcie-id", 0x0 },
        Package () { "link-id", 0x1 },
        Package () { "top-intc-used", 1 },
        Package () { "top-intc-id", 0 },
        Package () { "msix-supported", 0 },
      }
    })

    Name (_PRT, Package (){
      Package () {0xFFFF, 0, 0, 64},         // INT_A
      Package () {0xFFFF, 1, 0, 65},         // INT_B
      Package () {0xFFFF, 2, 0, 66},         // INT_C
      Package () {0xFFFF, 3, 0, 67},         // INT_D

      Package () {0x2FFFF, 0, 0, 68},         // INT_A
      Package () {0x2FFFF, 1, 0, 69},         // INT_B
      Package () {0x2FFFF, 2, 0, 70},         // INT_C
      Package () {0x2FFFF, 3, 0, 71},         // INT_D

      Package () {0x3FFFF, 0, 0, 72},         // INT_A
      Package () {0x3FFFF, 1, 0, 73},         // INT_B
      Package () {0x3FFFF, 2, 0, 74},         // INT_C
      Package () {0x3FFFF, 3, 0, 75},         // INT_D

      Package () {0x4FFFF, 0, 0, 76},         // INT_A
      Package () {0x4FFFF, 1, 0, 77},         // INT_B
      Package () {0x4FFFF, 2, 0, 78},         // INT_C
      Package () {0x4FFFF, 3, 0, 79},         // INT_D

      Package () {0x8FFFF, 0, 0, 80},         // INT_A
      Package () {0x8FFFF, 1, 0, 81},         // INT_B
      Package () {0x8FFFF, 2, 0, 82},         // INT_C
      Package () {0x8FFFF, 3, 0, 83},         // INT_D

      Package () {0xCFFFF, 0, 0, 84},         // INT_A
      Package () {0xCFFFF, 1, 0, 85},         // INT_B
      Package () {0xCFFFF, 2, 0, 86},         // INT_C
      Package () {0xCFFFF, 3, 0, 87},         // INT_D

      Package () {0xEFFFF, 0, 0, 88},         // INT_A
      Package () {0xEFFFF, 1, 0, 89},         // INT_B
      Package () {0xEFFFF, 2, 0, 90},         // INT_C
      Package () {0xEFFFF, 3, 0, 91},         // INT_D

      Package () {0xFFFFF, 0, 0, 92},         // INT_A
      Package () {0xFFFFF, 1, 0, 93},         // INT_B
      Package () {0xFFFFF, 2, 0, 94},         // INT_C
      Package () {0xFFFFF, 3, 0, 95},         // INT_D
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0x0,                    // AddressGranularity
        0x40,                   // AddressMinimum - Minimum Bus Number
        0x7f,                   // AddressMaximum - Maximum Bus Number
        0,                      // AddressTranslation - Set to 0
        0x40                    // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x00000000,        // Granularity
        0x60000000,        // Min Base Address
        0x8FFFFFFF,        // Max Base Address
        0x4400000000,      // Translate
        0x30000000         // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,               // Granularity
        0x4500000000,      // Min Base Address pci address
        0x47FFFFFFFF,      // Max Base Address
        0x0,               // Translate
        0x300000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,               // Granularity
        0x0000400000,      // Min Base Address
        0x00007FFFFF,      // Max Base Address
        0x4410000000,      // Translate
        0x0000400000       // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (
        ResourceProducer, ,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,
        0x0,               // MIN
        0x1effffffff,      // MAX
        0x0,               // TRA
        0x1f00000000,      // LEN
        , ,)
    })

  } // Device(PCI1)

  // PCIe Root bus
  Device (PCI2)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 2)         // Segment of this Root complex
    Name (_BBN, 0x80)      // Base Bus Number
    Name (_CCA, 1)

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "cdns,max-outbound-regions", 16 },
        Package () { "cdns,no-bar-match-nbits", 48 },
        Package () { "vendor-id", 0x1E30 },
        Package () { "device-id", 0x2042 },
        Package () { "pcie-id", 0x1 },
        Package () { "link-id", 0x0 },
        Package () { "top-intc-used", 0 },
      }
    })

    // PCI Routing Table
    Name (_PRT, Package () {
      Package () { 0xFFFF, 0, 0, 123 },   // INTA
      Package () { 0xFFFF, 1, 0, 123 },   // INTB
      Package () { 0xFFFF, 2, 0, 123 },   // INTC
      Package () { 0xFFFF, 3, 0, 123 },   // INTD
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0,                   // AddressGranularity
        0x80,                // AddressMinimum - Minimum Bus Number
        0xff,                // AddressMaximum - Maximum Bus Number
        0,                   // AddressTranslation - Set to 0
        0x80                 // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,        // Granularity
        0x0090000000,        // Min Base Address
        0x00FFFFFFFF,        // Max Base Address
        0x4800000000,        // Translate
        0x0070000000         // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                // Granularity
        0x4900000000,       // Min Base Address pci address
        0x4BFFFFFFFF,       // Max Base Address
        0x0,                // Translate
        0x0300000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,               // Granularity
        0x800000,          // Min Base Address
        0xFFFFFF,          // Max Base Address
        0x4810000000,      // Translate
        0x800000           // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (
        ResourceProducer, ,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,
        0x0,               // MIN
        0x1effffffff,      // MAX
        0x0000000000,      // TRA
        0x1f00000000,      // LEN
        , ,)
    })

    Device (RES2)
    {
      Name (_HID, "SG200001" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x2)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory (
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x7062000000,                       // Range Minimum
          0x7063FFFFFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0002000000,                       // Length
          , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI2)


  Device (RESP)  //reserve for ecam resource
  {
    Name (_HID, "PNP0C02")
    Name (_CRS, ResourceTemplate (){
      // ECAM space for PCI0 [bus 00-3f]
      QWordMemory (
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0000000000,                       // Granularity
        0x4000000000,                       // Range Minimum
        0x4003FFFFFF,                       // Range Maximum
        0x0000000000,                       // Translation Offset
        0x0004000000,                       // Length
        , , , AddressRangeMemory, TypeStatic)

      // ECAM space for PCI2 [bus 40-7f]
      QWordMemory (
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0000000000,                        // Granularity
        0x4400000000,                        // Range Minimum
        0x4403FFFFFF,                        // Range Maximum
        0x0000000000,                        // Translation Offset
        0x0004000000,                        // Length
        , , , AddressRangeMemory, TypeStatic)

      // ECAM space for PCI1 [bus 80-ff]
      QWordMemory (
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0000000000,                         // Granularity
        0x4800000000,                         // Range Minimum
        0x4807FFFFFF,                         // Range Maximum
        0x0000000000,                         // Translation Offset
        0x0008000000,                         // Length
        , , , AddressRangeMemory, TypeStatic)
   })
  }

}
