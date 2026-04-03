## @file
#  Platform description.
#
# Copyright (c) 2017 - 2020, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##


  #
  # Generic EDKII Driver
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf

  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf

  #
  # Emulated variables for stages 1-4
  #
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
  }

  #
  # Real variables for stages 5+
  #
  !if gMinPlatformPkgTokenSpaceGuid.PcdStandaloneMmEnable == TRUE
    MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteStandaloneMm.inf
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableStandaloneMm.inf {
      <LibraryClasses>
        NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
        NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLibStandaloneMm.inf
    }

  !else
    MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
      <LibraryClasses>
        NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
        NULL|MdeModulePkg/Library/VarCheckHiiLib/VarCheckHiiLib.inf
        NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
    }
  !endif

  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf

  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf

  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf {
    <LibraryClasses>
!if gMinPlatformPkgTokenSpaceGuid.PcdSerialTerminalEnable == TRUE
      NULL|MinPlatformPkg/Library/SerialPortTerminalLib/SerialPortTerminalLib.inf
!endif
  }

!if gMinPlatformPkgTokenSpaceGuid.PcdSerialTerminalEnable == TRUE
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
!endif

  MdeModulePkg/Universal/DriverHealthManagerDxe/DriverHealthManagerDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
!if gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable == TRUE
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
!endif
!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
!endif
  }

  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf

  #UefiCpuPkg/CpuDxe/CpuDxe.inf

  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  PcAtChipsetPkg/HpetTimerDxe/HpetTimerDxe.inf

  #MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf

  #MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  #MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  #MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf

  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf

  #MdeModulePkg/Universal/Console/GraphicsOutputDxe/GraphicsOutputDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf

  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf

  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf

  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf

  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }

  !if gMinPlatformPkgTokenSpaceGuid.PcdStandaloneMmEnable == TRUE
    StandaloneMmPkg/Drivers/StandaloneMmIplPei/StandaloneMmIplPei.inf
    StandaloneMmPkg/Core/StandaloneMmCore.inf {
      <LibraryClasses>
        DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
        HobPrintLib|MdeModulePkg/Library/HobPrintLib/HobPrintLib.inf
    }

    MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterStandaloneMm.inf
    MdeModulePkg/Universal/StatusCodeHandler/Smm/StatusCodeHandlerStandaloneMm.inf
    UefiCpuPkg/CpuIo2Smm/CpuIo2StandaloneMm.inf
    MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceStandaloneMm.inf
  !else
    MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
    MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf

    MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterSmm.inf
    MdeModulePkg/Universal/StatusCodeHandler/Smm/StatusCodeHandlerSmm.inf
    UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
    MdeModulePkg/Universal/LockBox/SmmLockBox/SmmLockBox.inf
    MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceSmm.inf
  !endif

  MdeModulePkg/Universal/SmmCommunicationBufferDxe/SmmCommunicationBufferDxe.inf

  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.inf
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf

!if gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable == TRUE
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable == TRUE
  SecurityPkg/Tcg/MemoryOverwriteControl/TcgMor.inf
  SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
      NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
  }
  !if gMinPlatformPkgTokenSpaceGuid.PcdStandaloneMmEnable == TRUE
    SecurityPkg/Tcg/Tcg2Smm/Tcg2StandaloneMm.inf
  !else
    SecurityPkg/Tcg/Tcg2Smm/Tcg2Smm.inf
  !endif

  SecurityPkg/Tcg/Tcg2Acpi/Tcg2Acpi.inf
  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf
!endif

!if gMinPlatformPkgTokenSpaceGuid.PcdPerformanceEnable == TRUE
  ShellPkg/DynamicCommand/DpDynamicCommand/DpDynamicCommand.inf
!endif
