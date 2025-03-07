// /** @file
// Acpi LINK.ASL
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/

// Use this information when determining the Possible IRQs that can be
// used in a given system.
//
// The following IRQs are always in use by legacy devices:
//              0  = System Timer
//              2  = 8259 PIC
//              8  = RTC
//              9  = SCI Interrupt (It may be used, we choose not to)
//              13 = Co-processor Error
//
// The following may be in use by legacy devices:
//              1  = If using PS/2 Keyboard
//              3  = If COMx Port Enabled and IRQ = 3
//              4  = If COMx Port Enabled and IRQ = 4
//              5  = If LPT Port Enabled and IRQ = 5
//              6  = If FDC Enabled
//              7  = If LPT Port Enabled and IRQ = 7
//              12 = If using PS/2 Mouse
//              14 = Primary IDE (If populated and in Compatibility Mode)
//              15 = Secondary IDE (If populated and in Compatibility Mode)
//
// The following will never be in use by legacy devices:
//              10 = Assign to PARC, PCRC, PERC, PGRC
//              11 = Assign to PBRC, PDRC, PFRC, PHRC

Device(LNKA)                       // PARC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))    // PCI Interrupt Link Device
  Name(_UID, 1)                    // Unique to other Link Devices

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PARC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSA)
  }

  // Current IRQ Resource Setting.
  Method(_CRS, 0, Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PARC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PARC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PARC, LNotEqual(PARC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}

Device(LNKB)                            // PBRC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))
  Name(_UID, 2)

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PBRC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSB)
  }

  // Current IRQ Resource Setting.
  Method(_CRS, 0, Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PBRC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PBRC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PBRC, LNotEqual(PBRC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}

Device(LNKC)                            // PCRC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))
  Name(_UID, 3)

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PCRC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSA)
  }

  // Current IRQ Resource Setting.
  Method(_CRS, 0, Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PCRC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PCRC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PCRC, LNotEqual(PCRC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}

Device(LNKD)                            // PDRC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))
  Name(_UID, 4)

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PDRC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSD)
  }

  // Current IRQ Resource Setting.
  Method(_CRS, 0, Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PDRC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PDRC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PDRC, LNotEqual(PDRC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}

Device(LNKE)                            // PERC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))
  Name(_UID, 5)

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PERC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSE)
  }

  // Current IRQ Resource Setting.
  Method(_CRS,0,Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PERC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PERC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PERC, LNotEqual(PERC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}

Device(LNKF)                            // PFRC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))
  Name(_UID, 6)

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PFRC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSF)
  }

  // Current IRQ Resource Setting.
  Method(_CRS, 0, Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PFRC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PFRC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PFRC, LNotEqual(PFRC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}

Device(LNKG)                            // PGRC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))
  Name(_UID, 7)

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PGRC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSG)
  }

  // Current IRQ Resource Setting.
  Method(_CRS, 0, Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PGRC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PGRC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PGRC, LNotEqual(PGRC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}

Device(LNKH)                            // PHRC Routing Resource
{
  Name(_HID, EISAID("PNP0C0F"))
  Name(_UID, 8)

  // Disable the PCI IRQ.
  Method(_DIS, 0, Serialized)
  {
    Store(0x1F, PHRC)
  }

  // Possible IRQ Resource Setting.
  Method (_PRS, 0, Serialized)
  {
    return (PRSH)
  }

  // Current IRQ Resource Setting.
  Method(_CRS, 0, Serialized)
  {
    Name(RTLA, ResourceTemplate()
    {
      IRQ(Level, ActiveLow, Shared) {}
    })

    // Point to specific byte.
    CreateWordField(RTLA, 1, IRQ0)
    // Zero out IRQ mask bits 0-15
    Store(Zero, IRQ0)
    ShiftLeft(1, PHRC, IRQ0)
    Return(RTLA)
  }

  // Set IRQ Resource Setting.
  Method(_SRS, 1, Serialized)
  {
    // Point to the specific byte passed in
    CreateWordField(Arg0, 1, IRQ0)
    // Determine the IRQ bit to set and store it
    FindSetRightBit(IRQ0, Local0)
    Decrement(Local0)
    Store(Local0, PHRC)
  }

  // PCI IRQ Status.
  Method(_STA, 0, Serialized)
  {
    If(LAnd(PHRC, LNotEqual(PHRC, 0x1F)))
    {
      Return(0x000B)
    } Else {
      Return(0x0009)
    }
  }
}
