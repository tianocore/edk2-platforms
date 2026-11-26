// SPDX-License-Identifier: BSD-2-Clause-Patent
// https://spdx.org/licenses
//
// Copyright (C) 2023-2026 Marvell
//
// SSDT ACPI Table
//
// IMPORTANT: 'OEM Table ID' must match with mOemTableId (see AcpiPlatform.c)

#define TWSI_SPEED 100000 // clock-frequency = <100000>; in dts/cn10kb-common-linux.dtsi

 DefinitionBlock ("", "SSDT", 2, "MRVL", "ebb-103", 0x00000001)
 {
     // https://www.kernel.org/doc/html/v5.2/firmware-guide/acpi/enumeration.html
     // SPI serial bus support:
     // Child devices behind SPI bus have SpiSerialBus resource attached to them.
     // This is extracted automatically by the SPI core and the child devices
     // are enumerated once spi_register_master() is called by the bus driver.

     External (\_SB.SPI0, DeviceObj) // SPI0 defined in Dsdt.asl
     Scope (\_SB.SPI0)
     {
        // SPI defined for ebb102, ebb103 (see dts/ebb102-linux.dts, dts/ebb103-linux.dts)
        Device (FLA0) {               // Flash 0: MX25UM51245GXDI00
          Name (_HID, "PRP0001")      // ACPI_DT_NAMESPACE_HID
          Name (_DDN, "SPI Flash MX25UM51245GXDI00")  // Optional

          Method (_STA) {Return (0xF)}

          // Provide configuration data for SPI controller.
          Name (_CRS, ResourceTemplate() {
            // Refer to:
            // linux/Documentation/devicetree/bindings/spi/spi-controller.yaml
            // and dts/ebb102-linux.dts/dts/ebb103-linux.dts to set proper values of SpiSerialBus.
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
        // SPI defined for ebb102, ebb103 (see dts/ebb102-linux.dts, dts/ebb103-linux.dts)
        Device (FLA0) {               // Flash 0: MX25U51245GXDI00
          Name (_HID, "PRP0001")      // ACPI_DT_NAMESPACE_HID
          Name (_DDN, "SPI Flash MX25U51245GXDI00")  // Optional

          Method (_STA) {Return (0xF)}

          // Provide configuration data for SPI controller.
          Name (_CRS, ResourceTemplate() {
            // Refer to:
            // linux/Documentation/devicetree/bindings/spi/spi-controller.yaml
            // and dts/ebb102-linux.dts/dts/ebb103-linux.dts to set proper values of SpiSerialBus.
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
      // EMMC properties for ebb102, ebb103 (see dts/ebb102-linux.dts, dts/ebb103-linux.dts)
      Name (_DSD, Package() {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            // See dts/cn10kb-common-linux.dtsi
            Package () { "compatible", "cdns,sd6hc"},
            // The I/O PLL generates a fixed frequency of 800 MHz clock
            // EMMC_CLK = I/O PLL / (2 x 2) // CN103XX Clocking Diagram, Part 1
            Package () { "clock-frequency",          200000000}, // sdhci_cdns_sd6_phy_probe()
            Package () { "sdhci-caps-mask", 0x00FF000000000000}, // __sdhci_read_caps()
            Package () { "sdhci-caps",      0x800000771F6EC8A0}, // __sdhci_read_caps()
            // See ebb102, ebb103 (see dts/ebb102-linux.dts, dts/ebb103-linux.dts)
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
        |          TWSI 1 bus
        |
 +------+------+
 |             |          +------------+
 |             |          |            |  CH00  +-----------+
 |             |          |            +-------->           |
 |             |  TWSI 1  |            <--------+ Empty     |
 |             |   bus    |    0x70    |        |           |
 |     P2P0    +----------> PCA9548APW |        +-----------+
 |             <----------+     I2C    |
 |             |          | 8-ch switch|  CH01  +-----------+
 |             |          |            +-------->           |
 |             |          |            <--------+ Empty     |
 |             |          |            |        |           |
 +------+------+          |            |        +-----------+
        |                 |            |
        |                 |            |  CH02  +-----------+
        |                 |            +-------->           |
        |                 |            <--------+ Empty     |
        |                 |            |        |           |
        |                 |            |        +-----------+
        |                 |            |
        |                 |            |  CH03    +-------------+  GPIOS
        |                 |            +------+--->    0x3B     +------->
        |                 |            <----+-----+ PCA8574APW  |
        |                 |            |    | |   +-------------+
        |                 |            |    | |
        |                 |            |    | |   +--------------+
        |                 |            |    | +--->    0x50      |
        |                 |            |    +-----+ zSFPP EEPROM |
        |                 |            |          +--------------+
        |                 |            |
        |                 |            |  CH04    +-------------+  GPIOS
        |                 |            +------+--->    0x3B     +------->
        |                 |            <----+-----+ PCA8574APW  |
        |                 |            |    | |   +-------------+
        |                 |            |    | |
        |                 |            |    | |   +--------------+
        |                 |            |    | +--->    0x50      |
        |                 |            |    +-----+ zSFPP EEPROM |
        |                 |            |          +--------------+
        |                 |            |
        |                 |            |  CH05    +-------------+  GPIOS
        |                 |            +------+--->    0x3B     +------->
        |                 |            <----+-----+ PCA8574APW  |
        |                 |            |    | |   +-------------+
        |                 |            |    | |
        |                 |            |    | |   +--------------+
        |                 |            |    | +--->    0x50      |
        |                 |            |    +-----+ zSFPP EEPROM |
        |                 |            |          +--------------+
        |                 |            |
        |                 |            |  CH06  +-----------+
        |                 |            +-------->           |
        |                 |            <--------+ Empty     |
        |                 |            |        |           |
        |                 |            |        +-----------+
        |                 |            |
        |                 |            |  CH07  +-----------+
        |                 |            +-------->           |
        |                 |            <--------+ Empty     |
        |                 |            |        |           |
        |                 |            |        +-----------+
        |                 |            |
        |                 +------------+
        |
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
*/

     // -----------------------------------------
     // ----------------- TWSI 1 ----------------
     // -----------------------------------------

     External (\_SB.PCI0.P2P0, DeviceObj)
     Scope (\_SB.PCI0.P2P0)
     {
      Device (TWI1) {
        Name (_DDN, "TWSI Controller 1")
        Name (_ADR, 0x00180001) // Device#18h (24), Func#1 on bus (bus 1)
        Name (_UID, "TWSI Controller 1")

        Method (_STA)
        {
            Return(0xF)
        }

        Device (MUX0)  // Switch
        {
            Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
            Name (_DDN, "PCA9548APW I2C 8 channel switch")
            Name (_CRS, ResourceTemplate () {
                I2CSerialBusV2(0x70,,TWSI_SPEED,,"\\_SB.PCI0.P2P0.TWI1")})

            Name (_DSD, Package () {
                ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                Package () {
                    Package () {"compatible", "nxp,pca9548"},
                }
            })

            Device (CH00)   // Empty
            {
                Name (_ADR, 0)
            }

            Device (CH01)   // Empty
            {
                Name (_ADR, 1)
            }

            Device (CH02)   // Empty
            {
                Name (_ADR, 2)
            }

            Device (CH03)   // GSERM0_SDA:
                            // I2C ADDR 0x3B: PCA8574APW (I2C_8PORT_EXPANDER),
                            // I2C ADDR 0x50: zSFPP EEPROM
            {
                Name (_ADR, 3)

                Device (GPIO) // Address: 0x3B
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "PCA8574APW I/O expander")
                    Name (_CRS, ResourceTemplate () {
                      I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH03")

                      // https://docs.kernel.org/firmware-guide/acpi/gpio-properties.html
                      //Index 0, Pin 0
                      // GPIO Pin 0 GSERM0_DET_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO",) {0}
                      //Index 1, Pin 0
                      // GPIO Pin 1 GSERM0_RX_LOS_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO",) {1}
                      //Index 2, Pin 0
                      // GPIO Pin 2 GSERM0_TX_DIS: see schematics
                      GpioIo (Exclusive, PullDefault, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO",) {2}
                      //Index 3, Pin 0
                      // GPIO Pin 3 GSERM0_TX_FAULT_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO",) {3}
                      //Index 4, Pin 0
                      // GPIO Pin 6 GSERM0_LINK_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH03.GPIO",) {6}

                      // Interrupt GPIO_37: see schematics
                      GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                        "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 37 }
                    })

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
                      }
                    })
                }

                Device (EEPR)   // zSFPP EEPROM address: 0x50
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "Atmel AT24 compatible EEPROM")
                    Name (_CRS, ResourceTemplate () {
                        I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH03")})

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
                        //Package () {"size", 128},             // Optional: 128 bytes
                        //Package () {"pagesize", 8}            // Optional: 8 bytes page size
                      }
                    })
                }
            }

            Device (CH04)   // GSERM1_SDA:
                            // I2C ADDR 0x3B: PCA8574APW (I2C_8PORT_EXPANDER),
                            // I2C ADDR 0x50: zSFPP EEPROM
            {
                Name (_ADR, 4)

                Device (GPIO) // Address: 0x3B
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "PCA8574APW I/O expander")
                    Name (_CRS, ResourceTemplate () {
                      I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH04")

                      // https://docs.kernel.org/firmware-guide/acpi/gpio-properties.html
                      //Index 0, Pin 0
                      // GPIO Pin 0 GSERM1_DET_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO",) {0}
                      //Index 1, Pin 0
                      // GPIO Pin 1 GSERM1_RX_LOS_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO",) {1}
                      //Index 2, Pin 0
                      // GPIO Pin 2 GSERM1_TX_DIS: see schematics
                      GpioIo (Exclusive, PullDefault, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO",) {2}
                      //Index 3, Pin 0
                      // GPIO Pin 3 GSERM1_TX_FAULT_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO",) {3}
                      //Index 4, Pin 0
                      // GPIO Pin 6 GSERM1_LINK_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH04.GPIO",) {6}

                      // Interrupt GPIO_44: see schematics
                      GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                        "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 44 }
                    })

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
                      }
                    })
                }

                Device (EEPR)   // zSFPP EEPROM address: 0x50
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "Atmel AT24 compatible EEPROM")
                    Name (_CRS, ResourceTemplate () {
                        I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH04")})

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
                        //Package () {"size", 128},             // Optional: 128 bytes
                        //Package () {"pagesize", 8}            // Optional: 8 bytes page size
                      }
                    })
                }
            }

            Device (CH05)   // GSERM2_SDA:
                            // I2C ADDR 0x3B: PCA8574APW (I2C_8PORT_EXPANDER),
                            // I2C ADDR 0x50: zQSFPP EEPROM
            {
                Name (_ADR, 5)

                Device (GPIO) // Address: 0x3B
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "PCA8574APW I/O expander")
                    Name (_CRS, ResourceTemplate () {
                      I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH05")

                      // https://docs.kernel.org/firmware-guide/acpi/gpio-properties.html
                      // GPIO Pin 0 GSERM2_PRSNT_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionInputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH05.GPIO",) {0}
                      // GPIO Pin 1 QSFP0_INT_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH05.GPIO",) {1}
                      // GPIO Pin 2 QSFP0_MODSEL_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH05.GPIO",) {2}
                      // GPIO Pin 3 QSFP0_LPMODE: see schematics
                      GpioIo (Exclusive, PullDefault, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH05.GPIO",) {3}
                      // GPIO Pin 4 QSFP0_RESET_L: see schematics
                      GpioIo (Exclusive, PullDown, 0, 0, IoRestrictionOutputOnly, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH05.GPIO",) {4}

                      // Interrupt GPIO_45: see schematics
                      GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                        "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 45 }
                    })

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
                      }
                    })
                }

                Device (EEPR)   // zQSFPP EEPROM address: 0x50
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "Atmel AT24 compatible EEPROM")
                    Name (_CRS, ResourceTemplate () {
                        I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI1.MUX0.CH05")})

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
                        //Package () {"size", 128},             // Optional: 128 bytes
                        //Package () {"pagesize", 8}            // Optional: 8 bytes page size
                      }
                    })
                }
            }

            Device (CH06)   // Empty
            {
                Name (_ADR, 6)
            }

            Device (CH07)   // Empty
            {
                Name (_ADR, 7)
            }
        } // Device (MUX0)
      }   // Device (TWI1)
     }    // Scope (\_SB.PCI0.P2P0)

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
 }
