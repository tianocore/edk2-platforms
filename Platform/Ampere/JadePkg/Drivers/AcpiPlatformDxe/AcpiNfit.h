/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_NFIT_H_
#define ACPI_NFIT_H_

#include <Platform/Ac01.h>

#define NVDIMM_SK0         0
#define NVDIMM_SK1         1
#define NVDIMM_NUM_PER_SK  (AC01_NVDIMM_MAX_MCU_PER_SOCKET * AC01_NVDIMM_MAX_DIMM_PER_MCU)
#define ONE_GB             (1024 * 1024 * 1024)

typedef enum {
  NvdimmDisabled = 0,
  NvdimmNonHashed,
  NvdimmHashed,
  NvdimmModeMax
} NVDIMM_MODE;

typedef struct {
  BOOLEAN    Enabled;
  UINT64     NvdSize;
  UINT32     DeviceHandle;
  UINT16     PhysId;
  UINT8      InterleaveWays;
  UINT64     RegionOffset;
  UINT16     VendorId;
  UINT16     DeviceId;
  UINT16     RevisionId;
  UINT16     SubVendorId;
  UINT16     SubDeviceId;
  UINT16     SubRevisionId;
  UINT32     SerialNumber;
} NVDIMM_INFO;

typedef struct {
  UINT8          NvdRegionNum;
  UINT8          NvdRegionId[AC01_NVDIMM_MAX_REGION_PER_SOCKET];
  UINT8          NvdMode;
  UINT8          NvdNum;
  NVDIMM_INFO    NvdInfo[NVDIMM_NUM_PER_SK];
} NVDIMM_DATA;

#endif
