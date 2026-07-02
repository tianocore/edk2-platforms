## @file
# Common DSC content to begin Stage 2.
#
# Copyright (C) 2022 Theo Jehl<BR>
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses.Common]
  PciHostBridgeLib|MdeModulePkg/Library/PciHostBridgeLibNull/PciHostBridgeLibNull.inf

[LibraryClasses.Common.PEIM]
  MpInitLib               | UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.inf

[LibraryClasses.AARCH64.DXE_DRIVER, LibraryClasses.AARCH64.DXE_RUNTIME_DRIVER, LibraryClasses.AARCH64.DXE_SMM_DRIVER, LibraryClasses.AARCH64.UEFI_DRIVER, LibraryClasses.AARCH64.UEFI_APPLICATION, LibraryClasses.AARCH64.SMM_CORE]
  ResetSystemLib          | MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf

[Components.AARCH64]
  MinPlatformPkg/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPostMem.inf
  MinPlatformPkg/PlatformInit/PlatformInitPei/PlatformInitPostMem.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
