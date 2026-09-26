## @file
#  AmdMinBoardPkg.inc.dsc
#
#  Description file for AMD AmdMinBoardPkg
#
#  Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses]
  ReportFvLib|AmdMinBoardPkg/Library/PeiReportFvLib/PeiReportFvLib.inf
  SpcrDeviceLib|AmdMinBoardPkg/Library/SpcrDeviceLib/SpcrDeviceLib.inf

[LibraryClasses.common.SEC]
  PlatformSecLib|AmdMinBoardPkg/Library/PlatformSecLib/PlatformSecLib.inf

[LibraryClasses.common.PEIM]
  BoardInitLib|AmdMinBoardPkg/Library/PeiBoardInitPreMemLib/PeiBoardInitPreMemLib.inf
  SetCacheMtrrLib|AmdMinBoardPkg/Library/SetCacheMtrrLib/SetCacheMtrrLib.inf

[LibraryClasses.common.DXE_DRIVER]
  BoardBdsHookLib|AmdMinBoardPkg/Library/BoardBdsHookLib/BoardBdsHookLib.inf
  BoardInitLib|AmdMinBoardPkg/Library/DxeBoardInitLib/DxeBoardInitLib.inf
