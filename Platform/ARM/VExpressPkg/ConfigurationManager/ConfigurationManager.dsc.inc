## @file
#
#  Copyright (c) 2017, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
##

[Defines]
# [BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]

[BuildOptions]
# Required for pre-processing ASL files that include ArmPlatform.h
  *_*_*_ASLPP_FLAGS        = $(PLATFORM_FLAGS)

[LibraryClasses.common]

[Components.common]
  # Configuration Manager
  Platform/ARM/VExpressPkg/ConfigurationManager/ConfigurationManagerDxe/ConfigurationManagerDxe.inf {
    <LibraryClasses>
    # Platform ASL Tables
    PlatformAslTablesLib|Platform/ARM/VExpressPkg/ConfigurationManager/PlatformASLTablesLib/PlatformASLTablesLib.inf
  <BuildOptions>
   *_*_*_PLATFORM_FLAGS = -I$(BIN_DIR)/Platform/ARM/VExpressPkg/ConfigurationManager/PlatformASLTablesLib/PlatformASLTablesLib/OUTPUT
  }
