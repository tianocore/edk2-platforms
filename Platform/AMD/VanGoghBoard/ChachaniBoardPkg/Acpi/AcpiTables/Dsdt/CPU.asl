// /** @file
// Acpi CPU.asl
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/

Scope(\_PR)
{
  // Operator 'Processor(){}' is deprecated, and may be required for compatibility with some legacy OSes.
  // Unique Name, ID,   P_BLK Add,  P_BLK Length
  Processor(C000, 0x00, 0x00000410, 0x06) {}
  Processor(C001, 0x01, 0x00000410, 0x06) {}
  Processor(C002, 0x02, 0x00000410, 0x06) {}
  Processor(C003, 0x03, 0x00000410, 0x06) {}
  Processor(C004, 0x04, 0x00000410, 0x06) {}
  Processor(C005, 0x05, 0x00000410, 0x06) {}
  Processor(C006, 0x06, 0x00000410, 0x06) {}
  Processor(C007, 0x07, 0x00000410, 0x06) {}
} // End _PR
