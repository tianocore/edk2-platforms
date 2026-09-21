## @file
# AMD Platform Package DSC include file
# Provides [LibraryClasses] and [Components] sections for AmdPlatformPkg.
# Include this file from a platform DSC after base library mappings are defined.
#
# Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

!ifndef SIMNOW_PORT80_DEBUG
  DEFINE SIMNOW_PORT80_DEBUG = FALSE
!endif
!ifndef SIMNOW_SUPPORT
  DEFINE SIMNOW_SUPPORT = FALSE
!endif

[LibraryClasses.Common]
  TimerLib|AmdPlatformPkg/Library/CcxTscTimerLib/BaseTscTimerLib.inf
  AmdPostCodeLib|AmdPlatformPkg/Library/AmdPostCodeLibNull/AmdPostCodeLibNull.inf
  !if $(SIMNOW_PORT80_DEBUG) == TRUE AND $(SIMNOW_SUPPORT) == TRUE
    SerialPortLib|AmdPlatformPkg/Library/EmulatorSerialPort80RedirectLib/EmulatorSerialPort80Redirect.inf
  !endif

[LibraryClasses.Common.PEIM, LibraryClasses.Common.PEI_CORE]
  TimerLib|AmdPlatformPkg/Library/CcxTscTimerLib/PeiTscTimerLib.inf
  AmdPspTcgDonePeiDepexLib|AmdPlatformPkg/Library/AmdPspTcgDonePeiDepexLib/AmdPspTcgDonePeiDepexLib.inf
  AmdCpmGpioInitFinishedDepexLib|AmdPlatformPkg/Library/AmdCpmGpioInitFinishedDepexLib/AmdCpmGpioInitFinishedDepexLib.inf

[LibraryClasses.Common.PEIM]

[LibraryClasses.Common.DXE_CORE, LibraryClasses.Common.DXE_DRIVER, LibraryClasses.Common.DXE_RUNTIME_DRIVER, LibraryClasses.Common.DXE_SMM_DRIVER, LibraryClasses.Common.SMM_CORE, LibraryClasses.Common.UEFI_DRIVER, LibraryClasses.Common.UEFI_APPLICATION]
  TimerLib|AmdPlatformPkg/Library/CcxTscTimerLib/DxeTscTimerLib.inf

[LibraryClasses.Common.DXE_DRIVER]
  RedfishPlatformCredentialLib|AmdPlatformPkg/Library/PlatformRedfishCredentialLib/PlatformRedfishCredentialLib.inf

[LibraryClasses.Common.DXE_SMM_DRIVER]

[LibraryClasses.Common.SMM_CORE]

[Components.IA32]
  AmdPlatformPkg/Universal/Psp/PspPreTcgEventLogPei/PspPreTcgEventLogPei.inf
  AmdPlatformPkg/Universal/RestoreDefaultPei/RestoreDefault.inf
  AmdPlatformPkg/Universal/BoardId/Pei/AmdBoardIdPei.inf
  AmdPlatformPkg/Universal/BoardIdValidatePei/BoardIdValidatePei.inf
  AmdPlatformPkg/Universal/Hob/PlatformHobPei/PlatformHobPei.inf
  AmdPlatformPkg/Universal/SmmRelocatePei/SmmRelocatePei.inf

[Components.X64]
  AmdPlatformPkg/DynamicTables/ConfigurationManagerDxe/ConfigurationManagerDxe.inf
  AmdPlatformPkg/Universal/BoardId/Dxe/AmdBoardIdDxe.inf
  AmdPlatformPkg/Universal/Pci/PciLegacy/PciLegacy.inf
  AmdPlatformPkg/Universal/Pci/PciHotPlug/PciHotPlugInit.inf
  AmdPlatformPkg/Universal/PlatformCipherDxe/PlatformCipherDxe.inf
  AmdPlatformPkg/Universal/Hsti/PlatformHstiDxe/PlatformHstiDxe.inf
  AmdPlatformPkg/Universal/NvStoreRestoreDefaultSmm/NvStoreRestoreDefaultSmm.inf
  AmdPlatformPkg/Universal/SysTopologyReport/Dxe/SysTopologyReportDxe.inf
