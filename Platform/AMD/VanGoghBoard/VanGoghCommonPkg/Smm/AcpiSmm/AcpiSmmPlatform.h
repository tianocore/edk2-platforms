/** @file
Header file for SMM S3 Handler Driver.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013-2019 Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_SMM_DRIVER_H_
#define ACPI_SMM_DRIVER_H_
//
// Include files
//
//
// Driver Consumed Protocol Prototypes
//
#include <Protocol/GlobalNvsArea.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/PcdLib.h>
#include <Library/LockBoxLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/S3IoLib.h>
#include <Library/S3BootScriptLib.h>
#include <Guid/Acpi.h>
#include <Library/SmmServicesTableLib.h>
#include <Guid/SmramMemoryReserve.h>
#include <Library/HobLib.h>

//
// This structure stores the base and size of the ACPI reserved memory used when
// resuming from S3.  This region must be allocated by the platform code.
//
typedef struct {
  UINT32    AcpiReservedMemoryBase;
  UINT32    AcpiReservedMemorySize;
  UINT32    SystemMemoryLength;
} RESERVED_ACPI_S3_RANGE;

#define RESERVED_ACPI_S3_RANGE_OFFSET  (EFI_PAGE_SIZE - sizeof (RESERVED_ACPI_S3_RANGE))
#define MAX_SMRAM_RANGES               4

#endif
