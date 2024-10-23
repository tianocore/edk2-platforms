## @file
#
# Copyright (c) 2022, ADLink. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##


################################################################################
#
# Specific Baseboard Component
#
################################################################################

[LibraryClasses]
  #
  # Library for POST Code converting
  #
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[PcdsFixedAtBuild.common]
  #
  # Enable POST Code
  #
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0x00000008

################################################################################
#
# Specific Module Component
#
################################################################################
[Components.common]
  #
  # Driver to initialize GPIO
  #
  Drivers/GpioInitDxe/GpioInitDxe.inf
