/** @file

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiSmm.h>
#include <Register/Cpuid.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SmmBase2.h>
#include <Library/IoLib.h>
