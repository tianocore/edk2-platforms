/** @file
  SSDT PCI USB hos controller device template

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock (
  "AcpiSsdtUsb.aml",
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
  Scope (\_SB) {
    Name (SS1, Zero)
    Name (SS2, Zero)
    Name (SS3, One)
    Name (SS4, Zero)
    Name (PRWP, Package (0x02)
    {
      Zero,
      Zero
    })
    Method (GPRW, 2, NotSerialized)
    {
      PRWP [Zero] = Arg0
      Local0 = (SS1 << One)
      Local0 |= (SS2 << 0x02)
      Local0 |= (SS3 << 0x03)
      Local0 |= (SS4 << 0x04)
      If (((One << Arg1) & Local0))
      {
        PRWP [One] = Arg1
      }
      Else
      {
        Local0 >>= One
        FindSetRightBit (Local0, PRWP [One])
      }

      Return (PRWP)
    }

    // During boot, the _PRW method will be moved under XHC0 device
    Method (_PRW, 0, NotSerialized)
    {
      Return (GPRW (0x0B, 0x04))
    }
  }
}// End of ASL File

