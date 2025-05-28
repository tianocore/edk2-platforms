#  @file
#
#  Common flash description header for Xilinx platforms.
#  Contains needed components in DXE Phase.
#
#  Copyright (c) 2025, Linaro Ltd. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

###############################################
##                    DXE                    ##
###############################################

#
# Basic
#
INF MdeModulePkg/Core/Dxe/DxeMain.inf
INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf

#
# FDT support
# Note that you have to add your own devicetree files
#
INF EmbeddedPkg/Drivers/DtPlatformDxe/DtPlatformDxe.inf

#
# SD/MMC support
#
INF MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.inf
INF MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf
INF MdeModulePkg/Bus/Sd/SdDxe/SdDxe.inf

#
# Xilinx common initialization
#
INF Silicon/AMD/Xilinx/CommonPkg/Drivers/XilinxDxe/XilinxDxe.inf

#
# FAT filesystem + GPT/MBR partitioning
#
INF MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
INF MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
INF FatPkg/EnhancedFatDxe/Fat.inf
INF MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

#
# Console Support
#
INF MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
INF MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
INF MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
INF MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

#
# Architectural Protocols
#
INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
INF EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
INF MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
INF MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
INF MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
INF MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
INF ArmPkg/Drivers/ArmGicDxe/ArmGicV2Dxe.inf
INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf
INF MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
INF MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
INF MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
INF MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
INF EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf

###############################################
##                    BDS                    ##
###############################################

INF MdeModulePkg/Universal/BootManagerPolicyDxe/BootManagerPolicyDxe.inf
INF MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
INF MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
INF MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
INF MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
INF MdeModulePkg/Application/UiApp/UiApp.inf
INF ShellPkg/Application/Shell/Shell.inf
