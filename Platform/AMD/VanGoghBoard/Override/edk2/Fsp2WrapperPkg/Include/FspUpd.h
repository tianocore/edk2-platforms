/** @file
  Implements FspUpd.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FSPUPD_H_
#define FSPUPD_H_

#ifdef USE_EDKII_HEADER_FILE
  #include <FspEas.h>
  #include <stdint.h>
#else
  #include <fsp_h_c99.h>
#endif

#define FSPM_UPD_SIGNATURE  0x4D5F48474F474E56                     /* 'VNGOGH_M' */

#define FSPS_UPD_SIGNATURE  0x535F48474F474E56                     /* 'VNGOGH_S' */

#endif
