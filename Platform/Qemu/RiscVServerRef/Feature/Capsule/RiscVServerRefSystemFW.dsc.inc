## @file
#  FmpDxe driver for system firmware update.
#
#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  # System fip firmware image type guid.
  # This will be the filename of FmpDevicePkg and used to find firmware image
  DEFINE FMP_SYSTEM_DEVICE = 8F7A3C21-6D9E-4B85-A2F1-C7E934D1E6AB

[PcdsPatchableInModule.common]
  #
  # Firmware Update
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{GUID("$(FMP_SYSTEM_DEVICE)")}|VOID*|0x10

[Components.common]
  MdeModulePkg/Universal/EsrtFmpDxe/EsrtFmpDxe.inf
  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  }

FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # ESRT and FMP GUID for system device capsule update
      #
      FILE_GUID = $(FMP_SYSTEM_DEVICE)

    <PcdsFixedAtBuild>
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"RISC-V Server Reference System Firmware Device"

      #
      # Firmware revision that is reported by FMP
      # Can be overridden at build time with -D FIRMWARE_REVISION=0x...
      #
      !ifdef $(FIRMWARE_REVISION)
      gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision|$(FIRMWARE_REVISION)
      !endif

      #
      # ESRT and FMP Lowest Supported Version for this capsule update module
      # Override the DEC default (0x0) at build time with -D LSV=0x...
      #
      !ifdef $(LSV)
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|$(LSV)
      !else
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|0x00000001
      !endif

      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressWatchdogTimeInSeconds|2

      #
      # Capsule Update Progress Bar Color.  Set to Green (RGB) (0, 255, 0)
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressColor|0x0000FF00

      #
      # Certificates used to authenticate capsule update image
      # EDKII Test certificate
      #
      !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc

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
      CapsuleUpdatePolicyLib|Platform/Qemu/RiscVServerRef/Feature/Capsule/Library/CapsuleUpdatePolicyLib/CapsuleUpdatePolicyLib.inf

      #
      # Device specific library that processes a capsule and updates the FW storage device
      #
      FmpDeviceLib|Platform/Qemu/RiscVServerRef/Feature/Capsule/Library/FmpDeviceLib/FmpDeviceLib.inf
      VirtNorFlashDeviceLib|OvmfPkg/Library/VirtNorFlashDeviceLib/VirtNorFlashDeviceLib.inf
      PlatformFlashAccessLib|Platform/Qemu/RiscVServerRef/Feature/Capsule/Library/PlatformFlashAccessLib/PlatformFlashAccessLib.inf
  }
