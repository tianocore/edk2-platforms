// /** @file
// Acpi Dsdt.asl
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/

DefinitionBlock (
  "DSDT.aml",
  "DSDT",
  0x02,        // DSDT revision.
  "AMD   ",    // OEM ID (6 byte string)
  "EDK2    ",  // OEM table ID  (8 byte string)
  0x0          // OEM version of DSDT table (4 byte Integer)
)

// BEGIN OF ASL SCOPE
{
  // Miscellaneous services enabled in Project
  include ("GloblNvs.asl")
  include ("PciTree.asl")
  include ("Platform.asl")
  include ("FchShang.asi")
  //
  //  Processor Objects
  //
  include("_PR.asl")
  // System \_Sx states
  Name(\_S0, Package(4) {0x0,0x0,0,0}) // mandatory System state
  Name(\_S1, Package(4) {0x1,0x0,0,0})
  Name(\_S3, Package(4) {0x3,0x0,0,0})
  Name(\_S4, Package(4) {0x4,0x0,0,0})
  Name(\_S5, Package(4) {0x5,0x0,0,0}) // mandatory System state
}// End of ASL File
