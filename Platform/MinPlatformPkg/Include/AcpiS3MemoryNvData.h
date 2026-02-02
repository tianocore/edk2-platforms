/** @file
  Header file for NV data structure definition.

Copyright (c) 2021, Baruch Binyamin Doron
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ACPI_S3_MEMORY_NV_DATA_H__
#define __ACPI_S3_MEMORY_NV_DATA_H__

//
// NV data structure
//
typedef struct {
  EFI_PHYSICAL_ADDRESS  S3PeiMemBase;
  UINT64                S3PeiMemSize;
} ACPI_S3_MEMORY;

#define ACPI_S3_MEMORY_NV_NAME  L"S3MemoryInfo"

#endif
