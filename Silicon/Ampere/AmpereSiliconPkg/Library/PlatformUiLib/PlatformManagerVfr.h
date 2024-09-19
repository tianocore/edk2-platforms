/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_MANAGER_VFR_H_
#define PLATFORM_MANAGER_VFR_H_

#define FORMSET_GUID \
  { \
  0x6E7233C5, 0x2B79, 0x4383, { 0x81, 0x46, 0xD8, 0x6A, 0x9F, 0x0A, 0x0B, 0x99 } \
  }

//
// These are defined as the same with vfr file
//
#define LABEL_FORM_ID_OFFSET  0x0100
#define ENTRY_KEY_OFFSET      0x4000

#define PLATFORM_MANAGER_FORM_ID  0x1000

#define LABEL_ENTRY_LIST  0x1100
#define LABEL_END         0xffff

#endif /* PLATFORM_MANAGER_VFR_H_ */
