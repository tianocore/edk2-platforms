## @file
#  RISC-V EFI on RISC-V Server Reference platform
#
#  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#  Copyright (c) 2022, Ventana Micro Systems Inc. All rights reserved.<BR>
#  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = RiscVServerRef
  PLATFORM_GUID                  = 54AC0E13-B5AA-48E3-9C80-C80E97A99C71
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001c
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Qemu/RiscVServerRef/RiscVServerRef.fdf

  DEFINE RISCV_PEI_BOOTING       = TRUE
  DEFINE TTY_TERMINAL            = FALSE
  DEFINE DEBUG_ON_SERIAL_PORT    = TRUE

  #
  # Shell can be useful for debugging but should not be enabled for production
  #
  DEFINE BUILD_SHELL             = TRUE

  #
  # Network definition
  #
  DEFINE NETWORK_IP6_ENABLE             = TRUE
  DEFINE NETWORK_HTTP_BOOT_ENABLE       = TRUE
  DEFINE NETWORK_HTTP_ENABLE            = TRUE
  DEFINE NETWORK_TLS_ENABLE             = TRUE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS = TRUE
  DEFINE NETWORK_ISCSI_ENABLE           = FALSE
  DEFINE NETWORK_PXE_BOOT_ENABLE        = TRUE
  DEFINE NETWORK_VLAN_ENABLE            = FALSE

!include MdePkg/MdeLibs.dsc.inc
!include NetworkPkg/Network.dsc.inc

[BuildOptions]
  GCC:RELEASE_*_*_CC_FLAGS       = -DMDEPKG_NDEBUG
!ifdef $(SOURCE_DEBUG_ENABLE)
  GCC:*_*_RISCV64_GENFW_FLAGS    = --keepexceptiontable
!endif

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include Silicon/RISC-V/RiscV.dsc.inc

[LibraryClasses]
  # AML code generation (DynamicTablesPkg)
  AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf

  # Common Libraries
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf

  # Flash Libraries
  VirtNorFlashDeviceLib|OvmfPkg/Library/VirtNorFlashDeviceLib/VirtNorFlashDeviceLib.inf
  VirtNorFlashPlatformLib|Silicon/Qemu/RiscVServerRef/Library/NorFlashLib/NorFlashLib.inf

  # PCI Libraries
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  PciCapLib|OvmfPkg/Library/BasePciCapLib/BasePciCapLib.inf
  PciCapPciSegmentLib|OvmfPkg/Library/BasePciCapPciSegmentLib/BasePciCapPciSegmentLib.inf
  PciCapPciIoLib|OvmfPkg/Library/UefiPciCapPciIoLib/UefiPciCapPciIoLib.inf
  PciPcdProducerLib|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PciHostBridgeLib|Silicon/Qemu/RiscVServerRef/Library/PciHostBridgeLib/PciHostBridgeLib.inf

!if $(NETWORK_TLS_ENABLE) == TRUE
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
!endif

  #
  # CryptoPkg libraries needed by multiple firmware features
  #
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
!if $(NETWORK_TLS_ENABLE) == TRUE
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
!else
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
!endif
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  RngLib|MdeModulePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform.
#
################################################################################
[PcdsFeatureFlag]


