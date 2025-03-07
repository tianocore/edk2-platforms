/** @file

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _GNBREGISTERSRS_H_
#define _GNBREGISTERSRS_H_

#include "GnbRegistersRS/IOHC.h"

#ifndef NBIO_SPACE
#define  NBIO_SPACE(HANDLE, ADDRESS)  (ADDRESS + (HANDLE->RBIndex << 20))
#endif

#endif /*_GNBREGISTERSRS_H_*/
