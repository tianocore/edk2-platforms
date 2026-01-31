## @file
# Common DSC content to begin Stage 2 enabling
#
# @copyright
# Copyright (C) 2022 Theo Jehl
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses.Common]
  ResetSystemLib          | OvmfPkg/Library/ResetSystemLib/BaseResetSystemLib.inf
  PciHostBridgeLib        | OvmfPkg/Library/PciHostBridgeLib/PciHostBridgeLib.inf
  PciHostBridgeUtilityLib | OvmfPkg/Library/PciHostBridgeUtilityLib/PciHostBridgeUtilityLib.inf
  DxeHardwareInfoLib      | OvmfPkg/Library/HardwareInfoLib/DxeHardwareInfoLib.inf

[LibraryClasses.Common.PEI_CORE]
  TimerLib                | OvmfPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf

[LibraryClasses.Common.PEIM]
  MpInitLib               | UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.inf
  TimerLib                | OvmfPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf

[LibraryClasses.Common.DXE_DRIVER, LibraryClasses.Common.DXE_RUNTIME_DRIVER, LibraryClasses.Common.DXE_SMM_DRIVER, LibraryClasses.Common.UEFI_DRIVER, LibraryClasses.Common.UEFI_APPLICATION, LibraryClasses.Common.SMM_CORE]
  PciLib                  | OvmfPkg/Library/DxePciLibI440FxQ35/DxePciLibI440FxQ35.inf

[Components.$(PEI_ARCH)]
  UefiCpuPkg/CpuMpPei/CpuMpPei.inf
  MinPlatformPkg/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPostMem.inf
  MinPlatformPkg/PlatformInit/PlatformInitPei/PlatformInitPostMem.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf

[Components.$(DXE_ARCH)]
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
