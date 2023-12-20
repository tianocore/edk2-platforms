## @file
# This is a build description file for the Intelligent Platform Management Interface (IPMI) advanced feature.
# This file should be included into another package DSC file to build this feature.
#
# The DEC files are used by the utilities that parse DSC and
# INF files to generate AutoGen.c and AutoGen.h files
# for the build infrastructure.
#
# Copyright (c) 2019 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 1985 - 2023, American Megatrends International LLC. <BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
!ifndef $(PEI_ARCH)
  !error "PEI_ARCH must be specified to build this feature!"
!endif
!ifndef $(DXE_ARCH)
  !error "DXE_ARCH must be specified to build this feature!"
!endif

################################################################################
#
# Library Class section - list of all Library Classes needed by this feature.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  IpmiLib|MdeModulePkg/Library/BaseIpmiLibNull/BaseIpmiLibNull.inf

  IpmiCommandLib|ManageabilityPkg/Library/IpmiCommandLib/IpmiCommandLib.inf
  IpmiPlatformHookLib|IpmiFeaturePkg/Library/IpmiPlatformHookLibNull/IpmiPlatformHookLibNull.inf

[LibraryClasses.common.PEI_CORE,LibraryClasses.common.PEIM]
  IpmiBaseLib|IpmiFeaturePkg/Library/PeiIpmiBaseLib/PeiIpmiBaseLib.inf
  SsifInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/SsifInterfaceLib/PeiSsifInterfaceLib.inf
  IpmbInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/PeiIpmbInterfaceLib.inf

[LibraryClasses.common.DXE_DRIVER,LibraryClasses.common.UEFI_DRIVER]
  IpmiBaseLib|IpmiFeaturePkg/Library/IpmiBaseLib/IpmiBaseLib.inf

[LibraryClasses.common]
  BmcCommonInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/BmcCommonInterfaceLib.inf
  BtInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/BtInterfaceLib/BtInterfaceLib.inf
  ServerManagementLib|IpmiFeaturePkg/Library/ServerManagementLib/ServerManagementLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_DRIVER]
  SsifInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/SsifInterfaceLib/DxeSsifInterfaceLib.inf
  IpmbInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/DxeIpmbInterfaceLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER,LibraryClasses.common.MM_STANDALONE]
  IpmiBaseLib|IpmiFeaturePkg/Library/SmmIpmiBaseLib/SmmIpmiBaseLib.inf
  SsifInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/SsifInterfaceLib/SmmSsifInterfaceLib.inf
  IpmbInterfaceLib|IpmiFeaturePkg/Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/SmmIpmbInterfaceLib.inf

[LibraryClasses.common.MM_STANDALONE]
  ServerManagementLib|IpmiFeaturePkg/Library/ServerManagementLib/StandaloneMmServerManagementLib.inf

################################################################################
#
# Component section - list of all components that need built for this feature.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
################################################################################
#
# Feature PEI Components
#

# @todo: Change below line to [Components.$(PEI_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed.
[Components.IA32]
  #####################################
  # IPMI Feature Package
  #####################################

  # Add library instances here that are not included in package components and should be tested
  # in the package build.

  IpmiFeaturePkg/Library/IpmiPlatformHookLibNull/IpmiPlatformHookLibNull.inf

  #
  # Add components here that should be included in the package build.
  #
  IpmiFeaturePkg/GenericIpmi/Pei/PeiGenericIpmi.inf
  IpmiFeaturePkg/Frb/FrbPei.inf
  IpmiFeaturePkg/IpmiInit/PeiIpmiInit.inf
  IpmiFeaturePkg/BmcElog/PeiBmcElog.inf

#
# Feature DXE Components
#

# @todo: Change below line to [Components.$(DXE_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed.
[Components.X64]
  #####################################
  # IPMI Feature Package
  #####################################

  # Add library instances here that are not included in package components and should be tested
  # in the package build.

  IpmiFeaturePkg/Library/IpmiPlatformHookLibNull/IpmiPlatformHookLibNull.inf

  #
  # Add components here that should be included in the package build.
  #
  IpmiFeaturePkg/GenericIpmi/Dxe/GenericIpmi.inf
  IpmiFeaturePkg/Library/SmmIpmiBaseLib/SmmIpmiBaseLib.inf
  IpmiFeaturePkg/BmcAcpi/BmcAcpi.inf
  IpmiFeaturePkg/BmcAcpiState/BmcAcpiState.inf
  IpmiFeaturePkg/BmcAcpiSwChild/BmcAcpiSwChild.inf
  IpmiFeaturePkg/BmcAcpiSwChild/BmcAcpiSwChildStandaloneMm.inf
  IpmiFeaturePkg/BmcElog/DxeBmcElog.inf
  IpmiFeaturePkg/BmcElog/SmmBmcElog.inf
  IpmiFeaturePkg/BmcElog/StandaloneMmBmcElog.inf
  IpmiFeaturePkg/GenericElog/Dxe/GenericElog.inf
  IpmiFeaturePkg/GenericElog/Smm/GenericElog.inf
  IpmiFeaturePkg/GenericElog/Smm/GenericElogStandaloneMm.inf
  IpmiFeaturePkg/Frb/FrbDxe.inf
  IpmiFeaturePkg/IpmiRedirFru/IpmiRedirFru.inf
  IpmiFeaturePkg/GenericFru/GenericFru.inf
  IpmiFeaturePkg/IpmiInit/DxeIpmiInit.inf
  IpmiFeaturePkg/OsWdt/OsWdt.inf
  IpmiFeaturePkg/SolStatus/SolStatus.inf
