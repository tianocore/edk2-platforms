/** @file
  Source code file for Report CPU HOB library.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

/**
  Build and report the CPU HOB.

  This function creates a CPU HOB using the ARM physical address size
  and the Pre-PI CPU I/O size from PCDs.
**/
VOID
ReportCpuHob (
  VOID
  )
{
  BuildCpuHob (ArmGetPhysicalAddressBits (), PcdGet8 (PcdPrePiCpuIoSize));
}
