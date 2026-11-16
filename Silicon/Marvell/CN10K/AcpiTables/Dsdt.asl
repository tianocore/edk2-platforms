/** @file
 *
 *  Differentiated System Definition Table (DSDT)
 *
 *  Copyright (c) 2025-2026 Marvell
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *  https://spdx.org/licenses
 *
 **/

#define GENERIC_EVENT_DEVICE      0x800
#define PLATFORM_APEI             0x010
#define PCIE_CAPABILITY           0x010
#define OSC_ERROR                 0x004
#define CN10K_CPU_TYPE_103XX      0xBD

 DefinitionBlock ("", "DSDT", 2, "MRVL  ", "CN10K", 1)
 {
   Scope (\_SB_)
   {

     Device (CP00) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x00)
     }

     Device (CP01) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x01)
     }

     Device (CP02) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x02)
     }

     Device (CP03) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x03)
     }

     Device (CP04) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x04)
     }

     Device (CP05) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x05)
     }

     Device (CP06) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x06)
     }

     Device (CP07) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x07)
     }

     Device (CP08) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x08)
     }

     Device (CP09) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x09)
     }

     Device (CP10) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x0A)
     }

     Device (CP11) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x0B)
     }

     Device (CP12) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x0C)
     }

     Device (CP13) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x0D)
     }

     Device (CP14) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x0E)
     }

     Device (CP15) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x0F)
     }

     Device (CP16) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x10)
     }

     Device (CP17) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x11)
     }

     Device (CP18) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x12)
     }

     Device (CP19) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x13)
     }

     Device (CP20) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x14)
     }

     Device (CP21) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x15)
     }

     Device (CP22) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x16)
     }

     Device (CP23) {
       Name (_HID, "ACPI0007")
       Name (_UID, 0x17)
     }

     // Platform-Wide OSPM Capabilities
     //
     Method (_OSC, 4, NotSerialized)                                     \
     {                                                                   \
         /* PCIe _OSC Capabilities */                                    \
         If (Arg0 == ToUUID("0811B06E-4A27-44F9-8D60-3CBBC22E7B48")) {   \
             /* No support for any */                                    \
             CreateDWordField (Arg3, 4, CDW3)                            \
             CDW3 = GENERIC_EVENT_DEVICE                                 \
             CDW3 |= PLATFORM_APEI                                       \
             Return (Arg3)                                               \
         } Else {                                                        \
             CreateDWordField (Arg3, Zero, CDW1)                         \
             /* Set Unrecognized UUID error bit */                       \
             CDW1 |= OSC_ERROR                                           \
             Return (Arg3)                                               \
         }                                                               \
     }

     //
     // PCI Root Bridge
     //
     Device (PCI0) {
       //
       // Hardware ID must be PNP0A08, which maps to a PCIe root complex.
       // Section 6.1.5
       //
       Name (_HID, EISAID ("PNP0A08"))

       //
       // Optionally, include a compatible ID of PNP0A03, which maps to a PCI
       // root complex for use with pre-PCIe operating systems.
       // Section 6.1.2
       //
       Name (_CID, EISAID ("PNP0A03"))

       //
       // Declare the segment number of this root complex. Most systems only
       // have one segment, which is numbered 0.
       // Section 6.5.6
       //
       Name (_SEG, 0)

       //
       // Declare the base bus number, which is the bus number of the root
       // bus in this root complex. This is usually 0, but need not be.
       // For root complexes supporting multiple root busses, this should
       // be the lowest numbered root bus.
       // Section 6.5.5
       //
       Name (_BBN, 0)

       //
       // The _UID value provides a way of uniquely identifying a device
       // in the case where more than one instance of a specific device
       // is implemented with the same _HID/_CID. For systems with a
       // single root complex, this is usually just 0. For systems with
       // multiple root complexes, this should be different for each
       // root complex.
       // Section 6.1.12
       //
       Name (_UID, 0x00)

       // Cache coherency ON
       Name (_CCA, ONE)

         //SMMU_IIDR mapping
         OperationRegion (IIDR, SystemMemory, 0x830000000018ULL, 4)
         Field (IIDR, DWordAcc, NoLock, Preserve) {
          RSVD, 20,
          SOCI,  8
         }
           //
         // Declare the resources assigned to this root complex.
         // Section 6.2.2
         //
         Method (_PRS, 0, Serialized) {
             Local0 = ResourceTemplate () {
                         //
                         // Declare the range of bus numbers assigned to this root
                         // complex. In this example, the minimum bus number will be
                         // 0, the maximum bus number will be 0xFF, supporting
                         // 256 busses total.
                         // Section 19.5.141
                         //
                         WORDBusNumber (          // Bus number resource (0) the bridge produces bus numbers for its subsequent buses
                             ResourceProducer,      // bit 0 of general flags is 1
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is fixed
                             PosDecode,             // PosDecode
                             0x0000,                // Granularity
                             0x0000,                // Min
                             0x00FF,                // Max
                             0x0000,                // Translation
                             0x0100                 // Range Length = Max-Min+1
                             )

                         //
                         // Declare physical address ranges available to be assigned
                         // to PCIe BARs and Bridge/Root Port memory windows.
                         //
                         // To support 32-bit OSes and/or PCIe devices which only
                         // implement 32-bit BARs, firmware should supply at least
                         // one physical address ranged below 4GB with a size of at
                         // least 16MB (ideally larger).
                         //
                         // On systems supporting 64-bit addressing, to better
                         // support 64-bit OSes and PCIe devices which
                         // could benefit from availability of a very large address
                         // range, firmware should also supply at least one
                         // physical address range above 4GB with a size of at least
                         // 4GB.
                         //

                         //
                         // Declare the memory range >4GB to be used for BAR memory
                         // windows. This example declares a 16GB region starting at
                         // 0x8000000000.
                         // Section 19.5.102
                         //
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             Cacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000878000000000,    // Min
                             0x000087800FFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000010000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             MCFG                   // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000802000000000,    // Min
                             0x000080F0008FFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x000000D000900000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A0                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000828000000000,    // Min
                             0x00008281001FFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000100200000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A1                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000082C000000000,    // Min
                             0x000082C0001FFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000000200000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A2                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000086D000000000,    // Min
                             0x000086D0000FFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000000100000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A3                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000086E000000000,    // Min
                             0x000086E22FFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000230000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A4                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000087E002000000,    // Min
                             0x000087E027FFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000026000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A5                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000087E03C000000,    // Min
                             0x000087E0525FFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000016600000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A6                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000087E087000000,    // Min
                             0x000087E0FCFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000076000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A7                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000087E100000000,    // Min
                             0x000087E10BFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x000000000C000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A8                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x000087E080000000,    // Min
                             0x000087E080FFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000001000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             MCS                    // DescriptorName
                             )
                     }
             // If not CN10KB, set length to zero
             CreateQWordField (Local0, MCS._MAX, MAX)
             CreateQWordField (Local0, MCS._LEN, LEN)
             If (SOCI != CN10K_CPU_TYPE_103XX) {
              MAX = 0x000087E080000000
              LEN = 0
             }

             Return (Local0);
         }

         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878000000000)
         }
         Method (_CRS, 0, NotSerialized) {
           Return (_PRS ())
         }
         Method (_STA, 0, NotSerialized) {
           Return (0xF)
         }
         Method (_INI, 0, NotSerialized) {
         }

         //
         // Declare an _OSC (OS Control Handoff) method which takes 4 arguments.
         //
         // Argments:
         //   Arg0  A Buffer containing a UUID
         //   Arg1  An Integer containing a Revision ID of the buffer format
         //   Arg2  An Integer containing a count of entries in Arg3
         //   Arg3  A Buffer containing a list of DWORD capabilities
         // Return Value:
         //   A Buffer containing a list of capabilities
         // See the APCI spec, Section 6.2.10,
         // and the PCI FW spec, Section 4.5.
         //
         // The following is an example, and may need modification for
         // specific implementations.
         //
         Method (_OSC, 4, NotSerialized)
         {
             /* PCIe _OSC Capabilities */
             If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")) {
                 /* No support for any */
                 CreateDWordField (Arg3, 8, CDW3)
                 Store (Zero, CDW3)
                 Or (CDW3, 0x10, CDW3)
                 Return (Arg3)
             } Else {
                 CreateDWordField (Arg3, Zero, CDW1)
                 /* Set invalid UUID error bit */
                 Or (CDW1, 0x04, CDW1)
                 Return (Arg3)
             }
         }
     }

     Device (PCI1) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 1)
         Name (_BBN, 0)
         Name (_UID, 0x01)
         Name (_CCA, ONE)

         Method (_PRS, 0, NotSerialized) {
             Local0 = ResourceTemplate () {
                         WORDBusNumber (          // Bus number resource (0) the bridge produces bus numbers for its subsequent buses
                             ResourceProducer,      // bit 0 of general flags is 1
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is fixed
                             PosDecode,             // PosDecode
                             0x0000,                // Granularity
                             0x0000,                // Min
                             0x00FF,                // Max
                             0x0000,                // Translation
                             0x0100                 // Range Length = Max-Min+1
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000878010000000,    // Min
                             0x000087801FFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000010000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             MCFG                   // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000868000000000,    // Min
                             0x0000869FFFFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000002000000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A0                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO PEM0-BAR0
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x00008e0000000000,    // Min
                             0x00008e003FFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000040000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A1                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO PEM0-BAR4
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x00008e0f00000000,    // Min
                             0x00008e0f000fffff,    // Max
                             0x0000000000000000,    // translation
                             0x0000000000100000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A2                     // DescriptorName
                             )
                    }
             Return (Local0);
         }

         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878010000000)
         }
         Method (_CRS, 0, NotSerialized) {
           Return (_PRS ())
         }
         Method (_STA, 0, NotSerialized) {
           Return (0xF)
         }
         Method (_INI, 0, NotSerialized) {
         }

         Method (_OSC, 4, NotSerialized)
         {
             /* PCIe _OSC Capabilities */
             If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")) {
                 /* No support for any */
                 CreateDWordField (Arg3, 8, CDW3)
                 Store (Zero, CDW3)
                 Or (CDW3, 0x10, CDW3)
                 Return (Arg3)
             } Else {
                 CreateDWordField (Arg3, Zero, CDW1)
                 /* Set invalid UUID error bit */
                 Or (CDW1, 0x04, CDW1)
                 Return (Arg3)
             }
         }
     }

     Device (PCI2) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 2)
         Name (_BBN, 0)
         Name (_UID, 0x02)
         Name (_CCA, ONE)

        // Operation region to read enhanced allocation capabilities
        OperationRegion (EALL, SystemMemory, 0x0000878020100000, 0x400000)

         Method (_PRS, 0, NotSerialized) {
             Local0 = ResourceTemplate () {
                         WORDBusNumber (          // Bus number resource (0) the bridge produces bus numbers for its subsequent buses
                             ResourceProducer,      // bit 0 of general flags is 1
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is fixed
                             PosDecode,             // PosDecode
                             0x0000,                // Granularity
                             0x0000,                // Min
                             0x00FF,                // Max
                             0x0000,                // Translation
                             0x0100                 // Range Length = Max-Min+1
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000878020000000,    // Min
                             0x000087802FFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000010000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             MCFG                   // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000840000000000,    // Min
                             0x000085F4FFFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x000001F500000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A0                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x0000000001000000,    // Min
                             0x00000000037FFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000002800000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A1                     // DescriptorName
                             )
                         QWordMemory (              // Descriptor for 64-bit MMIO
                             ResourceProducer,      // bit 0 of general flags is 0
                             PosDecode,
                             MinFixed,              // Range is fixed
                             MaxFixed,              // Range is Fixed
                             NonCacheable,
                             ReadWrite,
                             0x00000000,            // Granularity
                             0x00000000FC000000,    // Min
                             0x00000000FEFFFFFF,    // Max
                             0x0000000000000000,    // translation
                             0x0000000003000000,    // Length
                             ,                      // ResourceSourceIndex
                             ,                      // ResourceSource
                             A2                     // DescriptorName
                             )
                             QWordMemory (           // Descriptor for 64-bit MMIO
                              ResourceProducer,      // bit 0 of general flags is 0
                              PosDecode,
                              MinFixed,              // Range is fixed
                              MaxFixed,              // Range is Fixed
                              NonCacheable,
                              ReadWrite,
                              0x00000000,            // Granularity
                              0x000086E080000000,    // Min
                              0x000086E0C047FFFF,    // Max
                              0x0000000000000000,    // translation
                              0x0000000040480000,    // Length
                              ,                      // ResourceSourceIndex
                              ,                      // ResourceSource
                              SDP                    // DescriptorName
                              )
                      }

            // Enhanced allocation capabilities entry 2 to get BAR4
            // Each entry 4 * 32-bits
            //
            Field (EALL, QwordAcc, NoLock, Preserve) {
              Offset (0xC4),
              FRST, 32,                     // First DW
              BLOW, 32,                     // Base low 32-bits
              MLOW, 32,                     // Max low 32-bits
              BHII, 32,                     // Base high 32-bits
              MHII, 32,                     // Max high 32-bits
            }

            CreateQWordField (Local0, A2._MIN, MCFS)
            CreateQWordField (Local0, A2._MAX, MCFE)
            CreateQWordField (Local0, A2._LEN, MCFL)

            MCFS = (BLOW & ~0x3) + (BHII << 32);
            MCFE = (MCFS + MCFL) - 1;

             Return (Local0);
         }


         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878020000000)
         }
         Method (_CRS, 0, NotSerialized) {
           Return (_PRS ())
         }
         Method (_STA, 0, NotSerialized) {
           Return (0xF)
         }
         Method (_INI, 0, NotSerialized) {
         }

         Method (_OSC, 4, NotSerialized)
         {
             /* PCIe _OSC Capabilities */
             If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")) {
                 /* No support for any */
                 CreateDWordField (Arg3, 8, CDW3)
                 Store (Zero, CDW3)
                 Or (CDW3, 0x10, CDW3)
                 Return (Arg3)
             } Else {
                 CreateDWordField (Arg3, Zero, CDW1)
                 /* Set invalid UUID error bit */
                 Or (CDW1, 0x04, CDW1)
                 Return (Arg3)
             }
         }
     }

     Device (PEM0) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 3)
         Name (_UID, 0x03)
         Name (_BBN, 0)
         Name (_CCA, ONE)

         Name (IO0T, 0x0000886000000000)
         Name (M0T,  0x0000882000000000)
         Name (M1T,  0x0000884000000000)
         Name (M2S,  0x00008E0E00000000)
         Name (M2E,  0x00008E0E000FFFFF)

          // INTx are hardcoded by secure firmware
        Name (_PRT, Package () {
          Package () { 0x0FFFF, 0, 0, 41 },   // INTA (9 + 32)
          Package () { 0x0FFFF, 1, 0, 42 },   // INTB
          Package () { 0x0FFFF, 2, 0, 43 },   // INTC
          Package () { 0x0FFFF, 3, 0, 44 },   // INTD
        })

         //Base address of PEM0 ECAM area
         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878030000000)
         }

         Include ("Pem.asl")
     }

     Device (PEM1) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 4)
         Name (_UID, 0x04)
         Name (_BBN, 0)
         Name (_CCA, ONE)

         Name (IO0T, 0x000088E000000000)
         Name (M0T,  0x000088A000000000)
         Name (M1T,  0x000088C000000000)
         Name (M2S,  0x00008E1E00000000)
         Name (M2E,  0x00008E1E000FFFFF)

          // INTx are hardcoded by secure firmware
        Name (_PRT, Package () {
          Package () { 0x0FFFF, 0, 0, 45 },   // INTA (13 + 32)
          Package () { 0x0FFFF, 1, 0, 46 },   // INTB
          Package () { 0x0FFFF, 2, 0, 47 },   // INTC
          Package () { 0x0FFFF, 3, 0, 48 },   // INTD
        })

         //Base address of PEM1 ECAM area
         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878040000000)
         }

         Include ("Pem.asl")
     }

     Device (PEM2) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 5)
         Name (_UID, 0x05)
         Name (_BBN, 0)
         Name (_CCA, ONE)

         Name (IO0T, 0x0000896000000000)
         Name (M0T,  0x0000892000000000)
         Name (M1T,  0x0000894000000000)
         Name (M2S,  0x00008E2E00000000)
         Name (M2E,  0x00008E2E000FFFFF)

          // INTx are hardcoded by secure firmware
        Name (_PRT, Package () {
          Package () { 0x0FFFF, 0, 0, 49 },   // INTA (17 + 32)
          Package () { 0x0FFFF, 1, 0, 50 },   // INTB
          Package () { 0x0FFFF, 2, 0, 51 },   // INTC
          Package () { 0x0FFFF, 3, 0, 52 },   // INTD
        })

         //Base address of PEM2 ECAM area
         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878050000000)
         }

         Include ("Pem.asl")
     }

     Device (PEM3) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 6)
         Name (_UID, 0x06)
         Name (_BBN, 0)
         Name (_CCA, ONE)

         Name (IO0T, 0x000089E000000000)
         Name (M0T,  0x000089A000000000)
         Name (M1T,  0x000089C000000000)
         Name (M2S,  0x00008E3E00000000)
         Name (M2E,  0x00008E3E000FFFFF)

          // INTx are hardcoded by secure firmware
        Name (_PRT, Package () {
          Package () { 0x0FFFF, 0, 0, 53 },   // INTA (21 + 32)
          Package () { 0x0FFFF, 1, 0, 54 },   // INTB
          Package () { 0x0FFFF, 2, 0, 55 },   // INTC
          Package () { 0x0FFFF, 3, 0, 56 },   // INTD
        })

         //Base address of PEM3 ECAM area
         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878060000000)
         }

         Include ("Pem.asl")
     }

     Device (PEM4) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 7)
         Name (_UID, 0x07)
         Name (_BBN, 0)
         Name (_CCA, ONE)

         Name (IO0T, 0x00008A6000000000)
         Name (M0T,  0x00008A2000000000)
         Name (M1T,  0x00008A4000000000)
         Name (M2S,  0x00008E4E00000000)
         Name (M2E,  0x00008E4E000FFFFF)

          // INTx are hardcoded by secure firmware
        Name (_PRT, Package () {
          Package () { 0x0FFFF, 0, 0, 57 },   // INTA (25 + 32)
          Package () { 0x0FFFF, 1, 0, 58 },   // INTB
          Package () { 0x0FFFF, 2, 0, 59 },   // INTC
          Package () { 0x0FFFF, 3, 0, 60 },   // INTD
        })

         //Base address of PEM0 ECAM area
         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878070000000)
         }

         Include ("Pem.asl")
     }

     Device (PEM5) {
         Name (_HID, EISAID ("PNP0A08"))
         Name (_CID, EISAID ("PNP0A03"))
         Name (_SEG, 8)
         Name (_UID, 0x08)
         Name (_BBN, 0)
         Name (_CCA, ONE)

         Name (IO0T, 0x00008AE000000000)
         Name (M0T,  0x00008AA000000000)
         Name (M1T,  0x00008AC000000000)
         Name (M2S,  0x00008E5E00000000)
         Name (M2E,  0x00008E5E000FFFFF)

          // INTx are hardcoded by secure firmware
        Name (_PRT, Package () {
          Package () { 0x0FFFF, 0, 0, 61 },   // INTA (29 + 32)
          Package () { 0x0FFFF, 1, 0, 62 },   // INTB
          Package () { 0x0FFFF, 2, 0, 63 },   // INTC
          Package () { 0x0FFFF, 3, 0, 64 },   // INTD
        })

         //Base address of PEM1 ECAM area
         Method (_CBA, 0, NotSerialized) {
           Return (0x0000878080000000)
         }

         Include ("Pem.asl")
     }

     // UART SBSA
     Device (COM0) {
         Name (_HID, "ARMH0011")
         Name (_UID, 0)

         Name (_CRS, ResourceTemplate() {
                 QWordMemory (ResourceConsumer,
                               PosDecode,
                               MinFixed,
                               MaxFixed,
                               NonCacheable,
                               ReadWrite,
                               0,
                               0x87E028000000,
                               0x87E028000FFF,
                               0,
                               0x1000)
                 Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x20 }
                 })
     }
     Device (COM1) {
         Name (_HID, "ARMH0011")
         Name (_UID, 1)

         Name (_CRS, ResourceTemplate() {
                 QWordMemory (ResourceConsumer,
                               PosDecode,
                               MinFixed,
                               MaxFixed,
                               NonCacheable,
                               ReadWrite,
                               0,
                               0x87E029000000,
                               0x87E029000FFF,
                               0,
                               0x1000)
                 Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x21 }
                 })
     }

      /* Although SPI controllers are available on PCI bus, define
         corresponding ACPI entries within _SB scope so that kernel
         platform driver (spi-cadence-xspi.c) is able to find ACPI companion.
         PCI configuration: VendorId:DeviceId 177d:a09b
         SPI0: PCC_DEV_CON_E : 0x30 -> Dev = bits <7:3> = 6, Fun = bits <2:0> = 0
         SPI1: PCC_DEV_CON_E : 0x38 -> Dev = bits <7:3> = 7, Fun = bits <2:0> = 0
      */
      Device (SPI0) { // SPI/MPI - Serial/Multiprocessor Peripheral Interface
        // Handler: linux/drivers/spi/spi-cadence-xspi.c
        Name (_HID, "PRP0001")          // ACPI_DT_NAMESPACE_HID
        Name (_UID, 0)
        Name (_DDN, "SPI controller 0") // Optional
        Name (_CCA, ONE) // Cache-coherent: dma-coherent in /dts/cn10ka-common-linux.dtsi

        Method (_STA) {Return (0xF)}

        Name (_CRS, ResourceTemplate() {

          // io - last register accessed by kernel driver (spi-cadence-xspi.c) is
          // SPI(0)_RF_MINICTRL_REGS_DLL_PHY_CTRL (0x1034) which is 4 bytes, so _LEN = 0x1038
          QWordMemory ( ResourceConsumer,// ResourceUsage
                        PosDecode,       // Decode
                        MinFixed,        // MinType
                        MaxFixed,        // MaxType
                        NonCacheable,    // MemType
                        ReadWrite,       // ReadWriteType
                        0,               // AddressGranularity
                        0x804000000000,  // MinAddress (HRM SPI(0)_PF_BAR0)
                        0x804000001037,  // MaxAddress
                        0,               // AddressTranslation
                        0x1038)          // AddressLength

          // sdma - direct access via SPI(0)_DIRECT_ACCESS(0)
          QWordMemory ( ResourceConsumer,// ResourceUsage
                        PosDecode,       // Decode
                        MinFixed,        // MinType
                        MaxFixed,        // MaxType
                        NonCacheable,    // MemType
                        ReadWrite,       // ReadWriteType
                        0,               // AddressGranularity
                        0x804010000000,  // MinAddress (HRM SPI(0)_DIRECT_ACCESS(0))
                        0x804010000007,  // MaxAddress
                        0,               // AddressTranslation
                        0x8)             // AddressLength (SPI(0)_DIRECT_ACCESS(0) is 8 byte)

          // aux - first register accessed by kernel driver (spi-cadence-xspi.c) is
          // SPI(0)_PHY_DATASLICE_RFILE_PHY_DQ_TIMING (0x2000). Last accessed register is
          // SPI(0)_CLK_CTR (0x4020) which is 8 bytes, so _LEN = 0x2028
          QWordMemory ( ResourceConsumer,// ResourceUsage
                        PosDecode,       // Decode
                        MinFixed,        // MinType
                        MaxFixed,        // MaxType
                        NonCacheable,    // MemType
                        ReadWrite,       // ReadWriteType
                        0,               // AddressGranularity
                        0x804000002000,  // MinAddress SPI(0)_PHY_DATASLICE_RFILE_PHY_DQ_TIMING
                        0x804000004027,  // MaxAddress SPI(0)_CLK_CTR
                        0,               // AddressTranslation
                        0x2028)          // AddressLength

          // xfer - first register accessed by kernel driver (spi-cadence-xspi.c) is
          // SPI(0)_XFER_FUNC_CTRL_READ_DATA(0) (0x8000). Last accessed register is
          // SPI(0)_XFER_CONST (0x8230) which is 8 bytes, so _LEN = 0x238
          QWordMemory ( ResourceConsumer,// ResourceUsage
            PosDecode,       // Decode
            MinFixed,        // MinType
            MaxFixed,        // MaxType
            NonCacheable,    // MemType
            ReadWrite,       // ReadWriteType
            0,               // AddressGranularity
            0x804000008000,  // MinAddress SPI(0)_XFER_FUNC_CTRL_READ_DATA(0)
            0x804000008237,  // MaxAddress SPI(0)_XFER_CONST
            0,               // AddressTranslation
            0x238)           // AddressLength

          // lock - preset with default values. MIN/MAX and LEN should be updated
          // with values from DT during runtime by AcpiPlatformDxe driver.
          // Values in DT are set by ATF during boot. See SPI_0_LOCK_DATA_BASE.
          QWordMemory ( ResourceConsumer,// ResourceUsage
            PosDecode,       // Decode
            MinFixed,        // MinType
            MaxFixed,        // MaxType
            NonCacheable,    // MemType
            ReadWrite,       // ReadWriteType
            0,               // AddressGranularity
            0x167C000,       // MinAddress SPI_0_LOCK_DATA_BASE
            0x167CFFF,       // MaxAddress SPI_0_LOCK_DATA_BASE + SPI_0_LOCK_DATA_SIZE - 1
            0,               // AddressTranslation
            0x1000)          // AddressLength SPI_0_LOCK_DATA_SIZE

          // See dts/cn10ka-common-linux.dtsi for interrupt number.
          // If not defined, kernel driver (spi-cadence-xspi.c) will switch to pooling mode.
          // NOTE: interrupt number might be updated at runtime by AcpiPlatformDxe driver.
          Interrupt(ResourceConsumer, Edge, ActiveHigh, Exclusive) { 0x7A } // 0x7A = 90 + 32
        })

        Name (_DSD, Package() {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
                // See dts/cn10ka-common-linux.dtsi
                Package () { "compatible", Package () { "marvell,cn10-xspi-nor", "cdns,xspi-nor" }},
                // "reg" used only when CONFIG_SPI_CADENCE_GPIO_WO = 1, see cdns_xspi_of_get_plat_data()
                Package () { "reg", 0x8040},      // see SPI0_BASE spi-cadence-xspi.c
                Package () { "cdns,read-size", 1} // Direct mode: 8 byte access
            }
        })
      } // SPI0

      Device (SPI1) { // SPI/MPI - Serial/Multiprocessor Peripheral Interface
          // Handler: linux/drivers/spi/spi-cadence-xspi.c
          Name (_HID, "PRP0001")          // ACPI_DT_NAMESPACE_HID
          Name (_UID, 1)
          Name (_DDN, "SPI controller 1") // Optional
          Name (_CCA, ONE) // Cache-coherent: dma-coherent in /dts/cn10ka-common-linux.dtsi

          Method (_STA) {Return (0xF)}

          Name (_CRS, ResourceTemplate() {

            // io - last register accessed by kernel driver (spi-cadence-xspi.c) is
            // SPI(1)_RF_MINICTRL_REGS_DLL_PHY_CTRL (0x1034) which is 4 bytes, so _LEN = 0x1038
            QWordMemory ( ResourceConsumer,// ResourceUsage
                          PosDecode,       // Decode
                          MinFixed,        // MinType
                          MaxFixed,        // MaxType
                          NonCacheable,    // MemType
                          ReadWrite,       // ReadWriteType
                          0,               // AddressGranularity
                          0x805000000000,  // MinAddress (HRM SPI(1)_PF_BAR0)
                          0x805000001037,  // MaxAddress
                          0,               // AddressTranslation
                          0x1038)          // AddressLength

            // sdma - direct access via SPI(1)_DIRECT_ACCESS(0)
            QWordMemory ( ResourceConsumer,// ResourceUsage
                          PosDecode,       // Decode
                          MinFixed,        // MinType
                          MaxFixed,        // MaxType
                          NonCacheable,    // MemType
                          ReadWrite,       // ReadWriteType
                          0,               // AddressGranularity
                          0x805010000000,  // MinAddress (HRM SPI(1)_DIRECT_ACCESS(0))
                          0x805010000007,  // MaxAddress
                          0,               // AddressTranslation
                          0x8)             // AddressLength (SPI(1)_DIRECT_ACCESS(0) is 8 byte)

            // aux - first register accessed by kernel driver (spi-cadence-xspi.c) is
            // SPI(1)_PHY_DATASLICE_RFILE_PHY_DQ_TIMING (0x2000). Last accessed register is
            // SPI(1)_CLK_CTR (0x4020) which is 8 bytes, so _LEN = 0x2028
            QWordMemory ( ResourceConsumer,// ResourceUsage
                          PosDecode,       // Decode
                          MinFixed,        // MinType
                          MaxFixed,        // MaxType
                          NonCacheable,    // MemType
                          ReadWrite,       // ReadWriteType
                          0,               // AddressGranularity
                          0x805000002000,  // MinAddress SPI(1)_PHY_DATASLICE_RFILE_PHY_DQ_TIMING
                          0x805000004027,  // MaxAddress SPI(1)_CLK_CTR
                          0,               // AddressTranslation
                          0x2028)          // AddressLength

          // xfer - first register accessed by kernel driver (spi-cadence-xspi.c) is
          // SPI(1)_XFER_FUNC_CTRL_READ_DATA(0) (0x8000). Last accessed register is
          // SPI(1)_XFER_CONST (0x8230) which is 8 bytes, so _LEN = 0x238
          QWordMemory ( ResourceConsumer,// ResourceUsage
            PosDecode,       // Decode
            MinFixed,        // MinType
            MaxFixed,        // MaxType
            NonCacheable,    // MemType
            ReadWrite,       // ReadWriteType
            0,               // AddressGranularity
            0x805000008000,  // MinAddress SPI(1)_XFER_FUNC_CTRL_READ_DATA(0)
            0x805000008237,  // MaxAddress SPI(1)_XFER_CONST
            0,               // AddressTranslation
            0x238)           // AddressLength

          // lock - preset with default values. MIN/MAX and LEN should be updated
          // with values from DT during runtime by AcpiPlatformDxe driver.
          // Values in DT are set by ATF during boot. See SPI_1_LOCK_DATA_BASE.
          QWordMemory ( ResourceConsumer,// ResourceUsage
            PosDecode,       // Decode
            MinFixed,        // MinType
            MaxFixed,        // MaxType
            NonCacheable,    // MemType
            ReadWrite,       // ReadWriteType
            0,               // AddressGranularity
            0x167D000,       // MinAddress SPI_1_LOCK_DATA_BASE
            0x167DFFF,       // MaxAddress SPI_1_LOCK_DATA_BASE + SPI_1_LOCK_DATA_SIZE - 1
            0,               // AddressTranslation
            0x1000)          // AddressLength SPI_1_LOCK_DATA_SIZE

            // See dts/cn10ka-common-linux.dtsi for interrupt number.
            // If not defined, kernel driver (spi-cadence-xspi.c) will switch to pooling mode.
            // NOTE: interrupt number might be updated at runtime by AcpiPlatformDxe driver.
            Interrupt(ResourceConsumer, Edge, ActiveHigh, Exclusive) { 0x7B } // 0x7B = 91 + 32
          })

          Name (_DSD, Package() {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
                  // See dts/cn10ka-common-linux.dtsi
                  Package () { "compatible", Package () { "marvell,cn10-xspi-nor", "cdns,xspi-nor" }},
                  Package () { "reg", 0x8050},      // see SPI1_BASE spi-cadence-xspi.c
                  Package () { "cdns,read-size", 1} // Direct mode: 8 byte access
              }
          })
      } // SPI1

      /* Although EMMC controller is available on PCI bus, define
         corresponding ACPI entries within _SB scope so that kernel
         platform driver (sdhci-cadence.c) is able to find ACPI companion.
         PCI configuration: VendorId:DeviceId 177d:a095
         EMMC: PCC_DEV_CON_E : 0x58 -> Dev = bits <7:3> = B, Fun = bits <2:0> = 0
      */
      Device (EMMC) { // eMMC interface unit
        // Handler: linux/drivers/mmc/host/sdhci-cadence.c
        Name (_HID, "PRP0001")              // ACPI_DT_NAMESPACE_HID
        Name (_DDN, "eMMC interface unit")  // Optional
        Name (_CCA, ONE) // Cache-coherent: dma-coherent in /dts/cn10ka-common-linux.dtsi

        Method (_STA) {Return (0xF)}

        Name (_CRS, ResourceTemplate() {

          // Reference: see dts/cn10ka-common-linux.dtsi and reg defined for mmc.
          // Last register accessed by kernel driver (sdhci-cadence.c) is
          // EMMC(0)_PHY_CTB_RFILE_PHY_GPIO_CTRL_0 (0x2088) which is 4 bytes, so _LEN = 0x208C

          QWordMemory ( ResourceConsumer,// ResourceUsage
            PosDecode,       // Decode
            MinFixed,        // MinType
            MaxFixed,        // MaxType
            NonCacheable,    // MemType
            ReadWrite,       // ReadWriteType
            0,               // AddressGranularity
            0x824000000000,  // MinAddress (HRM EMMC(0)_PF_BAR0)
            0x82400000208B,  // MaxAddress
            0,               // AddressTranslation
            0x208C)          // AddressLength

          // See dts/cn10ka-common-linux.dtsi for interrupt number.
          // Mandatory: see sdhci_pltfm_init() called by sdhci-cadence.c
          Interrupt(ResourceConsumer, Edge, ActiveHigh, Exclusive) { 0x61 } // 0x61 = 65 + 32

          ClockInput(200, 1, MHz, Fixed,,)
        })

        // As EMMC interface properties varies for different board types,
        // _DSD with properties is defined in SSDT table meant for given board.

      } // MMC

     Scope (PCI0) {
       // Handler: linux/drivers/gpio/gpio-thunderx.c
       Device (GPI0) {
          // VendorId DeviceId: 177D:A00A, 76 general purpose I/O pins
          Name (_ADR, 0x00110000) // Device#1h (17), Func#0 on host bridge PCI0
          Name (_UID, 0)

          Method (_STA) {
              Return(0xF)
          }
       }

       Device (P2P0) {  // PCI-PCI bridge: Bridge for MRML, creating ECAM 0's bus 1.
         // PCC Bridge Vendor and Device ID Register: PCCBR_XXX_ID
         // -> VENDID        = PCC_VENDOR_E::CAVIUM
         // -> DEVID <15:8>  = PCC_PROD_E::GEN = 0xA0
         // -> DEVID <7:0>   = PCC_DEV_IDL_E::PCCBR = 0x02
         // VendorId DeviceId: 177D:A002
         Name (_ADR, 0x00010000) // Device#1h, Func#0 on host bridge PCI0
         Name (_UID, 0)

         Device (MDIO) {// Management Data Input/Output (MDIO) or Serial Management Interface (SMI)
             // VendorId DeviceId: 177D:A02B
             // Handler 5.15: linux/drivers/net/mdio/mdio-thunder.c
             // Handler 5.4:  linux/drivers/net/phy/mdio-thunder.c
             Name (_DDN, "Management Data Input/Output")
             Name (_ADR, 0x00010003)     // Device#1h, Func#3 on bus 1
             Name (_UID, 0)

             Method (_STA, 0, NotSerialized) {
               Return (0xF)
             }

             Method (_INI, 0, Serialized) {
             }

             Device (SMI0) {
               Name (_ADR, 0)
               Name (_UID, 0)
               Name (_CRS, ResourceTemplate() {
                        QWordMemory (ResourceConsumer,
                                     PosDecode,
                                     MinFixed,
                                     MaxFixed,
                                     NonCacheable,
                                     ReadWrite,
                                     0,
                                     0x87E005003800,
                                     0x87E00500382F,
                                     0,
                                     0x30)
               })

               /* // Optional: linux/drivers/net/phy/mdio-thunder.c
               Name (_DSD, Package () {
                 ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                 Package () {
                     //Package () {"clock-freq", 16666666},
                 }
               })*/

               Method (_STA, 0, NotSerialized) {
                   Return (0xF)
               }

               Method (_INI, 0, Serialized) {
               }
             } //SMI0

             Device (SMI1) {
               Name (_ADR, 1)
               Name (_UID, 1)
               Name (_CRS, ResourceTemplate() {
                        QWordMemory (ResourceConsumer,
                                     PosDecode,
                                     MinFixed,
                                     MaxFixed,
                                     NonCacheable,
                                     ReadWrite,
                                     0,
                                     0x87E005003880,
                                     0x87E0050038AF,
                                     0,
                                     0x30)
               })

               Method (_STA, 0, NotSerialized) {
                   Return (0xF)
               }

               Method (_INI, 0, Serialized) {
               }
             } //SMI1
         } // MDIO

         Device (TWI2) {
             // -> VENDID        = PCC_VENDOR_E::CAVIUM
             // -> DEVID <15:8>  = PCC_PROD_E::GEN = 0xA0
             // -> DEVID <7:0>   = PCC_DEV_IDL_E::TWSI = 0x12
             // VendorId DeviceId: 177D:A012
             // 0x177D - Cavium Vendor ID (PCI SIG), 0xA012 - PCI_DEVICE_ID_THUNDER_TWSI
             Name (_DDN, "TWSI Controller 2")
             // _ADR: Named object that evaluates to the interface’s address on its parent bus.
             // _ADR is a standard device configuration control method defined in the ACPI spec.
             // Required only for devices on a bus that has standard enumeration mechanism.
             Name (_ADR, 0x00180002) // Device#18h (24), Func#2 on parent bus (bus 1)
             Name (_UID, "TWSI Controller 2")

             Method (_STA)
             {
                 Return(0xF)
             }
             /*
             // _DSD IMPLEMENTATION GUIDE VERSION 2.0 JAN 2021
             Name (_DSD, Package () {
                 ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                 Package () {  // linux/drivers/i2c/busses/i2c-thunderx-pcidrv.c
                     Package () {"clock-frequency", 100000}, // Optional: 100kHz
                     Package () {"sclk", 800000000},         // Optional
                 }
             })*/
         } //TWI2

         Device (TWI3) {
             Name (_DDN, "TWSI Controller 3")
             Name (_ADR, 0x00180003) // Device#18h (24), Func#3 on bus (bus 1)
             Name (_UID, "TWSI Controller 3")

             Method (_STA)
             {
                 Return(0xF)
             }
         } //TWI3

         Device (TWI4) {
             Name (_DDN, "TWSI Controller 4")
             Name (_ADR, 0x00180004) // Device#18h (24), Func#4 on bus (bus 1)
             Name (_UID, "TWSI Controller 4")

             Method (_STA)
             {
                 Return(0xF)
             }
         } //TWI4

        // Only EBB106 has child devices attached to TWSI controller 5.
        // Thus define TWI5 controller under Ssdt-ebb106.asl, so it's available only for EBB106.
        // asim          exposes none TWSI    devices to kernel
        // ebb102/ebb103 exposes TWSI 1/2     devices to kernel
        // ebb106        exposes TWSI 2/3/4/5 devices to kernel
        // ebb106        exposes TWSI 2/3/4   devices to kernel
        // crb106        exposes none TWSI    devices to kernel
        // Considering above some controllers (e.g. TWSI 3 for ebb102/ebb103) might not
        // have any child devices probed in kernel.

       } // P2P0
     } // Scope (PCI0)
   }   // _SB

 }     // DefinitionBlock
