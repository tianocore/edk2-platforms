## @file
#  The main build description file for the KodiakMinPlatformPkg
#
#  Copyright (c) 2022 Theo Jehl<BR>
#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DSC_SPECIFICATION           = 0x0001001E
  PLATFORM_GUID               = FB9BF1D9-B995-4B8C-A813-4AE8E292D041
  PLATFORM_NAME               = KodiakMinPlatformPkg
  PLATFORM_VERSION            = 1.0
  SUPPORTED_ARCHITECTURES     = AARCH64
  FLASH_DEFINITION            = $(PLATFORM_NAME)/$(PLATFORM_NAME).fdf
  OUTPUT_DIRECTORY            = Build/$(PLATFORM_NAME)
  BUILD_TARGETS               = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER            = ALL
  SMM_REQUIRED                = FALSE

!include KodiakMinPlatformPkg/KodiakMinPlatformPkg.dsc.inc
