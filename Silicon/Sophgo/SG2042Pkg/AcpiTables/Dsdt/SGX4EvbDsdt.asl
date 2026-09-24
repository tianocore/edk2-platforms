/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "SG2042AcpiHeader.h"

DefinitionBlock ("DsdtTable.aml", "DSDT", 2, "SOPHGO", "SG20    ",
                 EFI_ACPI_RISCV_OEM_REVISION) {
  include ("Cpu.asl")
  include ("CommonDevices.asl")
  include ("Uart.asl")
  include ("Mmc.asl")
  //include ("Ethernet.asl")
  include ("Intc.asl")
  include ("Pci.asl")

  Scope (\_SB_.I2C1)
  {
    Device (MCU0) {
      Name (_HID, "SG200022")
      Method(_STA, 0, NotSerialized) {
        Return (0x0f)
      }
      Method(_ADR) {
        Return(0x17) // MCU slave address
      }
      Name (_CRS, ResourceTemplate () {
        I2cSerialBusV2 (0x17, ControllerInitiated, 0x00061A80, AddressingMode7Bit, "\\_SB.I2C1", 0, ResourceConsumer,,)
      })
      Name (_DSD, Package () {
        ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (0x02) { "reg", 0x17 },
        }
      })
    }
  }
}
