/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HW_MONITOR_HII_H_
#define HW_MONITOR_HII_H_

/*
#define PLATFORM_INFO_FORMSET_GUID \
  { \
    0x8DF0F6FB, 0x65A5, 0x434B, { 0xB2, 0xA6, 0xCE, 0xDF, 0xD2, 0x0A, 0x96, 0x8A } \
  }
*/
// {1F99F615-2A3D-4FB1-AA4B-83FB47ECB2EF}
#define HW_MONITOR_FORMSET_GUID \
  { \
    0x1F99F615, 0x2A3D, 0x4FB1, {0xAA, 0x4B, 0x83, 0xFB, 0x47, 0xEC, 0xB2, 0xEF} \
  }

#define LABEL_UPDATE  0x2223
#define LABEL_END     0x2224

#define HW_MONITOR_FORM_ID  0x1

#endif
