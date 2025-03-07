/** @file
Header file for the PCH SPI SMM Driver.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FCH_SPI_SMM_H_
#define FCH_SPI_SMM_H_

#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PciExpressLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/TimerLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/SpiFlashDeviceLib.h>

#include <Protocol/SmmReadyToBoot.h>
#include <Protocol/Spi.h>
#include <Protocol/SpiCommon.h>

#endif
