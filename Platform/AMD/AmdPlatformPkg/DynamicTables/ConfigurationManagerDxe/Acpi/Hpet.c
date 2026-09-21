/** @file
  This file implements the ACPI table updates for the Configuration Manager DXE driver.

  It includes functions to update the HPET table information in the Platform Repository.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "../ConfigurationManager.h"
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Update HPET table information in Platform Repository.

  @param[in]  PlatformRepo  The pointer to the Platform Repository.

  @retval EFI_SUCCESS  The HPET table information is updated successfully.
  @retval EFI_INVALID_PARAMETER  The input parameter is NULL.
**/
EFI_STATUS
EFIAPI
UpdateHpetTableInfo (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  )
{
  UINT64  HpetCapabilities;
  UINTN   Index;
  UINTN   TableIndex;

  if (PlatformRepo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HpetCapabilities = MmioRead64 (PlatformRepo->HpetInfo.BaseAddressLower32Bit);
  if ((HpetCapabilities == 0) || (HpetCapabilities == MAX_UINT64)) {
    for (Index = 0; Index < PlatformRepo->CurrentAcpiTableCount; Index++) {
      if (PlatformRepo->CmAcpiTableList[Index].TableGeneratorId == CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdHpet)) {
        for (TableIndex = Index; TableIndex < PlatformRepo->CurrentAcpiTableCount - 1; TableIndex++) {
          CopyMem (&PlatformRepo->CmAcpiTableList[TableIndex], &PlatformRepo->CmAcpiTableList[TableIndex + 1], sizeof (CM_STD_OBJ_ACPI_TABLE_INFO));
        }

        ZeroMem (&PlatformRepo->CmAcpiTableList[PlatformRepo->CurrentAcpiTableCount - 1], sizeof (CM_STD_OBJ_ACPI_TABLE_INFO));
        PlatformRepo->CurrentAcpiTableCount--;
        break;
      }
    }

    for (Index = 0; Index < PlatformRepo->CurrentAcpiTableCount; Index++) {
      if (PlatformRepo->CmAcpiTableList[Index].TableGeneratorId == CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtHpet)) {
        for (TableIndex = Index; TableIndex < PlatformRepo->CurrentAcpiTableCount - 1; TableIndex++) {
          CopyMem (&PlatformRepo->CmAcpiTableList[TableIndex], &PlatformRepo->CmAcpiTableList[TableIndex + 1], sizeof (CM_STD_OBJ_ACPI_TABLE_INFO));
        }

        ZeroMem (&PlatformRepo->CmAcpiTableList[PlatformRepo->CurrentAcpiTableCount - 1], sizeof (CM_STD_OBJ_ACPI_TABLE_INFO));
        PlatformRepo->CurrentAcpiTableCount--;
        break;
      }
    }
  }

  /// Set HPET OemTableId from AMD PCD (see PcdAmdAcpiHpetOemTableId in AmdPlatformPkg.dec).
  for (Index = 0; Index < PlatformRepo->CurrentAcpiTableCount; Index++) {
    if (PlatformRepo->CmAcpiTableList[Index].AcpiTableSignature ==
        EFI_ACPI_6_5_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE)
    {
      if (PcdGet64 (PcdAmdAcpiHpetOemTableId) != 0) {
        PlatformRepo->CmAcpiTableList[Index].OemTableId =
          PcdGet64 (PcdAmdAcpiHpetOemTableId);
      } else {
        PlatformRepo->CmAcpiTableList[Index].OemTableId =
          PcdGet64 (PcdAcpiDefaultOemTableId);
      }

      break;
    }
  }

  return EFI_SUCCESS;
}
