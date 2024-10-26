## @file
# This package provides advanced feature functionality about Serial Port Console Redirection Table (SPCR).
# This file should be included into another package DSC file to build this feature.
#
# The DEC files are used by the utilities that parse DSC and
# INF files to generate AutoGen.c and AutoGen.h files
# for the build infrastructure.
#
# Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
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

[LibraryClasses]
  SpcrDeviceLib|SpcrFeaturePkg/Library/SpcrDeviceLibNull/SpcrDeviceLibNull.inf

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
[Components.$(DXE_ARCH)]
  SpcrFeaturePkg/SpcrAcpiDxe/SpcrAcpiDxe.inf
