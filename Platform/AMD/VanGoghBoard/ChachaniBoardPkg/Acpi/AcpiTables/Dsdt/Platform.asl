// /** @file
// Acpi Platform.asl
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/

//
// Create a Global MUTEX.
//
Mutex(MUTX, 0)

// The _PIC Control Method is optional for ACPI design.  It allows the
// OS to inform the ASL code which interrupt controller is being used,
// the 8259 or APIC.  The reference code in this document will address
// PCI IRQ Routing and resource allocation for both cases.
//
// The values passed into _PIC are:
//       0 = 8259
//       1 = IOAPIC
Method(\_PIC, 1)
{
  Store(Arg0, PICM)
}

// Prepare to sleep
Method(_PTS, 1)
{
  // DEBUG INFO
  Or(Arg0, 0x50, Local0) // 5x means enter Sx state
  Store(Local0, \_SB.P80H)

  // Todo for System Specific
}

// System Wake
Method(_WAK, 1)
{
  // Debug Info
  Or(Arg0, 0xE0, Local0) // Ex means exit Sx state
  Store(Local0, \_SB.P80H)

  If (LEqual(ARG0, 0x03)) {
    Notify (\_SB.PWRB, 0x2)
  }
  Return (Package () {0x00, 0x00}) // Should return a Package containing two Integers containing status and the power supply S-state
}

//
// System Bus
//
Scope(\_SB)
{
  Scope(PCI0)
  {
    Method(_INI, 0)
    {
      // Check for a specific OS which supports _OSI.
      If(CondRefOf(\_OSI, Local0))
      {
        // Use OSYS for Windows Compatibility.
        If(\_OSI("Windows 2009"))   // Windows 7 or Windows Server 2008 R2
        {
        }
        If(\_OSI("Windows 2012"))   // Windows 8 or Windows Server 2012
        {
        }
        If(\_OSI("Windows 2013"))   //Windows Blue
        {
        }
        If(\_OSI("Windows 2015"))   //Windows 10
        {
        }
        If (\_OSI("Linux"))         //Linux
        {
        }
      }
    }

    Method(NHPG, 0, Serialized)
    {
    }

    Method(NPME, 0, Serialized)
    {
    }
  } // end Scope(PCI0)

  // Power button
  Device(PWRB)
  {
    Name(_HID, EISAID("PNP0C0C"))
    Method(_STA, 0)
    {                                   // return status of device
      Return(0x0B)
    }
  } //End of Device(PWRB)

  // SPI 1 controller that be connected to MCU (connect to PD controller).
  Device(SPI1)
  {
    Name(_HID, "AMDI0062")
    Name(_UID, Zero)

    Name(_CRS, ResourceTemplate()
    {
      Memory32Fixed (ReadWrite, 0xFEC13000, 0x00000100)
      GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0x0000, "\\_SB.GPIO", ,) {91} // AGPIO91 for MCU
    })

    Method (_STA, 0, NotSerialized)
    {
      Return (0x0F)
    }

    // USBC port
    Device(CON0)
    {
      Name(_ADR, Zero)
      Name(_DSD, Package(){
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package(){
          Package(){"usb-role-switch", \_SB.PCI0.GP17.DRD0},
          Package(){"power-role", "dual"},
          Package(){"data-role", "dual"},
        }
      })
    }
  } //End of Device(SPI1)

} // end Scope(\_SB)

Name(PICM, 0)   // Global Name, returns current Interrupt controller mode; updated from _PIC control method
