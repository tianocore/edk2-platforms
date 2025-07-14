## @file
# This is a build description file for the Network advanced feature.
# This file should be included into another package DSC file to build this feature.
#
# The DEC files are used by the utilities that parse DSC and
# INF files to generate AutoGen.c and AutoGen.h files
# for the build infrastructure.
#
# Copyright (c) 2019 - 2021, Intel Corporation. All rights reserved.<BR>
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

  !include NetworkPkg/NetworkDefines.dsc.inc

################################################################################
#
# PCD Section - list of EDK II PCD Entries modified by the feature.
#
################################################################################
[PcdsFixedAtBuild.$(DXE_ARCH)]
  !include NetworkPkg/NetworkFixedPcds.dsc.inc

[PcdsDynamicDefault.$(DXE_ARCH)]
  !include NetworkPkg/NetworkDynamicPcds.dsc.inc

################################################################################
#
# Library Class section - list of all Library Classes needed by this feature.
#
################################################################################

[LibraryClasses]
  !include NetworkPkg/NetworkLibs.dsc.inc
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf

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
# Feature DXE Components
#
[Components.$(DXE_ARCH)]
  #####################################
  # Network Feature Package
  #####################################

  # Add library instances here that are not included in package components and should be tested
  # in the package build.

  # Add components here that should be included in the package build.
  !include NetworkPkg/NetworkComponents.dsc.inc
