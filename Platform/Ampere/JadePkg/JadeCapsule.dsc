## @file
#
# Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
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
  PLATFORM_NAME                  = Jade
  PLATFORM_GUID                  = 34C87B13-434A-4767-88FB-2D0CD2AED46F
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
  OUTPUT_DIRECTORY               = Build/Jade
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Ampere/JadePkg/JadeCapsule.fdf

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE UEFI_ATF_IMAGE          = Build/Jade/jade_tfa_uefi.bin
  DEFINE SCP_IMAGE               = Build/Jade/altra_scp.slim
