/** @file
  Implements CommonHeader.h
  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef COMMON_HEADER_H_
#define COMMON_HEADER_H_

#include <PiDxe.h>

#include <Protocol/PciPlatform.h>
#include <Protocol/PciIo.h>

#include <Library/DxeServicesLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

#endif
