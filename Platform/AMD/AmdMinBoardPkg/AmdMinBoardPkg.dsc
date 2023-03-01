## @file
#  AmdMinBoardPkg.dsc
#
#  Description file for AMD AmdMinBoardPkg
#
#  Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DSC_SPECIFICATION           = 1.30
  PLATFORM_GUID               = 88F8A9AE-2FA0-4D58-A6F9-05F635C05F4E
  PLATFORM_NAME               = AmdMinBoardPkg
  PLATFORM_VERSION            = 0.1
  OUTPUT_DIRECTORY            = Build/$(PLATFORM_NAME)
  BUILD_TARGETS               = DEBUG | RELEASE | NOOPT
  SUPPORTED_ARCHITECTURES     = IA32 | X64

[Packages]
  AmdMinBoardPkg/AmdMinBoardPkg.dec

