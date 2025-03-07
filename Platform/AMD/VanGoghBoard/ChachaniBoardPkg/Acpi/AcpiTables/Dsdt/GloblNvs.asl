// /** @file
// Acpi GloblNvs.asl
//
// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// **/

OperationRegion(GNVS, SystemMemory, 0xFFFF0000, 0xAA55)
Field(GNVS, AnyAcc, NoLock, Preserve) {
  Offset(0),
  TOPM, 32,      // Top Of Memory
  NAPC, 8,       // NbIoApic
  PCBA, 32,      // PcieBaseAddress
  PCBL, 32,      // PcieBaseLimit
}
