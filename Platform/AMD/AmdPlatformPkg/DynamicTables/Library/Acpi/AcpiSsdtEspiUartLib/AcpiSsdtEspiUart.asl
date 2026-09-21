/*****************************************************************************
 *
 * Copyright (C) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 *****************************************************************************/
/*
  ACPI SSDT ESPI UART device resources
*/

DefinitionBlock (
  "AcpiSsdtEspiUart.aml",
  "SSDT",
  0x02, // SSDT revision.
        // A Revision field value greater than or equal to 2 signifies that integers
        // declared within the Definition Block are to be evaluated as 64-bit values
  "AMDINC",   // OEM ID (6 byte string)
  "AMDCRB  ", // OEM table ID  (8 byte string)
  0x00 // OEM version of SSDT table (4 byte Integer)
)

// BEGIN OF ASL SCOPE
{
  External (_SB_.PC00.LPC0, DeviceObj)

  Scope (\_SB.PC00.LPC0) {
    Device (COM1)
    {
        Name (_HID, EisaId ("PNP0501") /* 16550A-compatible COM Serial Port */)  // _HID: Hardware ID
        Name (_UID, One)  // _UID: Unique ID
        Name (_DDN, "COM1")  // _DDN: DOS Device Name
        Method (_STA, 0, NotSerialized)  // _STA: Status
        {
            Return (0x0F)
        }
        Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
        {
            IO (Decode16,
                0x03F8,             // Range Minimum
                0x03F8,             // Range Maximum
                0x01,               // Alignment
                0x08,               // Length
                )
            IRQ (Edge, ActiveHigh, Shared, )
                {4}
            UartSerialBusV2 (0x0001C200, DataBitsEight, StopBitsOne,
                0x00, LittleEndian, ParityTypeNone, FlowControlNone,
                0x0001, 0x0001, "COM1",
                0x00, ResourceConsumer, , Exclusive,
                )
        })
    }
  }
}// End of ASL File
