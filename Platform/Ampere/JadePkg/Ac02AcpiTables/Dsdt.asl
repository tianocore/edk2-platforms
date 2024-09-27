/** @file

  Copyright (c) 2023 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock("Dsdt.aml", "DSDT", 0x02, "Ampere", "Jade", 1) {
  //
  // Board Model
  Name(\BDMD, "Altra Max Jade Board")
  Name(TPMF, 0)  // TPM presence
  Name(AERF, 0)  // PCIe AER Firmware-First

  Include ("MHPP.asi")

  Scope(\_SB) {

    Include ("CommonDevices.asi")

    Scope(\_SB.GED0) {
        Method(_EVT, 1, Serialized) {
          Switch (ToInteger(Arg0)) {
            Case (84) { // GHES interrupt
              Notify (HED0, 0x80)
            }

            Case (44) { // doorbell notification (Insertion/ejection)
              local0 = DOUT & 0x00FF0000
              if (local0 == 0x00010000) {
                local0 = STA4 & 0xFFFFFFFF
                if (local0) {
                  Store(local0, STA4) // clear interrupt
                }
                local0 = A050 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P1, 1) // insertion action
                }
                local0 = A051 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P2, 1) // insertion action
                }
                local0 = A052 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P3, 1) // insertion action
                }
                local0 = A053 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P4, 1) // insertion action
                }
                local0 = A040 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P1, 1) // insertion action
                }
                local0 = A041 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P2, 1) // insertion action
                }
                local0 = A042 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P3, 1) // insertion action
                }
                local0 = A043 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P4, 1) // insertion action
                }
                local0 = A160 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P1, 1) // insertion action
                }
                local0 = A161 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P2, 1) // insertion action
                }
                local0 = A162 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P3, 1) // insertion action
                }
                local0 = A163 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P4, 1) // insertion action
                }

                local0 = A170 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P1, 1) // insertion action
                }
                local0 = A171 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P2, 1) // insertion action
                }
                local0 = A172 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P3, 1) // insertion action
                }
                local0 = A173 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P4, 1) // insertion action
                }

                local0 = A150 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P1, 1) // insertion action
                }
                local0 = A151 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P2, 1) // insertion action
                }
                local0 = A152 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P3, 1) // insertion action
                }
                local0 = A153 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P4, 1) // insertion action
                }


                local0 = A120 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P1, 1) // insertion action
                }
                local0 = A121 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P2, 1) // insertion action
                }
                local0 = A122 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P3, 1) // insertion action
                }
                local0 = A123 & 0xFF000000
                if ((local0 == 0x01000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P4, 1) // insertion action
                }
              }
              elseif (local0 == 0x00000000) {
                local0 = STA4 & 0xFFFFFFFF
                if (local0) {
                  Store(local0, STA4) // clear interrupt
                }
                local0 = A050 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI5.P2P1, 1) // ejection action
                  A050 = 0x10000500 // inprogress remove
                }
                local0 = A051 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI5.P2P2, 1) // ejection action
                  A051 = 0x10010500 // inprogress remove
                }
                local0 = A052 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI5.P2P3, 1) // ejection action
                  A052 = 0x10020500 // inprogress remove
                }
                local0 = A053 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI5.P2P4, 1) // ejection action
                  A053 = 0x10030500 // inprogress remove
                }
                local0 = A040 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI4.P2P1, 1) // ejection action
                  A040 = 0x10000400 // inprogress remove
                }
                local0 = A041 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI4.P2P2, 1) // ejection action
                  A041 = 0x10010400 // inprogress remove
                }
                local0 = A042 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI4.P2P3, 1) // ejection action
                  A042 = 0x10020400 // inprogress remove
                }
                local0 = A043 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCI4.P2P4, 1) // ejection action
                  A043 = 0x10030400 // inprogress remove
                }
                local0 = A160 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIE.P2P1, 1) // ejection action
                  A160 = 0x10000601 // inprogress remove
                }
                local0 = A161 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIE.P2P2, 1) // ejection action
                  A161 = 0x10010601 // inprogress remove
                }
                local0 = A162 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIE.P2P3, 1) // ejection action
                  A162 = 0x10020601 // inprogress remove
                }
                local0 = A163 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIE.P2P4, 1) // ejection action
                  A163 = 0x10030601 // inprogress remove
                }

                local0 = A170 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIF.P2P1, 1) // ejection action
                  A170 = 0x10000701 // inprogress remove
                }
                local0 = A171 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIF.P2P2, 1) // ejection action
                  A171 = 0x10010701 // inprogress remove
                }
                local0 = A172 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIF.P2P3, 1) // ejection action
                  A172 = 0x10020701 // inprogress remove
                }
                local0 = A173 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIF.P2P4, 1) // ejection action
                  A173 = 0x10030701 // inprogress remove
                }

                local0 = A150 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCID.P2P1, 1) // ejection action
                  A150 = 0x10000501 // inprogress remove
                }
                local0 = A151 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCID.P2P2, 1) // ejection action
                  A151 = 0x10010501 // inprogress remove
                }
                local0 = A152 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCID.P2P3, 1) // ejection action
                  A152 = 0x10020501 // inprogress remove
                }
                local0 = A153 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCID.P2P4, 1) // ejection action
                  A153 = 0x10030501 // inprogress remove
                }

                local0 = A120 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIA.P2P1, 1) // ejection action
                  A120 = 0x10000201 // inprogress remove
                }
                local0 = A121 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIA.P2P2, 1) // ejection action
                  A121 = 0x10010201 // inprogress remove
                }
                local0 = A122 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIA.P2P3, 1) // ejection action
                  A122 = 0x10020201 // inprogress remove
                }
                local0 = A123 & 0xFF000000
                if (local0 == 0x00000000) {
                  Notify (\_SB.PCIA.P2P4, 1) // ejection action
                  A123 = 0x10030201 // inprogress remove
                }
              }
              elseif (local0 == 0x00020000) { // is either unstable link or too fast insertion?
                local0 = STA4 & 0xFFFFFFFF
                if (local0) {
                  Store(local0, STA4) // clear interrupt
                }
                local0 = A050 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P1, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI5.P2P1, 1) // insertion action
                }
                local0 = A051 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P2, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI5.P2P2, 1) // insertion action
                }
                local0 = A052 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P3, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI5.P2P3, 1) // insertion action
                }
                local0 = A053 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P4, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI5.P2P4, 1) // insertion action
                }
                local0 = A040 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P1, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI4.P2P1, 1) // insertion action
                }
                local0 = A041 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P2, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI4.P2P2, 1) // insertion action
                }
                local0 = A042 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P3, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI4.P2P3, 1) // insertion action
                }
                local0 = A043 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P4, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCI4.P2P4, 1) // insertion action
                }
                local0 = A160 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P1, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIE.P2P1, 1) // insertion action
                }
                local0 = A161 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P2, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIE.P2P2, 1) // insertion action
                }
                local0 = A162 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P3, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIE.P2P3, 1) // insertion action
                }
                local0 = A163 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P4, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIE.P2P4, 1) // insertion action
                }

                local0 = A170 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P1, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIF.P2P1, 1) // insertion action
                }
                local0 = A171 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P2, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIF.P2P2, 1) // insertion action
                }
                local0 = A172 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P3, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIF.P2P3, 1) // insertion action
                }
                local0 = A173 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P4, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIF.P2P4, 1) // insertion action
                }

                local0 = A150 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P1, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCID.P2P1, 1) // insertion action
                }
                local0 = A151 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P2, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCID.P2P2, 1) // insertion action
                }
                local0 = A152 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P3, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCID.P2P3, 1) // insertion action
                }
                local0 = A153 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P4, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCID.P2P4, 1) // insertion action
                }

                local0 = A120 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P1, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIA.P2P1, 1) // insertion action
                }
                local0 = A121 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P2, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIA.P2P2, 1) // insertion action
                }
                local0 = A122 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P3, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIA.P2P3, 1) // insertion action
                }
                local0 = A123 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P4, 3) // force remove bridge port and port itself
                  Notify (\_SB.PCIA.P2P4, 1) // insertion action
                }
              }
              elseif (local0 == 0x00030000) { // got incomplete remove before?
                local0 = STA4 & 0xFFFFFFFF
                if (local0) {
                  Store(local0, STA4) // clear interrupt
                }
                local0 = A050 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P1.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI5.P2P1, 1) // insertion action
                }
                local0 = A051 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P2.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI5.P2P2, 1) // insertion action
                }
                local0 = A052 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P3.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI5.P2P3, 1) // insertion action
                }
                local0 = A053 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI5.P2P4.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI5.P2P4, 1) // insertion action
                }
                local0 = A040 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P1.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI4.P2P1, 1) // insertion action
                }
                local0 = A041 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P2.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI4.P2P2, 1) // insertion action
                }
                local0 = A042 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P3.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI4.P2P3, 1) // insertion action
                }
                local0 = A043 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCI4.P2P4.S0F0, 3) // remove port itself
                  Notify (\_SB.PCI4.P2P4, 1) // insertion action
                }
                local0 = A160 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P1.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIE.P2P1, 1) // insertion action
                }
                local0 = A161 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P2.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIE.P2P2, 1) // insertion action
                }
                local0 = A162 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P3.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIE.P2P3, 1) // insertion action
                }
                local0 = A163 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIE.P2P4.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIE.P2P4, 1) // insertion action
                }

                local0 = A170 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P1.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIF.P2P1, 1) // insertion action
                }
                local0 = A171 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P2.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIF.P2P2, 1) // insertion action
                }
                local0 = A172 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P3.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIF.P2P3, 1) // insertion action
                }
                local0 = A173 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIF.P2P4.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIF.P2P4, 1) // insertion action
                }

                local0 = A150 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P1.S0F0, 3) // remove port itself
                  Notify (\_SB.PCID.P2P1, 1) // insertion action
                }
                local0 = A151 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P2.S0F0, 3) // remove port itself
                  Notify (\_SB.PCID.P2P2, 1) // insertion action
                }
                local0 = A152 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P3.S0F0, 3) // remove port itself
                  Notify (\_SB.PCID.P2P3, 1) // insertion action
                }
                local0 = A153 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCID.P2P4.S0F0, 3) // remove port itself
                  Notify (\_SB.PCID.P2P4, 1) // insertion action
                }

                local0 = A120 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P1.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIA.P2P1, 1) // insertion action
                }
                local0 = A121 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P2.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIA.P2P2, 1) // insertion action
                }
                local0 = A122 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P3.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIA.P2P3, 1) // insertion action
                }
                local0 = A123 & 0xFF000000
                if ((local0 == 0x0E000000) || (local0 == 0x0F000000)) {
                  Notify (\_SB.PCIA.P2P4.S0F0, 3) // remove port itself
                  Notify (\_SB.PCIA.P2P4, 1) // insertion action
                }
              }
            }
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
