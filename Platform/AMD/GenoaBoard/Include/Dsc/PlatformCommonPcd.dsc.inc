#;*****************************************************************************
#; Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
#; SPDX-License-Identifier: BSD-2-Clause-Patent
#;*****************************************************************************

[Defines]
!ifndef SECURE_BOOT_ENABLE
  DEFINE SECURE_BOOT_ENABLE = TRUE
!endif
!ifndef PLATFORM_SECURE
  DEFINE PLATFORM_SECURE       = FALSE
!endif
  DEFINE NETWORK_IP6_ENABLE    = FALSE

  #
  # Redfish support
  #
  DEFINE REDFISH_ENABLE                 = FALSE

  #
  # Set Platform Redfish configuration
  #
!if $(REDFISH_ENABLE) == TRUE

  # Enable BMC USB NIC as the Redfish transport interface
  DEFINE USB_NETWORK_SUPPORT            = TRUE

  # Allow HTTP connection for Redfish
  DEFINE NETWORK_SNP_ENABLE             = TRUE
  DEFINE NETWORK_IP6_ENABLE             = TRUE
  DEFINE NETWORK_IP4_ENABLE             = TRUE
  DEFINE NETWORK_HTTP_ENABLE            = TRUE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS = TRUE
!endif

[Packages]
  AmdCpmPkg/AmdCpmPkg.dec
  AmdMinBoardPkg/AmdMinBoardPkg.dec
  AmdPlatformPkg/AmdPlatformPkg.dec
  ManageabilityPkg/ManageabilityPkg.dec
  MinPlatformPkg/MinPlatformPkg.dec
  Network/NetworkFeaturePkg/NetworkFeaturePkg.dec
  PcAtChipsetPkg/PcAtChipsetPkg.dec
  SecurityPkg/SecurityPkg.dec
  SpcrFeaturePkg/SpcrFeaturePkg.dec
  UefiCpuPkg/UefiCpuPkg.dec

# MinPlatformPkg includes
!include AdvancedFeaturePkg/Include/AdvancedFeaturesPcd.dsc

[PcdsFixedAtBuild]
  #
  # Key Boot Stage
  #
  # Please select the Boot Stage here.
  # Stage 1 - enable debug (system deadloop after debug init)
  # Stage 2 - mem init (system deadloop after mem init)
  # Stage 3 - boot to shell only
  # Stage 4 - boot to OS
  # Stage 5 - boot to OS with security boot enabled
  # Stage 6 - boot with advanced features enabled
  #
  gMinPlatformPkgTokenSpaceGuid.PcdBootStage|6

  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchI2c2Irq|0x0C
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchI2c3Irq|0x0D
  gAmdCpmPkgTokenSpaceGuid.PcdAmdAcpiBertTableHeaderOemTableId|$(PLATFORM_CRB_TABLE_ID)
  gAmdCpmPkgTokenSpaceGuid.PcdAmdAcpiHestTableHeaderOemTableId|$(PLATFORM_CRB_TABLE_ID)
  gAmdCpmPkgTokenSpaceGuid.PcdAmdAcpiEinjTableHeaderOemTableId|$(PLATFORM_CRB_TABLE_ID)

  # Set ROM Armor Selection
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdPspRomArmorSelection|1

  #
  # Set EFI Shell file description
  #
  gMinPlatformPkgTokenSpaceGuid.PcdShellFileDesc|L"Internal UEFI Shell 2.2"

  #
  # BSP Broadcast Method for the first-time wakeup of APs
  #
  gUefiCpuPkgTokenSpaceGuid.PcdFirstTimeWakeUpAPsBySipi|FALSE

