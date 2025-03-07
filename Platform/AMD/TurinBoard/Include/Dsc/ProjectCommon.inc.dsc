## @file
#
#  Copyright (C) 2023 -2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses]
  # AML library
  AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf

  # AMD AGESA
  AmdCalloutLib|AgesaModulePkg/Library/AmdCalloutLib/AmdCalloutLib.inf
  AmlGenerationLib|AgesaModulePkg/Library/DxeAmlGenerationLib/AmlGenerationLib.inf
  OemAgesaCcxPlatformLib|AgesaPkg/Addendum/Ccx/OemAgesaCcxPlatformLibNull/OemAgesaCcxPlatformLibNull.inf
  PciHostBridgeLib|AgesaModulePkg/Library/DxeAmdPciHostBridgeLib/PciHostBridgeLib.inf
  PciSegmentInfoLib|AgesaPkg/Addendum/PciSegments/PciExpressPciSegmentInfoLib/PciExpressPciSegmentInfoLib.inf
  ResetSystemLib|AgesaModulePkg/Library/FchBaseResetSystemLib/FchBaseResetSystemLib.inf
  TimerLib|AgesaModulePkg/Library/CcxTscTimerLib/DxeTscTimerLib.inf
  !if $(SIMNOW_SUPPORT) == TRUE
    AmdPostCodeLib|AgesaModulePkg/Library/AmdPostCodeEmuLib2/AmdPostCodeEmuLib.inf
  !endif

  # EDKII Generic
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciSegmentLib|MdePkg/Library/PciSegmentLibSegmentInfo/BasePciSegmentLibSegmentInfo.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf

  # MinPlatformPkg
  AslUpdateLib|MinPlatformPkg/Acpi/Library/DxeAslUpdateLib/DxeAslUpdateLib.inf
  BoardAcpiTableLib|MinPlatformPkg/Acpi/Library/BoardAcpiTableLibNull/BoardAcpiTableLibNull.inf
  PlatformBootManagerLib|MinPlatformPkg/Bds/Library/DxePlatformBootManagerLib/DxePlatformBootManagerLib.inf
  TestPointCheckLib|MinPlatformPkg/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf

  # AMD Platform
  PlatformSecLib|AmdMinBoardPkg/Library/PlatformSecLib/PlatformSecLib.inf
  ReportFvLib|AmdMinBoardPkg/Library/PeiReportFvLib/PeiReportFvLib.inf
  FchEspiCmdLib|AgesaModulePkg/Library/FchEspiCmdLib/FchEspiCmdLib.inf

  # Manageability
  IpmiCommandLib|ManageabilityPkg/Library/IpmiCommandLib/IpmiCommandLib.inf

  # SPCR Device
  SpcrDeviceLib|AmdMinBoardPkg/Library/SpcrDeviceLib/SpcrDeviceLib.inf

  !if $(LOGGING_ENABLE)
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !else
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !endif

  !if $(SOURCE_DEBUG_ENABLE)
    DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
    PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  !else
    PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  !endif

  !if $(SERIAL_PORT) == "NONE"
    SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  !endif

  !if $(SIMNOW_PORT80_DEBUG) == TRUE
    SerialPortLib|AmdPlatformPkg/Library/SimulatorSerialPortLibPort80/SimulatorSerialPortLibPort80.inf
  !endif

  PlatformHookLib|AmdCpmPkg/Library/CommonLib/BasePlatformHookLibAmdFchUart/BasePlatformHookLibAmdFchUart.inf

!if $(REDFISH_ENABLE) == TRUE
  #
  # edk2 Redfish foundation
  #
  !include RedfishPkg/RedfishLibs.dsc.inc
  #
  # edk2 Redfish foundation platform libraries
  #
  RedfishPlatformHostInterfaceLib|RedfishPkg/Library/PlatformHostInterfaceBmcUsbNicLib/PlatformHostInterfaceBmcUsbNicLib.inf
  RedfishPlatformCredentialLib|RedfishPkg/Library/PlatformCredentialLibNull/PlatformCredentialLibNull.inf
  RedfishContentCodingLib|RedfishPkg/Library/RedfishContentCodingLibNull/RedfishContentCodingLibNull.inf
