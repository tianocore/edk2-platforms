/** @file

  Header file of AMD FCH platform initialization library.
  
  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FCH_PLATFORM_INIT_H_
#define FCH_PLATFORM_INIT_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/PciLib.h>
#include <IndustryStandard/SmBios.h>

#include <FchRegistersCommon.h>

#define SPI_BASE  0xFEC10000ul

#endif // FCH_PLATFORM_INIT_H_
