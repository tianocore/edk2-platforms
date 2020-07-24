## @file
#  dsc include file for Configuration Manager
#
#  Copyright (c) 2017 - 2020, Arm Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]

[BuildOptions]
# Required for pre-processing ASL files that include ArmPlatform.h
  *_*_*_ASLPP_FLAGS        = $(PLATFORM_FLAGS)

[LibraryClasses.common]

[Components.common]
  # Configuration Manager
  Platform/ARM/VExpressPkg/ConfigurationManager/ConfigurationManagerDxe/ConfigurationManagerDxe.inf
