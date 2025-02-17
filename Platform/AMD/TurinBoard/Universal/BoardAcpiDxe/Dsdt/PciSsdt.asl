/*****************************************************************************
 *
 * Copyright (C) 2020-2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 *****************************************************************************/

/*
  ACPI FCH device resources
*/

DefinitionBlock (
  "PciSsdt.aml",
  "SSDT",
  0x02, // SSDT revision.
        // A Revision field value greater than or equal to 2 signifies that integers
        // declared within the Definition Block are to be evaluated as 64-bit values
  "AMD   ",   // OEM ID (6 byte string)
  "AmdTable",// OEM table ID  (8 byte string)
  0x00 // OEM version of SSDT table (4 byte Integer)
)

// BEGIN OF ASL SCOPE
{
  Scope (\_SB) {
    Include ("AmdPci.asi")
  }
}// End of ASL File

