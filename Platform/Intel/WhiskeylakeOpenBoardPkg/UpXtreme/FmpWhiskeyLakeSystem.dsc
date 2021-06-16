#/** @file
# FmpDxe driver for Whiskey Lake system firmware update.
#
# Copyright (c) 2018 - 2020, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#
#**/

  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # ESRT and FMP GUID for system firmware capsule update
      #
      FILE_GUID = $(FMP_WHISKEY_LAKE_SYSTEM)
    <PcdsFixedAtBuild>
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"Whiskey Lake System Firmware Device"

      #
      # ESRT and FMP Lowest Support Version for this capsule update module
      # 000.000.000.000
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|0x00000000

      gPlatformModuleTokenSpaceGuid.PcdSystemFirmwareFmpLowestSupportedVersion|0x00000000
      gPlatformModuleTokenSpaceGuid.PcdSystemFirmwareFmpVersion|0x00000000
      gPlatformModuleTokenSpaceGuid.PcdSystemFirmwareFmpVersionString|L"000.000.000.000"

      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressWatchdogTimeInSeconds|4

      #
      # Capsule Update Progress Bar Color.  Set to Purple (RGB) (255, 0, 255)
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressColor|0x00FF00FF

      #
      # Certificates used to authenticate capsule update image
      #
      !include WhiskeylakeOpenBoardPkg/UpXtreme/FmpCertificate.dsc

    <LibraryClasses>
      #
      # Generic libraries that are used "as is" by all FMP modules
      #
      FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
      FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
      FmpDependencyLib|FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf
      FmpDependencyCheckLib|FmpDevicePkg/Library/FmpDependencyCheckLibNull/FmpDependencyCheckLibNull.inf
      FmpDependencyDeviceLib|FmpDevicePkg/Library/FmpDependencyDeviceLibNull/FmpDependencyDeviceLibNull.inf
      #
      # Platform specific capsule policy library
      #
      CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
      #
      # Device specific library that processes a capsule and updates the FW storage device
      #
      FmpDeviceLib|WhiskeylakeOpenBoardPkg/Features/Capsule/Library/FmpDeviceLib/FmpDeviceLib.inf
  }
