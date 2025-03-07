/** @file

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GNBREGISTERSBRH_H_
#define GNBREGISTERSBRH_H_

#include "GnbRegistersBRH/IOHC.h"

#ifndef NBIO_SPACE
#define  NBIO_SPACE(HANDLE, ADDRESS)  (ADDRESS + ((HANDLE->RBIndex & 0x3) << 20))
#endif

#endif /* GNBREGISTERSBRH_H_ */