!endif

[LibraryClasses.IA32.SEC]
  # AGESA
  TimerLib|AgesaModulePkg/Library/CcxTscTimerLib/BaseTscTimerLib.inf

  # MinPlatformPkg
  SetCacheMtrrLib|MinPlatformPkg/Library/SetCacheMtrrLib/SetCacheMtrrLibNull.inf

[LibraryClasses.IA32.PEIM, LibraryClasses.IA32.PEI_CORE]
  # AGESA
  TimerLib|AgesaModulePkg/Library/CcxTscTimerLib/PeiTscTimerLib.inf

[LibraryClasses.common.PEIM]

  # MinPlatformPkg
  ReportCpuHobLib|MinPlatformPkg/PlatformInit/Library/ReportCpuHobLib/ReportCpuHobLib.inf
  TestPointLib|MinPlatformPkg/Test/Library/TestPointLib/PeiTestPointLib.inf
  !if $(TARGET) == DEBUG
    TestPointCheckLib|MinPlatformPkg/Test/Library/TestPointCheckLib/PeiTestPointCheckLib.inf
  !endif
  ReportCpuHobLib|MinPlatformPkg/PlatformInit/Library/ReportCpuHobLib/ReportCpuHobLib.inf

  # AMD Platform
  SetCacheMtrrLib|AmdMinBoardPkg/Library/SetCacheMtrrLib/SetCacheMtrrLib.inf

[LibraryClasses.common.SEC, LibraryClasses.common.PEIM, LibraryClasses.common.PEI_CORE]
  PciLib|MdePkg/Library/PeiPciLibPciCfg2/PeiPciLibPciCfg2.inf
  PciSegmentLib|MdePkg/Library/PeiPciSegmentLibPciCfg2/PeiPciSegmentLibPciCfg2.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.DXE_SMM_DRIVER, LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
    TimerLib|AgesaModulePkg/Library/CcxTscTimerLib/DxeTscTimerLib.inf

[LibraryClasses.Common.DXE_DRIVER]
  PlatformSocLib|TurinBoard/Library/DxePlatformSocLib/DxePlatformSocLib.inf
  # MinPlatformPkg
  TestPointLib|MinPlatformPkg/Test/Library/TestPointLib/DxeTestPointLib.inf

  !if gAmdPlatformPkgTokenSpaceGuid.PcdRomArmorEnable == TRUE
    SpiHcPlatformLib|TurinBoard/Library/SpiHcRomArmorPlatformLib/SpiHcPlatformLibDxe.inf
  !else
    SpiHcPlatformLib|AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibDxe.inf
  !endif

  !if $(TARGET) == DEBUG
    TestPointCheckLib|MinPlatformPkg/Test/Library/TestPointCheckLib/DxeTestPointCheckLib.inf
  !endif

  # IPMI Library for invoking IPMI protocol
  IpmiLib|MdeModulePkg/Library/DxeIpmiLibIpmiProtocol/DxeIpmiLibIpmiProtocol.inf

[LibraryClasses.Common.DXE_CORE, LibraryClasses.Common.DXE_DRIVER, LibraryClasses.Common.DXE_SMM_DRIVER]
  # MinPlatformPkg
  BoardBootManagerLib|BoardModulePkg/Library/BoardBootManagerLib/BoardBootManagerLib.inf
  BoardBdsHookLib|AmdMinBoardPkg/Library/BoardBdsHookLib/BoardBdsHookLib.inf

[LibraryClasses.Common.DXE_SMM_DRIVER]
  # EDKII Generic
  !if $(RUNTIME_LOGGING_ENABLE)
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !else
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !endif
  TestPointLib|MinPlatformPkg/Test/Library/TestPointLib/SmmTestPointLib.inf
  !if $(TARGET) == DEBUG
    TestPointCheckLib|MinPlatformPkg/Test/Library/TestPointCheckLib/SmmTestPointCheckLib.inf
  !endif

  # AMD Platform
  AmdPspFlashAccLib|AgesaPkg/Addendum/Psp/AmdPspFlashAccSpiNorLibSmm/AmdPspFlashAccSpiNorLibSmm.inf
  PlatformPspRomArmorWhitelistLib|AgesaPkg/Addendum/Psp/PspRomArmorWhitelistLib/PspRomArmorWhitelistLib.inf

  !if gAmdPlatformPkgTokenSpaceGuid.PcdRomArmorEnable == TRUE
    SpiHcPlatformLib|TurinBoard/Library/SpiHcRomArmorPlatformLib/SpiHcPlatformLibSmm.inf
  !else
    SpiHcPlatformLib|AmdPlatformPkg/Library/SpiHcPlatformLib/SpiHcPlatformLibSmm.inf
  !endif
