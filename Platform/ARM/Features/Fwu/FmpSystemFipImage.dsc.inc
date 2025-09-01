#/** @file
# FmpDxe driver for AArch64 firmware update.
#
#  Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#
#**/
[Defines]
  # System fip firmware image type guid.
  # This will be the filename of FmpDevicePkg and used to find firmware image
  DEFINE FMP_SYSTEM_FIP_IMAGE_GUID = 49757D90-6C22-11EE-A556-1757EBA0420C

[PcdsPatchableInModule.common]
  #
  # Firmware Update
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{GUID("$(FMP_SYSTEM_FIP_IMAGE_GUID)")}|VOID*|0x10

[Components.common]
  MdeModulePkg/Universal/EsrtDxe/EsrtDxe.inf
  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  }

  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # ESRT and FMP GUID for system firmware capsule update
      #
      FILE_GUID = $(FMP_SYSTEM_FIP_IMAGE_GUID)

    <PcdsFixedAtBuild>
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"System Fip Firmware Image"

      #
      # ESRT and FMP Lowest Support Version for this capsule update module
      # 000.000.000.000
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|0x00000000

      gArmTokenSpaceGuid.PcdSystemFirmwareFmpLowestSupportedVersion|0x00000000
      gArmTokenSpaceGuid.PcdSystemFirmwareFmpVersion|0x00000000
      gArmTokenSpaceGuid.PcdSystemFirmwareFmpVersionString|"000.000.000.000"

      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressWatchdogTimeInSeconds|4

      #
      # Lock all updatable firmware devices at Exit boot Services.
      # This allows to update firmware via CapsuleApp.
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceLockEventGuid|{GUID(gEfiEventExitBootServicesGuid)}

      #
      # Capsule Update Progress Bar Color.  Set to Purple (RGB) (255, 0, 255)
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressColor|0x00FF00FF

      #
      # Certificates are used to authenticate capsule update image.
      # Root certificate should be converted to Pcd format using BinToPcd.py
      #
      #
      # Certificate used for FmpDevicePkg.
      #
      # Without generating Pcd certificate file, we couldn't build properly
      # with build option, ENABLE_FIRMWARE_UPDATE == TRUE.
      #
      # If you have your own certificate chain,
      # Please generate the Pcd file with your Root certificate:
      #     openssl x509 -in {ROOT_CERT_FILE} -out Root.cer -outform DER
      #     BinToPcd.py -i Root.cer gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr \
      #                 -x -o {CERT_PCD_FILE_PATH}
      # And Build with:
      #    -D FMP_SYSTEM_FIP_CERT_PCD_FILE={CERT_PCD_FILE_PATH}
      #
      # For test and development, you can generate all test certificates chain
      # and required Pcd file with:
      #     python3 Features/Fwu/GenTestCert.py
      #
      # NOTE: This tool should run after edk2 build setup.
      #
      # The tool generates all test certificates chain in 'TestCert' directory
      # and generate below then Pcd file in.
      #
      #    Platform/ARM/VExpressPkg/Root.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
      #
      #
      # And Build with:
      #    -D FMP_SYSTEM_FIP_CERT_PCD_FILE=Platform/ARM/VExpressPkg/Root.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
      #
      # For more detail, Please see "Quick Start" Section of
      # Platform/ARM/Features/Fwu/Readme.md
      #
      !include $(FMP_SYSTEM_FIP_CERT_PCD_FILE)

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
      CapsuleUpdatePolicyLib|Platform/ARM/Library/CapsuleUpdateRuntimePolicyLib/CapsuleUpdateRuntimePolicyLib.inf
      #
      # Device specific library that processes a capsule and updates the FW storage device
      #
      FmpDeviceLib|ArmPkg/Library/FmpDevicePsaFwuLib/FmpDevicePsaFwuLib.inf
  }
