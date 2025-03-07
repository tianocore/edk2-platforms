// /** @file
// Acpi Lpc0.asl
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/

External(\_SB.ALIB, MethodObj)
#define CpmReadTable                                    M049
#define CpmMainTable                                    M128
#define CpmEcSupport                                    0x77
External(CpmReadTable, MethodObj)
External(CpmMainTable)

// Define the needed LPC registers used by ASL.

Device(LPC0)
{
  Name(_ADR,0x140003)

  OperationRegion(ILBR, SystemMemory, 0xC00, 0x2)
  Field(ILBR, AnyAcc, NoLock, Preserve) {
    PIDX, 8,
    PDAT, 8
  }
  IndexField(PIDX, PDAT, ByteAcc, NoLock, Preserve) {
    PARC, 8,        // INT A
    PBRC, 8,        // INT B
    PCRC, 8,        // INT C
    PDRC, 8,        // INT D
    PERC, 8,        // INT E
    PFRC, 8,        // INT F
    PGRC, 8,        // INT G
    PHRC, 8,        // INT H
  }

  OperationRegion(DBG0, SystemIO, 0x80, 0x2)
  Field(DBG0, WordAcc, NoLock, Preserve) {
    P80H, 16
  }

  Include ("LINK.ASL")
  //
  // Super I/O
  //
Device(EC0)
{
  Name(_HID,  EISAID("PNP0C09"))  // PNP ID
  Mutex(Z009,0)
  // Name(RHGP, 3)   // To be patched to Name(_GPE, 3) if HW_Reduced_ACPI is false
  Name(_GPE, 5) // GPIO21 to be configured as EC_SCI#, it maps to Gevent#5
  Method(_STA,0,NotSerialized) {
    If (LEqual (CpmReadTable (CpmMainTable, CpmEcSupport), 1))
    {
      Return(0x0F)
    }
    else
    {
      Return(0)
    }
  }
  Name(OKEC, Zero)

  // EC resources
  Method(_CRS, 0, NotSerialized) {
    Name(BUF0,ResourceTemplate() {
      IO (Decode16, 0x662, 0x662, 0x1, 0x1)
      IO (Decode16, 0x666, 0x666, 0x1, 0x1)
    })
    Name(BUF1,ResourceTemplate() {
      IO(Decode16,0x662,0x662,0x1,0x1)
      IO(Decode16,0x666,0x666,0x1,0x1)
      GpioInt (
        Edge,
        ActiveLow,
        ExclusiveAndWake,
        PullUp,
        0,
        "\\_SB.GPIO",
        0x00,
        ResourceConsumer,
        ,
      ) {22}    // GPIO Interrupt Connection resource for LPC_PME_L/GEVENT3
    })

/*
//-    If(LEqual(RDHW, 0x0)) {   // HW_REDUCED_ACPI enabled?
//-      Return(BUF0)
//-    } Else {
//-      Return(BUF1)
//-    }
*/
         Return(BUF0)
  }

  /* ------------------------------------------------------------------------*/
  // Name: _REG - This method is called by drivers to register installation
  //              and removal.  For example; OS calls _REG(3,1) to indicate
  //              to the ACPI BIOS that the Embedded Controller Driver is
  //              present and functional.
  //
  //              ARG0: 0=Memory
  //                    1=I/O
  //                    2=PCI Config
  //                    3=Embedded Controller
  //                    4=SMBus
  //
  //              ARG1: 0=Handler Not Connected
  //                    1=Handler Connected
  //
  Method(_REG, 2) {
    If (LEqual(Arg0, 0x03)) {
      // EC EnableAcpi
      if (LNot(Acquire(\_SB.PCI0.LPC0.EC0.Z009,300))) {
        Store(\_SB.PCI0.LPC0.EC0.STAS, Local0)
        Or(Local0, 0x04, Local1)
        Store(Local1, \_SB.PCI0.LPC0.EC0.STAS)
        Release(\_SB.PCI0.LPC0.EC0.Z009)
      }

      Store(Arg1, OKEC)
    }
  } // end of _REG


Mutex(QEVT,0)

OperationRegion(ERAM, EmbeddedControl, 0, 0xFF)
Field(ERAM, ByteAcc, NoLock, Preserve)
{
  Offset(0xCF),   // Miscellaneous Status and Control
  STAS,8,         // Bit[1] - 1, BIOS enable AC/DC switch
                  //          0, BIOS disable AC/DC switch
                  // Bit[2] - 1, BIOS enable ACPI mode
                  //          0, BIOS disable ACPI mode
}

}// END device EC0

} // End of \_SB.PCI0.LPC0

scope(\_SB)
{
  OperationRegion(ILBR, SystemMemory, 0xC00, 0x2)
  Field(ILBR, AnyAcc, NoLock, Preserve) {
    PIDX, 8,
    PDAT, 8
  }
  IndexField(PIDX, PDAT, ByteAcc, NoLock, Preserve) {
    PARC, 8,        // INT A
    PBRC, 8,        // INT B
    PCRC, 8,        // INT C
    PDRC, 8,        // INT D
    PERC, 8,        // INT E
    PFRC, 8,        // INT F
    PGRC, 8,        // INT G
    PHRC, 8,        // INT H
  }

  OperationRegion(DBG0, SystemIO, 0x80, 0x2)
  Field(DBG0, WordAcc, NoLock, Preserve) {
    P80H, 16
  }

  Include ("LINK.ASL")
}