[LibraryClasses.Common.SMM_CORE]
  # EDKII Generic
  !if $(RUNTIME_LOGGING_ENABLE)
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !else
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !endif

[LibraryClasses.Common.DXE_RUNTIME_DRIVER]
  PciSegmentLib|MdePkg/Library/PciSegmentLibSegmentInfo/DxeRuntimePciSegmentLibSegmentInfo.inf

  !if $(RUNTIME_LOGGING_ENABLE)
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !else
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !endif

[Components.IA32]
  !include MinPlatformPkg/Include/Dsc/CorePeiInclude.dsc

  # AGESA
  AgesaPkg/Addendum/PciSegments/PciExpressPciCfg2/PciExpressPciCfg2.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  }

  # AGESA FCH Platform initialization
  !if $(EMULATION) == FALSE
    TurinBoard/Universal/FchPlatformInitPei/FchPlatformInitPei.inf
  !endif

  # EDKII Generic
  # SEC Core
  UefiCpuPkg/SecCore/SecCore.inf {
    <LibraryClasses>
      SecBoardInitLib|MinPlatformPkg/PlatformInit/Library/SecBoardInitLibNull/SecBoardInitLibNull.inf
  }

  # PEIM
  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf {
    <LibraryClasses>
    !if $(LOGGING_ENABLE)
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    !else
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
    !endif
  }
  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf {
    <LibraryClasses>
      NULL|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
  }

  # MinPlatformPkg
  MinPlatformPkg/PlatformInit/ReportFv/ReportFvPei.inf
  MinPlatformPkg/PlatformInit/PlatformInitPei/PlatformInitPreMem.inf {
    <LibraryClasses>
      BoardInitLib|AmdMinBoardPkg/Library/PeiBoardInitPreMemLib/PeiBoardInitPreMemLib.inf
  }
  MinPlatformPkg/PlatformInit/PlatformInitPei/PlatformInitPostMem.inf {
    <LibraryClasses>
      BoardInitLib|MinPlatformPkg/PlatformInit/Library/BoardInitLibNull/BoardInitLibNull.inf
  }
  !if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
    MinPlatformPkg/Tcg/Tcg2PlatformPei/Tcg2PlatformPei.inf
  !endif

  # AMD Platform
  !if $(PREDEFINED_FABRIC_RESOURCES) == TRUE
    $(PROCESSOR_PATH)/Universal/DfResourcesPei/DfResourcesPei.inf
  !endif

