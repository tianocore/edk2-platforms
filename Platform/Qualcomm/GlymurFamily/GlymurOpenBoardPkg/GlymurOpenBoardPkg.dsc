## @file
#  The main build description file for the GlymurOpenBoardPkg
#
#  Board package built on top of the GlymurMinPlatformPkg silicon layer.
#
#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DSC_SPECIFICATION           = 0x0001001E
  PLATFORM_GUID               = 356921BF-DC19-472D-BAAE-47820F69AC3E
  PLATFORM_NAME               = GlymurOpenBoardPkg
  PLATFORM_VERSION            = 1.0
  SUPPORTED_ARCHITECTURES     = AARCH64
  FLASH_DEFINITION            = $(PLATFORM_NAME)/$(PLATFORM_NAME).fdf
  OUTPUT_DIRECTORY            = Build/$(PLATFORM_NAME)
  BUILD_TARGETS               = DEBUG | RELEASE | NOOPT
  SKUID_IDENTIFIER            = ALL
  SMM_REQUIRED                = FALSE

!include GlymurMinPlatformPkg/GlymurMinPlatformPkg.dsc.inc
