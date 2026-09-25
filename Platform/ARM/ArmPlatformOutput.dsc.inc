## @file
#  Common output directory for Arm platform DSC files.
#
#  Copyright (c) 2026, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
!ifdef $(ARM_DEFAULT_OUTPUT_DIRECTORY)
  OUTPUT_DIRECTORY               = $(ARM_DEFAULT_OUTPUT_DIRECTORY)
!else
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
!endif
!endif