[PcdsFeatureFlag]
  #
  # MinPlatformPkg Configuration
  #
  gMinPlatformPkgTokenSpaceGuid.PcdStopAfterDebugInit|FALSE
  gMinPlatformPkgTokenSpaceGuid.PcdStopAfterMemInit|FALSE
  gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable|FALSE
  gMinPlatformPkgTokenSpaceGuid.PcdPerformanceEnable|FALSE
  gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable|FALSE
  gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly|FALSE
  gMinPlatformPkgTokenSpaceGuid.PcdSerialTerminalEnable|TRUE

  gEfiMdeModulePkgTokenSpaceGuid.PcdPciBusHotplugDeviceSupport|FALSE

  !if gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 1
    gMinPlatformPkgTokenSpaceGuid.PcdStopAfterDebugInit|TRUE
  !endif

  !if gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 2
    gMinPlatformPkgTokenSpaceGuid.PcdStopAfterDebugInit|FALSE
    gMinPlatformPkgTokenSpaceGuid.PcdStopAfterMemInit|TRUE
  !endif

  !if gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 3
    gMinPlatformPkgTokenSpaceGuid.PcdStopAfterMemInit|FALSE
    gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly|TRUE
  !endif

  !if gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 4
    gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly|FALSE
  !endif

  !if gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 5
    !if $(SIMNOW_SUPPORT) == FALSE || $(EMULATION) == FALSE
      gNetworkFeaturePkgTokenSpaceGuid.PcdNetworkFeatureEnable|TRUE
      gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable|$(SECURE_BOOT_ENABLE)
      gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable|TRUE
    !endif
    gSpcrFeaturePkgTokenSpaceGuid.PcdSpcrFeatureEnable|TRUE
    gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosFeatureEnable|TRUE
  !endif

# Below include file should be here
# after PcdBootStage is set.
# and after respective features are enabled/disabled depends on PcdBootStage
!include AdvancedFeaturePkg/Include/AdvancedFeatures.dsc

  #
  # Below are Manageability feature knobs.
  #
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiBmcAcpi|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiBmcElog|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiEnable|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiFrb|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiFru|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiOsWdt|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiSolStatus|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeMctpEnable|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxePldmEnable|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxePldmSmbiosTransferEnable|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityPeiIpmiEnable|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityPeiIpmiFrb|FALSE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilitySmmIpmiEnable|FALSE
  # Enable IPMI feature for boot stage >=5 and only for real SoC
  !if (gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 5) && ($(SIMNOW_SUPPORT) == FALSE || $(EMULATION) == FALSE)
    gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiEnable|TRUE
  !endif

  !if $(SIMNOW_SUPPORT) == TRUE || $(EMULATION) == TRUE
    gEfiMdeModulePkgTokenSpaceGuid.PcdPciBusHotplugDeviceSupport|FALSE
  !endif

  # MdeModulePkg
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwarePerformanceDataTableS3Support|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDegradeResourceForOptionRom|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|TRUE

  # Uefi Cpu Package
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmBlockStartupThisAp|TRUE
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmStackGuard|FALSE
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmEnableBspElection|FALSE
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmFeatureControlMsrLock|FALSE

  # ACPI
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE

  # Enable ROM Armor
  gAmdPlatformPkgTokenSpaceGuid.PcdRomArmorEnable|TRUE

[PcdsFixedAtBuild.IA32]
  #
  # Temporary DRAM space for SEC->PEI transition (256KB)
  # AMD_ENABLE_UEFI_STACK (Flat32.asm) divides: 1/2 Heap + 1/2 Stack
  #
  gAmdMinBoardPkgTokenSpaceGuid.PcdTempRamBase|0x00100000
  gAmdMinBoardPkgTokenSpaceGuid.PcdTempRamSize|0x00100000

  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeiStackSize|0x80000

