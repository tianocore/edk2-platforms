#/** @file
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#  https://spdx.org/licenses
#
#  Copyright (C) 2023 Marvell
#
#  The main build description file for OdysseyPkg.
#**/

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = OdysseyPkg  # PLAT=ody
  PLATFORM_GUID                  = 7E7000DE-F50F-46AE-9B2C-903225F72B13
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
!ifdef $(EDK2_OUT_DIR) # Custom output directory, e.g. -D EDK2_OUT_DIR=Build/XYZ
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Marvell/$(PLATFORM_NAME)/$(PLATFORM_NAME).fdf

# dsc.inc file can be used in case there are different variants/boards of Odyssey family.
# Per-board additional components shall be defined in exclusive dsc.inc files.
!include Silicon/Marvell/$(PLATFORM_NAME)/$(PLATFORM_NAME).dsc.inc

[LibraryClasses]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf   # used by PlatformSmbiosDxe
  ArmMmuLib|ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
  ArmSmcLib|ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf # used by SmcLib

  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf # used by SpiNorDxe

  # USB Requirements
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf # used by UsbKbDxe
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf

[LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf # used by BaseBmpSupportLib
!if $(SECURE_BOOT_ENABLE) == TRUE
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
!endif
# ShellPkg/Application/Shell/Shell.inf -> UefiShellCommandLib -> OrderedCollectionLib
  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf # used by CapsuleApp
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf

[BuildOptions]
# GCC will generate code that runs on processors as idicated by -march
# Single = (append) allows flags appendixes coming from [BuildOptions] defined in specific INFs.
  GCC:*_*_AARCH64_PLATFORM_FLAGS = -DPLAT=0xBF -march=armv8.2-a -fdiagnostics-color -fno-diagnostics-show-caret
################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFixedAtBuild.common]

  # Generic Watchdog
  gArmTokenSpaceGuid.PcdGenericWatchdogControlBase|0x8020000A0000
  gArmTokenSpaceGuid.PcdGenericWatchdogRefreshBase|0x8020000B0000
  gArmTokenSpaceGuid.PcdGenericWatchdogEl2IntrNum|0x1A

  #  BIT0  - Initialization message.<BR>
  #  BIT1  - Warning message.<BR>
  #  BIT2  - Load Event message.<BR>
  #  BIT3  - File System message.<BR>
  #  BIT6  - Information message.<BR>
  #  DEBUG_ERROR     0x80000000  // Error
  # NOTE: Adjust according to needs. See MdePkg.dec for bits definition.
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8000004F

  # The size of volatile buffer. This buffer is used to store VOLATILE attribute variables.
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x00040000

  gArmTokenSpaceGuid.PcdVFPEnabled|1

  # Set ARM PCD: Odyssey: up to 80 Neoverse V2 cores (code named Demeter)
  # Used to setup secondary cores stacks and ACPI PPTT.
  gArmPlatformTokenSpaceGuid.PcdCoreCount|80

  # Stacks for MPCores in Normal World, Non-Trusted DRAM
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x2E000000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000

  # System Memory (40 - 1TB of DRAM)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x00004000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x10000000000

  # Size of the region used by UEFI in permanent memory (Reserved 128MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x08000000

  ## PL011 - Serial Terminal
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x87e028000000

  # ARM General Interrupt Controller
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x801000000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x801000080000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x801000020000

  # Hardcoded terminal: TTYTERM, NOT defined in UEFI SPEC
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|4

  # UART port Divisor setting based on clock 16.66Mhz and baud 115200
  gArmPlatformTokenSpaceGuid.PL011UartInteger|9
  gArmPlatformTokenSpaceGuid.PL011UartFractional|2

[PcdsDynamicDefault.common]

  # Indicates if Variable driver will enable emulated variable NV mode.
  # Reset by SpiNorDxe driver when SPI is in place and can handle storing EFI Variables.
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components]

  #
  # SEC Phase modules
  #

  # UEFI is placed in RAM by bootloader
  ArmPlatformPkg/PrePi/PeiUniCore.inf {
    <LibraryClasses>
      # SoC specific implementation of ArmPlatformLib
      ArmPlatformLib|Silicon/Marvell/OdysseyPkg/Library/OdysseyLib/OdysseyLib.inf
  }

  #
  # PEI Phase modules
  #
  # PEI phase is skipped. SEC jumps directly to DXE.

  #
  # Core DXE modules
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf

  #
  # DXE Status codes
  #
!if $(DEBUG) == 1
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
!endif

  #
  # PI DXE Drivers producing Architectural Protocols (EFI Services)
  #
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
  }
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!else
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
!endif
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
  }

  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  Silicon/Marvell/Drivers/Wdt/GtiWatchdogDxe/GtiWatchdogDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  # Produces gEfiFaultTolerantWriteProtocolGuid needed for non-volatile UEFI variable storage.
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf

  #
  # RTC Support
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf {
    <LibraryClasses>
      RealTimeClockLib|EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf
  }

  #
  # ARM Support
  #
  ArmPkg/Drivers/ArmGic/ArmGicDxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf

  #
  # Multiple Console IO support
  #
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
