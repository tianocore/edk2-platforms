/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock("Dsdt.aml", "DSDT", 0x02, "ADLINK", "AADP", 1) {
  //
  // Board Model
  Name(\BDMD, "AADP Board")
  Name(TPMF, 0)  // TPM presence
  Name(AERF, 0)  // PCIe AER Firmware-First

  Scope(\_SB) {

    Include ("CommonDevices.asi")

    Scope(\_SB.GED0) {
        Method(_EVT, 1, Serialized) {
          Switch (ToInteger(Arg0)) {
            Case (84) { // GHES interrupt
              Notify (HED0, 0x80)
            }
          }
        }
    }

    Include ("PCI-S0.asi")
    Include ("PCI-PDRC.asi")
  }

  Include ("CPU.asi")
  Include ("PMU.asi")

} // DSDT