[PcdsFixedAtBuild]
  # Console/Uart settings
  !if $(SERIAL_PORT) == "FCH_MMIO"
    # MMIO based flow control UART0, this option is ideal for physical serial cable attached
    gAmdCpmPkgTokenSpaceGuid.PcdFchUartPort|0
    ## Base address of 16550 serial port registers in MMIO or I/O space.
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0xFEDC9000
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|4
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|48000000
    # Cannot assign PCD to PCD, hence setting the SPCR IRQ here
    gSpcrFeaturePkgTokenSpaceGuid.PcdSpcrInterrupt|3
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortSelect|0x0001
  !endif

  !if $(SERIAL_PORT) == "FCH_IO"
    # Legacy based flow control UART0, this option is ideal for physical serial cable attached
    gAmdCpmPkgTokenSpaceGuid.PcdFchUartPort|0
    ## Base address of 16550 serial port registers in MMIO or I/O space.
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x3F8
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0x00
    # Cannot assign PCD to PCD, hence setting the SPCR IRQ here
    gSpcrFeaturePkgTokenSpaceGuid.PcdSpcrInterrupt|3
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortSelect|0x0100
  !endif

  !if $(SERIAL_PORT) == "BMC_SOL"
    # MMIO based non-flow control UART1, this option is ideal for physical serial cable attached
    gAmdCpmPkgTokenSpaceGuid.PcdFchUartPort|1
    ## Base address of 16550 serial port registers in MMIO or I/O space.
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0xFEDCA000
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|4
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|48000000
    # Cannot assign PCD to PCD, hence setting the SPCR IRQ here
    gSpcrFeaturePkgTokenSpaceGuid.PcdSpcrInterrupt|0xE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortSelect|0x0002
  !endif

  !if $(SERIAL_PORT) == "BMC_SOL_IO"
    # Legacy based non-flow control UART1, this option is ideal for physical serial cable attached
    gAmdCpmPkgTokenSpaceGuid.PcdFchUartPort|1
    ## Base address of 16550 serial port registers in MMIO or I/O space.
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x3F8
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0x00
    # Cannot assign PCD to PCD, hence setting the SPCR IRQ here
    gSpcrFeaturePkgTokenSpaceGuid.PcdSpcrInterrupt|0xE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortSelect|0x0200
  !endif

  !if $(SERIAL_PORT) == "BMC_ESPI"
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x3F8
    gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0x00
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortSelect|0x0000
  !endif

  # Indicates the receive FIFO depth of UART controller.
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultReceiveFifoDepth|64

  # Default Value of PlatformLangCodes Variable.
  gEfiMdePkgTokenSpaceGuid.PcdUefiVariableDefaultPlatformLangCodes|"en-US"

  ## The mask is used to control ReportStatusCodeLib behavior.
  #  BIT0 - Enable Progress Code.
  #  BIT1 - Enable Error Code.
  #  BIT2 - Enable Debug Code.
  !ifdef $(INTERNAL_IDS)
    gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
  !else
    gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x00
  !endif

  #
  # Debug Masks
  #
  # //
  # // Declare bits for PcdDebugPropertyMask
  # //
  # DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED       0x01
  # DEBUG_PROPERTY_DEBUG_PRINT_ENABLED        0x02
  # DEBUG_PROPERTY_DEBUG_CODE_ENABLED         0x04
  # DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED       0x08
  # DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED  0x10
  # DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED    0x20
  # //
  # // Declare bits for PcdFixedDebugPrintErrorLevel and the ErrorLevel parameter of DebugPrint()
  # //
  # DEBUG_INIT           0x00000001  // Initialization
  # DEBUG_WARN           0x00000002  // Warnings
  # DEBUG_LOAD           0x00000004  // Load events
  # DEBUG_FS             0x00000008  // EFI File system
  # DEBUG_POOL           0x00000010  // Alloc & Free's
  # DEBUG_PAGE           0x00000020  // Alloc & Free's
  # DEBUG_INFO           0x00000040  // Informational debug messages
  # DEBUG_DISPATCH       0x00000080  // PEI/DXE/SMM Dispatchers
  # DEBUG_VARIABLE       0x00000100  // Variable
  # DEBUG_BM             0x00000400  // Boot Manager
  # DEBUG_BLKIO          0x00001000  // BlkIo Driver
  # DEBUG_NET            0x00004000  // SNI Driver
  # DEBUG_UNDI           0x00010000  // UNDI Driver
  # DEBUG_LOADFILE       0x00020000  // UNDI Driver
  # DEBUG_EVENT          0x00080000  // Event messages
  # DEBUG_GCD            0x00100000  // Global Coherency Database changes
  # DEBUG_CACHE          0x00200000  // Memory range cachability changes
  # DEBUG_VERBOSE        0x00400000  // Detailed debug messages that may significantly impact boot performance
  # DEBUG_MANAGEABILITY  0x00800000  // Detailed debug and payload message of manageability
  #                                  // related modules, such Redfish, IPMI, MCTP and etc.
  # DEBUG_ERROR          0x80000000  // Error

  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
  !if $(DEBUG_DISPATCH_ENABLE)
    gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x808000CF
  !else
    gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel|0x8080004F
  !endif

  #
  # AGESA Debug
  #
  !if ($(IDS_DEBUG_ENABLE) == TRUE) AND ($(SERIAL_PORT) != "NONE")
    # IdsDebugPrint Filter. Refer to Library/IdsLib.h for details.
    # 0x100401008A30042C (GNB_TRACE | PCIE_MISC | NB_MISC | GFX_MISC  | CPU_TRACE | MEM_FLOW |
    #                     MEM_STATUS | MEM_PMU | FCH_TRACE | MAIN_FLOW | TEST_POINT | PSP_TRACE)
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintFilter|0x1004010300300400
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintEnable|TRUE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortEnable|TRUE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortDetectCableConnection|FALSE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPort|gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase
  !else
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintEnable|FALSE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintSerialPortEnable|FALSE
  !endif

  !if gMinPlatformPkgTokenSpaceGuid.PcdPerformanceEnable == TRUE
    gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x1
    gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|80
  !else
    gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x00
  !endif

  #
  # Specifies the initial value for Register_D in RTC.
  # Reason for change:
  #  PcRtc.c wants to see register D bit 7 (VRT) high almost immediately after writing the below value,
  #  which clears it with the default UEFI value of zero. The AMD FCH updates this bit only once per 4-1020ms (1020ms default).
  #  This causes function RtcWaitToUpdate to return an error. Preset VRT to 1 to avoid this.
  #
  gPcAtChipsetPkgTokenSpaceGuid.PcdInitialValueRtcRegisterD|0x80

  #
  # SMBIOS
  #
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdSmbiosT16MaximumCapacity|0x80000000
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdSmbiosSocketDesignationSocket0|"P0"
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdSmbiosSocketDesignationSocket1|"P1"

  #
  # PCIe Config-space MMIO (1MB per bus, 256MB)
  #
  !if $(SIMNOW_SUPPORT) == FALSE || $(EMULATION) == FALSE
    gEfiMdeModulePkgTokenSpaceGuid.PcdPcieResizableBarSupport|TRUE
  !endif

  !if $(PCIE_MULTI_SEGMENT) == TRUE
    gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x3FFB00000000
    gAmdMinBoardPkgTokenSpaceGuid.PcdPciExpressBaseAddressLow|0x0
    gAmdMinBoardPkgTokenSpaceGuid.PcdPciExpressBaseAddressHi|0x3FFB
  !else
    gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
    gAmdMinBoardPkgTokenSpaceGuid.PcdPciExpressBaseAddressLow|0xE0000000
    gAmdMinBoardPkgTokenSpaceGuid.PcdPciExpressBaseAddressHi|0x0
  !endif

  #
  # Boot
  #
  # PCDs to set the default size of the different UEFI memory types to promote
  # contiguous UEFI memory allocation.  These values are used by
  # AmdCommon/Pei/PlatformInitPei/MemoryInitPei.c to reserve
  # default chunks for each memory type when gEfiMemoryTypeInformationGuid
  # variable is not set.  These values can be updated to prevent reboot because
  # MdeModulePkg/Library/UefiBootManagerLib/BmMisc.c:
  # BmSetMemoryTypeInformationVariable() sets gEfiMemoryTypeInformationGuid at
  # the end of post to reserve more memory.  Serial output from this code will
  # display sizes required, which can then be updated in these PCDs.
  # Memory Type 09
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiAcpiReclaimMemorySize        | 0x400
  # Memory Type 0A
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiAcpiNvsMemorySize            | 0x400
  # Memory Type 00
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiReservedMemorySize           | 0x5000
  # Memory Type 06
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiRtDataMemorySize             | 0x800
  # Memory Type 05
  gMinPlatformPkgTokenSpaceGuid.PcdPlatformEfiRtCodeMemorySize             | 0x100
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|10
  !if $(EMULATION) == TRUE
    gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|0
  !endif
  # 462CAA21-7614-4503-836E-8AB6F4662331 (UiApp FILE_GUID)
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ GUID("462CAA21-7614-4503-836E-8AB6F4662331") }

  # 1GB page support
  gEfiMdeModulePkgTokenSpaceGuid.PcdUse1GPageTable|TRUE

  #
  # ACPI
  #
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemEnableAcpiSwSmi|0xA0
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemDisableAcpiSwSmi|0xA1

  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId|"AMD   "
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision|0x00000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorId|0x20444D41
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorRevision|0x00000001

  gMinPlatformPkgTokenSpaceGuid.PcdFadtPreferredPmProfile|4
  gMinPlatformPkgTokenSpaceGuid.PcdFadtIaPcBootArch|0x0000
  gMinPlatformPkgTokenSpaceGuid.PcdFadtFlags|0x0002052D
  gPcAtChipsetPkgTokenSpaceGuid.PcdHpetBaseAddress|0xFED00000
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdAcpiCpuSsdtProcessorScopeInSb|TRUE

  # NOTE, below PCD should match with gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgFchIoapicId
  gMinPlatformPkgTokenSpaceGuid.PcdIoApicId|0x80
  gMinPlatformPkgTokenSpaceGuid.PcdPcIoApicCount|4
  # NOTE, below PCD should match with gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgIoApicIdBase
  gMinPlatformPkgTokenSpaceGuid.PcdPcIoApicIdBase|0xF1
  gMinPlatformPkgTokenSpaceGuid.PcdPcIoApicAddressBase|0xFEC00000
  gMinPlatformPkgTokenSpaceGuid.PcdLocalApicAddress|0xFEE00000
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiEnableSwSmi|gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemEnableAcpiSwSmi
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiDisableSwSmi|gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemDisableAcpiSwSmi
  !if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
    gMinPlatformPkgTokenSpaceGuid.PcdWsmtProtectionFlags|0x07
  !endif

  # Max Cpu constraints
  gMinPlatformPkgTokenSpaceGuid.PcdMaxCpuThreadCount|2

  #
  # EFI NV Storage
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0xA000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0xA000

  #
  # AGESA NBIO
  #
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgIommuMMIOAddressReservedEnable|TRUE
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgIoApicMMIOAddressReservedEnable|TRUE
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgIoApicIdPreDefineEn|TRUE   #### Makes PEI assign IOAPIC IDs
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgIoApicIdBase|0xF1

  gEfiAmdAgesaPkgTokenSpaceGuid.PcdCompliantEdkIIAcpiSdtProtocol|TRUE

  # AGESA FCH
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiPm1EvtBlkAddr|0x800
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiPm1CntBlkAddr|0x804
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiPmTmrBlkAddr|0x808
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgCpuControlBlkAddr|0x810
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiGpe0BlkAddr|0x820
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemBeforePciRestoreSwSmi|0x00
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemAfterPciRestoreSwSmi|0x00
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemSpiUnlockSwSmi|0xB7
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdFchOemSpiLockSwSmi|0xB8
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdNumberOfPhysicalSocket|gMinPlatformPkgTokenSpaceGuid.PcdMaxCpuSocketCount
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdAmdMemCfgMaxPostPackageRepairEntries|0x3F

  # Disable S3 support
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiS3Enable|FALSE

  ## Toggle for whether the VariablePolicy engine should allow disabling.
  # The engine is enabled at power-on, but the interface allows the platform to
  # disable enforcement for servicing flexibility. If this PCD is disabled, it will block the ability to
  # disable the enforcement and VariablePolicy enforcement will always be ON.
  #   TRUE - VariablePolicy can be disabled by request through the interface (until interface is locked)
  #   FALSE - VariablePolicy interface will not accept requests to disable and is ALWAYS ON
  # @Prompt Allow VariablePolicy enforcement to be disabled.
  gEfiMdeModulePkgTokenSpaceGuid.PcdAllowVariablePolicyEnforcementDisable|TRUE

  #
  # FALSE: The board is not a FSP wrapper (FSP binary not used)
  # TRUE:  The board is a FSP wrapper (FSP binary is used)
  #
  gMinPlatformPkgTokenSpaceGuid.PcdFspWrapperBootMode|FALSE

  # TRUE  - 5-Level Paging will be enabled.
  # FALSE - 5-Level Paging will not be enabled.
  gEfiMdeModulePkgTokenSpaceGuid.PcdUse5LevelPageTable|FALSE

  # Specifies stack size in bytes for each processor in SMM.
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmStackSize|0x10000
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmShadowStackSize|0x10000
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdAcpiTableHeaderOemId|gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId
  !if $(EMULATION) == TRUE
    # enable IDS prints for emulation to port80
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdIdsDebugPrintEmulationAutoDetect|$(IDS_DEBUG_ENABLE)
    gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdAmdPspRecoveryFlagDetectEnable|FALSE
  !endif

  # Secureboot
  !if gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable == TRUE
    gEfiSecurityPkgTokenSpaceGuid.PcdUserPhysicalPresence|TRUE
  !endif
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiGpe0BlockAddress|gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiGpe0BlkAddr
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiGpe1BlockAddress|0
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiPm1AControlBlockAddress|gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiPm1CntBlkAddr
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiPm1AEventBlockAddress|gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiPm1EvtBlkAddr
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiPm1BControlBlockAddress|0
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiPm1BEventBlockAddress|0
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiPm2ControlBlockAddress|0
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiPmTimerBlockAddress|gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgAcpiPmTmrBlkAddr
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiGpe0BlockLength|0x8
  gMinPlatformPkgTokenSpaceGuid.PcdFadtDutyOffset|0x1
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPm1aEvtBlkAccessSize|0x2
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPm1aCntBlkAccessSize|0x2
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXPmTmrBlkAccessSize|0x03
  gMinPlatformPkgTokenSpaceGuid.PcdAcpiXGpe0BlkAccessSize|0x1

  #
  # The base address of temporary page table for accessing PCIE MMIO base address above 4G in PEI phase.
  #
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdPeiTempPageTableBaseAddress|0x60000000
  !if $(DRAM_BOOT) == TRUE
    gMinPlatformPkgTokenSpaceGuid.PcdShellFile|{GUID(68198A68-D249-4826-BC5E-45DF0CCA2A53)}
    gMinPlatformPkgTokenSpaceGuid.PcdShellFileDesc|L"Emulation Linux Loader"
  !endif

  # To create MPDMA devices under RB named as PCIx
  gAmdCpmPkgTokenSpaceGuid.UsePciXAslName|TRUE

  #
  # edk2 Redfish foundation
  #
  !if $(REDFISH_ENABLE) == TRUE
    gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceDevicePath.DevicePathMatchMode|DEVICE_PATH_MATCH_MAC_NODE
    gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceDevicePath.DevicePathNum|1
    #
    # Below is the MAC address of network adapter on EDK2 Emulator platform.
    # You can use ifconfig under EFI shell to get the MAC address of network adapter on EDK2 Emulator platform.
    #
    gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceDevicePath.DevicePath|{DEVICE_PATH("MAC(005056c00009,0x1)")}
    gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceAccessModeInBand|True
    gEfiRedfishPkgTokenSpaceGuid.PcdRedfishDiscoverAccessModeInBand|True
  !endif

  #
  # USB Network (Communication Device Class) drivers
  #
  !if $(USB_NETWORK_SUPPORT) == TRUE
    # Set USB NIC Rate Limiting
    gEfiMdeModulePkgTokenSpaceGuid.PcdEnableUsbNetworkRateLimiting|TRUE
  !endif

