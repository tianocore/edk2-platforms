## @file
#
# Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>
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
  PLATFORM_NAME                  = Jade
  PLATFORM_GUID                  = 7BDD00C0-68F3-4CC1-8775-F0F00572019F
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
  OUTPUT_DIRECTORY               = Build/Jade
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Ampere/JadePkg/Jade.fdf

  #
  # Defines for default states. These can be changed on the command line.
  # -D FLAG=VALUE
  #

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
  #  DEBUG_VERBOSE   0x00400000  // Detailed debug messages that may
  #                              // significantly impact boot performance
  #  DEBUG_ERROR     0x80000000  // Error
  !if $(TARGET) == RELEASE
    DEFINE DEBUG_PRINT_ERROR_LEVEL = 0x80000002
  !else
    DEFINE DEBUG_PRINT_ERROR_LEVEL = 0x8000000F
  !endif
  DEFINE FIRMWARE_VER            = 0.01.001
  DEFINE SECURE_BOOT_ENABLE      = TRUE
  DEFINE TPM2_ENABLE             = TRUE
  DEFINE INCLUDE_TFTP_COMMAND    = TRUE
  DEFINE PLATFORM_CONFIG_UUID    = 84BC921F-9D4A-4D1D-A1A1-1AE13EDD07E5

  #
  # Network definition
  #
  DEFINE NETWORK_IP6_ENABLE                  = FALSE
  DEFINE NETWORK_HTTP_BOOT_ENABLE            = TRUE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS      = TRUE
  DEFINE NETWORK_TLS_ENABLE                  = TRUE
  DEFINE REDFISH_ENABLE                      = TRUE

!include MdePkg/MdeLibs.dsc.inc

# Include default Ampere Platform DSC file
!include Silicon/Ampere/AmpereAltraPkg/AmpereAltraPkg.dsc.inc

################################################################################
#
# Specific Platform Library
#
################################################################################
[LibraryClasses]
  #
  # ACPI Libraries
  #
  AcpiLib|EmbeddedPkg/Library/AcpiLib/AcpiLib.inf

  #
  # Pcie Board
  #
  BoardPcieLib|Platform/Ampere/JadePkg/Library/BoardPcieLib/BoardPcieLib.inf

  OemMiscLib|Platform/Ampere/JadePkg/Library/OemMiscLib/OemMiscLib.inf

  PlatformBmcReadyLib|Platform/Ampere/JadePkg/Library/PlatformBmcReadyLib/PlatformBmcReadyLib.inf
  IOExpanderLib|Platform/Ampere/JadePkg/Library/IOExpanderLib/IOExpanderLib.inf

  #
  # EFI Redfish drivers
  #
!if $(REDFISH_ENABLE) == TRUE
  RedfishContentCodingLib|RedfishPkg/Library/RedfishContentCodingLibNull/RedfishContentCodingLibNull.inf
  RedfishPlatformHostInterfaceLib|RedfishPkg/Library/PlatformHostInterfaceBmcUsbNicLib/PlatformHostInterfaceBmcUsbNicLib.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  #
  # RTC Library: Common RTC
  #
  RealTimeClockLib|Platform/Ampere/JadePkg/Library/PCF85063RealTimeClockLib/PCF85063RealTimeClockLib.inf

################################################################################
#
# Specific Platform Pcds
#
################################################################################
[PcdsFeatureFlag.common]
  #
  # Activate AcpiSdtProtocol
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE

  #
  # Flag to indicate option of using default or specific platform Port Map table
  #
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.UseDefaultConfig|TRUE

