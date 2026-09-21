/*****************************************************************************
 *
 * Copyright (C) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 *****************************************************************************/
/*
  ACPI FCH device resources
*/

DefinitionBlock (
  "AcpiSsdtSysDev.aml",
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
    #include "AcpiSsdtSysDev.asi"
  }
}// End of ASL File

