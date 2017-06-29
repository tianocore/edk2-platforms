/** @file
  SSDT for UART

  Copyright (c) 2014-2017, ARM Ltd. All rights reserved.<BR>
    This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
        Return(0xF)
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
