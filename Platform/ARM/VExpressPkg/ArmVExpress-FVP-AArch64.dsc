#
#  Copyright (c) 2011-2021, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
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

  # To allow the use of ueif secure variable feature, set this to TRUE.
  DEFINE ENABLE_UEFI_SECURE_VARIABLE = FALSE

!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  DEFINE ENABLE_STMM             = TRUE
!else
  DEFINE ENABLE_STMM             = FALSE
!endif

!ifndef ARM_FVP_RUN_NORFLASH
  DEFINE EDK2_SKIP_PEICORE=TRUE
!else
  DEFINE EDK2_SKIP_PEICORE=FALSE
!endif

  DT_SUPPORT                     = FALSE

!include MdePkg/MdeLibs.dsc.inc
!include Platform/ARM/VExpressPkg/ArmVExpress.dsc.inc
!include DynamicTablesPkg/DynamicTables.dsc.inc

[LibraryClasses.common]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmPlatformLib|Platform/ARM/VExpressPkg/Library/ArmVExpressLibRTSM/ArmVExpressLib.inf
  ArmMmuLib|UefiCpuPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf

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

!if $(ENABLE_STMM) == TRUE
  MmUnblockMemoryLib|MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf
!endif

  DtPlatformDtbLoaderLib|Platform/ARM/VExpressPkg/Library/ArmVExpressDtPlatformDtbLoaderLib/ArmVExpressDtPlatformDtbLoaderLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  ArmFfaLib|MdeModulePkg/Library/ArmFfaLib/ArmFfaDxeLib.inf
  ArmPlatformSysConfigLib|Platform/ARM/VExpressPkg/Library/ArmVExpressSysConfigRuntimeLib/ArmVExpressSysConfigRuntimeLib.inf

[LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf

  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciHostBridgeLib|Platform/ARM/VExpressPkg/Library/ArmVExpressPciHostBridgeLib/ArmVExpressPciHostBridgeLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf

[BuildOptions]
  GCC:*_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/Platform/ARM/VExpressPkg/Include/Platform/RTSM
!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  GCC:*_*_*_CC_FLAGS = -DENABLE_UEFI_SECURE_VARIABLE
!endif

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]

  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  ## Disable Runtime Variable Cache.
  gEfiMdeModulePkgTokenSpaceGuid.PcdEnableVariableRuntimeCache|FALSE
!endif

[PcdsFixedAtBuild.common]
  # Only one core enters UEFI, and PSCI is implemented in EL3 by ATF
  gArmPlatformTokenSpaceGuid.PcdCoreCount|1

  #
  # NV Storage PCDs. Use base of 0x0C000000 for NOR1
  #
!if $(ENABLE_UEFI_SECURE_VARIABLE) == FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x0FFC0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x0FFD0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x0FFE0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00010000
!endif

  #
  # Set the base address and size of the buffer used
  # by MM_COMMUNICATE for communication between the
  # Normal world edk2 and the StandaloneMm image at S-EL0.
  # This buffer is allocated in TF-A.
  # This value based on TF-A !ENABLE_RME where Normal shared area
  # is located in (2GB - 17MB) as much as 1MB.
  #
!if $(ENABLE_STMM) == TRUE
  ## MM Communicate
  gArmTokenSpaceGuid.PcdMmBufferBase|0xFEF00000
  gArmTokenSpaceGuid.PcdMmBufferSize|0x10000
!endif

  # Non-Trusted SRAM
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x2E000000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000

  # System Memory
  # When RME is supported by the FVP the top 64MB of DRAM1 (i.e. at the top
  # of the 32bit address space) is reserved for four-world support in TF-A.
  # And Normal shared area with Secure world is reserved 1MB from
  # (2GB - 65MB).
  # Therefore, set the default System Memory size to (2GB - 65MB).
  #
  # +-------------------------------------+ 0x80000000 (PcdSystemMemoryBase)
  # |                                     |
  # |                                     |
  # |                                     |
  # |     System Memory  (2GB - 65MB)     |
  # |                                     |
  # |                                     |
  # +-------------------------------------+ 0xfbf00000 (PcdSystemMemoryBase + PcdSystemMemorySize)
  # |   Reserved for normal world (1MB)   |
  # |   (NS buffer, pesudo CRB and etc)   |
  # +-------------------------------------+ 0xfc000000
  # |   Reserved for secure world (64MB)  |
  # |     (RME, StandaloneMm and etc)     |
  # +-------------------------------------+ 0xffffffff
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x7BF00000

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
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x1c0a0000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultReceiveFifoDepth|0
  gArmPlatformTokenSpaceGuid.PL011UartInterrupt|0x26

  ## PL011 Serial Debug UART (DBG2)
  gArmPlatformTokenSpaceGuid.PcdSerialDbgRegisterBase|0x1c0b0000
  gArmPlatformTokenSpaceGuid.PcdSerialDbgUartBaudRate|115200
  gArmPlatformTokenSpaceGuid.PcdSerialDbgUartClkInHz|24000000

  ## PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x1C170000

  ## SBSA Watchdog Count
  gArmPlatformTokenSpaceGuid.PcdWatchdogCount|1
  gArmTokenSpaceGuid.PcdGenericWatchdogControlBase|0x2a440000
  gArmTokenSpaceGuid.PcdGenericWatchdogRefreshBase|0x2a450000

