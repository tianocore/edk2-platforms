## @file
#  Compoenent description file specific for Morello FVP Platform
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
  PLATFORM_NAME                  = morellofvp
  PLATFORM_GUID                  = CB995FFD-EAEF-4d5E-8A4B-3213B39CD14A
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
  FLASH_DEFINITION               = Platform/ARM/Morello/MorelloPlatformFvp.fdf
  BUILD_NUMBER                   = 1

  # Network definition
  DEFINE NETWORK_ISCSI_ENABLE    = FALSE

# include common/basic libraries from MdePkg.
!include MdePkg/MdeLibs.dsc.inc

!include Platform/ARM/Morello/MorelloPlatform.dsc.inc
!include Platform/ARM/VExpressPkg/ArmVExpress.dsc.inc
!include DynamicTablesPkg/DynamicTables.dsc.inc
!include Platform/ARM/Morello/ConfigurationManager/ConfigurationManagerFvp.dsc.inc

[LibraryClasses.common]
  # Platform Library
  ArmPlatformLib|Platform/ARM/Morello/Library/PlatformLib/PlatformLibFvp.inf

  # Virtio Support
  VirtioLib|OvmfPkg/Library/VirtioLib/VirtioLib.inf
  VirtioMmioDeviceLib|OvmfPkg/Library/VirtioMmioDeviceLib/VirtioMmioDeviceLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf

[LibraryClasses.common.DXE_DRIVER]
  PciHostBridgeLib|Platform/ARM/Morello/Library/PciHostBridgeLib/PciHostBridgeLibFvp.inf

[PcdsFeatureFlag.common]
  gArmMorelloTokenSpaceGuid.PcdVirtioBlkSupported|TRUE
  gArmMorelloTokenSpaceGuid.PcdVirtioNetSupported|TRUE

[PcdsFixedAtBuild.common]
  # Virtio Disk
  gArmMorelloTokenSpaceGuid.PcdVirtioBlkBaseAddress|0x1C170000
  gArmMorelloTokenSpaceGuid.PcdVirtioBlkSize|0x10000
  gArmMorelloTokenSpaceGuid.PcdVirtioBlkInterrupt|128

  # Virtio Net
  gArmMorelloTokenSpaceGuid.PcdVirtioNetBaseAddress|0x1C180000
  gArmMorelloTokenSpaceGuid.PcdVirtioNetSize|0x10000
  gArmMorelloTokenSpaceGuid.PcdVirtioNetInterrupt|134

  # Runtime Variable storage
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvStoreReserved|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800

  #FVP Specific PCD values for PCIe
  gArmTokenSpaceGuid.PcdPciBusMax|15
  gArmTokenSpaceGuid.PcdPciIoBase|0x0
  gArmTokenSpaceGuid.PcdPciIoSize|0x00400000
  gArmTokenSpaceGuid.PcdPciMmio32Base|0x60000000
  gArmTokenSpaceGuid.PcdPciMmio32Size|0x0F000000
  gArmTokenSpaceGuid.PcdPciMmio64Base|0x900000000
  gArmTokenSpaceGuid.PcdPciMmio64Size|0x2000000000

  gArmMorelloTokenSpaceGuid.PcdPciMmio64MaxBase|0x28FFFFFFFF

  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x20000000
  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation|0x6F000000
  gEfiMdePkgTokenSpaceGuid.PcdPciMmio32Translation|0x0
  gEfiMdePkgTokenSpaceGuid.PcdPciMmio64Translation|0x0

[Components.common]
  OvmfPkg/VirtioBlkDxe/VirtioBlk.inf
  OvmfPkg/VirtioNetDxe/VirtioNet.inf

  # Platform driver
  Platform/ARM/Morello/Drivers/PlatformDxe/PlatformDxeFvp.inf
  # PEI Phase modules
  Platform/ARM/Morello/Drivers/MorelloNtFwConfigPei/Fvp.inf

  #
  # Semi-hosting filesystem
  #
  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf
  # Runtime Variable support
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }

  ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe/LcdGraphicsOutputDxe.inf {
    <LibraryClasses>
      LcdHwLib|Platform/ARM/Morello/Library/LcdHwMaliDxxLib/LcdHwMaliDxxLib.inf
      LcdPlatformLib|Platform/ARM/Morello/Library/LcdPlatformLibMorello/LcdPlatformLibMorelloFvp.inf
  }
