## @file
#  Compoenent description file specific for Morello SoC Platform
#
#  Copyright (c) 2021 - 2023, ARM Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = morellosoc
  PLATFORM_GUID                  = 8AC37B62-713D-449D-876D-06AD1B8E67E5
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = NOOPT|DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/Morello/MorelloPlatformSoc.fdf
  BUILD_NUMBER                   = 1

!include Platform/ARM/Morello/MorelloPlatform.dsc.inc
!include Platform/ARM/VExpressPkg/ArmVExpress.dsc.inc
!include DynamicTablesPkg/DynamicTables.dsc.inc
!include Platform/ARM/Morello/ConfigurationManager/ConfigurationManagerSoc.dsc.inc

# include common/basic libraries from MdePkg.
!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  # Platform Library
  ArmPlatformLib|Platform/ARM/Morello/Library/PlatformLib/PlatformLibSoc.inf

  #USB Requirement
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf

  # NOR flash support
  NorFlashInfoLib|EmbeddedPkg/Library/NorFlashInfoLib/NorFlashInfoLib.inf

  [LibraryClasses.common.DXE_DRIVER]
  PciHostBridgeLib|Platform/ARM/Morello/Library/PciHostBridgeLib/PciHostBridgeLibSoc.inf
  PciSegmentLib|Platform/ARM/Morello/Library/PciSegmentLib/PciSegmentLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|Platform/ARM/Morello/Library/PciExpressLib/PciExpressLib.inf

[PcdsFixedAtBuild.common]
  # PCIe
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|24
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport|FALSE

  # NOR flash support
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x1AF40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00020000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x1AF20000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00020000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x1AF00000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00020000

[Components.common]
  # Platform driver
  Platform/ARM/Morello/Drivers/PlatformDxe/PlatformDxeSoc.inf
  # PEI Phase modules
  Platform/ARM/Morello/Drivers/MorelloNtFwConfigPei/Soc.inf


  # NOR flash support
  Platform/ARM/Morello/Drivers/CadenceQspiDxe/CadenceQspiDxe.inf {
      <LibraryClasses>
      NorFlashPlatformLib|Platform/ARM/Morello/Library/NorFlashLib/NorFlashLib.inf
  }

  # Usb Support
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  # NVMe boot devices
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  # Runtime Variable support
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|EmbeddedPkg/Library/NvVarStoreFormattedLib/NvVarStoreFormattedLib.inf
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