[PcdsFixedAtBuild]
  gAmpereTokenSpaceGuid.PcdPcieHotPlugGpioResetMap|0x3F

  #
  # Setting Portmap table
  #
  #   * Elements of array:
  #     - 0:  Index of Portmap entry in Portmap table structure (Vport).
  #     - 1:  Socket number (Socket).
  #     - 2:  Root complex port for each Portmap entry (RcaPort).
  #     - 3:  Root complex sub-port for each Portmap entry (RcaSubPort).
  #     - 4:  Select output port of IO expander (PinPort).
  #     - 5:  I2C address of IO expander that CPLD backplane simulates (I2cAddress).
  #     - 6:  Address of I2C switch between CPU and CPLD backplane (MuxAddress).
  #     - 7:  Channel of I2C switch (MuxChannel).
  #     - 8:  It is set from PcieHotPlugSetGPIOMapCmd () function to select GPIO[16:21] (PcdPcieHotPlugGpioResetMap) or I2C for PCIe reset purpose.
  #     - 9:  Segment of root complex (Segment).
  #     - 10: SSD slot index on the front panel of backplane (DriveIndex).
  #
  #   * Caution:
  #     - The last array ({ 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF }) require if no fully structured used.
  #     - Size of Portmap table: PortMap[MAX_PORT_MAP_ENTRY][sizeof(PCIE_HOT_PLUG_PORTMAP_ENTRY)] <=> PortMap[96][11].
  #   * Example: Bellow configuration is the configuration for Portmap table of Mt. Jade 2U platform.
  #
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[0]|{ 0, 0, 2, 0, 0, 0x00, 0x00, 0x0, 0, 1, 0xFF }   # S0 RCA2.0
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[1]|{ 1, 0, 3, 0, 1, 0x00, 0x00, 0x0, 0, 0, 0xFF }   # S0 RCA3.0
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[2]|{ 2, 0, 4, 0, 2, 0x27, 0x70, 0x1, 0, 2, 6 }      # S0 RCB0.0 - SSD6
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[3]|{ 3, 0, 4, 2, 3, 0x27, 0x70, 0x1, 0, 2, 7 }      # S0 RCB0.2 - SSD7
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[4]|{ 4, 0, 4, 4, 0, 0x25, 0x70, 0x1, 0, 2, 2 }      # S0 RCB0.4 - SSD2
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[5]|{ 5, 0, 4, 6, 1, 0x25, 0x70, 0x1, 0, 2, 3 }      # S0 RCB0.6 - SSD3
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[6]|{ 6, 0, 5, 0, 0, 0x24, 0x70, 0x1, 0, 3, 0 }      # S0 RCB1.0 - SSD0
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[7]|{ 7, 0, 5, 2, 1, 0x24, 0x70, 0x1, 0, 3, 1 }      # S0 RCB1.2 - SSD1
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[8]|{ 8, 0, 5, 4, 2, 0x26, 0x70, 0x1, 0, 3, 4 }      # S0 RCB1.4 - SSD4
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[9]|{ 9, 0, 5, 6, 3, 0x26, 0x70, 0x1, 0, 3, 5 }      # S0 RCB1.6 - SSD5
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[10]|{ 10, 0, 6, 0, 2, 0x00, 0x00, 0x0, 0, 4, 0xFF } # S0 RCB2.0
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[11]|{ 11, 0, 6, 2, 3, 0x00, 0x00, 0x0, 0, 4, 0xFF } # S0 RCB2.2
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[12]|{ 12, 0, 6, 4, 0, 0x00, 0x00, 0x0, 0, 4, 0xFF } # S0 RCB2.4
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[13]|{ 13, 0, 7, 0, 1, 0x00, 0x00, 0x0, 0, 5, 0xFF } # S0 RCB3.0
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[14]|{ 14, 0, 7, 4, 2, 0x00, 0x00, 0x0, 0, 5, 0xFF } # S0 RCB3.4
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[15]|{ 15, 0, 7, 6, 3, 0x00, 0x00, 0x0, 0, 5, 0xFF } # S0 RCB3.6
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[16]|{ 16, 1, 2, 0, 0, 0x26, 0x70, 0x2, 0, 6, 20 }   # S1 RCA2.0 - SSD20
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[17]|{ 17, 1, 2, 1, 1, 0x26, 0x70, 0x2, 0, 6, 21 }   # S1 RCA2.1 - SSD21
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[18]|{ 18, 1, 2, 2, 2, 0x27, 0x70, 0x2, 0, 6, 22 }   # S1 RCA2.2 - SSD22
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[19]|{ 19, 1, 2, 3, 3, 0x27, 0x70, 0x2, 0, 6, 23 }   # S1 RCA2.3 - SSD23
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[20]|{ 20, 1, 3, 0, 0, 0x00, 0x00, 0x0, 0, 7, 0xFF } # S1 RCA3.0
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[21]|{ 21, 1, 3, 2, 1, 0x00, 0x00, 0x0, 0, 7, 0xFF } # S1 RCA3.2
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[22]|{ 22, 1, 4, 0, 0, 0x00, 0x00, 0x0, 0, 8, 0xFF } # S1 RCB0.0
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[23]|{ 23, 1, 4, 4, 2, 0x25, 0x70, 0x2, 0, 8, 18 }   # S1 RCB0.4 - SSD18
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[24]|{ 24, 1, 4, 6, 3, 0x25, 0x70, 0x2, 0, 8, 19 }   # S1 RCB0.6 - SSD19
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[25]|{ 25, 1, 5, 0, 0, 0x24, 0x70, 0x2, 0, 9, 16 }   # S1 RCB1.0 - SSD16
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[26]|{ 26, 1, 5, 2, 1, 0x24, 0x70, 0x2, 0, 9, 17 }   # S1 RCB1.2 - SSD17
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[27]|{ 27, 1, 5, 4, 2, 0x00, 0x00, 0x0, 0, 9, 0xFF } # S1 RCB1.4
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[28]|{ 28, 1, 6, 0, 3, 0x25, 0x70, 0x4, 0, 10, 11 }  # S1 RCB2.0 - SSD11
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[29]|{ 29, 1, 6, 2, 2, 0x25, 0x70, 0x4, 0, 10, 10 }  # S1 RCB2.2 - SSD10
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[30]|{ 30, 1, 6, 4, 1, 0x27, 0x70, 0x4, 0, 10, 15 }  # S1 RCB2.4 - SSD15
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[31]|{ 31, 1, 6, 6, 0, 0x27, 0x70, 0x4, 0, 10, 14 }  # S1 RCB2.6 - SSD14
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[32]|{ 32, 1, 7, 0, 3, 0x26, 0x70, 0x4, 0, 11, 13 }  # S1 RCB3.0 - SSD13
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[33]|{ 33, 1, 7, 2, 2, 0x26, 0x70, 0x4, 0, 11, 12 }  # S1 RCB3.2 - SSD12
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[34]|{ 34, 1, 7, 4, 1, 0x24, 0x70, 0x4, 0, 11, 9 }   # S1 RCB3.4 - SSD9
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[35]|{ 35, 1, 7, 6, 0, 0x24, 0x70, 0x4, 0, 11, 8 }   # S1 RCB3.6 - SSD8
  gAmpereTokenSpaceGuid.PcdPcieHotPlugPortMapTable.PortMap[36]|{ 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF }       # Require if no fully structure used

