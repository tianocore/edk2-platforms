## @file
# Common DSC content to begin MinPlatformPkg Stage 4.
#
# Board-specific components (ACPI, storage/USB host controllers, SMBIOS
# subclass providers, etc.) are supplied by the including board DSC.
#
# Copyright (c) 2022 Theo Jehl<BR>
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Components.AARCH64]
  #
  # SMBIOS support
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
