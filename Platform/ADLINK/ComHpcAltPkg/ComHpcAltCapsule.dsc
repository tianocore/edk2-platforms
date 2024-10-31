## @file
#
# Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>
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
  PLATFORM_NAME                  = ComHpcAlt
  PLATFORM_GUID                  = A4365AA5-0696-4E90-BB5A-ABC1BF6BFAB0
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
  OUTPUT_DIRECTORY               = Build/ComHpcAlt
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ADLINK/ComHpcAltPkg/ComHpcAltCapsule.fdf

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE UEFI_TFA_IMAGE          = Build/ComHpcAlt/comhpcalt_tfa_uefi.bin
  DEFINE SCP_IMAGE               = altra_scp_signed_2.10.20230517.slim
