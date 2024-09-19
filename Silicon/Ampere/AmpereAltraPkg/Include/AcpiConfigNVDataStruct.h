/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_CONFIG_NV_DATA_STRUC_H_
#define ACPI_CONFIG_NV_DATA_STRUC_H_

#pragma pack(1)

//
// ACPI Configuration NV data structure definition
//
typedef struct {
  UINT32    EnableApeiSupport;
  UINT32    AcpiCppcEnable;
  UINT32    AcpiLpiEnable;
  UINT32    Reserved[6];
} ACPI_CONFIG_VARSTORE_DATA;

#pragma pack()

#endif /* ACPI_CONFIG_NV_DATA_STRUC_H_ */
