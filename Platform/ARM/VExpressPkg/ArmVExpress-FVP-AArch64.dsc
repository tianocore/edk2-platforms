#
#  Copyright (c) 2011-2018, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ArmVExpress-FVP-AArch64
  PLATFORM_GUID                  = 0de70077-9b3b-43bf-ba38-0ea37d77141b
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/ArmVExpress-FVP-AArch64
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/ARM/VExpressPkg/ArmVExpress-FVP-AArch64.fdf

!ifndef ARM_FVP_RUN_NORFLASH
  DEFINE EDK2_SKIP_PEICORE=1
!endif

  DT_SUPPORT                     = FALSE

!include Platform/ARM/VExpressPkg/ArmVExpress.dsc.inc
!ifdef DYNAMIC_TABLES_FRAMEWORK
  !include DynamicTablesPkg/DynamicTables.dsc.inc
  !include Platform/ARM/VExpressPkg/ConfigurationManager/ConfigurationManager.dsc.inc
!endif

[LibraryClasses.common]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmPlatformLib|Platform/ARM/VExpressPkg/Library/ArmVExpressLibRTSM/ArmVExpressLib.inf
  ArmMmuLib|ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf

  ArmPlatformSysConfigLib|Platform/ARM/VExpressPkg/Library/ArmVExpressSysConfigLib/ArmVExpressSysConfigLib.inf
!ifdef EDK2_ENABLE_PL111
  LcdHwLib|ArmPlatformPkg/Library/PL111Lcd/PL111Lcd.inf
  LcdPlatformLib|Platform/ARM/VExpressPkg/Library/PL111LcdArmVExpressLib/PL111LcdArmVExpressLib.inf
!endif

  # Virtio Support
  VirtioLib|OvmfPkg/Library/VirtioLib/VirtioLib.inf
  VirtioMmioDeviceLib|OvmfPkg/Library/VirtioMmioDeviceLib/VirtioMmioDeviceLib.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
!endif

  DtPlatformDtbLoaderLib|Platform/ARM/VExpressPkg/Library/ArmVExpressDtPlatformDtbLoaderLib/ArmVExpressDtPlatformDtbLoaderLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  ArmPlatformSysConfigLib|Platform/ARM/VExpressPkg/Library/ArmVExpressSysConfigRuntimeLib/ArmVExpressSysConfigRuntimeLib.inf

[LibraryClasses.common.SEC]
  ArmPlatformLib|Platform/ARM/VExpressPkg/Library/ArmVExpressLibRTSM/ArmVExpressLibSec.inf

[LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf

[BuildOptions]
  GCC:*_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/Platform/ARM/VExpressPkg/Include/Platform/RTSM


################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]

  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

[PcdsFixedAtBuild.common]
  # Only one core enters UEFI, and PSCI is implemented in EL3 by ATF
  gArmPlatformTokenSpaceGuid.PcdCoreCount|1

  #
  # NV Storage PCDs. Use base of 0x0C000000 for NOR1
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x0FFC0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x0FFD0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x0FFE0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00010000

  gArmTokenSpaceGuid.PcdVFPEnabled|1

  # Stacks for MPCores in Normal World
  # Non-Trusted SRAM
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x2E000000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000
  gArmPlatformTokenSpaceGuid.PcdCPUCoreSecondaryStackSize|0x0

  # System Memory (2GB - 16MB of Trusted DRAM at the top of the 32bit address space)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x7F000000

  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000

  ## Trustzone enable (to make the transition from EL3 to NS EL2 in ArmPlatformPkg/Sec)
  gArmTokenSpaceGuid.PcdTrustzoneSupport|TRUE

  #
  # ARM PrimeCell
  #

  ## SP805 Watchdog - Motherboard Watchdog at 24MHz
  gArmPlatformTokenSpaceGuid.PcdSP805WatchdogBase|0x1C0F0000
  gArmPlatformTokenSpaceGuid.PcdSP805WatchdogClockFrequencyInHz|24000000

  ## PL011 - Serial Terminal
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x1c090000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultReceiveFifoDepth|0
  gArmPlatformTokenSpaceGuid.PL011UartInterrupt|0x25

  ## PL011 Serial Debug UART
  gArmPlatformTokenSpaceGuid.PcdSerialDbgRegisterBase|0x1c0a0000
  gArmPlatformTokenSpaceGuid.PcdSerialDbgUartBaudRate|115200
  gArmPlatformTokenSpaceGuid.PcdSerialDbgUartClkInHz|24000000

  # SBSA Generic Watchdog
  gArmTokenSpaceGuid.PcdGenericWatchdogEl2IntrNum|59

  ## PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x1C170000

  ## SBSA Watchdog Count
!ifndef DISABLE_SBSA_WATCHDOG
  gArmPlatformTokenSpaceGuid.PcdWatchdogCount|1
!endif

!ifdef EDK2_ENABLE_PL111
  ## PL111 Versatile Express Motherboard controller
  gArmPlatformTokenSpaceGuid.PcdPL111LcdBase|0x1C1F0000
!endif

  ## PL180 MMC/SD card controller
  gArmPlatformTokenSpaceGuid.PcdPL180SysMciRegAddress|0x1C010048
  gArmPlatformTokenSpaceGuid.PcdPL180MciBaseAddress|0x1C050000

  #
  # ARM General Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x2f000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x2f100000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x2C000000

  #
  # ARM Architectural Timer Frequency
  #
  # Set tick frequency value to 100Mhz
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|100000000

  # the entire FVP address space can be covered by 36 bit VAs
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuMemorySize|36

  #
  # ACPI Table Version
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiExposedTableVersions|0x20

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]

  #
  # PEI Phase modules
  #