[PcdsFixedAtBuild.common]
  #
  # Platform config UUID
  #
  gAmpereTokenSpaceGuid.PcdPlatformConfigUuid|"$(PLATFORM_CONFIG_UUID)"

  # Clearing BIT0 in this PCD prevents installing a 32-bit SMBIOS entry point,
  # if the entry point version is >= 3.0. AARCH64 OSes cannot assume the
  # presence of the 32-bit entry point anyway (because many AARCH64 systems
  # don't have 32-bit addressable physical RAM), and the additional allocations
  # below 4 GB needlessly fragment the memory map. So expose the 64-bit entry
  # point only, for entry point versions >= 3.0.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosEntryPointProvideMethod|0x2

!if $(SECURE_BOOT_ENABLE) == TRUE
  # Override the default values from SecurityPkg to ensure images
  # from all sources are verified in secure boot
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04
!endif

[PcdsDynamicDefault.common.DEFAULT]

[PcdsDynamicExDefault.common.DEFAULT]
  gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareImageDescriptor|{0x0}|VOID*|0x100
  gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{0x31, 0xca, 0x8b, 0xf0, 0x2e, 0x54, 0xea, 0x4c, 0x8b, 0x48, 0x8e, 0x54, 0xf9, 0x42, 0x25, 0x94}
  gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareFileGuid|{0xed, 0x06, 0x1c, 0x43, 0xe2, 0x4f, 0x8f, 0x43, 0x98, 0xa3, 0xa9, 0xb1, 0xfd, 0x92, 0x30, 0x19}

