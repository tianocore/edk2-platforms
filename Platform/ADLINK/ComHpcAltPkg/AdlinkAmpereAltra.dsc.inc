## @file
#
# Copyright (c) 2022, ADLink. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##


################################################################################
#
# Specific Platform Library
#
################################################################################
[LibraryClasses]
  #
  # Pcie Board
  #
  BoardPcieLib|Library/BoardPcieLib/BoardPcieLib.inf

  MmcLib|Library/MmcLib/MmcLib.inf
  NVLib|Library/NVLib/NVLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  #
  # RTC Library: Common RTC
  #
  RealTimeClockLib|Library/PCF8563RealTimeClockLib/PCF8563RealTimeClockLib.inf

[PcdsFixedAtBuild.common]
  # set baudrate to match with MMC
  gArmPlatformTokenSpaceGuid.PcdSerialDbgUartBaudRate|57600

################################################################################
#
# Specific Module Component
#
################################################################################
[Components.common]

  #
  # set MMC power off type
  #
  Drivers/MmcSetPowerOffType/Dxe/MmcSetPowerOffTypeDxe.inf
  Drivers/MmcSetPowerOffType/Pei/MmcSetPowerOffTypePei.inf

  #
  # POST code thru MMC and utilize Intel POST code map
  #
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf {
    <LibraryClasses>
      PostCodeLib|Library/PostCodeLibMmc/PostCodeLibMmc.inf
      PostCodeMapLib|PostCodeDebugFeaturePkg/Library/PostCodeMapLib/PostCodeMapLib.inf
  }

  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf {
    <LibraryClasses>
      PostCodeLib|Library/PostCodeLibMmc/PostCodeLibMmc.inf
      PostCodeMapLib|PostCodeDebugFeaturePkg/Library/PostCodeMapLib/PostCodeMapLib.inf
  }
  #
  # Application to reboot to Firmware User Interface (BIOS setup)
  #
  Application/FwUi/FwUi.inf
