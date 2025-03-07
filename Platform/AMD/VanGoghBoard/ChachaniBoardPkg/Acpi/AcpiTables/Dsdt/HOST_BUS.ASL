// /** @file
// Acpi HOST_BUS.ASL
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/


//
// BUS, I/O, and MMIO resources
//
Method(_CRS, 0, Serialized)
{
  CreateDWordField(RES0, ^PM01._MIN, PMMN)
  CreateDWordField(RES0, ^PM01._MAX, PMMX)
  CreateDWordField(RES0, ^PM01._LEN, PMLN)

  Store(TOPM, PMMN)
  Subtract(PCBA, 1, PMMX)
  Subtract(PCBA, PMMN, PMLN)

  Return(RES0)
}

Name( RES0,ResourceTemplate()
{
  // Bus
  WORDBusNumber (          // Bus number resource (0); the bridge produces bus numbers for its subsequent buses
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

  // IO
  IO (Decode16, 0x70, 0x77, 0x01, 0x08)         //Consumed resource (0xCF8-0xCFF)
  IO (Decode16, 0xCF8, 0xCF8, 0x01, 0x08)       //Consumed resource (0xCF8-0xCFF)

  WORDIO (                 // Consumed-and-produced resource (all I/O below CF8)
    ResourceProducer,      // bit 0 of general flags is 0
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    PosDecode,
    EntireRange,
    0x0000,                // Granularity
    0x0000,                // Min
    0x006F,                // Max
    0x0000,                // Translation
    0x0070                 // Range Length
  )

  WORDIO (                 // Consumed-and-produced resource
    ResourceProducer,      // bit 0 of general flags is 0
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    PosDecode,
    EntireRange,
    0x0000,                // Granularity
    0x0078,                // Min
    0x0CF7,                // Max
    0x0000,                // Translation
    0x0C80                 // Range Length
  )

  WORDIO (                 // Consumed-and-produced resource (all I/O above CFF)
    ResourceProducer,      // bit 0 of general flags is 0
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    PosDecode,
    EntireRange,
    0x0000,                // Granularity
    0x0D00,                // Min
    0xFFFF,                // Max
    0x0000,                // Translation
    0xF300                 // Range Length
  )

  // MMIO
  DWORDMEMORY (            // Descriptor for legacy VGA video RAM
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x000A0000,            // Min
    0x000BFFFF,            // Max
    0x00000000,            // Translation
    0x00020000             // Range Length
  )

  DWORDMEMORY (            // Descriptor for legacy OptionRom
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x000C0000,            // Min
    0x000DFFFF,            // Max
    0x00000000,            // Translation
    0x00020000             // Range Length
  )

  DWORDMEMORY (            // Descriptor for BIOS Area
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x000E0000,            // Min
    0x000FFFFF,            // Max
    0x00000000,            // Translation
    0x00020000             // Range Length
  )

  DWORDMEMORY (            // Descriptor for PCI MMIO
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x80000000,            // Min
    0xDFFFFFFF,            // Max
    0x00000000,            // Translation
    0x60000000             // Range Length
    ,,,
    PM01
  )
})

//Name(GUID,UUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))
Name(GUID, Buffer()
{
  0x5b, 0x4d, 0xdb, 0x33,
  0xf7, 0x1f,
  0x1c, 0x40,
  0x96, 0x57,
  0x74, 0x41, 0xc0, 0x3d, 0xd7, 0x66
})


Name(SUPP, 0)    // PCI _OSC Support Field value
Name(CTRL, 0)    // PCI _OSC Control Field value

Method(_OSC, 4, Serialized)
{
  // Check for proper UUID
  // Save the capabilities buffer
  Store(Arg3, Local0)

  // Create DWord-adressable fields from the Capabilties Buffer
  CreateDWordField(Local0, 0, CDW1)
  CreateDWordField(Local0, 4, CDW2)
  CreateDWordField(Local0, 8, CDW3)

  // Check for proper UUID
  If(LEqual(Arg0, GUID))
  {
    // Save Capabilities DWord2 & 3
    Store(CDW2, SUPP)
    Store(CDW3, CTRL)

    If(Not(And(CDW1, 1)))    // Query flag clear?
    {
      // Disable GPEs for features granted native control.
      If(And(CTRL, 0x02))
      {
        NHPG()
      }
      If(And(CTRL, 0x04))      // PME control granted?
      {
        NPME()
      }
    }

    If(LNotEqual(Arg1, One))
    {
      // Unknown revision
      Or(CDW1, 0x08, CDW1)
    }

    If(LNotEqual(CDW3, CTRL))
    {
      // Capabilities bits were masked
      Or(CDW1, 0x10, CDW1)
    }
    // Update DWORD3 in the buffer
    And(CTRL, 0xFE, CTRL)
    Store(CTRL, CDW3)
    Return(Local0)
  } Else  {
    Or(CDW1, 4, CDW1)         // Unrecognized UUID
    Return(Local0)
  }
} // End _OSC
