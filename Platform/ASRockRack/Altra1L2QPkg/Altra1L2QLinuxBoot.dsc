## @file
#
# Copyright (c) 2020 - 2022, Ampere Computing LLC. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = Altra1L2Q
  PLATFORM_GUID                  = 57ce30d1-ad4d-41a0-a611-41ed20d33e50
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
  OUTPUT_DIRECTORY               = Build/Altra1L2Q
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ASRockRack/Altra1L2QPkg/Altra1L2QLinuxBoot.fdf

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #

  # DEBUG_INIT      0x00000001       // Initialization
  # DEBUG_WARN      0x00000002       // Warnings
  # DEBUG_LOAD      0x00000004       // Load events
  # DEBUG_FS        0x00000008       // EFI File system
  # DEBUG_POOL      0x00000010       // Alloc & Free (pool)
  # DEBUG_PAGE      0x00000020       // Alloc & Free (page)
  # DEBUG_INFO      0x00000040       // Informational debug messages
  # DEBUG_DISPATCH  0x00000080       // PEI/DXE/SMM Dispatchers
  # DEBUG_VARIABLE  0x00000100       // Variable
  # DEBUG_BM        0x00000400       // Boot Manager
  # DEBUG_BLKIO     0x00001000       // BlkIo Driver
  # DEBUG_NET       0x00004000       // Network Io Driver
  # DEBUG_UNDI      0x00010000       // UNDI Driver
  # DEBUG_LOADFILE  0x00020000       // LoadFile
  # DEBUG_EVENT     0x00080000       // Event messages
  # DEBUG_GCD       0x00100000       // Global Coherency Database changes
  # DEBUG_CACHE     0x00200000       // Memory range cachability changes
  # DEBUG_VERBOSE   0x00400000       // Detailed debug messages that may
  #                                  // significantly impact boot performance
  # DEBUG_MANAGEABILITY  0x00800000  // Detailed debug and payload manageability messages
  #                                  // related to modules such as Redfish, IPMI, MCTP etc.
  # DEBUG_ERROR  0x80000000          // Error
  DEFINE DEBUG_PRINT_ERROR_LEVEL = 0x8000004F
  DEFINE FIRMWARE_VER            = 0.01.001
  DEFINE EDK2_SKIP_PEICORE       = TRUE

!include MdePkg/MdeLibs.dsc.inc

# Include default Ampere Platform DSC file
!include Silicon/Ampere/AmpereAltraPkg/AmpereAltraLinuxBootPkg.dsc.inc

#
# Specific Platform Library
#
[LibraryClasses.common]
  #
  # ACPI Libraries
  #
  AcpiLib|EmbeddedPkg/Library/AcpiLib/AcpiLib.inf
  AcpiHelperLib|Platform/Ampere/AmperePlatformPkg/Library/AcpiHelperLib/AcpiHelperLib.inf

  #
  # Pcie Board
  #
  BoardPcieLib|Platform/ASRockRack/Altra1L2QPkg/Library/BoardPcieLib/BoardPcieLib.inf

  IOExpanderLib|Platform/ASRockRack/Altra1L2QPkg/Library/IOExpanderLib/IOExpanderLib.inf

  PlatformBmcReadyLib|Platform/ASRockRack/Altra1L2QPkg/Library/PlatformBmcReadyLib/PlatformBmcReadyLib.inf
  OemMiscLib|Platform/ASRockRack/Altra1L2QPkg/Library/OemMiscLib/OemMiscLib.inf

[LibraryClasses.common.PEIM]
  SmbusLib|MdePkg/Library/PeiSmbusLibSmbus2Ppi/PeiSmbusLibSmbus2Ppi.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  #
  # RTC Library: Common RTC
  #
  RealTimeClockLib|Platform/ASRockRack/Altra1L2QPkg/Library/PCF85063RealTimeClockLib/PCF85063RealTimeClockLib.inf

[LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_DRIVER]
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf

[PcdsFixedAtBuild.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0307

  gAmpereTokenSpaceGuid.PcdSmbiosTables0MajorVersion|$(MAJOR_VER)
  gAmpereTokenSpaceGuid.PcdSmbiosTables0MinorVersion|$(MINOR_VER)
!ifdef $(FIRMWARE_VER)
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"$(FIRMWARE_VER)"
!endif

  # Clearing BIT0 in this PCD prevents installing a 32-bit SMBIOS entry point,
  # if the entry point version is >= 3.0. AARCH64 OSes cannot assume the
  # presence of the 32-bit entry point anyway (because many AARCH64 systems
  # don't have 32-bit addressable physical RAM), and the additional allocations
  # below 4 GB needlessly fragment the memory map. So expose the 64-bit entry
  # point only, for entry point versions >= 3.0.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosEntryPointProvideMethod|0x2

#
# Specific Platform Component
#
[Components.common]

  #
  # ACPI
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  Platform/ASRockRack/Altra1L2QPkg/Drivers/AcpiPlatformDxe/AcpiPlatformDxe.inf
  Silicon/Ampere/AmpereAltraPkg/AcpiCommonTables/AcpiCommonTables.inf
  Platform/ASRockRack/Altra1L2QPkg/AcpiTables/AcpiTables.inf
  Platform/ASRockRack/Altra1L2QPkg/Ac02AcpiTables/Ac02AcpiTables.inf

  #
  # SMBIOS
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  ArmPkg/Universal/Smbios/ProcessorSubClassDxe/ProcessorSubClassDxe.inf
  ArmPkg/Universal/Smbios/SmbiosMiscDxe/SmbiosMiscDxe.inf
  Platform/ASRockRack/Altra1L2QPkg/Drivers/SmbiosPlatformDxe/SmbiosPlatformDxe.inf

  MdeModulePkg/Application/HelloWorld/HelloWorld.inf
