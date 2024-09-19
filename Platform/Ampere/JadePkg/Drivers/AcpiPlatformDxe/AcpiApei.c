/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/FlashLib.h>
#include <Library/NVParamLib.h>
#include <NVParamDef.h>

#include "AcpiApei.h"

UINT8  AMPERE_GUID[16]                        = { 0x8d, 0x89, 0xed, 0xe8, 0x16, 0xdf, 0xcc, 0x43, 0x8e, 0xcc, 0x54, 0xf0, 0x60, 0xef, 0x15, 0x7f };
CHAR8  DEFAULT_BERT_REBOOT_MSG[BERT_MSG_SIZE] = "Unknown reboot reason";

BOOLEAN
IsBertStatusValid (
  BERT_ERROR_STATUS  *BertStatus
  )
{
  if (CompareMem (BertStatus->Guid, AMPERE_GUID, sizeof (AMPERE_GUID)) == 0) {
    if (BertStatus->BertRev == CURRENT_BERT_VERSION) {
      return TRUE;
    }
  }

  return FALSE;
}

/*
  Create a default BertErrorStatus In SPINOR
 */
VOID
CreateDefaultBertStatus (
  VOID
  )
{
  UINTN              Length     = sizeof (BERT_ERROR_STATUS);
  BERT_ERROR_STATUS  BertStatus = { 0 };

  CopyMem (BertStatus.Guid, AMPERE_GUID, sizeof (AMPERE_GUID));
  BertStatus.BertRev     = CURRENT_BERT_VERSION;
  BertStatus.DefaultBert = 1;
  BertStatus.Overflow    = 0;
  BertStatus.PendingUefi = 0;
  BertStatus.PendingBmc  = 0;

  FlashEraseCommand (BERT_SPI_ADDRESS_STATUS, Length);
  FlashWriteCommand (
    BERT_SPI_ADDRESS_STATUS,
    &BertStatus,
    Length
    );
}

/*
  Update BertErrorStatus In SPINOR
 */
VOID
UpdateBertStatus (
  BERT_ERROR_STATUS  *BertStatus
  )
{
  UINTN  Length = sizeof (BERT_ERROR_STATUS);

  BertStatus->DefaultBert = 1;
  BertStatus->PendingUefi = 0;
  if (BertStatus->PendingBmc == 0) {
    BertStatus->Overflow = 0;
  }

  FlashEraseCommand (BERT_SPI_ADDRESS_STATUS, Length);
  FlashWriteCommand (
    BERT_SPI_ADDRESS_STATUS,
    BertStatus,
    Length
    );
}

STATIC VOID
AcpiApeiUninstallTable (
  UINT32  Signature
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  EFI_ACPI_SDT_PROTOCOL    *AcpiTableSdtProtocol;
  EFI_ACPI_SDT_HEADER      *Table;
  UINTN                    TableKey;
  UINTN                    TableIndex;

  /*
   * Get access to ACPI tables
   */
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: Unable to locate ACPI table protocol\n", __func__, __LINE__));
    return;
  }

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiTableSdtProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: Unable to locate ACPI table support protocol\n", __func__, __LINE__));
    return;
  }

  /*
   * Search for ACPI Table Signature
   */
  TableIndex = 0;
  Status     = AcpiLocateTableBySignature (
                 AcpiTableSdtProtocol,
                 Signature,
                 &TableIndex,
                 (EFI_ACPI_DESCRIPTION_HEADER **)&Table,
                 &TableKey
                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d Unable to get ACPI table\n", __func__, __LINE__));
    return;
  }

  /*
   * Uninstall ACPI Table
   */
  Status = AcpiTableProtocol->UninstallAcpiTable (AcpiTableProtocol, TableKey);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: Unable to uninstall table\n", __func__, __LINE__));
  }
}

VOID
AdjustBERTRegionLen (
  UINT32  Len
  )
{
  EFI_STATUS                                   Status;
  EFI_ACPI_SDT_PROTOCOL                        *AcpiTableSdtProtocol;
  UINTN                                        TableKey;
  UINTN                                        TableIndex;
  EFI_ACPI_6_3_BOOT_ERROR_RECORD_TABLE_HEADER  *Table;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "APEI: Unable to locate ACPI table support protocol\n"));
    return;
  }

  /*
   * Search for ACPI Table Signature
   */
  TableIndex = 0;
  Status     = AcpiLocateTableBySignature (
                 AcpiTableSdtProtocol,
                 EFI_ACPI_6_3_BOOT_ERROR_RECORD_TABLE_SIGNATURE,
                 &TableIndex,
                 (EFI_ACPI_DESCRIPTION_HEADER **)&Table,
                 &TableKey
                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d Unable to get ACPI BERT table\n", __func__, __LINE__));
    return;
  }

  /*
   * Adjust Boot Error Region Length
   */
  Table->BootErrorRegionLength = Len;

  AcpiUpdateChecksum ((UINT8 *)Table, Table->Header.Length);
}

