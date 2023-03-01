## @file
#  PlatformPkg.dsc
#
#  Description file for AMD PlatformPkg
#
#  Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DSC_SPECIFICATION           = 1.30
  PLATFORM_GUID               = 2F7C29F2-7F35-4B49-B97D-F0E61BD42FC0
  PLATFORM_NAME               = PlatformPkg
  PLATFORM_VERSION            = 0.1
  OUTPUT_DIRECTORY            = Build/$(PLATFORM_NAME)
  BUILD_TARGETS               = DEBUG | RELEASE | NOOPT
  SUPPORTED_ARCHITECTURES     = IA32 | X64

[Packages]
  PlatformPkg/PlatformPkg.dec
