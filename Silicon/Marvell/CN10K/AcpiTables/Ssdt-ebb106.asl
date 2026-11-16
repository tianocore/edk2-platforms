// SPDX-License-Identifier: BSD-2-Clause-Patent
// https://spdx.org/licenses
//
// Copyright (C) 2023-2026 Marvell
//
// SSDT ACPI Table
//
// IMPORTANT: 'OEM Table ID' must match with mOemTableId (see AcpiPlatform.c)

#define TWSI_SPEED 100000 // clock-frequency = <100000>; in dts/cn10ka-common-linux.dtsi

 DefinitionBlock ("", "SSDT", 2, "MRVL", "ebb-106", 0x00000001)
 {
     // https://www.kernel.org/doc/html/v5.2/firmware-guide/acpi/enumeration.html
     // SPI serial bus support:
     // Child devices behind SPI bus have SpiSerialBus resource attached to them.
     // This is extracted automatically by the SPI core and the child devices
     // are enumerated once spi_register_master() is called by the bus driver.

     External (\_SB.SPI0, DeviceObj) // SPI0 defined in Dsdt.asl
     Scope (\_SB.SPI0)
     {
        // SPI defined for ebb106 (reference: dts/ebb106-linux.dts)
        Device (FLA0) {               // Flash 0: MX25UM51245GXDI00
          Name (_HID, "PRP0001")      // ACPI_DT_NAMESPACE_HID
          Name (_DDN, "SPI Flash MX25UM51245GXDI00")  // Optional

          Method (_STA) {Return (0xF)}

          // Provide configuration data for SPI controller.
          Name (_CRS, ResourceTemplate() {
            // Refer to:
            // linux/Documentation/devicetree/bindings/spi/spi-controller.yaml
            // and dts/ebb106-linux.dts to set proper values within SpiSerialBus.
            // Parser within: linux/drivers/spi/spi.c -> acpi_spi_add_resource().
            SpiSerialBus (
              0,                          // Chip select (reg)
              PolarityLow,                // Chip select is active low (no spi-cs-high)
              FourWireMode,               // Full duplex
              8,                          // Bits per word is 8 (byte)
              ControllerInitiated,        // Don't care
              25000000,                   // 25 MHz (spi-max-frequency)
              ClockPolarityLow,           // SPI mode 0 (no spi-cpol)
              ClockPhaseFirst,            // SPI mode 0 (no spi-cpha)
              "\\_SB.SPI0"                // SPI host controller for this device
              )
          }) // _CRS

          // Link child device to corresponding driver (linux/drivers/mtd/spi-nor/spi-nor.c).
          // According to linux/Documentation/devicetree/bindings/mtd/jedec,spi-nor.txt
          Name (_DSD, Package() {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
                Package () { "compatible", Package () { "jedec,spi-nor", "spi-flash"}},
                Package () { "reg", 0}, // See cdns_xspi_of_get_plat_data()
                // Those are read by kernel driver to set DUAL/QUAD/OCTAL SPI mode.
                Package () { "spi-tx-bus-width", 4},
                Package () { "spi-rx-bus-width", 4}
            }
          })  // _DSD
        }     // FLA0
     }        // Scope SPI0

     External (\_SB.SPI1, DeviceObj) // SPI1 defined in Dsdt.asl
     Scope (\_SB.SPI1)
     {
        // SPI defined for ebb106 (reference: dts/ebb106-linux.dts)
        Device (FLA0) {               // Flash 0: MX25U51245GXDI00
          Name (_HID, "PRP0001")      // ACPI_DT_NAMESPACE_HID
          Name (_DDN, "SPI Flash MX25U51245GXDI00")  // Optional

          Method (_STA) {Return (0xF)}

          // Provide configuration data for SPI controller.
          Name (_CRS, ResourceTemplate() {
            // Refer to:
            // linux/Documentation/devicetree/bindings/spi/spi-controller.yaml
            // and dts/ebb106-linux.dts to set proper values within SpiSerialBus.
            // Parser within: linux/drivers/spi/spi.c -> acpi_spi_add_resource().
            SpiSerialBus (
              0,                          // Chip select (reg)
              PolarityLow,                // Chip select is active low (no spi-cs-high)
              FourWireMode,               // Full duplex
              8,                          // Bits per word is 8 (byte)
              ControllerInitiated,        // Don't care
              25000000,                   // 25 MHz (spi-max-frequency)
              ClockPolarityLow,           // SPI mode 0 (no spi-cpol)
              ClockPhaseFirst,            // SPI mode 0 (no spi-cpha)
              "\\_SB.SPI1"                // SPI host controller for this device
              )
          }) // _CRS

          // Link child device to corresponding driver (linux/drivers/mtd/spi-nor/spi-nor.c).
          // According to linux/Documentation/devicetree/bindings/mtd/jedec,spi-nor.txt
          Name (_DSD, Package() {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
                Package () { "compatible", Package () { "jedec,spi-nor", "spi-flash"}},
                Package () { "reg", 0}, // See cdns_xspi_of_get_plat_data()
                // Those are read by kernel driver to set DUAL/QUAD/OCTAL SPI mode.
                Package () { "spi-tx-bus-width", 4},
                Package () { "spi-rx-bus-width", 4}
            }
          })  // _DSD
        }     // FLA0

        Device (TPM0) {               // Infineon’s Trusted Platform Module (TPM) SLB9670
          Name (_HID, "SMO0768")      // See linux/drivers/char/tpm/tpm_tis_spi.c
          Name (_DDN, "TPM SLB9670")  // Optional

          Method (_STA) {Return (0xF)}

          // Provide configuration data for SPI controller.
          Name (_CRS, ResourceTemplate() {
            // Refer to:
            // linux/Documentation/devicetree/bindings/spi/spi-controller.yaml
            // and dts/ebb106-linux.dts to set proper values within SpiSerialBus.
            // Parser within: linux/drivers/spi/spi.c -> acpi_spi_add_resource().
            SpiSerialBus (
              1,                          // Chip select (reg)
              PolarityLow,                // Chip select is active low (no spi-cs-high)
              FourWireMode,               // Full duplex
              8,                          // Bits per word is 8 (byte)
              ControllerInitiated,        // Don't care
              10000000,                   // 10 MHz (spi-max-frequency)
              ClockPolarityLow,           // SPI mode 0 (no spi-cpol)
              ClockPhaseFirst,            // SPI mode 0 (no spi-cpha)
              "\\_SB.SPI1"                // SPI host controller for this device
              )
          })  // _CRS

          // linux/drivers/spi/spi-cadence-xspi.c -> cdns_xspi_of_get_plat_data()
          Name (_DSD, Package() {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
                Package () { "reg", 1}
            }
          })  // _DSD
        }     // TPM0
     }        // Scope SPI1

     External (\_SB.EMMC, DeviceObj) // EMMC defined in Dsdt.asl
     Scope (\_SB.EMMC)
     {
      // EMMC properties for ebb106 (reference: dts/ebb106-linux.dts)
      Name (_DSD, Package() {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            // See dts/cn10ka-common-linux.dtsi
            Package () { "compatible", "cdns,sd6hc"},
            // The I/O PLL generates a fixed frequency of 800 MHz clock
            // EMMC_CLK = I/O PLL / (2 x 2) // CN106XXS Clocking Diagram, Part 1
            Package () { "clock-frequency",          200000000}, // sdhci_cdns_sd6_phy_probe()
            Package () { "sdhci-caps-mask", 0x00FF000000000000}, // __sdhci_read_caps()
            Package () { "sdhci-caps",      0x800000771F6EC8A0}, // __sdhci_read_caps()
            // See ebb106 (reference: dts/ebb106-linux.dts)
            Package () { "cdns,iocell_input_delay",        650}, // sdhci_cdns_sd6_get_delay_params
            Package () { "cdns,iocell_output_delay",      1800}, // sdhci_cdns_sd6_get_delay_params
            //Package () { "cdns,delay_element",              24}, // sdhci_cdns_sd6_get_delay_params
            Package () { "cdns,mode",               "emmc_sdr"}, // sdhci_cdns_sd6_phy_probe()
            Package () { "bus-width",                        8}, // mmc_of_parse()
            Package () { "max-frequency",            200000000}, // mmc_of_parse()
            Package () { "cap-mmc-highspeed",                1}, // mmc_of_parse()
            Package () { "mmc-hs200-1_8v",                   1}, // mmc_of_parse()
            Package () { "mmc-hs400-1_8v",                   1}, // mmc_of_parse()
            Package () { "no-sdio",                          1}, // mmc_of_parse()
            Package () { "no-sd",                            1}, // mmc_of_parse()
            Package () { "voltage-ranges", Package () {1800, 1800}}, // mmc_of_parse_voltage()
            Package () { "non-removable",                    1}, // mmc_of_parse()
            Package () { "mmc-cmd23",                        1},

            // HS200 - Tuning Params
            // kernel: sdhci_cdns_sd6_get_delay_params()

            // Used to update drive strength on card side
            //Package () { "fixed-emmc-driver-type", 2}, // mmc_of_parse()

            /* Host side drive strength and slew configurations.
            Include the following parameters if needed to override default values.
            Drive strength:
            0x0 = 55 ohm.
            0x1 = 45 ohm.
            0x2 = 33 ohm.     // default
            0x3 = 20 ohm.
            Slew:
            0x0 = Weakest.
            ...
            0x3 = Strongest.  // default */
            //Package () { "cdns,host_slew",  0x2}, // sdhci_cdns_sd6_phy_probe()
            //Package () { "cdns,host_drive", 0x3}, // sdhci_cdns_sd6_phy_probe()
        }
      })
     } // Scope EMMC

