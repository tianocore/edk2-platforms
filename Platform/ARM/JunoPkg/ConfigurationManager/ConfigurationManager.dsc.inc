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

[LibraryClasses.common]

[Components.common]
  # Configuration Manager
  Platform/ARM/JunoPkg/ConfigurationManager/ConfigurationManagerDxe/ConfigurationManagerDxe.inf {
    <LibraryClasses>
    # Platform ASL Tables
    PlatformAslTablesLib|Platform/ARM/JunoPkg/ConfigurationManager/PlatformASLTablesLib/PlatformASLTablesLib.inf
  <BuildOptions>
   *_*_*_PLATFORM_FLAGS = -I$(BIN_DIR)/Platform/ARM/JunoPkg/ConfigurationManager/PlatformASLTablesLib/PlatformASLTablesLib/OUTPUT
  }