[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdRiscVFeatureOverride|0xFFFFFFFFFFFFFFF0
  gUefiRiscVPlatformPkgTokenSpaceGuid.PcdRiscVPeiBootEnable|TRUE

  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|0
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x2

  # DEBUG_ASSERT_ENABLED       0x01
  # DEBUG_PRINT_ENABLED        0x02
  # DEBUG_CODE_ENABLED         0x04
  # CLEAR_MEMORY_ENABLED       0x08
  # ASSERT_BREAKPOINT_ENABLED  0x10
  # ASSERT_DEADLOOP_ENABLED    0x20

!ifdef $(SOURCE_DEBUG_ENABLE)
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2F
!endif

  #  DEBUG_INIT      0x00000001  // Initialization
  #  DEBUG_WARN      0x00000002  // Warnings
  #  DEBUG_LOAD      0x00000004  // Load events
  #  DEBUG_FS        0x00000008  // EFI File system
  #  DEBUG_POOL      0x00000010  // Alloc & Free (pool)
  #  DEBUG_PAGE      0x00000020  // Alloc & Free (page)
  #  DEBUG_INFO      0x00000040  // Informational debug messages
  #  DEBUG_DISPATCH  0x00000080  // PEI/DXE/SMM Dispatchers
  #  DEBUG_VARIABLE  0x00000100  // Variable
  #  DEBUG_BM        0x00000400  // Boot Manager
  #  DEBUG_BLKIO     0x00001000  // BlkIo Driver
  #  DEBUG_NET       0x00004000  // SNP Driver
  #  DEBUG_UNDI      0x00010000  // UNDI Driver
  #  DEBUG_LOADFILE  0x00020000  // LoadFile
  #  DEBUG_EVENT     0x00080000  // Event messages
  #  DEBUG_GCD       0x00100000  // Global Coherency Database changes
  #  DEBUG_CACHE     0x00200000  // Memory range cachability changes
  #  DEBUG_VERBOSE   0x00400000  // Detailed debug messages that may significantly impact boot performance
  #  DEBUG_ERROR     0x80000000  // Error

!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000047
!endif

!ifdef $(SOURCE_DEBUG_ENABLE)
  gEfiSourceLevelDebugPkgTokenSpaceGuid.PcdDebugLoadImageMethod|0x2
!endif

  #
  # Optional feature to help prevent EFI memory map fragments
  # Turned on and off via: PcdPrePiProduceMemoryTypeInformationHob
  # Values are in EFI Pages (4K). DXE Core will make sure that
  # at least this much of each type of memory can be allocated
  # from a single memory range. This way you only end up with
  # maximum of two fragments for each type in the memory map
  # (the memory used, and the free memory that was prereserved
  # but not used).
  #
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiACPIReclaimMemory|10
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiACPIMemoryNVS|96
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiReservedMemoryType|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiRuntimeServicesData|300
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiRuntimeServicesCode|150
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiBootServicesCode|1000
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiBootServicesData|6000
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiLoaderCode|20
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiLoaderData|0

  # Serial Port
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x10000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|9600
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|3686400
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|1

  gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE

################################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamicDefault]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0

  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|5

  # set PcdPciExpressBaseAddress to MAX_UINT64, which signifies that this
  # PCD and PcdPciDisableBusEnumeration above have not been assigned yet
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xFFFFFFFFFFFFFFFF
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation|0x0

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform.
#
################################################################################
[Components]
  #
  # Platform Driver
  #
  OvmfPkg/VirtNorFlashDxe/VirtNorFlashDxe.inf

  #
  # RISC-V IOMMU
  #
  Silicon/Qemu/RiscVServerRef/Drivers/RiscVIoMmuDxe/RiscVIoMmuDxe.inf

  # PCI support
  #
  UefiCpuPkg/CpuMmio2Dxe/CpuMmio2Dxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf

  # SATA/AHCI Support
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf

  #
  # SCSI Bus and Disc drivers.
  #
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  #
  # NVME support
  #
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  #
  # X86Emulator for running X86 OptionROM code of e1000 card.
  #
  Emulator/X86EmulatorDxe/X86EmulatorDxe.inf

  #
  # Shell add-on commands
  #
  ShellPkg/DynamicCommand/TftpDynamicCommand/TftpDynamicCommand.inf
  ShellPkg/DynamicCommand/HttpDynamicCommand/HttpDynamicCommand.inf

  #
  # RNG driver backed by pseudo-RNG (timer-based, not for production)
  #
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf

  #
  # ACPI table generation
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  Platform/RISC-V/PlatformPkg/Drivers/AcpiPlatformDxe/AcpiPlatformDxe.inf