[PcdsDynamicDefault.common]
  #
  # Set MMIO Above4GB at the 1TB boundary
  #
  !if $(PCIE_MULTI_SEGMENT) == TRUE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdMmioAbove4GLimit|0x3FFBFFFFFFFF
  !else
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdMmioAbove4GLimit|0x7FBFFFFFFFF
  !endif

  # IO Resource padding in bytes, default 4KB, override to 0.
  gAmdMinBoardPkgTokenSpaceGuid.PcdPciHotPlugResourcePadIo|0x00

  #
  # Flash NV Storage
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0

  #
  # AGESA FCH
  #
    gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdHpetEnable|TRUE
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdHpetMsiDis|FALSE
    gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdNoneSioKbcSupport|TRUE
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgFchIoapicId|gMinPlatformPkgTokenSpaceGuid.PcdIoApicId

  # Tell AGESA how you want the UART configured for serial output
  # FchRTDeviceEnableMap
  # < BIT4 - LPC : PcdLpcEnable
  # < BIT5 - I2C0 : FchRTDeviceEnableMap[BIT5]
  # < BIT6 - I2C1 : FchRTDeviceEnableMap[BIT6]
  # < BIT7 - I2C2 : FchRTDeviceEnableMap[BIT7]
  # < BIT8 - I2C3 : FchRTDeviceEnableMap[BIT8]
  # < BIT9 - I2C4 : FchRTDeviceEnableMap[BIT9]
  # < BIT10 - I2C5 : FchRTDeviceEnableMap[BIT10]
  # < BIT11 - UART0 : FchRTDeviceEnableMap[BIT11]
  # < BIT12 - UART1 : FchRTDeviceEnableMap[BIT12]
  # < BIT16 - UART2 : FchRTDeviceEnableMap[BIT13]
  # < BIT18 - SD : PcdEmmcEnable and PcdEmmcType < 5
  # < BIT26 - UART3 : FchRTDeviceEnableMap[BIT26]
  # < BIT27 - eSPI : PcdEspiEnable - read-only.
  # < BIT28 - eMMC : PcdEmmcEnable - read-only.
  gEfiAmdAgesaModulePkgTokenSpaceGuid.FchRTDeviceEnableMap|0x00001F60
    # FchUartLegacyEnable
  # 0-disable, 1- 0x2E8/2EF, 2 - 0x2F8/2FF, 3 - 0x3E8/3EF, 4 - 0x3F8/3FF
  !if $(SERIAL_PORT) == "FCH_IO"
    gEfiAmdAgesaPkgTokenSpaceGuid.FchUart0LegacyEnable|0x04
    gEfiAmdAgesaPkgTokenSpaceGuid.FchUart1LegacyEnable|0x03
  !elseif  $(SERIAL_PORT) == "BMC_SOL_IO"
    gEfiAmdAgesaPkgTokenSpaceGuid.FchUart0LegacyEnable|0x03
    gEfiAmdAgesaPkgTokenSpaceGuid.FchUart1LegacyEnable|0x04
  !endif

  #
  # AGESA APCB Recovery
  #
  # TO-DO: Temporarily disable Apcb Recovery, to suppress debug ASSERT.
  !if $(SIMNOW_SUPPORT) == TRUE || $(EMULATION) == TRUE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdPspApcbRecoveryEnable|FALSE
  !endif

  #
  # AGESA NBIO
  #
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgGnbIoapicId|gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdCfgIoApicIdBase

  #
  # AGESA BMC (NBIO)
  #
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdEarlyBmcLinkTraining|TRUE
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdEarlyBmcLinkSocket|0
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdEarlyBmcLinkDie|0
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdEarlyBmcLinkLaneNum|134

  #
  # AGESA USB
  #
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdXhciOcPolarityCfgLow|TRUE
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdXhciUsb31OcPinSelect|0xFFFF1010
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdXhciUsb20OcPinSelect|0xFFFFFFFFFFFF1010

  !if $(USB_SUPPORT)
    ### USB 3.0 controller0
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci0Enable|TRUE
    ### USB 3.0 controller1
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci1Enable|TRUE
    ### USB3.0 controller0 on MCM-1
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci2Enable|FALSE
    ### USB3.0 controller1 on MCM-1
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci3Enable|FALSE

    gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdXhciSsid|0x00000000
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhciECCDedErrRptEn|FALSE
  !else
    ### USB 3.0 controller0
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci0Enable|FALSE
    ### USB 3.0 controller1
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci1Enable|FALSE
    ### USB3.0 controller0 on MCM-1
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci2Enable|FALSE
    ### USB3.0 controller1 on MCM-1
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdXhci3Enable|FALSE
  !endif

