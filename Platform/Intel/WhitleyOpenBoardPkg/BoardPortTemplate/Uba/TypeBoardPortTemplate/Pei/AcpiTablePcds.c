/** @file
  ACPI table pcds update.

  @copyright
  Copyright 2015 - 2021 Intel Corporation. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PeiBoardInit.h"
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Guid/PlatformInfo.h>
#include <UncoreCommonIncludes.h>
#include <Cpu/CpuIds.h>

EFI_STATUS
TypeBoardPortTemplatePlatformUpdateAcpiTablePcds (
  VOID
  )
{
  CHAR8     AcpiName10nm[]    = "EPRP10NM";     // USED for identify ACPI table for 10nm in systmeboard dxe driver
  CHAR8     OemTableIdXhci[]  = "xh_nccrb";

  UINTN     Size;
  EFI_STATUS Status;

  EFI_HOB_GUID_TYPE                     *GuidHob;

  DEBUG ((DEBUG_INFO, "Uba Callback: PlatformUpdateAcpiTablePcds entered\n"));

  GuidHob = GetFirstGuidHob (&gEfiPlatformInfoGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  //#
  //#ACPI items
  //#
  Size = AsciiStrSize (AcpiName10nm);
  Status = PcdSetPtrS (PcdOemSkuAcpiName , &Size, AcpiName10nm);
  DEBUG ((DEBUG_INFO, "%a TypeBoardPortTemplate ICX\n", __func__));
  ASSERT_EFI_ERROR (Status);

  Size = AsciiStrSize (OemTableIdXhci);
  Status = PcdSetPtrS (PcdOemTableIdXhci , &Size, OemTableIdXhci);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
