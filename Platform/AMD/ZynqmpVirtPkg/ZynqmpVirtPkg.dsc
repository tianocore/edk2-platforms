#  @file
#
#  Example Platform Description File for ZynqMP-based Platform
#
#  Copyright (c) 2025, Linaro Ltd. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ZynqmpVirtPkg
  PLATFORM_GUID                  = FC5F6C0A-3A48-4B14-86CD-4EB35B7EE851
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001A
  OUTPUT_DIRECTORY               = Build/ZynqmpVirtPkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/AMD/ZynqmpVirtPkg/ZynqmpVirtPkg.fdf

  !include Silicon/AMD/ZynqMP/ZynqMP.dsc.inc

  [Components.common]
  Platform/AMD/ZynqmpVirtPkg/DeviceTree/ZynqmpDeviceTree.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild.common]
  # UART
  # In this case, UART1 is used.
  # By default, the address is set to UART0
  gZynqMPTokenSpaceGuid.PcdSerialRegisterBase|0xFF010000

  # TF-A
  # In this case, it's located in DRAM,
  # so we need to reserve memory.
  gZynqMPTokenSpaceGuid.PcdTfaInDram|TRUE
  gZynqMPTokenSpaceGuid.PcdTfaMemoryBase|0x7FFFD000
  gZynqMPTokenSpaceGuid.PcdTfaMemorySize|0x00003000

  # OP-TEE
  # In this case, it's not enabled.
  # So, no need to override default values of:
  # gZynqMPTokenSpaceGuid.PcdEnableOptee
  # gZynqMPTokenSpaceGuid.PcdOpteeMemoryBase
  # gZynqMPTokenSpaceGuid.PcdOpteeMemorySize

  # SDHCI Write Protection
  # In this case, write protection detection is enabled.
  # By default, WP detection is disabled.
  gZynqMPTokenSpaceGuid.PcdEnableMmcWPDetection|TRUE
