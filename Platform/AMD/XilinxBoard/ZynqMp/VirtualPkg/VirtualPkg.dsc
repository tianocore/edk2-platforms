#  @file
#
#  Example Platform Description File for ZynqMP-based Platform
#
#  Copyright (c) 2025, Linaro Ltd. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ZynqMpVirtualPkg
  PLATFORM_GUID                  = FC5F6C0A-3A48-4B14-86CD-4EB35B7EE851
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001A
  OUTPUT_DIRECTORY               = Build/ZynqMpVirtualPkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/AMD/XilinxBoard/ZynqMp/VirtualPkg/VirtualPkg.fdf

  !include Silicon/AMD/Xilinx/ZynqMpPkg/ZynqMpPkg.dsc.inc

  [Components.common]
  Platform/AMD/XilinxBoard/ZynqMp/VirtualPkg/DeviceTree/DeviceTree.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild.common]
  # UART
  # In this case, UART1 is used.
  # By default, the address is set to UART0
  gZynqMpTokenSpaceGuid.PcdSerialRegisterBase|0xFF010000

  # TF-A
  # In this case, it's located in DRAM,
  # so we need to reserve memory.
  gZynqMpTokenSpaceGuid.PcdTfaInDram|TRUE
  gZynqMpTokenSpaceGuid.PcdTfaMemoryBase|0x7FFFD000
  gZynqMpTokenSpaceGuid.PcdTfaMemorySize|0x00003000

  # OP-TEE
  # In this case, it's not enabled.
  # So, no need to override default values of:
  # gZynqMpTokenSpaceGuid.PcdEnableOptee
  # gZynqMpTokenSpaceGuid.PcdOpteeMemoryBase
  # gZynqMpTokenSpaceGuid.PcdOpteeMemorySize

  # SDHCI
  # This board uses SD controller at 0xFF170000.
  # So, no need to change the default value of:
  # gXilinxTokenSpaceGuid.PcdSdhciBase

  # Write protection detection is enabled.
  # By default, WP detection is disabled.
  gXilinxTokenSpaceGuid.PcdEnableMmcWPDetection|TRUE

  # Mmc driver delays are also kept as default,
  # so no need to tweak the values of:
  # gXilinxTokenSpaceGuid.PcdMmcStallAfterCmdSend
  # gXilinxTokenSpaceGuid.PcdMmcStallAfterResponseReceive
  # gXilinxTokenSpaceGuid.PcdMmcStallAfterDataWrite
  # gXilinxTokenSpaceGuid.PcdMmcStallAfterDataRead
  # gXilinxTokenSpaceGuid.PcdMmcStallAfterRegisterWrite
  # gXilinxTokenSpaceGuid.PcdMmcStallAfterRetry

  # Extra memory
  # This board only has base 2GB DDR. So, extra
  # memory is disabled. By default, another 2GB
  # DRAM is enabled at 0x8000000000
  gZynqMpTokenSpaceGuid.PcdUseExtraMemory|FALSE

  # Base address and size are ignored in case of
  # PcdUseExtraMemory = FALSE, so no need to change:
  # gZynqMpTokenSpaceGuid.PcdExtraMemoryBase
  # gZynqMpTokenSpaceGuid.PcdExtraMemorySize

  # Base DDR can be changed via:
  # gArmTokenSpaceGuid.PcdSystemMemoryBase
  # gArmTokenSpaceGuid.PcdSystemMemorySize