!if $(SATA_SUPPORT)
  ### @brief FCH-SATA enables
  ### @details Select whether or not the FCH Sata controller is active.
  ### @li  TRUE  - This option is active.
  ### @li  FALSE - This option is turned off.
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdSataEnable|TRUE
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdSataStaggeredSpinup|TRUE
!else
  ### @brief FCH-SATA enables
  ### @details Select whether or not the FCH Sata controller is active.
  ### @li  TRUE  - This option is active.
  ### @li  FALSE - This option is turned off.
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdSataEnable|FALSE
!endif

  # NVDIMM feature
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdNvdimmEnable|FALSE

  #
  # Firmware Revision
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"AMD"
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision|$(FIRMWARE_REVISION_NUM)
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"$(FIRMWARE_VERSION_STR)"
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareReleaseDateString|L"$(RELEASE_DATE)"

  # MinPlatformPkg, 1's position enables respective ioapic
  gMinPlatformPkgTokenSpaceGuid.PcdPcIoApicEnable|0x0F

[PcdsPatchableInModule]
    gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel

[PcdsDynamicHii.X64.DEFAULT]
  !if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
    gEfiSecurityPkgTokenSpaceGuid.PcdTcgPhysicalPresenceInterfaceVer|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x0|"1.3"|NV,BS
    gEfiSecurityPkgTokenSpaceGuid.PcdTpm2AcpiTableRev|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x8|4|NV,BS
  !endif