[Components.X64]
  !include MinPlatformPkg/Include/Dsc/CoreDxeInclude.dsc

  # CPM
  AgesaModulePkg/Universal/AmdAutoDynamicCommand/BRH/AmdAutoDynamicCommand.inf {
    <PcdsFixedAtBuild>
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }
  AgesaModulePkg/Universal/AmdAutoDynamicCommand/BRH/AmdAutoToolApp.inf
  AmdCpmPkg/Addendum/Oem/$(PLATFORM_CRB)/Dxe/PspPlatformDriver/PspPlatform.inf

  # MinPlatformPkg
  MinPlatformPkg/PlatformInit/PlatformInitDxe/PlatformInitDxe.inf {
  <LibraryClasses>
    BoardInitLib|AmdMinBoardPkg/Library/DxeBoardInitLib/DxeBoardInitLib.inf
  }
  BoardModulePkg/BoardBdsHookDxe/BoardBdsHookDxe.inf {
    <LibraryClasses> 
      NULL|AmdPlatformPkg/Library/AmdBdsBootConfigLib/AmdBdsBootConfigLib.inf
  }
  MinPlatformPkg/Test/TestPointDumpApp/TestPointDumpApp.inf
  MinPlatformPkg/Test/TestPointStubDxe/TestPointStubDxe.inf

  # EDKII Generic
  UefiCpuPkg/CpuDxe/CpuDxe.inf
  MdeModulePkg/Universal/SectionExtractionDxe/SectionExtractionDxe.inf
  !if $(SOURCE_DEBUG_ENABLE)
    SourceLevelDebugPkg/DebugAgentDxe/DebugAgentDxe.inf {
      <LibraryClasses>
        DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
    }
  !endif
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf

  # USB
  !if $(USB_SUPPORT)
    MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf
  !endif

  # NVME
  !if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE && $(NVME_SUPPORT) == TRUE
    MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
  !endif

  # SATA
  !if $(SATA_SUPPORT)
    MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
    MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
    MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
    MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
    MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  !endif

  # SMM
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <LibraryClasses>
      MmSaveStateLib|UefiCpuPkg/Library/MmSaveStateLib/AmdMmSaveStateLib.inf
      SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/AmdSmmCpuFeaturesLib.inf
      SmmCpuPlatformHookLib|UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
      !if $(LOGGING_ENABLE)
        DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      !else
        DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      !endif
      !if $(SOURCE_DEBUG_ENABLE)
        DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SmmDebugAgentLib.inf
      !endif
    <PcdsPatchableInModule>
      #
      # Disable DEBUG_CACHE because SMI entry/exit may change MTRRs
      #
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x801000C7
  }

  MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf {
    <LibraryClasses>
      # AMD Platform SMM Core hook
      SmmCorePlatformHookLib|AmdPlatformPkg/Library/SmmCorePlatformHookLib/SmmCorePlatformHookLib.inf
      # SMM core hook for SPI host controller
      NULL|AmdPlatformPkg/Library/SmmCoreAmdSpiHcHookLib/SmmCoreAmdSpiHcHookLib.inf
  }

  # File System Modules
  !if gMinPlatformPkgTokenSpaceGuid.PcdPerformanceEnable == TRUE
    MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf
  !endif

  # EFI Shell
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellAcpiViewCommandLib/UefiShellAcpiViewCommandLib.inf
      ## NULL|ShellPkg/Library/UefiShellTftpCommandLib/UefiShellTftpCommandLib.inf
      BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
      FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf

    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|16000
  }

  # Security
  !if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
    MinPlatformPkg/Tcg/Tcg2PlatformDxe/Tcg2PlatformDxe.inf {
      <LibraryClasses>
        TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLib/PeiDxeTpmPlatformHierarchyLib.inf
    }
    UefiCpuPkg/MicrocodeMeasurementDxe/MicrocodeMeasurementDxe.inf
    MdeModulePkg/Universal/SmbiosMeasurementDxe/SmbiosMeasurementDxe.inf
  !endif

  !if $(USE_EMULATED_VARIABLE_STORE) == TRUE
    # these modules are included in MinPlatformPkg in
    # edk2-platforms\Platform\Intel\MinPlatformPkg\Include\Dsc\CoreDxeInclude.dsc
    # removing these modules being loaded by adding depex condition which is
    # always false
    MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf {
      <LibraryClasses>
        NULL|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
    }
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf {
      <LibraryClasses>
        NULL|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
    }
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
      <LibraryClasses>
        NULL|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
    }
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
      <PcdsFixedAtBuild>
        gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
    }
    # emulation bios uses emulated variable store
    # hence turning off the variable protection feature
    AmdCpmPkg/Features/AmdVariableProtection/AmdVariableProtection.inf {
      <LibraryClasses>
        NULL|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
    }
  !else
    !if gMinPlatformPkgTokenSpaceGuid.PcdBootStage >= 4
      MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
    !endif
  !endif

  !if $(SIMNOW_SUPPORT) == FALSE || $(EMULATION) == FALSE
    Drivers/ASpeed/ASpeedGopBinPkg/ASpeedAst2600GopDxe.inf
  !endif

  # ACPI
  !if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE
    MinPlatformPkg/Acpi/AcpiTables/AcpiPlatform.inf
    $(PROCESSOR_PATH)/Universal/BoardAcpiDxe/BoardAcpiDxe.inf
    AmdPlatformPkg/Universal/Acpi/AcpiCommon/AcpiCommon.inf
  !endif

  # SPI
  MdeModulePkg/Bus/Spi/SpiHc/SpiHcDxe.inf
  MdeModulePkg/Bus/Spi/SpiBus/SpiBusDxe.inf
  AmdPlatformPkg/Universal/Spi/BoardSpiConfig/BoardSpiConfigDxe.inf

  !if gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly == FALSE && $(USE_EMULATED_VARIABLE_STORE) == FALSE
    AmdPlatformPkg/Universal/Spi/BoardSpiConfig/BoardSpiConfigSmm.inf
    MdeModulePkg/Bus/Spi/SpiHc/SpiHcSmm.inf
    MdeModulePkg/Bus/Spi/SpiBus/SpiBusSmm.inf
    MdeModulePkg/Bus/Spi/SpiNorFlashJedecSfdp/SpiNorFlashJedecSfdpSmm.inf
    AmdPlatformPkg/Universal/Spi/AmdSpiFvb/AmdSpiFvbSmm.inf
    MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmmDxe.inf
    AmdPlatformPkg/Universal/Spi/EspiNorFlash/EspiNorFlashSmm.inf
  !else
    AmdPlatformPkg/Universal/Spi/AmdSpiFvb/AmdSpiFvbDxe.inf
  !endif

  # HII
  AmdPlatformPkg/Universal/HiiConfigRouting/AmdConfigRouting.inf

  # LOGO
  AmdPlatformPkg/Universal/LogoDxe/LogoDxe.inf

  # PCI HotPlug
  !if gEfiMdeModulePkgTokenSpaceGuid.PcdPciBusHotplugDeviceSupport == TRUE
    AmdMinBoardPkg/PciHotPlug/PciHotPlugInit.inf
    AmdCpmPkg/Addendum/Oem/$(PLATFORM_CRB)/Dxe/ServerHotplugDxe/ServerHotplugDxe.inf
  !endif

  !if gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable == TRUE
    AmdPlatformPkg/Universal/SecureBoot/SecureBootDefaultKeysInit/SecureBootDefaultKeysInit.inf
  !endif

  # Turn off post package repair for emulation
  !if $(EMULATION) == TRUE
    AgesaModulePkg/Mem/AmdMemPprSmmDriver/AmdMemPprSmmDriver.inf {
        <LibraryClasses>
          NULL|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
    }
    EmulationToolsPkg/EmuLinuxLoader/EmuLinuxLoader.inf
  !endif

  !include ManageabilityPkg/Include/Manageability.dsc
  ManageabilityPkg/Universal/IpmiProtocol/Dxe/IpmiProtocolDxe.inf {
    <LibraryClasses>
      ManageabilityTransportLib|ManageabilityPkg/Library/ManageabilityTransportKcsLib/Dxe/DxeManageabilityTransportKcs.inf
  }

  #
  # edk2 Redfish foundation
  #
