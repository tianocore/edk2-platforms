#
#  Copyright (c) 2021-2026, Marvell International All rights reserved.
#  Copyright (c) 2014-2016, Cavium Inc. All rights reserved.
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = Cn10ka
  PLATFORM_GUID                  = 44e40dc4-0616-43bf-a42b-8536edab6755
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/Cn10ka
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Silicon/Marvell/CN10K/CN10K.fdf
  BOARD_DXE_FV_COMPONENTS        = Platform/Marvell/Cn10ka/Cn10ka.fdf.inc

!include Silicon/Marvell/CN10K/CN10K.dsc.inc

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Board
#
################################################################################

[PcdsFixedAtBuild.common]
  # NV Storage PCDs.
  gCN10KPlatformTokenSpaceGuid.PcdSpiNorChipSelect|0
  gCN10KPlatformTokenSpaceGuid.PcdSpiNorClkFrequency|25000000
  gCN10KPlatformTokenSpaceGuid.PcdSpiNorBusControllerNum|1
  gMarvellSiliconTokenSpaceGuid.PcdSpiMemoryMapped|FALSE
  gMarvellSiliconTokenSpaceGuid.PcdSpiVariableOffset|0x01E30000

  gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{GUID(E3B2F1A0-6C4D-4B8E-9F1A-2D7C5E8B3A61)}
  !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00040000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00040000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00040000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x00040000

  # Hardcoded terminal
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|4

  # Capsule on disk support
  gEfiMdeModulePkgTokenSpaceGuid.PcdCapsuleOnDiskSupport|TRUE

[PcdsDynamicDefault.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0x01E30000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0x01E70000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0x01EB0000

[PcdsDynamicHii.common.DEFAULT]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Board
#
################################################################################
[Components.common]
  #
  # USB NIC
  #
  Drivers/ASIX/Bus/Usb/UsbNetworking/Ax88772c/Ax88772c.inf
