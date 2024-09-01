/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/FlashLib.h>
#include <Library/NVParamLib.h>
#include <NVParamDef.h>

#include "AcpiApei.h"

UINT8 AMPERE_GUID[16] = {0x8d, 0x89, 0xed, 0xe8, 0x16, 0xdf, 0xcc, 0x43, 0x8e, 0xcc, 0x54, 0xf0, 0x60, 0xef, 0x15, 0x7f};
CHAR8 DEFAULT_BERT_REBOOT_MSG[BERT_MSG_SIZE] = "Unknown reboot reason";

STATIC VOID
AcpiApeiUninstallTable (
  UINT32 Signature
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_TABLE_PROTOCOL *AcpiTableProtocol;
  EFI_ACPI_SDT_PROTOCOL   *AcpiTableSdtProtocol;
  EFI_ACPI_SDT_HEADER     *Table;
  UINTN                   TableKey;
  UINTN                   TableIndex;

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
  Status = AcpiLocateTableBySignature (
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
  UINT32 Len
  )
{
  EFI_STATUS                                  Status;
  EFI_ACPI_SDT_PROTOCOL                       *AcpiTableSdtProtocol;
  UINTN                                       TableKey;
  UINTN                                       TableIndex;
  EFI_ACPI_6_3_BOOT_ERROR_RECORD_TABLE_HEADER *Table;

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
  Status = AcpiLocateTableBySignature (
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
PullBertSpinorData (
  APEI_CRASH_DUMP_DATA *BertErrorData
  )
{
  UINTN Length;

  Length = sizeof (*BertErrorData);

  FlashReadCommand (
    BERT_FLASH_OFFSET,
    BertErrorData,
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
  APEI_CRASH_DUMP_BERT_ERROR *WrappedError
  )
{
  UINT32 CrashSize;
  UINT8  CrashType;
  UINT8  CrashSubType;

  CrashSize = PLAT_CRASH_ITERATOR_SIZE *
              GetNumberOfSupportedSockets () *
              GetMaximumNumberOfCores ();
  CrashSize += 2 * (SMPRO_CRASH_SIZE + PMPRO_CRASH_SIZE + RASIP_CRASH_SIZE);
  CrashSize += sizeof (WrappedError->Bed.Vendor) + sizeof (WrappedError->Bed.BertRev);

  CrashType = WrappedError->Bed.Vendor.Type & RAS_TYPE_ERROR_MASK;
  CrashSubType = WrappedError->Bed.Vendor.SubType;

  WrappedError->Ges.BlockStatus.ErrorDataEntryCount = 1;
  WrappedError->Ges.BlockStatus.UncorrectableErrorValid = 1;
  WrappedError->Ged.ErrorSeverity = BERT_DEFAULT_ERROR_SEVERITY;
  WrappedError->Ged.Revision = GENERIC_ERROR_DATA_REVISION;

  if ((CrashType == RAS_TYPE_BERT && (CrashSubType == 0 || CrashSubType == BERT_UEFI_FAILURE))
    || (CrashType == RAS_TYPE_2P)) {
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
    WrappedError->Ges.DataLength = CrashSize + sizeof (WrappedError->Ged);
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
  APEI_BERT_ERROR_DATA *Data
  )
{
  Data->Type = RAS_TYPE_BERT_PAYLOAD3;
  AsciiStrCpyS (
    Data->Msg,
    BERT_MSG_SIZE,
    DEFAULT_BERT_REBOOT_MSG
    );
}

/*
 * Ensures BertErrorData In SPINOR matches
 * the record produced by CreateDefaultBertData.
 * @param  Bed    Crash dump Data
 */
VOID
WriteSpinorDefaultBertTable (
  APEI_CRASH_DUMP_DATA *Bed
  )
{
  UINT8                BertRev;
  UINTN                Length;
  UINT64               Offset;
  UINT32               MsgDiff;
  APEI_BERT_ERROR_DATA DefaultData = {0};

  CreateDefaultBertData (&DefaultData);
  if ((Bed->Vendor.Type != DefaultData.Type)) {
    Offset = BERT_FLASH_OFFSET +
             OFFSET_OF (APEI_CRASH_DUMP_DATA, Vendor) +
             OFFSET_OF (APEI_BERT_ERROR_DATA, Type);
    Length = sizeof (DefaultData.Type);
    FlashEraseCommand (Offset, Length);
    FlashWriteCommand (Offset, &(DefaultData.Type), Length);
  }

  if ((Bed->Vendor.SubType != DefaultData.SubType)) {
    Offset = BERT_FLASH_OFFSET +
             OFFSET_OF (APEI_CRASH_DUMP_DATA, Vendor) +
             OFFSET_OF (APEI_BERT_ERROR_DATA, SubType);
    Length = sizeof (DefaultData.SubType);
    FlashEraseCommand (Offset, Length);
    FlashWriteCommand (Offset, &(DefaultData.SubType), Length);
  }

  if ((Bed->Vendor.Instance != DefaultData.Instance)) {
    Offset = BERT_FLASH_OFFSET +
             OFFSET_OF (APEI_CRASH_DUMP_DATA, Vendor) +
             OFFSET_OF (APEI_BERT_ERROR_DATA, Instance);
    Length = sizeof (DefaultData.Instance);
    FlashEraseCommand (Offset, Length);
    FlashWriteCommand (Offset, &(DefaultData.Instance), Length);
  }

  MsgDiff = AsciiStrnCmp (Bed->Vendor.Msg, DefaultData.Msg, BERT_MSG_SIZE);
  if (MsgDiff != 0) {
    Offset = BERT_FLASH_OFFSET +
             OFFSET_OF (APEI_CRASH_DUMP_DATA, Vendor) +
             OFFSET_OF (APEI_BERT_ERROR_DATA, Msg);
    Length = sizeof (DefaultData.Msg);
    FlashEraseCommand (Offset, Length);
    FlashWriteCommand (Offset, &(DefaultData.Msg), Length);
  }

  if (Bed->BertRev != CURRENT_BERT_VERSION) {
    Offset = BERT_FLASH_OFFSET + OFFSET_OF (APEI_CRASH_DUMP_DATA, BertRev);
    Length = sizeof (Bed->BertRev);
    BertRev = CURRENT_BERT_VERSION;
    FlashEraseCommand (Offset, Length);
    FlashWriteCommand (Offset, &BertRev, Length);
  }

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
  EFI_STATUS Status;
  UINT32     Value;

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
  APEI_CRASH_DUMP_BERT_ERROR *Data
  )
{
  VOID *Blk = (VOID *)BERT_DDR_OFFSET;

  /*
   * writing sizeof data to ddr produces alignment error
   * this is a temporary workaround
   */
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
  APEI_CRASH_DUMP_BERT_ERROR *DDRError;

  DDRError =
    (APEI_CRASH_DUMP_BERT_ERROR *)
    AllocateZeroPool (sizeof (APEI_CRASH_DUMP_BERT_ERROR));

  if (DDRError == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (IsBertEnabled ()) {
    PullBertSpinorData (&(DDRError->Bed));
    if ((DDRError->Bed.BertRev == CURRENT_BERT_VERSION)) {
      WrapBertErrorData (DDRError);
      WriteDDRBertTable (DDRError);
    }
    WriteSpinorDefaultBertTable (&(DDRError->Bed));
  }

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
  EFI_STATUS Status;
  UINT32     Value;

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
  EFI_STATUS                                      Status;
  EFI_ACPI_SDT_PROTOCOL                           *AcpiTableSdtProtocol;
  EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_HEADER *HestTablePointer;
  UINTN                                           TableKey;
  UINTN                                           TableIndex;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiTableSdtProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "APEI: Unable to locate ACPI table support protocol\n"));
    return;
  }

  /*
   * Search for ACPI Table Signature
   */
  TableIndex = 0;
  Status = AcpiLocateTableBySignature (
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
  HestTablePointer->Header.Length -=
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
  EFI_STATUS                Status;
  ACPI_CONFIG_VARSTORE_DATA AcpiConfigData;
  UINTN                     BufferSize;

  BufferSize = sizeof (ACPI_CONFIG_VARSTORE_DATA);
  Status = gRT->GetVariable (
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
