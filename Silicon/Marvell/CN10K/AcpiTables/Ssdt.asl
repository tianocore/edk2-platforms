/*
 * Copyright (c) 2021-2026, Marvell International All rights reserved.
 *
 * CN10K [SSDT] ACPI Tables for Two Wire Serial Interface
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

/*
        |          TWSI 2 bus for EBB106, EBB106S
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
        |          TWSI 3 bus for EBB106
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
        |
        |          TWSI 3 bus for EBB106S
        |
 +------+------+
 |             |          +------------+
 |             |          |            |  CH00  +-----------+
 |             |          |            +-------->           |
 |             |  TWSI 3  |            <--------+ PCIe      |
 |             |   bus    |    0x71    |        | 16x Conn  |
 |     P2P0    +----------> PCA9548APW |        +-----------+
 |             <----------+     I2C    |
 |             |          | 8-ch switch|  CH01  +-----------+
 |             |          |            +-------->           |
 |             |          |            <--------+ PCIe      |
 |             |          |            |        | 16x Conn  |
 +------+------+          |            |        +-----------+
        |                 |            |
        |                 |            |  CH02  +-----------+
        |                 |            +-------->           |
        |                 |            <--------+ PCIe      |
        |                 |            |        | 16x Conn  |
        |                 |            |        +-----------+
        |                 |            |
        |                 |            |  CH03  +-----------+
        |                 |            +-------->           |
        |                 |            <--------+ Empty     |
        |                 |            |        |           |
        |                 |            |        +-----------+
        |                 |            |
        |                 |            |  CH04    +-------------+  GPIOS
        |                 |            +------+--->    0x3B     +------->
        |                 |            <----+-----+ PCA8574APW  |
        |                 |            |    | |   +-------------+
        |                 |            |    | |
        |                 |            |    | |   +-------------+
        |                 |            |    | +--->    0x50     |
        |                 |            |    +-----+ QSFP EEPROM |
        |                 |            |          +-------------+
        |                 |            |
        |                 |            |  CH05    +-------------+  GPIOS
        |                 |            +------+--->    0x3B     +------->
        |                 |            <----+-----+ PCA8574APW  |
        |                 |            |    | |   +-------------+
        |                 |            |    | |
        |                 |            |    | |   +-------------+
        |                 |            |    | +--->    0x50     |
        |                 |            |    +-----+ QSFP EEPROM |
        |                 |            |          +-------------+
        |                 |            |
        |                 |            |  CH06    +-------------+  GPIOS
        |                 |            +------+--->    0x3B     +------->
        |                 |            <----+-----+ PCA8574APW  |
        |                 |            |    | |   +-------------+
        |                 |            |    | |
        |                 |            |    | |   +-------------+
        |                 |            |    | +--->    0x50     |
        |                 |            |    +-----+ QSFP EEPROM |
        |                 |            |          +-------------+
        |                 |            |
        |                 |            |  CH07    +-------------+  GPIOS
        |                 |            +------+--->    0x3B     +------->
        |                 |            <----+-----+ PCA8574APW  |
        |                 |            |    | |   +-------------+
        |                 +------------+    | |
        |                                   | |   +-------------+
        |                                   | +--->    0x50     |
        |                                   +-----+ QSFP EEPROM |
        |                                         +-------------+
        |
        |          TWSI 4 bus for EBB106, EBB106S
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
        |          TWSI 5 bus (only for EBB106, not used for EBB106S)
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

#define TWSI_SPEED 100000

 DefinitionBlock ("", "SSDT", 2, "MRVL", "CN10KA", 0x00000001)
 {
     // -----------------------------------------
     // ----- TWSI 2 (for EBB106, EBB106S) ------
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
     // ------- TWSI 3 (for EBB106) -------
     // -----------------------------------------

     External (\_SB.PCI0.P2P0.TWI3, DeviceObj) // TWSI3
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
     // --------- TWSI 3 (for EBB106S) ----------
     // -----------------------------------------
/*
     External (\_SB.PCI0.P2P0.TWI3, DeviceObj) // TWI3 defined in Dsdt.asl
     Scope (\_SB.PCI0.P2P0.TWI3)
     {
        Device (MUX0)  // Switch
        {
            Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
            Name (_DDN, "PCA9548APW I2C 8 channel switch")
            Name (_CRS, ResourceTemplate () {
                I2CSerialBusV2(0x71,,TWSI_SPEED,,"\\_SB.PCI0.P2P0.TWI3")})

            Name (_DSD, Package () {
                ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                Package () {
                    Package () {"compatible", "nxp,pca9548"},
                }
            })

            Device (CH00)   // PCIE_S1_SCL, PCIE_S1_SDA -> PCIe 16x Connector
            {
                Name (_ADR, 0)
            }

            Device (CH01)   // PCIE_S2_SCL, PCIE_S2_SDA -> PCIe 16x Connector
            {
                Name (_ADR, 1)
            }

            Device (CH02)   // PCIE_S3_SCL, PCIE_S3_SDA -> PCIe 16x Connector
            {
                Name (_ADR, 2)
            }

            Device (CH03)   // Empty
            {
                Name (_ADR, 3)
            }

            Device (CH04)   // SW_QSFP1_I2C_SCL, SW_QSFP1_I2C_SSDA -> Switch ports [3:0] ZQSFP
                            // I2C ADDR 0x3B: PCA8574APW (I2C_8PORT_EXPANDER),
                            // I2C ADDR 0x50: zQSFP EEPROM
            {
                Name (_ADR, 4)

                Device (GPIO) // Address: 0x3B
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "PCA8574APW I/O expander")
                    Name (_CRS, ResourceTemplate () {
                      I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH04")

                      // Interrupt GPIO_49
                      GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                        "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 49 }
                    })

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
                      }
                    })
                }

                Device (EEPR)   // QSFP EEPROM address: 0x50
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "Atmel AT24 compatible EEPROM")
                    Name (_CRS, ResourceTemplate () {
                        I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH04")})

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

            Device (CH05)   // SW_QSFP2_I2C_SCL, SW_QSFP2_I2C_SSDA -> Switch ports [7:4] ZQSFP
                            // I2C ADDR 0x3B: PCA8574APW (I2C_8PORT_EXPANDER),
                            // I2C ADDR 0x50: zQSFP_PLUS_CONN
            {
                Name (_ADR, 5)

                Device (GPIO) // Address: 0x3B
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "PCA8574APW I/O expander")
                    Name (_CRS, ResourceTemplate () {
                      I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH05")

                      // Interrupt GPIO_50
                      GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                        "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 50 }
                    })

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
                      }
                    })
                }

                Device (EEPR)   // QSFP EEPROM address: 0x50
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "Atmel AT24 compatible EEPROM")
                    Name (_CRS, ResourceTemplate () {
                        I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH05")})

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
                      }
                    })
                }
            }

            Device (CH06)   // SW_QSFP3_I2C_SCL, SW_QSFP3_I2C_SSDA -> Switch ports [11:8] ZQSFP
                            // I2C ADDR 0x3B: PCA8574APW (I2C_8PORT_EXPANDER),
                            // I2C ADDR 0x50: zQSFP_PLUS_CONN
            {
                Name (_ADR, 6)

                Device (GPIO)   // Address: 0x3B
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "PCA8574APW I/O expander")
                    Name (_CRS, ResourceTemplate () {
                      I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH06")

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

                Device (EEPR)   // QSFP EEPROM address: 0x50
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "Atmel AT24 compatible EEPROM")
                    Name (_CRS, ResourceTemplate () {
                        I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH06")})

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
                      }
                    })
                }
            }

            Device (CH07)   // SW_QSFP4_I2C_SCL, SW_QSFP4_I2C_SSDA -> Switch ports [15:12] ZQSFP
                            // I2C ADDR 0x3B: PCA8574APW (I2C_8PORT_EXPANDER),
                            // I2C ADDR 0x50: zQSFP_PLUS_CONN
            {
                Name (_ADR, 7)

                Device (GPIO)   // Address: 0x3B
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "PCA8574APW I/O expander")
                    Name (_CRS, ResourceTemplate () {
                      I2CSerialBusV2(0x3B,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH07")

                      // Interrupt GPIO_52
                      GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                        "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 52 }
                    })

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible","nxp,pca8574"},// linux/drivers/gpio/gpio-pcf857x.c
                      }
                    })
                }

                Device (EEPR)   // QSFP EEPROM address: 0x50
                {
                    Name (_HID, "PRP0001") // ACPI_DT_NAMESPACE_HID
                    Name (_DDN, "Atmel AT24 compatible EEPROM")
                    Name (_CRS, ResourceTemplate () {
                        I2CSerialBusV2(0x50,,TWSI_SPEED,, "\\_SB.PCI0.P2P0.TWI3.MUX0.CH07")})

                    Name (_DSD, Package () {
                      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                      Package() {
                        Package() {"compatible", "atmel,24c01"},// linux/drivers/misc/eeprom/at24.c
                      }
                    })
                }
            }
        }
     }
*/
     // -----------------------------------------
     // ----- TWSI 4 (for EBB106, EBB106S) ------
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

              // Interrupt GPIO_51 EBB106
              GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
                "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 51 }
              // Interrupt GPIO_53 EBB106S
              //GpioInt(Level, ActiveLow, Exclusive, PullUp, 0x0000,
              //  "\\_SB.PCI0.GPIO", 0x00, ResourceConsumer, , ) { 53 }
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
     // ------- TWSI 5 (only for EBB106) -------
     // -----------------------------------------

     External (\_SB.PCI0.P2P0.TWI5, DeviceObj) // TWSI5
     Scope (\_SB.PCI0.P2P0.TWI5)
     {
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

            // Interrupt GPIO_52 EBB106
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
        }
     }
 }