/*
 * Retrieve Bert data from SPI NOR
 */
VOID
EFIAPI
PullBertSpinorData (
  UINT8   *BertData,
  UINT64  BertSpiAddress,
  UINTN   Length
  )
{
  FlashReadCommand (
    BertSpiAddress,
    BertData,
    Length
    );
}

/*
 * wrap raw bert error data
 *
 * @param  IN  BertErrorData     Bert Error record to be wrapped
 * @param  OUT WrappedError      Generic error data for OS to consume.
 */
VOID
WrapBertErrorData (
  APEI_CRASH_DUMP_BERT_ERROR  *WrappedError
  )
{
  UINT32  CrashSize;
  UINT8   CrashType;
  UINT8   CrashSubType;

  CrashSize = PLAT_CRASH_ITERATOR_SIZE *
              GetNumberOfSupportedSockets () *
              GetMaximumNumberOfCores ();
  CrashSize += 2 * (SMPRO_CRASH_SIZE + PMPRO_CRASH_SIZE + RASIP_CRASH_SIZE);
  CrashSize += sizeof (WrappedError->Bed.Vendor) + sizeof (WrappedError->Bed.BertRev);

  CrashType    = WrappedError->Bed.Vendor.Type & RAS_TYPE_ERROR_MASK;
  CrashSubType = WrappedError->Bed.Vendor.SubType;

  WrappedError->Ges.BlockStatus.ErrorDataEntryCount     = 1;
  WrappedError->Ges.BlockStatus.UncorrectableErrorValid = 1;
  WrappedError->Ged.ErrorSeverity                       = BERT_DEFAULT_ERROR_SEVERITY;
  WrappedError->Ged.Revision                            = GENERIC_ERROR_DATA_REVISION;

  if (  ((CrashType == RAS_TYPE_BERT) && ((CrashSubType == 0) || (CrashSubType == BERT_UEFI_FAILURE)))
     || (CrashType == RAS_TYPE_2P))
  {
    WrappedError->Ged.ErrorDataLength = sizeof (WrappedError->Bed.Vendor) +
                                        sizeof (WrappedError->Bed.BertRev);
    WrappedError->Ges.DataLength = sizeof (WrappedError->Bed.Vendor) +
                                   sizeof (WrappedError->Bed.BertRev) +
                                   sizeof (WrappedError->Ged);
    AdjustBERTRegionLen (
      sizeof (WrappedError->Bed.Vendor) +
      sizeof (WrappedError->Bed.BertRev) +
      sizeof (WrappedError->Ged) +
      sizeof (WrappedError->Ges)
      );
  } else {
    WrappedError->Ged.ErrorDataLength = CrashSize;
    WrappedError->Ges.DataLength      = CrashSize + sizeof (WrappedError->Ged);
    AdjustBERTRegionLen (
      CrashSize +
      sizeof (WrappedError->Ged) +
      sizeof (WrappedError->Ges)
      );
  }

  CopyMem (
    WrappedError->Ged.SectionType,
    AMPERE_GUID,
    sizeof (AMPERE_GUID)
    );
}

/*
 * create default bert error
 * Msg: Unknown reboot reason
 */
VOID
CreateDefaultBertData (
  APEI_CRASH_DUMP_DATA  *Data
  )
{
  Data->Vendor.Type = RAS_TYPE_BERT_PAYLOAD3;
  Data->BertRev     = CURRENT_BERT_VERSION;
  AsciiStrCpyS (
    Data->Vendor.Msg,
    BERT_MSG_SIZE,
    DEFAULT_BERT_REBOOT_MSG
    );
}

/*
 * Checks Status of NV_SI_RAS_BERT_ENABLED
 * Returns TRUE if enabled and FALSE if disabled
 */
BOOLEAN
IsBertEnabled (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  Status = NVParamGet (
             NV_SI_RAS_BERT_ENABLED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    // BERT is enabled by default
    return TRUE;
  }

  return (Value != 0) ? TRUE : FALSE;
}

/*
 * Write bert table to DDR
 */
VOID
WriteDDRBertTable (
  APEI_CRASH_DUMP_BERT_ERROR  *Data
  )
{
  VOID  *Blk = (VOID *)BERT_DDR_OFFSET;

  CopyMem (Blk, Data, BERT_DDR_LENGTH);
}

/*
 * Update Bert Table
 */