[PcdsDynamicDefault]
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdCcxCfgPFEHEnable|TRUE
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdLegacyFree|TRUE
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdCxlProtocolErrorReporting|1
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdCxlComponentErrorReporting|1
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdEgressPoisonSeverityLo|0
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdEgressPoisonSeverityHi|0

  !ifdef $(INTERNAL_IDS)
    gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdAmdCcxSingleBitErrLogging|TRUE
  !endif

  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdAmdS3LibTableSize|0x100000
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdPspAntiRollbackLateSplFuse|TRUE
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdUsbRSOemConfigurationTable|{0x0D,0x10,0xB1,0x00,0x00,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x00,0x00,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x03,0x03,0x03,0x03,0x00,0x01,0x06,0x03,0x01,0x01,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05,0x01,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05,0x07,0x00,0x00,0x07,0x01,0x05,0x00,0x05,0x00,0x05,0x00,0x05}

  # Enable/Disable IOMMU (default TRUE)
  # gEfiAmdAgesaPkgTokenSpaceGuid.PcdCfgIommuSupport|FALSE

  gEfiAmdAgesaPkgTokenSpaceGuid.PcdIvInfoDmaReMap|FALSE
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdCStateIoBaseAddress|0x813

  # AGESA I2C SDA hold delay
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchI2c0SdaHold|0x35
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchI2c1SdaHold|0x35
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchI2c2SdaHold|0x35
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchI2c3SdaHold|0x35
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchI2c4SdaHold|0x35
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchI2c5SdaHold|0x35

  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdResetMode|0x07
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdFchCfgSmiCmdPortAddr|0xB2
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdTelemetry_VddcrSocfull_Scale_Current|0x50
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdTelemetry_VddcrVddfull_Scale_Current|0xFF

  !if $(SIMNOW_SUPPORT) == TRUE
    gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdSmuFeatureControlDefines|0x00030000
  !endif

  !if $(SIMNOW_SUPPORT) == TRUE || $(EMULATION) == TRUE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdMemPostPackageRepair|FALSE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdMemBootTimePostPackageRepair|FALSE
    gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdMemRuntimePostPackageRepair|FALSE
  !endif
  gEfiAmdAgesaModulePkgTokenSpaceGuid.PcdLpcEnable|FALSE

