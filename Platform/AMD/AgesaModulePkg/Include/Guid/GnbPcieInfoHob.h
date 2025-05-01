/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GNB_PCIE_HOB_INFO_H_
#define GNB_PCIE_HOB_INFO_H_

#define _GNB_PCIE_HOB_INFO_GUID \
{ \
  0x3eb1d90, 0xce14, 0x40d8, 0xa6, 0xba, 0x10, 0x3a, 0x8d, 0x7b, 0xd3, 0x2d \
}
extern EFI_GUID  gGnbPcieHobInfoGuid;

#pragma pack (push, 1)
#define MAX_NUMBER_OF_CORES_PER_COMPLEX  3
#define MAX_NUMBER_OF_PORTS_PER_COMPLEX  22

/// PCIe information HOB data
typedef struct _GNB_PCIE_INFORMATION_DATA_HOB {
  EFI_HOB_GUID_TYPE       EfiHobGuidType;             ///< GUID Hob type structure
  PCIE_PLATFORM_CONFIG    PciePlatformConfigHob;      ///< Platform Config Structure
  UINT32                  ComplexConfigs;             ///< Allocation for Max Complex Structure suported
} GNB_PCIE_INFORMATION_DATA_HOB;

#pragma pack (pop)

#endif /* GNB_PCIE_HOB_INFO_H_ */