EFI_STATUS
AcpiPopulateBert (
  VOID
  )
{
  APEI_CRASH_DUMP_BERT_ERROR  *DDRError;
  BERT_ERROR_STATUS           BertStatus;

  DDRError = (APEI_CRASH_DUMP_BERT_ERROR *)AllocateZeroPool (BERT_DDR_LENGTH);

  if (DDRError == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (IsBertEnabled ()) {
    PullBertSpinorData ((UINT8 *)&BertStatus, BERT_SPI_ADDRESS_STATUS, sizeof (BERT_ERROR_STATUS));
    if (IsBertStatusValid (&BertStatus)) {
      if (BertStatus.PendingUefi == 1) {
        PullBertSpinorData ((UINT8 *)&(DDRError->Bed), BERT_FLASH_OFFSET, sizeof (APEI_CRASH_DUMP_DATA));
        if (DDRError->Bed.BertRev == BertStatus.BertRev) {
          WrapBertErrorData (DDRError);
          WriteDDRBertTable (DDRError);
        } else {
          // If we are here something is out of sync. Reinit BERT section
          FlashEraseCommand (BERT_FLASH_OFFSET, 0x1000);
          CreateDefaultBertStatus ();
          goto AcpiPopulateBertEnd;
        }
      } else if (BertStatus.DefaultBert == 1) {
        CreateDefaultBertData (&(DDRError->Bed));
        WrapBertErrorData (DDRError);
        WriteDDRBertTable (DDRError);
      }

      UpdateBertStatus (&BertStatus);
    } else {
      FlashEraseCommand (BERT_FLASH_OFFSET, 0x1000);
      CreateDefaultBertStatus ();
    }
  }

AcpiPopulateBertEnd:
  FreePool (DDRError);
  return EFI_SUCCESS;
}

/*
 * Checks Status of NV_SI_RAS_SDEI_ENABLED
 * Returns TRUE if enabled and FALSE if disabled or error occurred
 */
BOOLEAN
IsSdeiEnabled (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  Status = NVParamGet (
             NV_SI_RAS_SDEI_ENABLED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    // SDEI is disabled by default
    return FALSE;
  }

  return (Value != 0) ? TRUE : FALSE;
}

STATIC
VOID
AcpiApeiHestUpdateTable1P (
  VOID
  )
{
  EFI_STATUS                                       Status;
  EFI_ACPI_SDT_PROTOCOL                            *AcpiTableSdtProtocol;
  EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *HestTablePointer;
  UINTN                                            TableKey;
  UINTN                                            TableIndex;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiTableSdtProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "APEI: Unable to locate ACPI table support protocol\n"));
    return;
  }

  /*
   * Search for ACPI Table Signature
   */
  TableIndex = 0;
  Status     = AcpiLocateTableBySignature (
                 AcpiTableSdtProtocol,
                 EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE,
                 &TableIndex,
                 (EFI_ACPI_DESCRIPTION_HEADER **)&HestTablePointer,
                 &TableKey
                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d Unable to get ACPI HEST table\n", __func__, __LINE__));
    return;
  }

  HestTablePointer->ErrorSourceCount -= HEST_NUM_ENTRIES_PER_SOC;
  HestTablePointer->Header.Length    -=
    (HEST_NUM_ENTRIES_PER_SOC *
     sizeof (EFI_ACPI_6_3_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE));

  AcpiUpdateChecksum ((UINT8 *)HestTablePointer, HestTablePointer->Header.Length);
}

/*
 * Update APEI
 *
 */
EFI_STATUS
EFIAPI
AcpiApeiUpdate (
  VOID
  )
{
  EFI_STATUS                 Status;
  ACPI_CONFIG_VARSTORE_DATA  AcpiConfigData;
  UINTN                      BufferSize;

  BufferSize = sizeof (ACPI_CONFIG_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      L"AcpiConfigNVData",
                      &gAcpiConfigFormSetGuid,
                      NULL,
                      &BufferSize,
                      &AcpiConfigData
                      );
  if (!EFI_ERROR (Status) && (AcpiConfigData.EnableApeiSupport == 0)) {
    AcpiApeiUninstallTable (EFI_ACPI_6_3_BOOT_ERROR_RECORD_TABLE_SIGNATURE);
    AcpiApeiUninstallTable (EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE);
    AcpiApeiUninstallTable (EFI_ACPI_6_3_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_SIGNATURE);
    AcpiApeiUninstallTable (EFI_ACPI_6_3_ERROR_INJECTION_TABLE_SIGNATURE);
  } else {
    if (!IsSlaveSocketActive ()) {
      AcpiApeiHestUpdateTable1P ();
    }
  }

  if (!IsSdeiEnabled ()) {
    AcpiApeiUninstallTable (EFI_ACPI_6_3_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_SIGNATURE);
  }

  return EFI_SUCCESS;
}
