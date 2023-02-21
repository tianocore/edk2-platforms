/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock("Dsdt.aml", "DSDT", 0x02, "Ampere", "Jade", 1) {
  //
  // Board Model
  Name(\BDMD, "Jade Board")
  Name(AERF, 0)  // PCIe AER Firmware-First
  Scope(\_SB) {

    //
    // Hardware Monitor
    Device(HM00) {
      Name(_HID, "APMC0D29")
      Name(_UID, "HWM0")
      Name(_DDN, "HWM0")
      Name(_CCA, ONE)
      Name(_STR, Unicode("Hardware Monitor Device"))
      Method(_STA, 0, NotSerialized) {
        return (0xF)
      }
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package() {
          Package() {"pcc-channel", 14}
        }
      })
    }

    //
    // Hardware Monitor
    Device(HM01) {
      Name(_HID, "APMC0D29")
      Name(_UID, "HWM1")
      Name(_DDN, "HWM1")
      Name(_CCA, ONE)
      Name(_STR, Unicode("Hardware Monitor Device"))
      Method(_STA, 0, NotSerialized) {
        return (0xF)
      }
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package() {
          Package() {"pcc-channel", 29}
        }
      })
    }

    //
    // Hardware Monitor
    Device(HM02) {
      Name(_HID, "AMPC0005")
      Name(_UID, "HWM2")
      Name(_DDN, "HWM2")
      Name(_CCA, ONE)
      Name(_STR, Unicode("AC01 SoC Hardware Monitor Device"))
      Method(_STA, 0, NotSerialized) {
        return (0xF)
      }
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,     // ResourceUsage
          PosDecode,            // Decode
          MinFixed,             // IsMinFixed
          MaxFixed,             // IsMaxFixed
          Cacheable,            // Cacheable
          ReadWrite,            // ReadAndWrite
          0x0000000000000000,   // AddressGranularity - GRA
          0x0000000088900000,   // AddressMinimum - MIN
          0x000000008891FFFF,   // AddressMaximum - MAX
          0x0000000000000000,   // AddressTranslation - TRA
          0x0000000000020000    // RangeLength - LEN
        )
      })
    }

    //
    // Hardware Monitor
    Device(HM03) {
      Name(_HID, "AMPC0005")
      Name(_UID, "HWM3")
      Name(_DDN, "HWM3")
      Name(_CCA, ONE)
      Name(_STR, Unicode("AC01 SoC Hardware Monitor Device"))
      Method(_STA, 0, NotSerialized) {
        return (0xF)
      }
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,     // ResourceUsage
          PosDecode,            // Decode
          MinFixed,             // IsMinFixed
          MaxFixed,             // IsMaxFixed
          Cacheable,            // Cacheable
          ReadWrite,            // ReadAndWrite
          0x0000000000000000,   // AddressGranularity - GRA
          0x00000000C0000000,   // AddressMinimum - MIN
          0x00000000C001FFFF,   // AddressMaximum - MAX
          0x0000000000000000,   // AddressTranslation - TRA
          0x0000000000020000    // RangeLength - LEN
        )
      })
    }

    //
    // DesignWare I2C on AHBC bus
    Device(I2C4) {
      Name(_HID, "APMC0D0F")
      Name(_UID, 4)
      Name(_STR, Unicode("Altra I2C Device"))
      Method(_STA, 0, NotSerialized) {
        return (0x0f)
      }
      Name(_CCA, ONE)
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,     // ResourceUsage
          PosDecode,            // Decode
          MinFixed,             // IsMinFixed
          MaxFixed,             // IsMaxFixed
          NonCacheable,         // Cacheable
          ReadWrite,            // ReadAndWrite
          0x0000000000000000,   // AddressGranularity - GRA
          0x00001000026B0000,   // AddressMinimum - MIN
          0x00001000026BFFFF,   // AddressMaximum - MAX
          0x0000000000000000,   // AddressTranslation - TRA
          0x0000000000010000    // RangeLength - LEN
        )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 105 }
      })
      Device (IPI) {
        Name(_HID, "AMPC0004")
        Name(_CID, "IPI0001")
        Name(_STR, Unicode("IPMI_SSIF"))
        Name(_UID, 0)
        Name(_CCA, ONE)
        Method(_STA, 0, NotSerialized) {
          Return (0x0f)
        }
        Method(_IFT) {
          Return(0x04) // IPMI SSIF
        }
        Method(_ADR) {
          Return(0x10) // SSIF slave address
        }
        Method(_SRV) {
          Return(0x0200) // IPMI Specification Revision
        }
        Name(_CRS, ResourceTemplate ()
        {
          I2cSerialBusV2 (0x0010, ControllerInitiated, 0x00061A80,
            AddressingMode7Bit, "\\_SB.I2C4",
            0x00, ResourceConsumer,, Exclusive,
            // Vendor specific data:
            // "BMC0",
            // Flags (2 bytes): SMBUS variable length (Bit 0), Read Checksum (Bit 1), Verify Checksum (Bit 2)
            RawDataBuffer () { 0x42, 0x4D, 0x43, 0x30, 0x7, 0x0 }
            )
        })
      }
      Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
      Name(FMCN, Package() { 0xA4, 0x13F, 0 })
    }

    //
    // Report APEI Errors to GHES via SCI notification.
    // SCI notification requires one GED and one HED Device
    //     GED = Generic Event Device (ACPI0013)
    //     HED = Hardware Error Device (PNP0C33)
    //
    Device(GED0) {
        Name(_HID, "ACPI0013")
        Name(_UID, Zero)
        Method(_STA) {
          Return (0xF)
        }
        Name(_CRS, ResourceTemplate () {
          Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 84 } // GHES
        })
        Method(_EVT, 1, Serialized) {
          Switch (ToInteger(Arg0)) {
            Case (84) { // GHES interrupt
              Notify (HED0, 0x80)
            }
          }
        }
    }

    // Shutdown button using GED.
    Device(GED1) {
        Name(_HID, "ACPI0013")
        Name(_CID, "ACPI0013")
        Name(_UID, One)
        Method(_STA) {
          Return (0xF)
        }
        Name(_CRS, ResourceTemplate () {
          Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 327 }
        })
        OperationRegion(PDDR, SystemMemory, 0x1000027B0004, 4)
        Field(PDDR, DWordAcc, NoLock, Preserve) {
          STDI, 8
        }

        OperationRegion(INTE, SystemMemory, 0x1000027B0030, 4)
        Field(INTE, DWordAcc, NoLock, Preserve) {
          STDE, 8
        }

        OperationRegion(INTT, SystemMemory, 0x1000027B0034, 4)
        Field(INTT, DWordAcc, NoLock, Preserve) {
          TYPE, 8
        }

        OperationRegion(INTP, SystemMemory, 0x1000027B0038, 4)
        Field(INTP, DWordAcc, NoLock, Preserve) {
          POLA, 8
        }

        OperationRegion(INTS, SystemMemory, 0x1000027B003c, 4)
        Field(INTS, DWordAcc, NoLock, Preserve) {
          STDS, 8
        }

        OperationRegion(INTC, SystemMemory, 0x1000027B0040, 4)
        Field(INTC, DWordAcc, NoLock, Preserve) {
          SINT, 8
        }

        OperationRegion(INTM, SystemMemory, 0x1000027B0044, 4)
        Field(INTM, DWordAcc, NoLock, Preserve) {
          MASK, 8
        }

        Method(_INI, 0, NotSerialized) {
          // Set level type, low active (shutdown)
          Store (0x00, TYPE)
          Store (0x00, POLA)
          // Set Input type (shutdown)
          Store (0x00, STDI)
          // Enable interrupt (shutdown)
          Store (0x80, STDE)
          // Unmask the interrupt.
          Store (0x00, MASK)
        }
        Method(_EVT, 1, Serialized) {
          Switch (ToInteger(Arg0)) {
            Case (327) {
              if (And (STDS, 0x80)) {
                //Clear the interrupt.
                Store (0x80, SINT)
                // Notify OSPM the power button is pressed
                Notify (\_SB.PWRB, 0x80)
              }
            }
          }
        }
    }

    // Power button device description
    Device(PWRB) {
        Name(_HID, EISAID("PNP0C0C"))
        Name(_UID, 0)
        Name(_CCA, ONE)
        Method(_STA, 0, Notserialized) {
            Return (0x0b)
        }
    }

    //
    // UART0 PL011
    Device(URT0) {
      Name(_HID, "ARMH0011")
      Name(_UID, 0)
      Name(_CCA, ONE)
      Method(_STA, 0, NotSerialized) {
        return (0x0f)
      }
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,     // ResourceUsage
          PosDecode,            // Decode
          MinFixed,             // IsMinFixed
          MaxFixed,             // IsMaxFixed
          NonCacheable,         // Cacheable
          ReadWrite,            // ReadAndWrite
          0x0000000000000000,   // AddressGranularity - GRA
          0x0000100002600000,   // AddressMinimum - MIN
          0x0000100002600FFF,   // AddressMaximum - MAX
          0x0000000000000000,   // AddressTranslation - TRA
          0x0000000000001000    // RangeLength - LEN
        )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 98 }
      })
    } // UART0

    //
    // UART2 PL011
    Device(URT2) {
      Name(_HID, "ARMH0011")
      Name(_UID, 1)
      Name(_CCA, ONE)
      Method(_STA, 0, NotSerialized) {
        return (0x0f)
      }
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,     // ResourceUsage
          PosDecode,            // Decode
          MinFixed,             // IsMinFixed
          MaxFixed,             // IsMaxFixed
          NonCacheable,         // Cacheable
          ReadWrite,            // ReadAndWrite
          0x0000000000000000,   // AddressGranularity - GRA
          0x0000100002620000,   // AddressMinimum - MIN
          0x0000100002620FFF,   // AddressMaximum - MAX
          0x0000000000000000,   // AddressTranslation - TRA
          0x0000000000001000    // RangeLength - LEN
        )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 100 }
      })
    } // UART1

    Device(HED0)
    {
        Name(_HID, EISAID("PNP0C33"))
        Name(_UID, Zero)
    }

    Device(NVDR) {
      Name(_HID, "ACPI0012")
      Method(_STA, 0, NotSerialized) {
        return (0xf)
      }
      Method (_DSM, 0x4, Serialized) {
        // Not support any functions for now
        Return (Buffer() {0})
      }
      Device (NVD1) {
        Name(_ADR, 0x0330)
        Name(SMRT, Buffer(13) {0})
        CreateDWordField(SMRT, 0, BSTA)
        CreateWordField(SMRT, 4, BHTH)
        CreateWordField(SMRT, 6, BTMP)
        CreateByteField(SMRT, 8, BETH)
        CreateByteField(SMRT, 9, BWTH)
        CreateByteField(SMRT, 10, BNLF)
        OperationRegion(BUF1, SystemMemory, 0x88980000, 16)
        Field (BUF1, DWordAcc, NoLock, Preserve) {
          STAT, 32, //Status
          HLTH, 16, //Module Health
          CTMP, 16, //Module Current Status
          ETHS, 8, //Error Threshold Status
          WTHS, 8, //Warning Threshold Status
          NVLF, 8, //NVM Lifetime
          ,     40 //Reserve
        }
        Method (_DSM, 0x4, Serialized) {
          //Accept only MSF Family type NVDIMM DSM functions
          If(LEqual(Arg0, ToUUID ("1ee68b36-d4bd-4a1a-9a16-4f8e53d46e05"))) {
            //Handle Func 0 query implemented commands
            If(LEqual(Arg2, 0)) {
              //Check revision and returned proper implemented commands
              //Support only health check for now
              Return (Buffer() {0x01, 0x08}) //Byte 0: 0x1
            }
            //Handle MSF DSM Func 11 Get Smart and Health Info
            If(LEqual(Arg2, 11)) {
              Store(\_SB.NVDR.NVD1.STAT, BSTA)
              Store(\_SB.NVDR.NVD1.HLTH, BHTH)
              Store(\_SB.NVDR.NVD1.CTMP, BTMP)
              Store(\_SB.NVDR.NVD1.ETHS, BETH)
              Store(\_SB.NVDR.NVD1.WTHS, BWTH)
              Store(\_SB.NVDR.NVD1.NVLF, BNLF)
              Return (SMRT)
            }
          }
          Return (Buffer() {0})
        }
        Method(_STA, 0, NotSerialized) {
          return (0xf)
        }
      }
      Device (NVD2) {
        Name(_ADR, 0x0770)
        Name(SMRT, Buffer(13) {0})
        CreateDWordField(SMRT, 0, BSTA)
        CreateWordField(SMRT, 4, BHTH)
        CreateWordField(SMRT, 6, BTMP)
        CreateByteField(SMRT, 8, BETH)
        CreateByteField(SMRT, 9, BWTH)
        CreateByteField(SMRT, 10, BNLF)
        OperationRegion(BUF1, SystemMemory, 0x88988000, 16)
        Field (BUF1, DWordAcc, NoLock, Preserve) {
          STAT, 32, //Status
          HLTH, 16, //Module Health
          CTMP, 16, //Module Current Status
          ETHS, 8, //Error Threshold Status
          WTHS, 8, //Warning Threshold Status
          NVLF, 8, //NVM Lifetime
          ,     40 //Reserve
        }
        Method (_DSM, 0x4, Serialized) {
          //Accept only MSF Family type NVDIMM DSM functions
          If(LEqual(Arg0, ToUUID ("1ee68b36-d4bd-4a1a-9a16-4f8e53d46e05"))) {
            //Handle Func 0 query implemented commands
            If(LEqual(Arg2, 0)) {
              //Check revision and returned proper implemented commands
              //Support only health check for now
              Return (Buffer() {0x01, 0x08}) //Byte 0: 0x1
            }
            //Handle MSF DSM Func 11 Get Smart and Health Info
            If(LEqual(Arg2, 11)) {
              Store(\_SB.NVDR.NVD2.STAT, BSTA)
              Store(\_SB.NVDR.NVD2.HLTH, BHTH)
              Store(\_SB.NVDR.NVD2.CTMP, BTMP)
              Store(\_SB.NVDR.NVD2.ETHS, BETH)
              Store(\_SB.NVDR.NVD2.WTHS, BWTH)
              Store(\_SB.NVDR.NVD2.NVLF, BNLF)
              Return (SMRT)
            }
          }
          Return (Buffer() {0})
        }
        Method(_STA, 0, NotSerialized) {
          return (0xf)
        }
      }
      Device (NVD3) {
        Name(_ADR, 0x1330)
        Name(SMRT, Buffer(13) {0})
        CreateDWordField(SMRT, 0, BSTA)
        CreateWordField(SMRT, 4, BHTH)
        CreateWordField(SMRT, 6, BTMP)
        CreateByteField(SMRT, 8, BETH)
        CreateByteField(SMRT, 9, BWTH)
        CreateByteField(SMRT, 10, BNLF)
        OperationRegion(BUF1, SystemMemory, 0xC0080000, 16)
        Field (BUF1, DWordAcc, NoLock, Preserve) {
          STAT, 32, //Status
          HLTH, 16, //Module Health
          CTMP, 16, //Module Current Status
          ETHS, 8, //Error Threshold Status
          WTHS, 8, //Warning Threshold Status
          NVLF, 8, //NVM Lifetime
          ,     40 //Reserve
        }
        Method (_DSM, 0x4, Serialized) {
          //Accept only MSF Family type NVDIMM DSM functions
          If(LEqual(Arg0, ToUUID ("1ee68b36-d4bd-4a1a-9a16-4f8e53d46e05"))) {
            //Handle Func 0 query implemented commands
            If(LEqual(Arg2, 0)) {
              //Check revision and returned proper implemented commands
              //Support only health check for now
              Return (Buffer() {0x01, 0x08}) //Byte 0: 0x1
            }
            //Handle MSF DSM Func 11 Get Smart and Health Info
            If(LEqual(Arg2, 11)) {
              Store(\_SB.NVDR.NVD3.STAT, BSTA)
              Store(\_SB.NVDR.NVD3.HLTH, BHTH)
              Store(\_SB.NVDR.NVD3.CTMP, BTMP)
              Store(\_SB.NVDR.NVD3.ETHS, BETH)
              Store(\_SB.NVDR.NVD3.WTHS, BWTH)
              Store(\_SB.NVDR.NVD3.NVLF, BNLF)
              Return (SMRT)
            }
          }
          Return (Buffer() {0})
        }
        Method(_STA, 0, NotSerialized) {
          return (0xf)
        }
      }
      Device (NVD4) {
        Name(_ADR, 0x1770)
        Name(SMRT, Buffer(13) {0})
        CreateDWordField(SMRT, 0, BSTA)
        CreateWordField(SMRT, 4, BHTH)
        CreateWordField(SMRT, 6, BTMP)
        CreateByteField(SMRT, 8, BETH)
        CreateByteField(SMRT, 9, BWTH)
        CreateByteField(SMRT, 10, BNLF)
        OperationRegion(BUF1, SystemMemory, 0xC0088000, 16)
        Field (BUF1, DWordAcc, NoLock, Preserve) {
          STAT, 32, //Status
          HLTH, 16, //Module Health
          CTMP, 16, //Module Current Status
          ETHS, 8, //Error Threshold Status
          WTHS, 8, //Warning Threshold Status
          NVLF, 8, //NVM Lifetime
          ,     40 //Reserve
        }
        Method (_DSM, 0x4, Serialized) {
          //Accept only MSF Family type NVDIMM DSM functions
          If(LEqual(Arg0, ToUUID ("1ee68b36-d4bd-4a1a-9a16-4f8e53d46e05"))) {
            //Handle Func 0 query implemented commands
            If(LEqual(Arg2, 0)) {
              //Check revision and returned proper implemented commands
              //Support only health check for now
              Return (Buffer() {0x01, 0x08}) //Byte 0: 0x1
            }
            //Handle MSF DSM Func 11 Get Smart and Health Info
            If(LEqual(Arg2, 11)) {
              Store(\_SB.NVDR.NVD4.STAT, BSTA)
              Store(\_SB.NVDR.NVD4.HLTH, BHTH)
              Store(\_SB.NVDR.NVD4.CTMP, BTMP)
              Store(\_SB.NVDR.NVD4.ETHS, BETH)
              Store(\_SB.NVDR.NVD4.WTHS, BWTH)
              Store(\_SB.NVDR.NVD4.NVLF, BNLF)
              Return (SMRT)
            }
          }
          Return (Buffer() {0})
        }
        Method(_STA, 0, NotSerialized) {
          return (0xf)
        }
      }
    }

    Include ("PCI-S0.Rca01.asi")
    Include ("PCI-S0.asi")
    Include ("PCI-S1.asi")
    Include ("PCI-PDRC.asi")
  }

  Include ("CPU.asi")
  Include ("PMU.asi")

} // DSDT