!ifdef EDK2_ENABLE_PL111
  ## PL111 Versatile Express Motherboard controller
  gArmPlatformTokenSpaceGuid.PcdPL111LcdBase|0x1C1F0000
!endif

  ## PL180 MMC/SD card controller
  gArmVExpressTokenSpaceGuid.PcdPL180SysMciRegAddress|0x1C010048
  gArmVExpressTokenSpaceGuid.PcdPL180MciBaseAddress|0x1C050000

  #
  # ARM Generic Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x2f000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x2f100000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x2C000000

  gArmTokenSpaceGuid.PcdGicIrsConfigFrameBase|0x2f1a0000

  #
  # PCI Root Complex
  #
  gArmTokenSpaceGuid.PcdPciBusMin|0
  gArmTokenSpaceGuid.PcdPciBusMax|255

  gArmTokenSpaceGuid.PcdPciMmio32Base|0x50000000
  gArmTokenSpaceGuid.PcdPciMmio32Size|0x10000000

  gArmTokenSpaceGuid.PcdPciMmio64Base|0x4000000000
  gArmTokenSpaceGuid.PcdPciMmio64Size|0x4000000000

  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x40000000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseSize|0x10000000

  #
  # ACPI Table Version
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiExposedTableVersions|0x20

[PcdsDynamicDefault.common]
  # ARM Generic Watchdog Interrupt number for GIC pre-v5
  # This will be overwritten when GICv5 is in use
  gArmTokenSpaceGuid.PcdGenericWatchdogEl2IntrNum|59

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]

  #
  # PEI Phase modules
  #
!if $(EDK2_SKIP_PEICORE) == TRUE
  # UEFI is placed in RAM by bootloader
  ArmPlatformPkg/PeilessSec/PeilessSec.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
      ArmPlatformLib|Platform/ARM/VExpressPkg/Library/ArmVExpressLibRTSM/ArmVExpressLib.inf
  }
!else
  # UEFI lives in FLASH and copies itself to RAM
  ArmPlatformPkg/Sec/Sec.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  ArmPlatformPkg/PlatformPei/PlatformPeim.inf
  ArmPlatformPkg/MemoryInitPei/MemoryInitPeim.inf
  ArmPkg/Drivers/CpuPei/CpuPei.inf
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

!if $(ENABLE_UEFI_SECURE_VARIABLE) == TRUE
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
!else
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|EmbeddedPkg/Library/NvVarStoreFormattedLib/NvVarStoreFormattedLib.inf
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
!endif

  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x1c090000
  }

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

  Platform/ARM/VExpressPkg/ConfigurationManager/ConfigurationManagerDxe/ConfigurationManagerDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x1c090000
      gArmPlatformTokenSpaceGuid.PL011UartInterrupt|0x25
  }

  ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  Platform/ARM/Drivers/NorFlashDxe/NorFlashDxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf

!ifdef EDK2_ENABLE_PL111
  ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe/LcdGraphicsOutputDxe.inf
!endif
  ArmPkg/Drivers/GenericWatchdogDxe/GenericWatchdogDxe.inf {
    <LibraryClasses>
      NULL|Platform/ARM/VExpressPkg/Library/ArmVExpressWatchdogLib/ArmVExpressWatchdogLib.inf
  }

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
  Platform/ARM/VExpressPkg/Drivers/PL180MciDxe/PL180MciDxe.inf

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
  FatPkg/EnhancedFatDxe/Fat.inf

  #
  # Bds
  #
  MdeModulePkg/Universal/BootManagerPolicyDxe/BootManagerPolicyDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/BootDiscoveryPolicyUiLib/BootDiscoveryPolicyUiLib.inf
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

  #
  # PCI Support
  #
  ArmPkg/Drivers/ArmPciCpuIo2Dxe/ArmPciCpuIo2Dxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf

  #
  # AHCI Support
  #
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf

  #
  # SATA Controller
  #
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf

!if $(ENABLE_STMM) == TRUE
  ArmPkg/Drivers/MmCommunicationDxe/MmCommunication.inf {
    <LibraryClasses>
      NULL|StandaloneMmPkg/Library/VariableMmDependency/VariableMmDependency.inf
  }
!endif
