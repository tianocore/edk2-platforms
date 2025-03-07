// /** @file
// Acpi PciTree.asl
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/

Scope(\_SB)
{
  //RTC
  Device(RTC)
  {
    Name(_HID, EISAID("PNP0B00"))

    Name(_CRS, ResourceTemplate()  // if HPET enabled
    {
      IO(Decode16, 0x70, 0x70, 0x01, 0x02)
    })

    Method(_STA,0,Serialized) {
      // Report RTC Battery is Prensent or Not Present.
      Return (0xF)
    }
  }

  // Thermal Zone
  Device(THMZ)
  {
    Name (_HID, "AMDI0065")
    Name (_UID, 0x00)

    Method (_STA, 0, NotSerialized)
    {
      Return (0x0F)
    }
  }

  Device(TMZ2)
  {
    Name (_HID, "AMDI0066")
    Name (_UID, 0x00)

    Method (_STA, 0, NotSerialized)
    {
      Return (0x0F)
    }
  }

  Device(TMZ3)
  {
    Name (_HID, "AMDI0067")
    Name (_UID, 0x00)

    Method (_STA, 0, NotSerialized)
    {
      Return (0x0F)
    }
  }

  Device(TMZ4)
  {
    Name (_HID, "AMDI0068")
    Name (_UID, 0x00)

    Method (_STA, 0, NotSerialized)
    {
      Return (0x0F)
    }
  }

  // HPET - High Performance Event Timer
  Device(HPET)
  {
    Name (_HID, EISAID("PNP0103"))
    Name (_UID, 0x00)

    Method (_STA, 0, NotSerialized)
    {
      Return (0x0F)
    }

    Method (_CRS, 0, Serialized)
    {
      Name (RBUF, ResourceTemplate ()
      {
        IRQNoFlags() {0}
        IRQNoFlags() {8}
        Memory32Fixed (ReadOnly,
                       0xFED00000,         // Address Base
                       0x00000400,         // Address Length
                      )
      })
      Return (RBUF)
    }
  }

  Name(PR00, Package() {
    // Device 1 Func 1:7 --GPP [0:6]
    Package(){ 0x0001FFFF, 0, LNKA, 0 },
    Package(){ 0x0001FFFF, 1, LNKB, 0 },
    Package(){ 0x0001FFFF, 2, LNKC, 0 },
    Package(){ 0x0001FFFF, 3, LNKD, 0 },

    // Device 8 Func 1:3 --Internal GPP [0:2]
    Package(){ 0x0008FFFF, 0, LNKE, 0 },
    Package(){ 0x0008FFFF, 1, LNKF, 0 },
    Package(){ 0x0008FFFF, 2, LNKG, 0 },

    // FCH On-Chip
    Package(){ 0x0014FFFF, 0, LNKA, 0 },
    Package(){ 0x0014FFFF, 1, LNKB, 0 },
    Package(){ 0x0014FFFF, 2, LNKC, 0 },
    Package(){ 0x0014FFFF, 3, LNKD, 0 },
  })
  Name(AR00, Package() {
    // Device 1 Func 1:7 --GPP [0:6]
    Package(){ 0x0001FFFF, 0, 0, 16 },
    Package(){ 0x0001FFFF, 1, 0, 17 },
    Package(){ 0x0001FFFF, 2, 0, 18 },
    Package(){ 0x0001FFFF, 3, 0, 19 },

    // Device 8 Func 1:3 --Internal GPP [0:2]
    Package(){ 0x0008FFFF, 0, 0, 20 },
    Package(){ 0x0008FFFF, 1, 0, 21 },
    Package(){ 0x0008FFFF, 2, 0, 22 },

    // FCH On-Chip
    Package(){ 0x0014FFFF, 0, 0, 16 },
    Package(){ 0x0014FFFF, 1, 0, 17 },
    Package(){ 0x0014FFFF, 2, 0, 18 },
    Package(){ 0x0014FFFF, 3, 0, 19 },
  })
  Name(NR00, Package() {
    // Device 1 Func 1:7 --GPP [0:6]
    Package(){ 0x0001FFFF, 0, 0, 32 },
    Package(){ 0x0001FFFF, 1, 0, 33 },
    Package(){ 0x0001FFFF, 2, 0, 34 },
    Package(){ 0x0001FFFF, 3, 0, 35 },

    // Device 8 Func 1:3 --Internal GPP [0:2]
    Package(){ 0x0008FFFF, 0, 0, 36 },
    Package(){ 0x0008FFFF, 1, 0, 37 },
    Package(){ 0x0008FFFF, 2, 0, 38 },

    // FCH On-Chip
    Package(){ 0x0014FFFF, 0, 0, 16 },
    Package(){ 0x0014FFFF, 1, 0, 17 },
    Package(){ 0x0014FFFF, 2, 0, 18 },
    Package(){ 0x0014FFFF, 3, 0, 19 },
  })

  Name(PR01, Package() {
    Package(){ 0x0000FFFF, 0, LNKA, 0 },
    Package(){ 0x0000FFFF, 1, LNKB, 0 },
    Package(){ 0x0000FFFF, 2, LNKC, 0 },
    Package(){ 0x0000FFFF, 3, LNKD, 0 },
  })
  Name(AR01, Package() {
    Package(){ 0x0000FFFF, 0, 0, 16 },
    Package(){ 0x0000FFFF, 1, 0, 17 },
    Package(){ 0x0000FFFF, 2, 0, 18 },
    Package(){ 0x0000FFFF, 3, 0, 19 },
  })
  Name(NR01, Package() {
    Package(){ 0x0000FFFF, 0, 0, 24 },
    Package(){ 0x0000FFFF, 1, 0, 25 },
    Package(){ 0x0000FFFF, 2, 0, 26 },
    Package(){ 0x0000FFFF, 3, 0, 27 },
  })

  Name(PR02, Package() {
    Package(){ 0x0000FFFF, 0, LNKE, 0 },
    Package(){ 0x0000FFFF, 1, LNKF, 0 },
    Package(){ 0x0000FFFF, 2, LNKG, 0 },
    Package(){ 0x0000FFFF, 3, LNKH, 0 },
  })
  Name(AR02, Package() {
    Package(){ 0x0000FFFF, 0, 0, 20 },
    Package(){ 0x0000FFFF, 1, 0, 21 },
    Package(){ 0x0000FFFF, 2, 0, 22 },
    Package(){ 0x0000FFFF, 3, 0, 23 },
  })
  Name(NR02, Package() {
    Package(){ 0x0000FFFF, 0, 0, 28 },
    Package(){ 0x0000FFFF, 1, 0, 29 },
    Package(){ 0x0000FFFF, 2, 0, 30 },
    Package(){ 0x0000FFFF, 3, 0, 31 },
  })

  Name(PR03, Package() {
    Package(){ 0x0000FFFF, 0, LNKA, 0 },
    Package(){ 0x0000FFFF, 1, LNKB, 0 },
    Package(){ 0x0000FFFF, 2, LNKC, 0 },
    Package(){ 0x0000FFFF, 3, LNKD, 0 },
  })
  Name(AR03, Package() {
    Package(){ 0x0000FFFF, 0, 0, 16 },
    Package(){ 0x0000FFFF, 1, 0, 17 },
    Package(){ 0x0000FFFF, 2, 0, 18 },
    Package(){ 0x0000FFFF, 3, 0, 19 },
  })
  Name(NR03, Package() {
    Package(){ 0x0000FFFF, 0, 0, 32 },
    Package(){ 0x0000FFFF, 1, 0, 33 },
    Package(){ 0x0000FFFF, 2, 0, 34 },
    Package(){ 0x0000FFFF, 3, 0, 35 },
  })

  Name(PR04, Package() {
    Package(){ 0x0000FFFF, 0, LNKE, 0 },
    Package(){ 0x0000FFFF, 1, LNKF, 0 },
    Package(){ 0x0000FFFF, 2, LNKG, 0 },
    Package(){ 0x0000FFFF, 3, LNKH, 0 },
  })
  Name(AR04, Package() {
    Package(){ 0x0000FFFF, 0, 0, 20 },
    Package(){ 0x0000FFFF, 1, 0, 21 },
    Package(){ 0x0000FFFF, 2, 0, 22 },
    Package(){ 0x0000FFFF, 3, 0, 23 },
  })
  Name(NR04, Package() {
    Package(){ 0x0000FFFF, 0, 0, 36 },
    Package(){ 0x0000FFFF, 1, 0, 37 },
    Package(){ 0x0000FFFF, 2, 0, 38 },
    Package(){ 0x0000FFFF, 3, 0, 39 },
  })

  Name(PR05, Package() {
    Package(){ 0x0000FFFF, 0, LNKA, 0 },
    Package(){ 0x0000FFFF, 1, LNKB, 0 },
    Package(){ 0x0000FFFF, 2, LNKC, 0 },
    Package(){ 0x0000FFFF, 3, LNKD, 0 },
  })
  Name(AR05, Package() {
    Package(){ 0x0000FFFF, 0, 0, 16 },
    Package(){ 0x0000FFFF, 1, 0, 17 },
    Package(){ 0x0000FFFF, 2, 0, 18 },
    Package(){ 0x0000FFFF, 3, 0, 19 },
  })
  Name(NR05, Package() {
    Package(){ 0x0000FFFF, 0, 0, 40 },
    Package(){ 0x0000FFFF, 1, 0, 41 },
    Package(){ 0x0000FFFF, 2, 0, 42 },
    Package(){ 0x0000FFFF, 3, 0, 43 },
  })

  Name(PR06, Package() {
    Package(){ 0x0000FFFF, 0, LNKE, 0 },
    Package(){ 0x0000FFFF, 1, LNKF, 0 },
    Package(){ 0x0000FFFF, 2, LNKG, 0 },
    Package(){ 0x0000FFFF, 3, LNKH, 0 },
  })
  Name(AR06, Package() {
    Package(){ 0x0000FFFF, 0, 0, 20 },
    Package(){ 0x0000FFFF, 1, 0, 21 },
    Package(){ 0x0000FFFF, 2, 0, 22 },
    Package(){ 0x0000FFFF, 3, 0, 23 },
  })
  Name(NR06, Package() {
    Package(){ 0x0000FFFF, 0, 0, 44 },
    Package(){ 0x0000FFFF, 1, 0, 45 },
    Package(){ 0x0000FFFF, 2, 0, 46 },
    Package(){ 0x0000FFFF, 3, 0, 47 },
  })

  Name(PR07, Package() {
    Package(){ 0x0000FFFF, 0, LNKA, 0 },
    Package(){ 0x0000FFFF, 1, LNKB, 0 },
    Package(){ 0x0000FFFF, 2, LNKC, 0 },
    Package(){ 0x0000FFFF, 3, LNKD, 0 },
  })
  Name(AR07, Package() {
    Package(){ 0x0000FFFF, 0, 0, 16 },
    Package(){ 0x0000FFFF, 1, 0, 17 },
    Package(){ 0x0000FFFF, 2, 0, 18 },
    Package(){ 0x0000FFFF, 3, 0, 19 },
  })
  Name(NR07, Package() {
    Package(){ 0x0000FFFF, 0, 0, 48 },
    Package(){ 0x0000FFFF, 1, 0, 49 },
    Package(){ 0x0000FFFF, 2, 0, 50 },
    Package(){ 0x0000FFFF, 3, 0, 51 },
  })

  Name(PR17, Package() {
    // Bus A; Device 0; Function 0; Internal GPU
    // Bus A; Device 0; Function 1; Display HD Audio Controller
    // Bus A; Device 0; Function 2; Cryptographic Coprocessor
    // Bus A; Device 0; Function 3; USB 3.1
    // Bus A; Device 0; Function 4; USB 3.1
    // Bus A; Device 0; Function 5; Audio Processor
    // Bus A; Device 0; Function 6; Audio Processor - HD Audio Controller
    // Bus A; Device 0; Function 7; (Non) Sensor Fusion Hub
    Package(){ 0x0000FFFF, 0, LNKC, 0 },
    Package(){ 0x0000FFFF, 1, LNKD, 0 },
    Package(){ 0x0000FFFF, 2, LNKA, 0 },
    Package(){ 0x0000FFFF, 3, LNKB, 0 },
  })
  Name(AR17, Package() {
    Package(){ 0x0000FFFF, 0, 0, 18 },
    Package(){ 0x0000FFFF, 1, 0, 19 },
    Package(){ 0x0000FFFF, 2, 0, 16 },
    Package(){ 0x0000FFFF, 3, 0, 17 },
  })
  Name(NR17, Package() {
    Package(){ 0x0000FFFF, 0, 0, 50 },
    Package(){ 0x0000FFFF, 1, 0, 51 },
    Package(){ 0x0000FFFF, 2, 0, 48 },
    Package(){ 0x0000FFFF, 3, 0, 49 },
  })
  Name(PR18, Package() {
    // Bus B; Device 0; Function 0; SATA
    // Bus B; Device 0; Function 1; CVIP
    Package(){ 0x0000FFFF, 0, LNKG, 0 },
    Package(){ 0x0000FFFF, 1, LNKH, 0 },
    Package(){ 0x0000FFFF, 2, LNKE, 0 },
    Package(){ 0x0000FFFF, 3, LNKF, 0 },
  })
  Name(AR18, Package() {
    Package(){ 0x0000FFFF, 0, 0, 22 },
    Package(){ 0x0000FFFF, 1, 0, 23 },
    Package(){ 0x0000FFFF, 2, 0, 20 },
    Package(){ 0x0000FFFF, 3, 0, 21 },
  })
  Name(NR18, Package() {
    Package(){ 0x0000FFFF, 0, 0, 46 },
    Package(){ 0x0000FFFF, 1, 0, 47 },
    Package(){ 0x0000FFFF, 2, 0, 44 },
    Package(){ 0x0000FFFF, 3, 0, 45 },
  })

  //---------------------------------------------------------------------------
  // List of IRQ resource buffers compatible with _PRS return format.
  //---------------------------------------------------------------------------
  // Naming legend:
  // RSxy, PRSy - name of the IRQ resource buffer to be returned by _PRS, "xy" - last two characters of IRQ Link name.
  // Note. PRSy name is generated if IRQ Link name starts from "LNK".
  // HLxy , LLxy - reference names, can be used to access bit mask of available IRQs. HL and LL stand for active High(Low) Level triggered Irq model.
  //---------------------------------------------------------------------------
  Name(PRSA, ResourceTemplate()         // Link name: LNKA
  {
    IRQ(Level, ActiveLow, Shared, LLKA) {3, 4, 5, 6, 10, 11, 12, 14, 15}
  })
  Alias(PRSA, PRSB)      // Link name: LNKB
  Alias(PRSA, PRSC)      // Link name: LNKC
  Alias(PRSA, PRSD)      // Link name: LNKD
  Alias(PRSA, PRSE)      // Link name: LNKE
  Alias(PRSA, PRSF)      // Link name: LNKF
  Alias(PRSA, PRSG)      // Link name: LNKG
  Alias(PRSA, PRSH)      // Link name: LNKH

  //---------------------------------------------------------------------------
  // Begin PCI tree object scope
  //---------------------------------------------------------------------------

  Device(PCI0)   // PCI Bridge "Host Bridge"
  {
    Name(_HID, EISAID("PNP0A08"))       // Indicates PCI Express/PCI-X Mode2 host hierarchy
    Name(_CID, EISAID("PNP0A03"))       // To support legacy OS that doesn't understand the new HID
    Name(_ADR, 0x00000000)
    Method(^BN00, 0) { return(0x0000) } // Returns default Bus number for Peer PCI busses. Name can be overriden with control method placed directly under Device scope
    Method(_BBN, 0) { return(BN00()) }  // Bus number, optional for the Root PCI Bus
    Name(_UID, 0x0000)                  // Unique Bus ID, optional

    Method(_PRT, 0) {
      If(PICM) {
        If (\NAPC) {
          Return(NR00)      // NB IOAPIC Enabled
        } Else {
          Return(AR00)      // NB IOAPIC Disabled
        }
      } Else {
        Return (PR00)       // PIC mode
      }
    }

    include("HOST_BUS.ASL")

    // Motherboard Resource
    Device(PMBR)
    {
      Name(_HID, EISAID("PNP0C02"))

      Name(BUF0, ResourceTemplate()
      {
        // PCI Express BAR _BAS and _LEN.
        Memory32Fixed(ReadOnly, 0, 0, PCIX)
      })

      Method(_CRS, 0, NotSerialized) {
        CreateDwordField(BUF0, ^PCIX._BAS, PCXB)
        CreateDwordField(BUF0, ^PCIX._LEN, PCXL)

        Store(\PCBA, PCXB)
        Subtract(\PCBL, \PCBA, Local0)
        Add(Local0, 1, Local0)
        Store(Local0, PCXL)

        Return (BUF0)
      }
    }

    // GPP0 (Bus 0 Dev 1 Fn 1)
    Device(GPP0)
    {
      Name(_ADR, 0x00010001)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
          If(PICM) {
            If (\NAPC) {
              Return(NR01)      // NB IOAPIC Enabled
            } Else {
              Return(AR01)      // NB IOAPIC Disabled
            }
          } Else {
            Return (PR01)       // PIC mode
          }
        }
    }

    // GPP1 (Bus 0 Dev 1 Fn 2)
    Device(GPP1)
    {
      Name(_ADR, 0x00010002)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
          If(PICM) {
            If (\NAPC) {
              Return(NR02)      // NB IOAPIC Enabled
            } Else {
              Return(AR02)      // NB IOAPIC Disabled
            }
          } Else {
            Return (PR02)       // PIC mode
          }
        }
    }

    // GPP2 (Bus 0 Dev 1 Fn 3)
    Device(GPP2)
    {
      Name(_ADR, 0x00010003)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
          If(PICM) {
            If (\NAPC) {
              Return(NR03)      // NB IOAPIC Enabled
            } Else {
              Return(AR03)      // NB IOAPIC Disabled
            }
          } Else {
            Return (PR03)       // PIC mode
          }
        }
    }

    // GPP3 (Bus 0 Dev 1 Fn 4)
    Device(GPP3)
    {
      Name(_ADR, 0x00010004)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
          If(PICM) {
            If (\NAPC) {
              Return(NR04)      // NB IOAPIC Enabled
            } Else {
              Return(AR04)      // NB IOAPIC Disabled
            }
          } Else {
            Return (PR04)       // PIC mode
          }
        }
    }

    // GPP4 (Bus 0 Dev 1 Fn 5)
    Device(GPP4)
    {
      Name(_ADR, 0x00010005)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
          If(PICM) {
            If (\NAPC) {
              Return(NR05)      // NB IOAPIC Enabled
            } Else {
              Return(AR05)      // NB IOAPIC Disabled
            }
          } Else {
            Return (PR05)       // PIC mode
          }
        }
    }

    // GPP5 (Bus 0 Dev 1 Fn 6)
    Device(GPP5)
    {
      Name(_ADR, 0x00010006)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
          If(PICM) {
            If (\NAPC) {
              Return(NR06)      // NB IOAPIC Enabled
            } Else {
              Return(AR06)      // NB IOAPIC Disabled
            }
          } Else {
            Return (PR06)       // PIC mode
          }
        }
    }

    // GPP6 (Bus 0 Dev 1 Fn 7)
    Device(GPP6)
    {
      Name(_ADR, 0x00010007)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
          If(PICM) {
            If (\NAPC) {
              Return(NR07)      // NB IOAPIC Enabled
            } Else {
              Return(AR07)      // NB IOAPIC Disabled
            }
          } Else {
            Return (PR07)       // PIC mode
          }
        }
    }

    // GP17 (Bus 0 Dev 8 Fn 1)
    Device(GP17)
    {
      Name(_ADR, 0x00080001)
      Name(_PRW, Package() {0x19, 4})

      Method(_PRT, 0) {
        If(PICM) {
          If (\NAPC) {
            Return(NR17)      // NB IOAPIC Enabled
          } Else {
            Return(AR17)      // NB IOAPIC Disabled
          }
        } Else {
          Return (PR17)       // PIC mode
        }
      }

      // Bus A Dev 0 Fn 0 - Internal GPU
      Device(VGA)
      {
          Name(_ADR, 0x00000000)
      }
      // Bus A Dev 0 Fn 1 - Display HD Audio Controller
      Device(DHDA)
      {
          Name(_ADR, 0x00000001)
      }
      // Bus A Dev 0 Fn 2 - Cryptographic Coprocessor
      Device(PCCP)
      {
          Name(_ADR, 0x00000002)
      }
      // Bus A Dev 0 Fn 3 - USB 3.1 DRD
      Device(DRD0)
      {
        Name(_ADR, 0x00000003)
        Name(_PRW, Package() {0x19, 4})
      }
      // Bus A Dev 0 Fn 4 - USB 3.1
      Device(XHC1)
      {
        Name(_ADR, 0x00000004)
        Name(_PRW, Package() {0x19, 4})
        Name(_S0W, 3)

        Device(RHUB)
        {
          Name(_ADR, 0x00000000)

          // HS port 0 (J5 USB-A)
          Device(PRT1) {
            Name(_ADR, 0x00000001)

            Name(UPC1, Package(4) { 0xFF, 0x03, 0x00000000, 0x00000000 } ) //USB 3 Standard-A connector

            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }

            Name (_PLD, Package (0x01)  // _PLD: Physical Location of Device
            {
              Buffer(0x14) {
                0x82,                   // Revision 2, Ignore color;
                0x00, 0x00, 0x00,       // 24-bit RGB value for the color of the device;
                0x00, 0x00, 0x00, 0x00, // Width & Height;
                0x11, 0x0C, 0x80, 0x00, //User visible, Group Token =0;
                                        // Group Position 1st;
                0x01, 0x00, 0x00, 0x00,
                0xFF, 0xFF, 0xFF, 0xFF  // Reserve
              }
            })
          }

          // HS port 1 (J33 WWAN)
          Device(PRT2) {
            Name(_ADR, 0x00000002)

            Name(UPC1, Package(4) { 0x00, 0xFF, 0x00000000, 0x00000000 } ) //Proprietary connector

            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }
          }

          // HS port 2 (U138 WWLAN_EVB)
          Device(PRT3) {
            Name(_ADR, 0x00000003)

            Name(UPC1, Package(4) { 0x00, 0xFF, 0x00000000, 0x00000000 } ) //Proprietary connector

            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }
          }

          // HS port 3 (J2 WLAN)
          Device(PRT4) {
            Name(_ADR, 0x00000004)

            Name(UPC1, Package(4) { 0x00, 0xFF, 0x00000000, 0x00000000 } ) //Proprietary connector
            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }
          }

          // HS port 4 (J29 Bottom)
          Device(PRT5) {
            Name(_ADR, 0x00000005)

            Name(UPC1, Package(4) { 0xFF, 0x00, 0x00000000, 0x00000000 } ) //Standard-A connector

            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }

            Name(PLD1, Package(1) {
              Buffer (0x14)
              {
              }
            })

            Method (_PLD,0,Serialized)
            {
              Return (PLD1)
            }
          }

          // HS port 5 (J29 Top)
          Device(PRT6) {
            Name(_ADR, 0x00000006)

            Name(UPC1, Package(4) { 0xFF, 0x03, 0x00000000, 0x00000000 } ) //USB 3 Standard-A connector

            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }

            Name(PLD1, Package() {
              Buffer (0x14)
              {
              }
            })

            Method (_PLD,0,Serialized)
            {
              Return (PLD1)
            }
          }

          // SS port 1 (J5 USB-A)
          Device(PRT7) {
            Name(_ADR, 0x00000007)

            Name(UPC1, Package(4) { 0xFF, 0x03, 0x00000000, 0x00000000 } ) //Standard-A

            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }

            Name (_PLD, Package (0x01)  // _PLD: Physical Location of Device
            {
              Buffer(0x14) {
                0x82,                   // Revision 2, Ignore color;
                0x00, 0x00, 0x00,       // 24-bit RGB value for the color of the device;
                0x00, 0x00, 0x00, 0x00, // Width & Height;
                0x11, 0x0C, 0x80, 0x00, //User visible, Group Token =0;
                                        // Group Position 1st;
                0x01, 0x00, 0x00, 0x00,
                0xFF, 0xFF, 0xFF, 0xFF  // Reserve
              }
            })
          }

          // SS port 2 (J33 WWAN)
          Device(PRT8) {
            Name(_ADR, 0x00000008)

            Name(UPC1, Package(4) { 0x00, 0xFF, 0x00000000, 0x00000000 } ) //Proprietary connector

            Method (_UPC,0,Serialized)
            {
              Return (UPC1)
            }
          }
        }
      }
      // Bus A Dev 0 Fn 5 - Audio Processor
      Device(ACP)
      {
        Name(_ADR, 0x00000005)
      }
      // Bus A Dev 0 Fn 6 - Audio Processor - HD Audio Controller
      Device(AZAL)
      {
        Name(_ADR, 0x00000006)
      }
      // Bus A Dev 0 Fn 7 - (Non) Sensor Fusion Hub
      Device(NSFH)
      {
        Name(_ADR, 0x00000007)
      }
    }

    // GP18 (Bus 0 Dev 8 Fn 2)
    Device(GP18)
    {
      Name(_ADR, 0x00080002)
      Name(_PRW, Package() {0x8, 4})

      Method(_PRT, 0) {
        If(PICM) {
          If (\NAPC) {
            Return(NR18)      // NB IOAPIC Enabled
          } Else {
            Return(AR18)      // NB IOAPIC Disabled
          }
        } Else {
          Return (PR18)       // PIC mode
        }
      }

      // Bus B Dev 0 Fn 0 - SATA
      Device(SATA)
      {
        Name(_ADR, 0x00000000)
      }
    }
    include("Lpc0.asl")

  } // end PCI0 Bridge "Host Bridge"
} // end _SB scope