!if $(REDFISH_ENABLE) == TRUE
  !include RedfishPkg/RedfishComponents.dsc.inc
!endif

  #
  # USB Network (Communication Device Class) drivers
  #
!if $(USB_NETWORK_SUPPORT) == TRUE
  MdeModulePkg/Bus/Usb/UsbNetwork/NetworkCommon/NetworkCommon.inf
  MdeModulePkg/Bus/Usb/UsbNetwork/UsbCdcEcm/UsbCdcEcm.inf
!endif

!if $(SIMNOW_SUPPORT) == FALSE && $(EMULATION) == FALSE
  AmdCpmPkg/Addendum/Oem/OobPprDxe/OobPprDxe.inf
!endif

  #
  ##For AMD PRM Feature Support##
  #

  #
  # PRM Libraries
  #
  PrmPkg/Library/DxePrmContextBufferLib/DxePrmContextBufferLib.inf

  #
  # PRM Module Discovery Library
  #
  PrmPkg/Library/DxePrmModuleDiscoveryLib/DxePrmModuleDiscoveryLib.inf

  #
  # PRM PE/COFF Library
  #
  PrmPkg/Library/DxePrmPeCoffLib/DxePrmPeCoffLib.inf

  #
  # PRM Module Loader Driver
  #
  PrmPkg/PrmLoaderDxe/PrmLoaderDxe.inf

  #
  # AMD DICE Protection Environment driver
  #
  AgesaPkg/Addendum/Psp/AmdPspDpeDxe/AmdPspDpeDxe.inf

  # Adds secure boot dependency to AmdVariableProtection feature
  !if gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable == FALSE
    AmdCpmPkg/Features/AmdVariableProtection/AmdVariableProtection.inf {
      <LibraryClasses>
        NULL|AmdPlatformPkg/Library/BaseAlwaysFalseDepexLib/BaseAlwaysFalseDepexLib.inf
    }
  !endif

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  #
  # PRM Package
  #
  PrmContextBufferLib|PrmPkg/Library/DxePrmContextBufferLib/DxePrmContextBufferLib.inf
  PrmModuleDiscoveryLib|PrmPkg/Library/DxePrmModuleDiscoveryLib/DxePrmModuleDiscoveryLib.inf
  PrmPeCoffLib|PrmPkg/Library/DxePrmPeCoffLib/DxePrmPeCoffLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER,LibraryClasses.common.SMM_CORE]
  PciExpressLib|MdePkg/Library/SmmPciExpressLib/SmmPciExpressLib.inf

