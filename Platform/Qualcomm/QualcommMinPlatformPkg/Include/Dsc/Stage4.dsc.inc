## @file
# Common DSC content to begin MinPlatformPkg Stage 4.
#
# Copyright (C) 2022 Theo Jehl<BR>
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


[LibraryClasses]

[LibraryClasses.Common.DXE_SMM_DRIVER]
  LockBoxLib              | MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf

[Components.AARCH64]
  MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf

  #
  # SMBIOS Support
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