[PcdsPatchableInModule]
  #
  # Console Resolution (HD mode)
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|1024
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|768

################################################################################
#
# Specific Platform Component
#
################################################################################
[Components.common]
  #
  # ACPI
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2B
  }
  Platform/Ampere/JadePkg/Drivers/AcpiPlatformDxe/AcpiPlatformDxe.inf
  Silicon/Ampere/AmpereAltraPkg/AcpiCommonTables/AcpiCommonTables.inf
  Platform/Ampere/JadePkg/AcpiTables/AcpiTables.inf
  Platform/Ampere/JadePkg/Ac02AcpiTables/Ac02AcpiTables.inf

  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf

  #
  # PCIe
  #
  Platform/Ampere/JadePkg/Drivers/PciPlatformDxe/PciPlatformDxe.inf

  #
  # VGA Aspeed
  #
  Drivers/ASpeed/ASpeedGopBinPkg/ASpeedAst2500GopDxe.inf

  #
  # SMBIOS
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  Platform/Ampere/JadePkg/Drivers/SmbiosPlatformDxe/SmbiosPlatformDxe.inf
  ArmPkg/Universal/Smbios/ProcessorSubClassDxe/ProcessorSubClassDxe.inf
  ArmPkg/Universal/Smbios/SmbiosMiscDxe/SmbiosMiscDxe.inf

  #
  # HII
  #
  Silicon/Ampere/AmpereAltraPkg/Drivers/RootComplexConfigDxe/RootComplexConfigDxe.inf
  Silicon/Ampere/AmpereAltraPkg/Drivers/PlatformInfoDxe/PlatformInfoDxe.inf
  Silicon/Ampere/AmpereAltraPkg/Drivers/MemInfoDxe/MemInfoDxe.inf
  Silicon/Ampere/AmpereAltraPkg/Drivers/CpuConfigDxe/CpuConfigDxe.inf
  Silicon/Ampere/AmpereAltraPkg/Drivers/AcpiConfigDxe/AcpiConfigDxe.inf
  Silicon/Ampere/AmpereAltraPkg/Drivers/RasConfigDxe/RasConfigDxe.inf
  Silicon/Ampere/AmpereSiliconPkg/Drivers/BmcConfigDxe/BmcConfigDxe.inf

  #
  # Firmware Capsule Update
  #
  Platform/Ampere/JadePkg/Capsule/SystemFirmwareDescriptor/SystemFirmwareDescriptor.inf
  MdeModulePkg/Universal/EsrtDxe/EsrtDxe.inf
  SignedCapsulePkg/Universal/SystemFirmwareUpdate/SystemFirmwareReportDxe.inf
  SignedCapsulePkg/Universal/SystemFirmwareUpdate/SystemFirmwareUpdateDxe.inf
  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf

  #
  # System Firmware Update
  #
  Silicon/Ampere/AmpereAltraPkg/Drivers/SystemFirmwareUpdateDxe/SystemFirmwareUpdateDxe.inf

  #
  # Redfish
  #
!if $(REDFISH_ENABLE) == TRUE
  MdeModulePkg/Bus/Usb/UsbNetwork/NetworkCommon/NetworkCommon.inf
  MdeModulePkg/Bus/Usb/UsbNetwork/UsbCdcEcm/UsbCdcEcm.inf
!include RedfishPkg/Redfish.dsc.inc
!endif