[BuildOptions]
  GCC:*_*_*_CC_FLAGS     = -D DISABLE_NEW_DEPRECATED_INTERFACES
  INTEL:*_*_*_CC_FLAGS   = /D DISABLE_NEW_DEPRECATED_INTERFACES
  MSFT:*_*_*_CC_FLAGS    = /D DISABLE_NEW_DEPRECATED_INTERFACES

  GCC:*_*_*_CC_FLAGS     = -D USE_EDKII_HEADER_FILE

  # Turn off DEBUG messages for Release Builds
  GCC:RELEASE_*_*_CC_FLAGS     = -D MDEPKG_NDEBUG
  INTEL:RELEASE_*_*_CC_FLAGS   = /D MDEPKG_NDEBUG
  MSFT:RELEASE_*_*_CC_FLAGS    = /D MDEPKG_NDEBUG

  !ifdef $(INTERNAL_IDS)
    GCC:*_*_*_CC_FLAGS     = -DINTERNAL_IDS
    INTEL:*_*_*_CC_FLAGS   = /D INTERNAL_IDS
    MSFT:*_*_*_CC_FLAGS    = /D INTERNAL_IDS
    MSFT:*_*_*_VFRPP_FLAGS = /D INTERNAL_IDS
    MSFT:*_*_*_ASLCC_FLAGS = /D INTERNAL_IDS
    MSFT:*_*_*_ASLPP_FLAGS = /D INTERNAL_IDS
    MSFT:*_*_*_PP_FLAGS    = /D INTERNAL_IDS
    MSFT:*_*_*_APP_FLAGS   = /D INTERNAL_IDS
  !endif

  !if $(EMULATION) == TRUE
    GCC:*_*_*_CC_FLAGS     = -D IDSOPT_PRESILICON_ENABLED=1
    INTEL:*_*_*_CC_FLAGS   = /D IDSOPT_PRESILICON_ENABLED=1
    MSFT:*_*_*_CC_FLAGS    = /D IDSOPT_PRESILICON_ENABLED=1
  !endif

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER, BuildOptions.common.EDKII.DXE_SMM_DRIVER, BuildOptions.common.EDKII.SMM_CORE]
  #Force modules to 4K alignment
  MSFT:*_*_*_DLINK_FLAGS = /ALIGN:4096
  GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000

[BuildOptions.common.EDKII.DXE_DRIVER, BuildOptions.common.EDKII.DXE_CORE, BuildOptions.common.EDKII.UEFI_DRIVER]
  #Force modules to 4K alignment
  MSFT:*_*_*_DLINK_FLAGS = /ALIGN:4096
  GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000

