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

  [LibraryClasses.common.DXE_DRIVER]
  PciHostBridgeLib|Platform/ARM/Morello/Library/PciHostBridgeLib/PciHostBridgeLibSoc.inf
  PciSegmentLib|Platform/ARM/Morello/Library/PciSegmentLib/PciSegmentLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|Platform/ARM/Morello/Library/PciExpressLib/PciExpressLib.inf

[PcdsFixedAtBuild.common]
  # PCIe
  gArmTokenSpaceGuid.PcdPciBusMax|255
  gArmTokenSpaceGuid.PcdPciMmio32Base|0x60000000
  gArmTokenSpaceGuid.PcdPciMmio32Size|0x0F000000
  gArmTokenSpaceGuid.PcdPciMmio64Base|0x900000000
  gArmTokenSpaceGuid.PcdPciMmio64Size|0x1FC0000000
  gArmTokenSpaceGuid.PcdPciIoSize|0x00400000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x28C0000000
  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation|0x6F000000
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|24

  # Runtime Variable storage
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvStoreReserved|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800

[Components.common]
  # Platform driver
  Platform/ARM/Morello/Drivers/PlatformDxe/PlatformDxeSoc.inf
  # PEI Phase modules
  Platform/ARM/Morello/Drivers/MorelloNtFwConfigPei/Soc.inf


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
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }
