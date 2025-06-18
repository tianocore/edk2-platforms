/** @file
  RISC-V Timer Architectural definition for U700 platform.

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2022, Boyang Han <yqszxx@gmail.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef U7_CLINT_H_
#define U7_CLINT_H_

#define CLINT_REG_BASE_ADDR  0x02000000
#define CLINT_REG_MTIME      0x0200BFF8
#define CLINT_REG_MTIMECMP0  0x02004000
#define CLINT_REG_MTIMECMP1  0x02004008
#define CLINT_REG_MTIMECMP2  0x02004010
#define CLINT_REG_MTIMECMP3  0x02004018
#define CLINT_REG_MTIMECMP4  0x02004020

#endif
