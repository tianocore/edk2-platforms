## @file
# Qualcomm Common Platform dsc.inc file.
#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################

[Defines]
!ifndef QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE
  DEFINE QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE = FALSE
!endif

[LibraryClasses.AARCH64.PEIM]

!if $(QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE) == TRUE
  DtFrameworkLib|QualcommCommonPlatformPkg/Library/BaseDtFrameworkLib/BaseDtFrameworkLib.inf
!else
  DtFrameworkLib|QualcommCommonPlatformPkg/Library/BaseDtFrameworkLib/BaseDtFrameworkLibNull.inf
!endif

[LibraryClasses.Common]
  PlatformInfoLib|QualcommCommonPlatformPkg/Library/PlatformInfoLib/PlatformInfoLib.inf

!if $(QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE) == TRUE
[Components.AARCH64]
  QualcommCommonPlatformPkg/Drivers/DtbExtnDxe/DtbExtnDxe.inf
!endif
