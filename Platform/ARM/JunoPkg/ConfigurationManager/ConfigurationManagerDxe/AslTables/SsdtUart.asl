/** @file
  SSDT for UART

  Copyright (c) 2014 - 2019, ARM Ltd. All rights reserved.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "ArmPlatform.h"

DefinitionBlock("SsdtUart.aml", "SSDT", 1, "ARMLTD", "ARM-JUNO", EFI_ACPI_ARM_OEM_REVISION) {
  Scope(_SB) {
    // UART PL011
    Device(COM2) {
      Name(_HID, "ARMH0011")
      Name(_CID, "PL011")
      Name(_UID, Zero)

      Method(_STA) {
        Return (0xF)
      }

      Method(_CRS, 0x0, NotSerialized) {
        Name(RBUF, ResourceTemplate() {
          Memory32Fixed(
            ReadWrite,
            FixedPcdGet64 (PcdSerialRegisterBase),
            0x1000
            )
          Interrupt(
            ResourceConsumer,
            Level,
            ActiveHigh,
            Exclusive
            ) {
              FixedPcdGet32 (PL011UartInterrupt)
            }
          })
        Return (RBUF)
      }
    }
  }
}
