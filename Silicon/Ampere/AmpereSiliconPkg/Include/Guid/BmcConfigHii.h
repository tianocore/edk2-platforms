/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BMC_CONFIG_HII_H_
#define BMC_CONFIG_HII_H_

#define BMC_CONFIG_FORMSET_GUID \
  { \
    0xC4D6ED50, 0x769D, 0x4319, { 0xEB, 0xB7, 0xCC, 0xDD, 0xC8, 0x9D, 0x3D, 0x2D } \
  }

extern EFI_GUID  gBmcConfigFormSetGuid;

#endif /* BMC_CONFIG_HII_H_ */
