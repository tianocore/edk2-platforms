## @file BoardModulePkgHostTest.dsc
#
#  BoardModulePkg DSC file used to build host-based unit tests.
#
#  Copyright (c) Microsoft Corporation.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = BoardModulePkgHostTest
  PLATFORM_GUID           = 67275336-A324-4F69-BD38-70A4C7898F06
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/BoardModulePkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]

[Components]
  #
  # Build HOST_APPLICATIONs that test the BoardModulePkg
  #

  #
  # Build HOST_APPLICATION Libraries With GoogleTest
  #
  BoardModulePkg/Test/Mock/Library/GoogleTest/MockBiosIdLib/MockBiosIdLib.inf
