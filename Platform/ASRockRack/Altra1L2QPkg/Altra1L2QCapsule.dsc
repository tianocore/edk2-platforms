## @file
#
# Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>
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
  PLATFORM_NAME                  = Altra1L2Q
  PLATFORM_GUID                  = 57ce30d1-ad4d-41a0-a611-41ed20d33e50
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
  OUTPUT_DIRECTORY               = Build/Altra1L2Q
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ASRockRack/Altra1L2QPkg/Altra1L2QCapsule.fdf

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE INCLUDE_TFA_FW          = TRUE
  DEFINE UEFI_IMAGE              = Build/Altra1L2Q/altra1l2q_uefi.bin
  DEFINE TFA_UEFI_IMAGE          = Build/Altra1L2Q/altra1l2q_tfa_uefi.bin