/*
        |          TWSI 2 bus
        |
 +------+------+
 |             |  TWSI 2
 |             |   bus    +-------------+
 |     P2P0    +---------->    0x68     |
 |             <----------+  Dallas RTC |
 |             |          +-------------+
 |             |
 +------+------+
        |
        |          TWSI 3 bus
        |
 +------+------+
 |             |
 |             |
 |             |  TWSI 3
 |             |   bus    +-------------+  GPIOS
 |     P2P0    +------+--->    0x3B     +------->
 |             <----+-----+ PCA8574APW  |
 |             |    | |   +-------------+
 |             |    | |
 |             |    | |   +-------------+
 |             |    | +--->    0x50     |
 +------+------+    +-----+ QSFP EEPROM |
        |                 +-------------+
        |
        |          TWSI 4 bus
        |
 +------+------+
 |             |
 |             |
 |             |  TWSI 4
 |             |   bus    +-------------+  GPIOS
 |     P2P0    +------+--->    0x3B     +------->
 |             <----+-----+ PCA8574APW  |
 |             |    | |   +-------------+
 |             |    | |
 |             |    | |   +-------------+
 |             |    | +--->    0x50     |
 +------+------+    +-----+  SFP EEPROM |
        |                 +-------------+
        |
        |          TWSI 5 bus
        |
 +------+------+
 |             |
 |             |
 |             |  TWSI 5
 |             |   bus    +-------------+  GPIOS
 |     P2P0    +------+--->    0x3B     +------->
 |             <----+-----+ PCA8574APW  |
 |             |    | |   +-------------+
 |             |    | |
 |             |    | |   +-------------+
 |             |    | +--->    0x50     |
 +------+------+    +-----+ QSFP EEPROM |
        |                 +-------------+
*/

     // -----------------------------------------
     // ----------------- TWSI 2 ----------------
     // -----------------------------------------

     External (\_SB.PCI0.P2P0.TWI2, DeviceObj) // TWI2 defined in Dsdt.asl
     Scope (\_SB.PCI0.P2P0.TWI2)
     {
        Device (RTC0)   // Dallas 1337 0x68
        {
            Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
            Name (_DDN, "Dallas 1337 RTC")
            Name (_UID, "Dallas 1337 RTC")
            Name (_CRS, ResourceTemplate () {
                I2CSerialBusV2(0x68,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI2")})

            Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
                Package () {"compatible", "dallas,ds1337"}, // linux/drivers/rtc/rtc-ds1307.c
              }
            })
        }
     }

     // -----------------------------------------
     // ----------------- TWSI 3 ----------------
     // -----------------------------------------

     External (\_SB.PCI0.P2P0.TWI3, DeviceObj) // TWI3 defined in Dsdt.asl
     Scope (\_SB.PCI0.P2P0.TWI3)
     {
        Device (GPIO)   // Address: 0x3B
        {
            Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
            Name (_DDN, "PCA8574APW I/O expander")
            Name (_UID, "PCA8574APW I/O expander")
            Name (_CRS, ResourceTemplate () {
              I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3")

              // Interrupt GPIO_50
              GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 50 }
            })

            Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
                Package () {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
              }
            })
        }

        Device (EEPR)   // SFP EEPROM address: 0x50
        {
            Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
            Name (_DDN, "Atmel AT24 compatible EEPROM")
            Name (_UID, "Atmel AT24 compatible EEPROM")
            Name (_CRS, ResourceTemplate() {
                I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3")})

            Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package() {
                Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
              }
            })
        }
     }

     // -----------------------------------------
     // ----------------- TWSI 4 ----------------
     // -----------------------------------------

     External (\_SB.PCI0.P2P0.TWI4, DeviceObj) // TWSI4
     Scope (\_SB.PCI0.P2P0.TWI4)
     {
        Device (GPIO)   // Address: 0x3B
        {
            Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
            Name (_UID, "PCA8574APW I/O expander")
            Name (_DDN, "PCA8574APW I/O expander")
            Name (_CRS, ResourceTemplate () {
              I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI4")

              // Interrupt GPIO_51
              GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 51 }
            })

            Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package() {
                Package() {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
              }
            })
        }

        Device (EEPR)   // SFP EEPROM address: 0x50
        {
            Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
            Name (_UID, "Atmel AT24 compatible EEPROM")
            Name (_DDN, "Atmel AT24 compatible EEPROM")
            Name (_CRS, ResourceTemplate() {
                I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI4")})

            Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package() {
                Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
              }
            })
        }
     }

     // -----------------------------------------
     // ----------------- TWSI 5 ----------------
     // -----------------------------------------
     External (\_SB.PCI0.P2P0, DeviceObj) // TWSI5
     Scope (\_SB.PCI0.P2P0)
     {
        Device (TWI5) {
          Name (_DDN, "TWSI Controller 5")
          Name (_ADR, 0x00180005) // Device#18h (24), Func#5 on bus (bus 1)
          Name (_UID, "TWSI Controller 5")

          Method (_STA)
          {
              Return(0xF)
          }

          Device (GPIO)   // Address: 0x3B
          {
              Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
              Name (_UID, "PCA8574APW I/O expander")
              Name (_DDN, "PCA8574APW I/O expander")
              Name (_CRS, ResourceTemplate () {
                I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI5")

                // Interrupt GPIO_52 EBB106
                GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                  "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 52 }
              })

              Name (_DSD, Package () {
                ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                Package () {
                  Package () {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
                }
              })
          }

          Device (EEPR)   // SFP EEPROM address: 0x50
          {
              Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
              Name (_UID, "Atmel AT24 compatible EEPROM")
              Name (_DDN, "Atmel AT24 compatible EEPROM")
              Name (_CRS, ResourceTemplate() {
                  I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI5")})

              Name (_DSD, Package () {
                ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                Package() {
                  Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
                }
              })
          } // EEPR
        }   // TWI5
     }      // Scope (\_SB.PCI0.P2P0)

     Scope (\_SB) {
        Device (PMU0) {
          Name (_HID, "MRVL000A")
          Name (_STR, Unicode ("MARVELL DDR PMU"))
          Name (_DSD, Package () {
           ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
           Package () {
            Package () { "compatible", "marvell,cn10k-ddr-pmu" },
            Package () { "marvell,ddr-speed", 0x00000fa0 },
           }
          })
          Method (_STA, 0, NotSerialized) {
           Return (0x0F)
          }

          Method (_CRS, 0, Serialized) {
           Name (RBUF, ResourceTemplate () {
            QWordMemory (ResourceConsumer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
            0x0000000000000000, // Granularity
            0x0000087E1C0000000, // Range Minimum
            0x0000087E1C000FFFF, // Range Maximum
            0x0000000000000000, // Translation Offset
            0x10000, // Length
            ,, , AddressRangeMemory, TypeStatic)
              })
              Return (RBUF)
            }
         }

         Device (PMU1) {
          Name (_HID, "MRVL000A")
          Name (_STR, Unicode ("MARVELL DDR PMU"))
          Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
            Package () { "compatible", "marvell,cn10k-ddr-pmu" },
            Package () { "marvell,ddr-speed", 0x00000fa0 },
           }
          })
          Method (_STA, 0, NotSerialized) {
           Return (0x0F)
          }

          Method (_CRS, 0, Serialized) {
           Name (RBUF, ResourceTemplate () {
            QWordMemory (ResourceConsumer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
            0x0000000000000000, // Granularity
            0x0000087E1C1000000, // Range Minimum
            0x0000087E1C100FFFF, // Range Maximum
            0x0000000000000000, // Translation Offset
            0x10000, // Length
            ,, , AddressRangeMemory, TypeStatic)
           })
             Return (RBUF)
            }
           }

         Device (PMU2) {
          Name (_HID, "MRVL000A")
          Name (_STR, Unicode ("MARVELL DDR PMU"))
          Name (_DSD, Package () {
           ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
                Package () { "compatible", "marvell,cn10k-ddr-pmu" },
                Package () { "marvell,ddr-speed", 0x00000fa0 },
              }
            })
            Method (_STA, 0, NotSerialized) {
              Return (0x0F)
            }

          Method (_CRS, 0, Serialized) {
           Name (RBUF, ResourceTemplate () {
                QWordMemory (ResourceConsumer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                0x0000000000000000, // Granularity
            0x0000087E1C2000000, // Range Minimum
            0x0000087E1C200FFFF, // Range Maximum
            0x0000000000000000, // Translation Offset
            0x10000, // Length
            ,, , AddressRangeMemory, TypeStatic)
              })
                Return (RBUF)
            }
          }

         Device (PMU3) {
          Name (_HID, "MRVL000A")
          Name (_STR, Unicode ("MARVELL DDR PMU"))
          Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
                Package () { "compatible", "marvell,cn10k-ddr-pmu" },
                Package () { "marvell,ddr-speed", 0x00000fa0 },
              }
            })
            Method (_STA, 0, NotSerialized) {
              Return (0x0F)
            }

            Method (_CRS, 0, Serialized) {
              Name (RBUF, ResourceTemplate () {
                QWordMemory (ResourceConsumer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                  0x0000000000000000, // Granularity
                  0x0000087E1C3000000, // Range Minimum
                  0x0000087E1C300FFFF, // Range Maximum
                  0x0000000000000000, // Translation Offset
                  0x10000, // Length
                  ,, , AddressRangeMemory, TypeStatic)
              })
              Return (RBUF)
            }
          }

         Device (PMU4) {
            Name (_HID, "MRVL000A")
            Name (_STR, Unicode ("MARVELL DDR PMU"))
            Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
                Package () { "compatible", "marvell,cn10k-ddr-pmu" },
                Package () { "marvell,ddr-speed", 0x00000fa0 },
                }
              })
              Method (_STA, 0, NotSerialized) {
                Return (0x0F)
              }

              Method (_CRS, 0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                  QWordMemory (ResourceConsumer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                    0x0000000000000000, // Granularity
                    0x0000087E1C4000000, // Range Minimum
                    0x0000087E1C400FFFF, // Range Maximum
                    0x0000000000000000, // Translation Offset
                    0x10000, // Length
                    ,, , AddressRangeMemory, TypeStatic)
                })
                 Return (RBUF)
              }
          }

         Device (PMU5) {
            Name (_HID, "MRVL000A")
            Name (_STR, Unicode ("MARVELL DDR PMU"))
            Name (_DSD, Package () {
              ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
              Package () {
                Package () { "compatible", "marvell,cn10k-ddr-pmu" },
                Package () { "marvell,ddr-speed", 0x00000fa0 },
              }
            })
            Method (_STA, 0, NotSerialized) {
              Return (0x0F)
            }

            Method (_CRS, 0, Serialized) {
              Name (RBUF, ResourceTemplate () {
                QWordMemory (ResourceConsumer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                  0x0000000000000000, // Granularity
                  0x0000087E1C5000000, // Range Minimum
                  0x0000087E1C500FFFF, // Range Maximum
                  0x0000000000000000, // Translation Offset
                  0x10000, // Length
                  ,, , AddressRangeMemory, TypeStatic)
              })
                Return (RBUF)
              }
          }

         Device (PMU6) {
          Name (_HID, "MRVL000B")
          Name (_STR, Unicode ("MARVELL TAD PMU"))
          Name (_DSD, Package () {
           ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
           Package () {
            Package () { "compatible", "marvell,cn10k-tad-pmu"},
            Package () { "marvell,tad-cnt", 0x30},
            Package () { "marvell,tad-page-size", 0x01000000},
            Package () { "marvell,tad-pmu-page-size", 0x00001000},
              }
            })
            Method (_STA, 0, NotSerialized) {
              Return (0x0F)
            }

            Method (_CRS, 0, Serialized) {
              Name (RBUF, ResourceTemplate () {
            QWordMemory (ResourceConsumer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                  0x0000000000000000, // Granularity
                  0x000087E280000000, // Range Minimum
                  0x000087E2AFFFFFFF, // Range Maximum
                  0x0000000000000000, // Translation Offset
                  0x30000000, // Length
                  ,, , AddressRangeMemory, TypeStatic)
              })
                Return (RBUF)
              }
            }
      } // \_SB
 }
