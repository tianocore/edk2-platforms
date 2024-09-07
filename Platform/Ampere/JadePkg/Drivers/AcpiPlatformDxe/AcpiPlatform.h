/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_PLATFORM_H_
#define ACPI_PLATFORM_H_

#include <Uefi.h>

#include <AcpiHeader.h>
#include <Guid/EventGroup.h>
#include <Guid/PlatformInfoHob.h>
#include <IndustryStandard/Acpi63.h>
#include <Library/ArmLib/ArmLibPrivate.h>
#include <Library/AcpiLib.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Platform/Ac01.h>
#include <Protocol/AcpiTable.h>

EFI_STATUS
AcpiPatchDsdtTable (
  VOID
  );

EFI_STATUS
AcpiInstallMadtTable (
  VOID
  );

EFI_STATUS
AcpiInstallNfitTable (
  VOID
  );

EFI_STATUS
AcpiPcctInitializeSharedMemory (
  VOID
  );

EFI_STATUS
AcpiInstallPcctTable (
  VOID
  );

EFI_STATUS
AcpiInstallPpttTable (
  VOID
  );

EFI_STATUS
AcpiInstallSlitTable (
  VOID
  );

EFI_STATUS
AcpiInstallSratTable (
  VOID
  );

EFI_STATUS
EFIAPI
AcpiInstallMcfg (
  VOID
  );

EFI_STATUS
EFIAPI
AcpiInstallIort (
  VOID
  );

/**
  Install SPMI (Service Processor Management Interface table) table.

  @retval EFI_SUCCESS   The table was installed successfully.
  @retval Others        Failed to install the table.

**/
EFI_STATUS
AcpiInstallSpmiTable (
  VOID
  );

EFI_STATUS
AcpiInstallBdatTable (
  VOID
  );

#endif /* ACPI_PLATFORM_H_ */
