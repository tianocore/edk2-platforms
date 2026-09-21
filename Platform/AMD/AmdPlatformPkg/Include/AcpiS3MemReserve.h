/** @file
  Definitions for ACPI S3.

  Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Base.h>

typedef struct RESERVED_ACPI_S3_RANGE RESERVED_ACPI_S3_RANGE;

#define RESERVED_ACPI_S3_RANGE_OFFSET  (EFI_PAGE_SIZE - sizeof (RESERVED_ACPI_S3_RANGE))

#pragma pack(push, 1)
struct RESERVED_ACPI_S3_RANGE {
  UINT32    AcpiReservedMemoryBase;
  UINT32    AcpiReservedMemorySize;
};

#pragma pack(pop)
