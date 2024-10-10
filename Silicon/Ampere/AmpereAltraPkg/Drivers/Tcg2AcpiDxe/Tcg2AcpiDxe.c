/** @file
  It updates TPM2 device in ACPI table and publish ACPI TPM2 table.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AcpiHeader.h>
#include <Guid/PlatformInfoHob.h>
#include <Guid/TpmInstance.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Tpm2Acpi.h>
#include <IndustryStandard/TpmPtp.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/Tcg2Protocol.h>

//
// PNP _HID for TPM2 device
//
#define TPM_HID_TAG        "NNNN0000"
#define TPM_HID_PNP_SIZE   8
#define TPM_HID_ACPI_SIZE  9

#define TPM_ACPI_OBJECT_PATH_LENGTH_MAX  256

STATIC PLATFORM_TPM2_CRB_INTERFACE_PARAMETERS  mPlatformTpm2InterfaceParams;
STATIC PLATFORM_TPM2_CONFIG_DATA               mPlatformTpm2ConfigData;

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER                               Header;
  // Flags field is replaced in version 4 and above
  //    BIT0~15:  PlatformClass      This field is only valid for version 4 and above
  //    BIT16~31: Reserved
  UINT32                                                    Flags;
  UINT64                                                    AddressOfControlArea;
  UINT32                                                    StartMethod;
  EFI_TPM2_ACPI_START_METHOD_SPECIFIC_PARAMETERS_ARM_SMC    PlatformSpecificParameters;
  UINT32                                                    Laml;                   // Optional
  UINT64                                                    Lasa;                   // Optional
} TPM2_ACPI_TABLE_ARM_SMC;

#pragma pack()

TPM2_ACPI_TABLE_ARM_SMC  mTpm2AcpiTtable = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE,
    TPM2_ACPI_TABLE_ARM_SMC,
    EFI_TPM2_ACPI_TABLE_REVISION
    ),
  1,                                                                           // BIT0~15:  PlatformClass
                                                                               // BIT16~31: Reserved
  0,                                                                           // Control Area
  EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_SMC, // StartMethod
};

EFI_STATUS
UpdateHID (
  EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  EFI_STATUS  Status;
  UINT8       *DataPtr;
  CHAR8       Hid[TPM_HID_ACPI_SIZE];
  UINT32      ManufacturerID;
  UINT32      FirmwareVersion1;
  UINT32      FirmwareVersion2;
  BOOLEAN     PnpHID;

  PnpHID = TRUE;

  //
  // Initialize HID with Default PNP string
  //
  ZeroMem (Hid, TPM_HID_ACPI_SIZE);

  //
  // Get Manufacturer ID
  //
  Status = Tpm2GetCapabilityManufactureID (&ManufacturerID);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "TPM_PT_MANUFACTURER 0x%08x\n", ManufacturerID));
    //
    // ManufacturerID defined in TCG Vendor ID Registry
    // may tailed with 0x00 or 0x20
    //
    if (((ManufacturerID >> 24) == 0x00) || ((ManufacturerID >> 24) == 0x20)) {
      //
      //  HID containing PNP ID "NNN####"
      //   NNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem (Hid, &ManufacturerID, 3);
    } else {
      //
      //  HID containing ACP ID "NNNN####"
      //   NNNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem (Hid, &ManufacturerID, 4);
      PnpHID = FALSE;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_MANUFACTURER failed %x!\n", Status));
    ASSERT (FALSE);
    return Status;
  }

  Status = Tpm2GetCapabilityFirmwareVersion (&FirmwareVersion1, &FirmwareVersion2);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_1 0x%x\n", FirmwareVersion1));
    DEBUG ((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_2 0x%x\n", FirmwareVersion2));
    //
    //   #### is Firmware Version 1
    //
    if (PnpHID) {
      AsciiSPrint (
        Hid + 3,
        TPM_HID_PNP_SIZE - 3,
        "%02d%02d",
        ((FirmwareVersion1 & 0xFFFF0000) >> 16),
        (FirmwareVersion1 & 0x0000FFFF)
        );
    } else {
      AsciiSPrint (
        Hid + 4,
        TPM_HID_ACPI_SIZE - 4,
        "%02d%02d",
        ((FirmwareVersion1 & 0xFFFF0000) >> 16),
        (FirmwareVersion1 & 0x0000FFFF)
        );
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_FIRMWARE_VERSION_X failed %x!\n", Status));
    ASSERT (FALSE);
    return Status;
  }

  //
  // Patch HID in ASL code before loading the SSDT.
  //
  for (DataPtr  = (UINT8 *)(Table + 1);
       DataPtr <= (UINT8 *)((UINT8 *)Table + Table->Length - TPM_HID_PNP_SIZE);
       DataPtr += 1)
  {
    if (AsciiStrCmp ((CHAR8 *)DataPtr, TPM_HID_TAG) == 0) {
      if (PnpHID) {
        CopyMem (DataPtr, Hid, TPM_HID_PNP_SIZE);
        //
        // if HID is PNP ID, patch the last byte in HID TAG to Noop
        //
        *(DataPtr + TPM_HID_PNP_SIZE) = AML_NOOP_OP;
      } else {
        CopyMem (DataPtr, Hid, TPM_HID_ACPI_SIZE);
      }

      DEBUG ((DEBUG_INFO, "TPM2 ACPI _HID is patched to %a\n", DataPtr));

      return Status;
    }
  }

  DEBUG ((DEBUG_ERROR, "TPM2 ACPI HID TAG for patch not found!\n"));
  return EFI_NOT_FOUND;
}

EFI_STATUS
UpdateAmlObject (
  EFI_ACPI_SDT_PROTOCOL  *AcpiTableSdtProtocol,
  EFI_ACPI_HANDLE        TableHandle,
  VOID                   *AslObjectPath,
  UINT32                 Data
  )
{
  EFI_STATUS          Status;
  EFI_ACPI_HANDLE     ChildHandle;
  EFI_ACPI_DATA_TYPE  DataType;
  CHAR8               *Buffer;
  UINTN               DataSize;

  Status = AcpiTableSdtProtocol->FindPath (
                                   TableHandle,
                                   AslObjectPath,
                                   &ChildHandle
                                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to find AML object path - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = AcpiTableSdtProtocol->GetOption (
                                   ChildHandle,
                                   0,
                                   &DataType,
                                   (VOID *)&Buffer,
                                   &DataSize
                                   );
  if (!EFI_ERROR (Status)) {
    //
    // NameOp buffer layout
    //  Byte[0]: Op Code
    //  Byte[4:1]: Name String
    //  Byte[5]: Type of byte prefix
    //  Byte[6]: Start byte of data
    //
    // Note: This just handles BYTE or DWORD data.
    //
    if (Buffer[0] != AML_NAME_OP) {
      return EFI_NOT_FOUND;
    } else {
      switch (Buffer[5]) {
        case AML_ZERO_OP:
        case AML_ONE_OP:
          Buffer[5] = (UINT8)Data;
          break;

        case AML_BYTE_PREFIX:
          Buffer[6] = (UINT8)Data;
          break;

        case AML_DWORD_PREFIX:
          CopyMem ((VOID *)&Buffer[6], (VOID *)&Data, sizeof (UINT32));
          break;

        default:
          return EFI_UNSUPPORTED;
          break;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateAcpiTpm2Device (
  VOID
  )
{
  CHAR8                   ObjectPath[TPM_ACPI_OBJECT_PATH_LENGTH_MAX];
  EFI_STATUS              Status;
  EFI_ACPI_SDT_PROTOCOL   *AcpiTableSdtProtocol;
  EFI_ACPI_SDT_HEADER     *Table;
  UINTN                   TableIndex;
  EFI_ACPI_TABLE_VERSION  TableVersion;
  UINTN                   TableKey;
  EFI_ACPI_HANDLE         TableHandle;
  BOOLEAN                 Tpm2DeviceStatus;

  //
  // By default, the TPM status is set to TRUE in the ACPI TPM device.
  // The status will be updated if the TPM devcie is not presense.
  //
  Tpm2DeviceStatus = TRUE;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate ACPI table protocol\n"));
    return Status;
  }

  //
  // Search for ACPI Table Signature
  //
  TableIndex = 0;
  while (!EFI_ERROR (Status)) {
    Status = AcpiTableSdtProtocol->GetAcpiTable (
                                     TableIndex,
                                     &Table,
                                     &TableVersion,
                                     &TableKey
                                     );
    if (!EFI_ERROR (Status)) {
      TableIndex++;

      ASSERT (Table != NULL);
      if (Table->Signature == EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
        DEBUG ((DEBUG_INFO, "%a:%d Found DSDT table \n", __func__, __LINE__));
        break;
      }
    }
  }

  Status = AcpiTableSdtProtocol->OpenSdt (TableKey, &TableHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d Failed to open DSDT table.\n", __func__, __LINE__));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Measure to PCR[0] with event EV_POST_CODE ACPI DATA.
  // The measurement has to be done before any update.
  // Otherwise, the PCR record would be different after TPM FW update
  // or the PCD configuration change.
  //
  TpmMeasureAndLogData (
    0,
    EV_POST_CODE,
    EV_POSTCODE_INFO_ACPI_DATA,
    ACPI_DATA_LEN,
    Table,
    Table->Length
    );

  //
  // Update _HID
  //
  Status = UpdateHID ((EFI_ACPI_DESCRIPTION_HEADER *)Table);
  ASSERT_EFI_ERROR (Status);

  //
  // Update Control Area Base and Length
  //
  AsciiSPrint (ObjectPath, sizeof (ObjectPath), "\\_SB.TPM0.CRBB");
  Status = UpdateAmlObject (
             AcpiTableSdtProtocol,
             TableHandle,
             (VOID *)ObjectPath,
             mPlatformTpm2InterfaceParams.AddressOfControlArea
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to update \\_SB.TPM0.CRBB \n", __func__));
    Tpm2DeviceStatus = FALSE;
    ASSERT_EFI_ERROR (Status);
  }

  ZeroMem (ObjectPath, sizeof (ObjectPath));
  AsciiSPrint (ObjectPath, sizeof (ObjectPath), "\\_SB.TPM0.CRBL");
  Status = UpdateAmlObject (
             AcpiTableSdtProtocol,
             TableHandle,
             (VOID *)ObjectPath,
             mPlatformTpm2InterfaceParams.ControlAreaLength
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to update \\_SB.TPM0.CRBL \n", __func__));
    Tpm2DeviceStatus = FALSE;
    ASSERT_EFI_ERROR (Status);
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TPM2 not detected!\n", __func__));
    Tpm2DeviceStatus = FALSE;
  }

  //
  // Update TPM Presense Object "TPMF"
  //
  if (Tpm2DeviceStatus) {
    ZeroMem (ObjectPath, sizeof (ObjectPath));
    AsciiSPrint (ObjectPath, sizeof (ObjectPath), "\\TPMF");
    Status = UpdateAmlObject (AcpiTableSdtProtocol, TableHandle, (VOID *)ObjectPath, 1);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to update \\TPMF \n", __func__));
      ASSERT_EFI_ERROR (Status);
    }
  }

  AcpiTableSdtProtocol->Close (TableHandle);
  AcpiUpdateChecksum ((UINT8 *)Table, Table->Length);

  return EFI_SUCCESS;
}

/**
  Publish TPM2 ACPI table

  @retval   EFI_SUCCESS     The TPM2 ACPI table is published successfully.
  @retval   Others          The TPM2 ACPI table is not published.

**/
EFI_STATUS
PublishTpm2 (
  VOID
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  UINTN                    TableKey;

  //
  // Measure to PCR[0] with event EV_POST_CODE ACPI DATA.
  // The measurement has to be done before any update.
  // Otherwise, the PCR record would be different after event log update
  // or the PCD configuration change.
  //
  TpmMeasureAndLogData (
    0,
    EV_POST_CODE,
    EV_POSTCODE_INFO_ACPI_DATA,
    ACPI_DATA_LEN,
    &mTpm2AcpiTtable,
    mTpm2AcpiTtable.Header.Length
    );

  mTpm2AcpiTtable.Header.Revision = PcdGet8 (PcdTpm2AcpiTableRev);
  DEBUG ((DEBUG_INFO, "Tpm2 ACPI table revision is %d\n", mTpm2AcpiTtable.Header.Revision));

  //
  // PlatformClass is only valid for version 4 and above
  //    BIT0~15:  PlatformClass
  //    BIT16~31: Reserved
  //
  if (mTpm2AcpiTtable.Header.Revision >= EFI_TPM2_ACPI_TABLE_REVISION_4) {
    mTpm2AcpiTtable.Flags = (mTpm2AcpiTtable.Flags & 0xFFFF0000) | PcdGet8 (PcdTpmPlatformClass);
    DEBUG ((DEBUG_INFO, "Tpm2 ACPI table PlatformClass is %d\n", (mTpm2AcpiTtable.Flags & 0x0000FFFF)));
  }

  mTpm2AcpiTtable.Laml = PcdGet32 (PcdTpm2AcpiTableLaml);
  mTpm2AcpiTtable.Lasa = PcdGet64 (PcdTpm2AcpiTableLasa);
  if ((mTpm2AcpiTtable.Header.Revision < EFI_TPM2_ACPI_TABLE_REVISION_4) ||
      (mTpm2AcpiTtable.Laml == 0) || (mTpm2AcpiTtable.Lasa == 0))
  {
    //
    // If version is smaller than 4 or Laml/Lasa is not valid, rollback to original Length.
    //
    mTpm2AcpiTtable.Header.Length = sizeof (EFI_TPM2_ACPI_TABLE);
  }

  mTpm2AcpiTtable.Header.Length = sizeof (TPM2_ACPI_TABLE_ARM_SMC);

  mTpm2AcpiTtable.AddressOfControlArea                     = mPlatformTpm2InterfaceParams.AddressOfControlArea;
  mTpm2AcpiTtable.PlatformSpecificParameters.Interrupt     = mPlatformTpm2InterfaceParams.InterruptMode;
  mTpm2AcpiTtable.PlatformSpecificParameters.Flags         = 0;
  mTpm2AcpiTtable.PlatformSpecificParameters.SmcFunctionId = mPlatformTpm2InterfaceParams.SmcFunctionId;

  //
  // Construct ACPI table
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  ASSERT_EFI_ERROR (Status);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        &mTpm2AcpiTtable,
                        mTpm2AcpiTtable.Header.Length,
                        &TableKey
                        );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
Tcg2AcpiEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  VOID               *GuidHob;
  PLATFORM_INFO_HOB  *PlatformHob;

  if (!CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm20DtpmGuid)) {
    DEBUG ((DEBUG_ERROR, "No TPM2 DTPM instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  GuidHob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (GuidHob == NULL) {
    return EFI_DEVICE_ERROR;
  }

  PlatformHob                  = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (GuidHob);
  mPlatformTpm2InterfaceParams = PlatformHob->Tpm2Info.Tpm2CrbInterfaceParams;
  mPlatformTpm2ConfigData      = PlatformHob->Tpm2Info.Tpm2ConfigData;

  Status = UpdateAcpiTpm2Device ();
  ASSERT_EFI_ERROR (Status);

  //
  // Set TPM2 ACPI table
  //
  Status = PublishTpm2 ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
