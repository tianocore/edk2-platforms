#
#  Copyright (c) 2025, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

# Per-platform additional content of the DXE phase firmware volume

  # ACPI support
  INF RuleOverride=ACPITABLE Platform/ARM/SgiPkg/AcpiTables/RdV3Cfg1AcpiTables.inf
