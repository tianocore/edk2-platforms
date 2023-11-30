## @file
# This is a build description file for the User Authentication advanced feature.
# This file should be included into another package DSC file to build this feature.
#
# The DEC files are used by the utilities that parse DSC and
# INF files to generate AutoGen.c and AutoGen.h files
# for the build infrastructure.
#
# Copyright (c) 2020 - 2023, Intel Corporation. All rights reserved.<BR>
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
  PlatformPasswordLib|UserAuthFeaturePkg/Library/PlatformPasswordLibNull/PlatformPasswordLibNull.inf
  UserPasswordLib|UserAuthFeaturePkg/Library/UserPasswordLib/UserPasswordLib.inf

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

#
# Feature DXE Components
#

# @todo: Change below line to [Components.$(DXE_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed.
[Components.X64]
  #####################################
  # User Authentication Feature Package
  #####################################

  # Add library instances here that are not included in package components and should be tested
  # in the package build.
  UserAuthFeaturePkg/Library/UserPasswordUiLib/UserPasswordUiLib.inf

  # Add components here that should be included in the package build.
  UserAuthFeaturePkg/UserAuthenticationDxeSmm/UserAuthenticationDxe.inf
  UserAuthFeaturePkg/UserAuthenticationDxeSmm/UserAuthentication2Dxe.inf
  UserAuthFeaturePkg/UserAuthenticationDxeSmm/UserAuthenticationSmm.inf
  UserAuthFeaturePkg/UserAuthenticationDxeSmm/UserAuthenticationStandaloneMm.inf
  UserAuthFeaturePkg/UserAuthenticationDxeSmm/UserAuthenticationStandaloneMmDxe.inf
