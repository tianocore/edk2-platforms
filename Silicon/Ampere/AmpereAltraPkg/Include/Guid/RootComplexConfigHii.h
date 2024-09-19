/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ROOT_COMPLEX_CONFIG_HII_H_
#define ROOT_COMPLEX_CONFIG_HII_H_

#include <Platform/Ac01.h>

#define ROOT_COMPLEX_CONFIG_FORMSET_GUID \
  { \
    0xE84E70D6, 0xE4B2, 0x4C6E, { 0x98,  0x51, 0xCB, 0x2B, 0xAC, 0x77, 0x7D, 0xBB } \
  }

extern EFI_GUID  gRootComplexConfigFormSetGuid;

//
// NV data structure definition
//
typedef struct {
  BOOLEAN    RCStatus[AC01_PCIE_MAX_ROOT_COMPLEX];
  UINT8      RCBifurcationLow[AC01_PCIE_MAX_ROOT_COMPLEX];
  UINT8      RCBifurcationHigh[AC01_PCIE_MAX_ROOT_COMPLEX];
  UINT32     SmmuPmu;
} ROOT_COMPLEX_CONFIG_VARSTORE_DATA;

#define ROOT_COMPLEX_CONFIG_VARSTORE_NAME  L"PcieIfrNVData"

#endif /* ROOT_COMPLEX_CONFIG_HII_H_ */
