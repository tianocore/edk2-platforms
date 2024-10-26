## @file
# This is a build description file for the S3 advanced feature.
# This file should be included into another package DSC file to build this feature.
#
# The DEC files are used by the utilities that parse DSC and
# INF files to generate AutoGen.c and AutoGen.h files
# for the build infrastructure.
#
# Copyright (c) 2019 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2022, Baruch Binyamin Doron.<BR>
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

[PcdsFixedAtBuild]
  # Attempts to improve performance at the cost of more DRAM usage
  gEfiMdeModulePkgTokenSpaceGuid.PcdShadowPeimOnS3Boot|TRUE

################################################################################
#
# Library Class section - list of all Library Classes needed by this feature.
#
################################################################################

[LibraryClasses.common.PEIM]
  SmmControlLib|IntelSiliconPkg/Feature/SmmControl/Library/PeiSmmControlLib/PeiSmmControlLib.inf

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_SMM_DRIVER]
  #######################################
  # Edk2 Packages
  #######################################
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf

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
[Components.$(PEI_ARCH)]
  #####################################
  # S3 Feature Package
  #####################################

  # Add components here that should be included in the package build.
  S3FeaturePkg/S3Pei/S3Pei.inf
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationPei.inf
  UefiCpuPkg/Universal/Acpi/S3Resume2Pei/S3Resume2Pei.inf

#
# Feature DXE Components
#
[Components.$(DXE_ARCH)]
  #####################################
  # S3 Feature Package
  #####################################

  # Add components here that should be included in the package build.
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationSmm.inf
  S3FeaturePkg/S3Dxe/S3Dxe.inf
  MdeModulePkg/Universal/Acpi/S3SaveStateDxe/S3SaveStateDxe.inf
  UefiCpuPkg/CpuS3DataDxe/CpuS3DataDxe.inf
  MdeModulePkg/Universal/Acpi/BootScriptExecutorDxe/BootScriptExecutorDxe.inf