!ifdef EDK2_SKIP_PEICORE
  # UEFI is placed in RAM by bootloader
  ArmPlatformPkg/PrePi/PeiUniCore.inf {
    <LibraryClasses>
      ArmPlatformLib|Platform/ARM/VExpressPkg/Library/ArmVExpressLibRTSM/ArmVExpressLib.inf
  }
!else
  # UEFI lives in FLASH and copies itself to RAM
  ArmPlatformPkg/PrePeiCore/PrePeiCoreUniCore.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  ArmPlatformPkg/PlatformPei/PlatformPeim.inf
  ArmPlatformPkg/MemoryInitPei/MemoryInitPeim.inf
  ArmPkg/Drivers/CpuPei/CpuPei.inf
  Nt32Pkg/BootModePei/BootModePei.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }
!endif

  #
  # DXE
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }

  #
  # Architectural Protocols
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
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf

  #
  # ACPI Support
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf {
!if $(DT_SUPPORT) == TRUE
  <LibraryClasses>
    NULL|EmbeddedPkg/Library/PlatformHasAcpiLib/PlatformHasAcpiLib.inf
!endif
  }

!ifndef DYNAMIC_TABLES_FRAMEWORK
  MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf
  Platform/ARM/VExpressPkg/AcpiTables/AcpiTables.inf
!endif

  ArmPkg/Drivers/ArmGic/ArmGicDxe.inf
  ArmPlatformPkg/Drivers/NorFlashDxe/NorFlashDxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf
!ifdef EDK2_ENABLE_PL111
  ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe/LcdGraphicsOutputDxe.inf
!endif
  ArmPlatformPkg/Drivers/SP805WatchdogDxe/SP805WatchdogDxe.inf

  # SMBIOS Support

  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

  #
  # Semi-hosting filesystem
  #
  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf

  #
  # Multimedia Card Interface
  #
  EmbeddedPkg/Universal/MmcDxe/MmcDxe.inf
  ArmPlatformPkg/Drivers/PL180MciDxe/PL180MciDxe.inf

  #
  # Platform Driver
  #
  Platform/ARM/VExpressPkg/Drivers/ArmVExpressDxe/ArmFvpDxe.inf
  OvmfPkg/VirtioBlkDxe/VirtioBlk.inf

  #
  # FAT filesystem + GPT/MBR partitioning
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  #
  # Bds
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

!if $(DT_SUPPORT) == TRUE
  #
  # FDT installation
  #
  EmbeddedPkg/Drivers/DtPlatformDxe/DtPlatformDxe.inf
!endif
