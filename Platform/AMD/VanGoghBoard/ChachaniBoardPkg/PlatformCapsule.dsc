## @file
#  PlatformCapsule.dsc file
#
# Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PROJECT_PKG                    = ChachaniBoardPkg

  PLATFORM_NAME                  = Chachani
  PLATFORM_GUID                  = EB34F10A-CF09-4391-B509-2860A1DE17DE
  PLATFORM_VERSION               = 0.1
  FLASH_DEFINITION               = $(PROJECT_PKG)/PlatformCapsule.fdf
  OUTPUT_DIRECTORY               = Build/$(PROJECT_PKG)
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

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
#       specified in the FDF file. For example: Shell binary, FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################