!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInitializationPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{GUID({ 0x286bf25a, 0xc2c3, 0x408c, { 0xb3, 0xb4, 0x25, 0xe6, 0x75, 0x8b, 0x73, 0x17 } })}
  gMinPlatformPkgTokenSpaceGuid.PcdTrustedConsoleInputDevicePath|{DEVICE_PATH("VenHw(D3987D4B-971A-435F-8CAF-4967EB627241)/Uart(115200,8,N,1)/VenMsg(E0C14753-F9BE-11D2-9A0C-0090273FC14D),UsbClass(0xFFFF,0xFFFF,0x03,0x01,0x01)")}
  gMinPlatformPkgTokenSpaceGuid.PcdTrustedConsoleOutputDevicePath|{DEVICE_PATH("VenHw(D3987D4B-971A-435F-8CAF-4967EB627241)/Uart(115200,8,N,1)/VenMsg(E0C14753-F9BE-11D2-9A0C-0090273FC14D),PcieRoot(0xB)/Pci(0x05,0x02)/Pci(0x00,0x00)/Pci(0x00,0x00)/Pci(0x00,0x00)/AcpiAdr(0x80010100)")}
  gEfiSecurityPkgTokenSpaceGuid.PcdActiveTpmInterfaceType|0x00
  gEfiSecurityPkgTokenSpaceGuid.PcdTcg2PhysicalPresenceFlags|0x700E0
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdPspSystemTpmConfig|0x00
!endif

  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseSize|0x10000000
