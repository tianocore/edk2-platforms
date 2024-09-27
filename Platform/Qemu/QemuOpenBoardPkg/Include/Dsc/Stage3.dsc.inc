## @file
# Common DSC content to begin Stage 3 enabling
#
# @copyright
# Copyright (C) 2022 Theo Jehl
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses.Common]
  PlatformBootManagerLib  | OvmfPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  BootLogoLib             | MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  NvVarsFileLib           | OvmfPkg/Library/NvVarsFileLib/NvVarsFileLib.inf
  QemuFwCfgS3Lib          | OvmfPkg/Library/QemuFwCfgS3Lib/DxeQemuFwCfgS3LibFwCfg.inf
  QemuLoadImageLib        | OvmfPkg/Library/X86QemuLoadImageLib/X86QemuLoadImageLib.inf
  QemuBootOrderLib        | OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf
  PlatformBmPrintScLib    | OvmfPkg/Library/PlatformBmPrintScLib/PlatformBmPrintScLib.inf
  XenPlatformLib          | OvmfPkg/Library/XenPlatformLib/XenPlatformLib.inf
  LoadLinuxLib            | OvmfPkg/Library/LoadLinuxLib/LoadLinuxLib.inf
  SerializeVariablesLib   | OvmfPkg/Library/SerializeVariablesLib/SerializeVariablesLib.inf
  BoardBootManagerLib     | QemuOpenBoardPkg/Library/BoardBootManagerLib/BoardBootManagerLib.inf
  LocalApicLib            | UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  IoLib                   | MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciExpressLib           | MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PcdLib                  | MdePkg/Library/DxePcdLib/DxePcdLib.inf
  PciLib                  | MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  DebugLib                | MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  SerialPortLib           | PcAtChipsetPkg/Library/SerialIoLib/SerialIoLib.inf

[Components.$(DXE_ARCH)]
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib | MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    }
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  UefiCpuPkg/CpuDxe/CpuDxe.inf
  PcAtChipsetPkg/HpetTimerDxe/HpetTimerDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/Console/GraphicsOutputDxe/GraphicsOutputDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  OvmfPkg/QemuVideoDxe/QemuVideoDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Bus/Isa/IsaBusDxe/IsaBusDxe.inf
  MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KeyboardDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  PcAtChipsetPkg/Bus/Pci/IdeControllerDxe/IdeControllerDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  OvmfPkg/QemuRamfbDxe/QemuRamfbDxe.inf

  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellProtocolInteractivityLib|ShellPkg/Library/UefiShellProtocolInteractivityLib/ShellProtocolInteractivityLib.inf
      ShellProtocolsLib|ShellPkg/Library/UefiShellProtocolsLib/ShellProtocolsLib.inf
      ShellCommandLib | ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL | ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL | ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
      HandleParsingLib | ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      PrintLib | MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib | ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask | 0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize | FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize | 8000
  }

  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  OvmfPkg/PlatformDxe/Platform.inf
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf
  MdeModulePkg/Application/UiApp/UiApp.inf
  OvmfPkg/IoMmuDxe/IoMmuDxe.inf
  MdeModulePkg/Universal/Acpi/S3SaveStateDxe/S3SaveStateDxe.inf
  OvmfPkg/SioBusDxe/SioBusDxe.inf
  MdeModulePkg/Bus/Pci/PciSioSerialDxe/PciSioSerialDxe.inf
