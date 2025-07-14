## @file
# RISC-V DXE FDF include file common for all platforms.
#
# This file can be included to a platform FDF by using
# "!include Silicon/RISC-V/RiscV.fdf.inc".
#
# Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#

#
# DXE Phase modules
#
INF  MdeModulePkg/Core/Dxe/DxeMain.inf

# RISC-V Core Drivers
INF  UefiCpuPkg/CpuTimerDxeRiscV64/CpuTimerDxeRiscV64.inf
INF  UefiCpuPkg/CpuDxeRiscV64/CpuDxeRiscV64.inf

#ACPI or DT
INF  Platform/RISC-V/PlatformPkg/Drivers/PlatformHasAcpiDtDxe/PlatformHasAcpiDtDxe.inf

INF  EmbeddedPkg/Drivers/FdtClientDxe/FdtClientDxe.inf
INF  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf

INF  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
INF  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
INF  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf

INF  MdeModulePkg/Bus/I2c/I2cDxe/I2cDxe.inf
INF  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
INF  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
INF  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
INF  MdeModulePkg/Universal/Metronome/Metronome.inf
INF  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
INF  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
INF  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
INF  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
INF  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
INF  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf


#
# Environment Variables Protocol
#
INF  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
INF  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf

#
# Multiple Console IO support
#
INF  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
INF  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
INF  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
INF  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
INF  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!endif

#
# Bds support
#
INF  MdeModulePkg/Application/UiApp/UiApp.inf
INF  MdeModulePkg/Logo/LogoDxe.inf
INF  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
INF  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
INF  MdeModulePkg/Universal/DriverHealthManagerDxe/DriverHealthManagerDxe.inf
INF  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
INF  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
INF  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf

#
# FAT filesystem + GPT/MBR partitioning
#
INF  FatPkg/EnhancedFatDxe/Fat.inf
INF  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
INF  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
INF  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

!ifndef $(SOURCE_DEBUG_ENABLE)
INF  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
!endif

INF  ShellPkg/Application/Shell/Shell.inf
