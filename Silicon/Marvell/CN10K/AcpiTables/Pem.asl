/** @file
 *
 *  CN10K PEM Table
 *
 *  Copyright (c) 2021-2026 Marvell
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *  https://spdx.org/licenses
 *
 **/

Method (_OSC, 4, NotSerialized)
{
    /* PCIe _OSC Capabilities */
    If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")) {
        /* No support for any */
        CreateDWordField (Arg3, 8, CDW3)
        CDW3 = 0x10
        Return (Arg3)
    } Else {
        CreateDWordField (Arg3, Zero, CDW1)
        /* Set invalid UUID error bit */
        CDW1 |= 0x04
        Return (Arg3)
    }
}

Method (_PRS, 0, Serialized) {
    Local7 = ResourceTemplate () {
        WordBusNumber (            // Bus number resource (0) the bridge produces bus numbers for its subsequent buses
            ResourceProducer,      // bit 0 of general flags is 1
            MinFixed,              // Range is fixed
            MaxFixed,              // Range is fixed
            PosDecode,             // PosDecode
            0,                     // Granularity
            0,                     // Min
            0xFF,                  // Max
            0,                     // Translation
            0x100,                     // Range Length = Max-Min+1
            ,
            ,
            BUS0
            )
        QWordMemory (              // Descriptor for 64-bit MMIO                    This is MMIO for ECAM
            ResourceProducer,      // bit 0 of general flags is 0
            PosDecode,
            MinFixed,              // Range is fixed
            MaxFixed,              // Range is Fixed
            NonCacheable,
            ReadWrite,
            0,                     // Granularity
            0x000000000000,        // Min
            0x00000FFFFFFF,        // Max
            0,                     // Translation
            0x000010000000,        // Length
            ,                      // ResourceSourceIndex
            ,                      // ResourceSource
            MCFG,                  // DescriptorName
            AddressRangeMemory,    // Normal memory
            TypeStatic             // Secondary side is memory
            )
        QWordMemory (              // Descriptor for 64-bit MMIO PREF               This is MMIO for PEM space
            ResourceProducer,      // bit 0 of general flags is 0
            PosDecode,
            MinFixed,              // Range is fixed
            MaxFixed,              // Range is Fixed
            NonCacheable,
            ReadWrite,
            0x00000000,            // Granularity
            0x0000000000000000,    // Min
            0x00000000000FFFFF,    // Max
            0,                     // Translation
            0x0000000000100000,    // Length
            ,                      // ResourceSourceIndex
            ,                      // ResourceSource
            MEM2,                  // DescriptorName
            AddressRangeMemory,    // Normal memory
            TypeStatic             // Secondary side is memory
            )
        QWordMemory (              // Descriptor for 64-bit MMIO
            ResourceProducer,      // bit 0 of general flags is 0
            PosDecode,
            MinFixed,              // Range is fixed
            MaxFixed,              // Range is Fixed
            NonCacheable,
            ReadWrite,
            0x0,                   // Granularity
            0x0000000001000000,    // Min
            0x0000000FFFFFFFFF,    // Max
            0xA5A5A5A5A5A5A5A5,    // Translation
            0x0000000FFF000000,    // Length
            ,                      // ResourceSourceIndex
            ,                      // ResourceSource
            MEM0,                  // DescriptorName
            AddressRangeMemory,    // Normal memory
            TypeStatic             // Secondary side is memory
            )
        QWordMemory (              // Descriptor for 64-bit MMIO PREF
            ResourceProducer,      // bit 0 of general flags is 0
            PosDecode,
            MinFixed,              // Range is fixed
            MaxFixed,              // Range is Fixed
            Prefetchable,
            ReadWrite,
            0x00000000,            // Granularity
            0x0000000000000000,    // Min
            0x0000000FFFFFFFFF,    // Max
            0xA5A5A5A5A5A5A5A5,    // Translation
            0x0000001000000000,    // Length
            ,                      // ResourceSourceIndex
            ,                      // ResourceSource
            MEM1,                  // DescriptorName
            AddressRangeMemory,    // Normal memory
            TypeStatic             // Secondary side is memory
            )
        QWordMemory (              // Descriptor for 64-bit MMIO
            ResourceProducer,      // bit 0 of general flags is 0
            PosDecode,
            MinFixed,              // Range is fixed
            MaxFixed,              // Range is Fixed
            Cacheable,
            ReadWrite,
            0,                     // Granularity
            0x0000000000000000,    // Min
            0x00000000000FFFFF,    // Max
            0xA5A5A5A5A5A5A5A5,    // Translation
            0x0000000000100000,    // Length
            ,                      // ResourceSourceIndex
            ,                      // ResourceSource
            IO0,                   // DescriptorName
            AddressRangeMemory,    // Normal memory
            TypeStatic             // Secondary side is memory
            )
    } // _CRS

    CreateQWordField (Local7, MEM0._TRA, M0T)

    M0T  = ^M0T

    CreateQWordField (Local7, MEM1._TRA, M1T)

    M1T  = ^M1T

    CreateQWordField (Local7, IO0._TRA, IO0T)

    IO0T = ^IO0T

    CreateQWordField (Local7, MCFG._MIN, MCFS)
    CreateQWordField (Local7, MCFG._MAX, MCFE)
    CreateQWordField (Local7, MCFG._LEN, MCFL)

    MCFS = ^_CBA()
    MCFE  = MCFS + MCFL - 1

    CreateQWordField (Local7, MEM2._MIN, M2S)
    CreateQWordField (Local7, MEM2._MAX, M2E)
    CreateQWordField (Local7, MEM2._LEN, M2L)

    M2S  = ^M2S
    M2E  = ^M2E
    M2L  = M2E - M2S + 1

    Return (Local7)
}

Method (_CRS, 0, NotSerialized) {
    Return (_PRS ())
}

Method (_INI, 0, NotSerialized) {
}
