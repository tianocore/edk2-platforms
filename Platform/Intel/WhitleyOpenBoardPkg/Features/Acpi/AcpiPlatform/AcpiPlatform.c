/** @file
  ACPI Platform Driver

  @copyright
  Copyright 1999 - 2020 Intel Corporation. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatform.h"
#include "AcpiPlatformUtils.h"
#include "AcpiPlatformHooks.h"
#include <Library/PcdLib.h>
#include <Protocol/PciEnumerationComplete.h>
#include <IioSetupDefinitions.h>
#include <ProcessorPpmSetup.h>


#ifndef __GNUC__
#pragma optimize("", off)
#endif  //__GNUC__

extern SOCKET_IIO_CONFIGURATION                       mSocketIioConfiguration;
extern SOCKET_POWERMANAGEMENT_CONFIGURATION           mSocketPowermanagementConfiguration;
extern SOCKET_PROCESSORCORE_CONFIGURATION             mSocketProcessorCoreConfiguration;
extern EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE *mSpcrTable;
extern EFI_GUID                                       gEfiGlobalVariableGuid;
extern EFI_GUID                                       gEfiPmSsdtTableStorageGuid;

BIOS_ACPI_PARAM       *mAcpiParameter = NULL;
BOOLEAN               mFirstNotify;
SYSTEM_CONFIGURATION  mSystemConfiguration;
PCH_SETUP             mPchSetup;

UINT8                       mKBPresent = 0;
UINT8                       mMousePresent = 0;
EFI_IIO_UDS_PROTOCOL        *mIioUds2 = NULL;
extern CPU_CSR_ACCESS_VAR   *mCpuCsrAccessVarPtr;
UINT8                       mPStateEnable = 0;


VOID
EFIAPI
AcpiOnPciEnumCmplCallback (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
  EFI_STATUS    Status;

  Status = gBS->LocateProtocol (&gEfiPciEnumerationCompleteProtocolGuid, NULL, (VOID **) &Context);
  if (EFI_ERROR (Status)) {
    //
    // Skip the first dummy event signal.
    //
    return;
  }
  gBS->CloseEvent (Event);

  DEBUG ((DEBUG_INFO, "[ACPI] %a\n", __func__));
  AcpiVtdTablesInstall ();
}


VOID
EFIAPI
AcpiOnEndOfDxeCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((DEBUG_INFO, "[ACPI] %a\n", __func__));
  //
  // Installing ACPI Tables: NFIT, PCAT
  //
  InstallAndPatchAcpiTable (NVDIMM_FW_INTERFACE_TABLE_SIGNATURE);
  InstallAndPatchAcpiTable (NVDIMM_PLATFORM_CONFIG_ATTRIBUTE_TABLE_SIGNATURE);
}


//
// Enable SCI for ACPI aware OS at ExitBootServices
//
VOID
EFIAPI
AcpiOnExitBootServicesCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINT16        Pm1Cnt;

  gBS->CloseEvent (Event);

  DEBUG ((DEBUG_INFO, "[ACPI] %a\n", __func__));
  //
  // Enable SCI
  //
  Pm1Cnt = IoRead16 (mAcpiParameter->PmBase + R_ACPI_IO_PM1_CNT);
  Pm1Cnt |= B_ACPI_IO_PM1_CNT_SCI_EN;
  IoWrite16 (mAcpiParameter->PmBase + R_ACPI_IO_PM1_CNT, Pm1Cnt);
}

/**
  Disables the SW SMI Timer.
  ACPI events are disabled and ACPI event status is cleared.
  SCI mode is then enabled.

  Disable SW SMI Timer

  Clear all ACPI event status and disable all ACPI events
  Disable PM sources except power button
  Clear status bits

  Disable GPE0 sources
  Clear status bits

  Disable GPE1 sources
  Clear status bits

  Guarantee day-of-month alarm is invalid (ACPI 1.0 section 4.7.2.4)

  Enable SCI

  @param Event   - not used
  @param Context - not used

  @retval None
**/
STATIC
VOID
AcpiEnableAtReadyToBoot (
  VOID
  )
{
  UINT32      SmiEn;
  UINT8       Data8;
  UINT16      Pm1En;

  ASSERT (mAcpiParameter->PmBase != 0);

  SmiEn = IoRead32 (mAcpiParameter->PmBase + R_ACPI_IO_SMI_EN);

  //
  // Disable SW SMI Timer and legacy USB
  //
  SmiEn &= ~(B_ACPI_IO_SMI_EN_SWSMI_TMR | B_ACPI_IO_SMI_EN_LEGACY_USB | B_ACPI_IO_SMI_EN_LEGACY_USB2);


  //
  // And enable SMI on write to B_ACPI_IO_PM1_CNT_SLP_EN when SLP_TYP is written
  //
  SmiEn |= B_ACPI_IO_SMI_EN_ON_SLP_EN;
  IoWrite32(mAcpiParameter->PmBase + R_ACPI_IO_SMI_EN, SmiEn);

  //
  // Disable PM sources except power button
  //

  Pm1En = B_ACPI_IO_PM1_EN_PWRBTN;
  IoWrite16(mAcpiParameter->PmBase + R_ACPI_IO_PM1_EN, Pm1En);

  //
  // Guarantee day-of-month alarm is invalid (ACPI 1.0 section 4.7.2.4)
  //
  Data8 = RTC_ADDRESS_REGISTER_D;
  IoWrite8(R_IOPORT_CMOS_STANDARD_INDEX, Data8);
  Data8 = 0x0;
  IoWrite8(R_IOPORT_CMOS_STANDARD_DATA, Data8);

  //
  // Do platform specific stuff for ACPI enable SMI
  //


}

/**
  Executes ACPI Platform actions related with ready to boot event

  @param Event   - not used
  @param Context - not used

  @retval None
**/
STATIC
VOID
EFIAPI
AcpiOnReadyToBootCallback (
  IN     EFI_EVENT            Event,
  IN     VOID                *Context
  )
{
  EFI_STATUS                  Status;
  EFI_ACPI_DESCRIPTION_HEADER Table = {0};
  EFI_ACPI_TABLE_VERSION      TableVersion;
  UINTN                       TableHandle;
  EFI_ACPI_TABLE_PROTOCOL     *AcpiTable;
  EFI_CPUID_REGISTER          CpuidRegisters;
  SETUP_DATA                  SetupData;
  UINT8                       ARIForward;

  DYNAMIC_SI_LIBARY_PROTOCOL  *DynamicSiLibraryProtocol = NULL;

  Status = gBS->LocateProtocol (&gDynamicSiLibraryProtocolGuid, NULL, (VOID **) &DynamicSiLibraryProtocol);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return;
  }

  if (mFirstNotify) {
    return;
  }
  mFirstNotify = TRUE;
  DEBUG ((DEBUG_INFO, "[ACPI] %a\n", __func__));

  Status = GetEntireConfig (&SetupData);
  ASSERT_EFI_ERROR (Status);
  CopyMem (&mSystemConfiguration, &(SetupData.SystemConfig), sizeof(SYSTEM_CONFIGURATION));
  CopyMem (&mSocketIioConfiguration, &(SetupData.SocketConfig.IioConfig), sizeof(SOCKET_IIO_CONFIGURATION));
  CopyMem (&mSocketPowermanagementConfiguration, &(SetupData.SocketConfig.PowerManagementConfig), sizeof(SOCKET_POWERMANAGEMENT_CONFIGURATION));
  CopyMem (&mSocketProcessorCoreConfiguration, &(SetupData.SocketConfig.SocketProcessorCoreConfiguration), sizeof(SOCKET_PROCESSORCORE_CONFIGURATION));
  CopyMem (&mPchSetup, &(SetupData.PchSetup), sizeof(PCH_SETUP));

  mAcpiParameter->TpmEnable = mSystemConfiguration.TpmEnable;

  //
  // CpuPm.Asl: External (CSEN, FieldUnitObj)
  //
  mAcpiParameter->CStateEnable = TRUE;
  //
  // CpuPm.Asl: External (C3EN, FieldUnitObj)
  //
  mAcpiParameter->C3Enable     = mSocketPowermanagementConfiguration.C3Enable;
  //
  // CpuPm.Asl: External (C6EN, FieldUnitObj)
  //
  AsmCpuid (CPUID_MONITOR_MWAIT, &CpuidRegisters.RegEax, &CpuidRegisters.RegEbx, &CpuidRegisters.RegEcx, &CpuidRegisters.RegEdx);
  //
  // If C6 is not supported by CPU, disregard setup C6 knob value
  //
  if (((CpuidRegisters.RegEdx >> 12) & 0xF) > 0) {
    if (mSocketPowermanagementConfiguration.C6Enable == PPM_AUTO) {
      mAcpiParameter->C6Enable = 1;  // POR Default = Enabled
    } else {
      mAcpiParameter->C6Enable = mSocketPowermanagementConfiguration.C6Enable;
    }
  } else {
    mAcpiParameter->C6Enable = 0;
    DEBUG ((DEBUG_INFO, "Cpu does not support C6 state\n"));
  }

  if (mAcpiParameter->C6Enable && mAcpiParameter->C3Enable) {  //C3 and C6 enable are exclusive
    mAcpiParameter->C6Enable = 1;
    mAcpiParameter->C3Enable = 0;
  }
  //
  // CpuPm.Asl: External (C7EN, FieldUnitObj)
  //
  mAcpiParameter->C7Enable     = 0;
  //
  // CpuPm.Asl: External (OSCX, FieldUnitObj)
  //
  mAcpiParameter->OSCX         = mSocketPowermanagementConfiguration.OSCx;
  //
  // CpuPm.Asl: External (MWOS, FieldUnitObj)
  //
  mAcpiParameter->MonitorMwaitEnable = 1;
  //
  // CpuPm.Asl: External (PSEN, FieldUnitObj)
  //
  mAcpiParameter->PStateEnable = mPStateEnable;
  //
  // CpuPm.Asl: External (HWAL, FieldUnitObj)
  //
  mAcpiParameter->HWAllEnable = mSocketPowermanagementConfiguration.ProcessorEistPsdFunc;

  mAcpiParameter->KBPresent    = mKBPresent;
  mAcpiParameter->MousePresent = mMousePresent;
  mAcpiParameter->TStateEnable = mSocketPowermanagementConfiguration.TStateEnable;
  //
  // Debug mode indicator for Acpi use
  //
  mAcpiParameter->DebugModeIndicator = (UINT8)PcdGet8 (PcdDebugModeEnable);
  DEBUG ((DEBUG_ERROR, "DebugModeIndicator = %x\n", mAcpiParameter->DebugModeIndicator));

  //
  // Fine grained T state
  //
  AsmCpuid (CPUID_THERMAL_POWER_MANAGEMENT,  &CpuidRegisters.RegEax, &CpuidRegisters.RegEbx, &CpuidRegisters.RegEcx, &CpuidRegisters.RegEdx);
  if ((((CPUID_THERMAL_POWER_MANAGEMENT_EAX*)&CpuidRegisters.RegEax)->Bits.ECMD) && mSocketPowermanagementConfiguration.TStateEnable) {
    mAcpiParameter->TStateFineGrained = 1;
  }
  if (((CPUID_THERMAL_POWER_MANAGEMENT_EAX*)&CpuidRegisters.RegEax)->Bits.HWP_Notification != 0) {
    mAcpiParameter->HwpInterrupt = 1;
  }
  //
  // CpuPm.Asl: External (HWEN, FieldUnitObj)
  //
  mAcpiParameter->HWPMEnable = DetectHwpFeature ();

  mAcpiParameter->EmcaEn    = mSystemConfiguration.EmcaEn;
  mAcpiParameter->WheaSupportEn  = mSystemConfiguration.WheaSupportEn;

  mAcpiParameter->PcieAcpiHotPlugEnable = (UINT8) (BOOLEAN) (mSocketIioConfiguration.PcieAcpiHotPlugEnable != 0);
  //
  // Initialize USB3 mode from setup data
  //
  // If mode != manual control
  //  just copy mode from setup
  //
  if (mPchSetup.PchUsbManualMode != 1) {
    mAcpiParameter->XhciMode = mPchSetup.PchUsbManualMode;
  }

  //
  // Get ACPI IO Base Address
  //
  mAcpiParameter->PmBase = DynamicSiLibraryProtocol->PmcGetAcpiBase ();
  DEBUG ((DEBUG_INFO, "ACPI IO Base Address = %x\n", mAcpiParameter->PmBase));

  //
  // When X2APIC enabled and VTD support enabled, Enable ApicIdOverrided parameter to update ACPI table.
  //
  if (mSocketIioConfiguration.VTdSupport && mSocketProcessorCoreConfiguration.ProcessorX2apic) {
    mAcpiParameter->ApicIdOverrided = 1;
  }

  //
  // Find the AcpiTable protocol
  //
  Status = LocateSupportProtocol (
            &gEfiAcpiTableProtocolGuid,
            gEfiAcpiTableStorageGuid,
            (VOID **) &AcpiTable,
            FALSE
            );

  ASSERT_EFI_ERROR (Status);

  TableVersion    = EFI_ACPI_TABLE_VERSION_2_0;
  Table.Signature = EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE;
  Status = PlatformUpdateTables ((EFI_ACPI_COMMON_HEADER *)&Table, &TableVersion);
  if (!EFI_ERROR (Status)) {
    //
    // Add SPCR table
    //
    if (mSpcrTable != NULL) {
      DEBUG ((DEBUG_INFO, "mSpcrTable->Header.Length=%d\n", mSpcrTable->Header.Length));
      DEBUG ((DEBUG_INFO, "install=%x\n", &(AcpiTable->InstallAcpiTable)));
      DEBUG ((DEBUG_INFO, "acpit=%x\n",   AcpiTable));
      DEBUG ((DEBUG_INFO, "mSpcr=%x\n",   mSpcrTable));
      DEBUG ((DEBUG_INFO, "len   =%d\n",  mSpcrTable->Header.Length));

      TableHandle = 0;
      Status = AcpiTable->InstallAcpiTable (
        AcpiTable,
        mSpcrTable,
        mSpcrTable->Header.Length,
        &TableHandle
        );
      ASSERT_EFI_ERROR (Status);
    } else {
      DEBUG ((DEBUG_INFO, "Warning: mSpcrTable is NULL\n"));
    }
  }
  if (mSpcrTable != NULL) {
    gBS->FreePool (mSpcrTable);
  }

  AcpiEnableAtReadyToBoot();

  Status = GetOptionData (&gEfiSetupVariableGuid,  OFFSET_OF(SYSTEM_CONFIGURATION, ARIForward), &ARIForward, sizeof(UINT8));
  ASSERT_EFI_ERROR (Status);
  if (!ARIForward) {
    DisableAriForwarding ();
  }

}


/**
  Installs ACPI Platform tables

  @param None

  @retval EFI_SUCCESS -  Operation completed successfully.
**/
STATIC
EFI_STATUS
EFIAPI
AcpiPlatformEarlyAcpiTablesInstall (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    AcpiStatus;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTable;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FwVol;
  INTN                          Instance = 0;
  EFI_ACPI_COMMON_HEADER        *CurrentTable;
  EFI_ACPI_TABLE_VERSION        TableVersion;
  UINTN                         TableHandle;
  UINT32                        FvStatus;
  UINT32                        Size;

  //
  // Find the AcpiTable protocol
  //
  Status = LocateSupportProtocol (
            &gEfiAcpiTableProtocolGuid,
            gEfiAcpiTableStorageGuid,
            (VOID **) &AcpiTable,
            FALSE
            );

  ASSERT_EFI_ERROR (Status);

  //
  // Locate the firmware volume protocol
  //
  Status = LocateSupportProtocol (
            &gEfiFirmwareVolume2ProtocolGuid,
            gEfiAcpiTableStorageGuid,
            (VOID **) &FwVol,
            TRUE
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Read tables from the storage file.
  //
  while (!EFI_ERROR (Status)) {
    CurrentTable = NULL;
    TableVersion = EFI_ACPI_TABLE_VERSION_NONE;
    TableHandle = 0;

    Status = FwVol->ReadSection (
                      FwVol,
                      &gEfiAcpiTableStorageGuid,
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID **) &CurrentTable,
                      (UINTN *) &Size,
                      &FvStatus
                      );

    if (!EFI_ERROR (Status)) {

      DEBUG ((DEBUG_INFO, "[ACPI] Table '%c%c%c%c' found in FwVol\n",
              ((CHAR8*)&CurrentTable->Signature)[0], ((CHAR8*)&CurrentTable->Signature)[1],
              ((CHAR8*)&CurrentTable->Signature)[2], ((CHAR8*)&CurrentTable->Signature)[3]));

      //
      // Check if table should be processed or will be updated later
      //
      if (CurrentTable->Signature != NVDIMM_FW_INTERFACE_TABLE_SIGNATURE
          && CurrentTable->Signature != NVDIMM_PLATFORM_CONFIG_ATTRIBUTE_TABLE_SIGNATURE
          ) {
        //
        // Allow platform specific code to reject the table or update it
        //
        AcpiStatus = AcpiPlatformHooksIsActiveTable (CurrentTable); //SystemBoard);
        if (!EFI_ERROR (AcpiStatus)) {
          //
          // Perform any table specific updates.
          //
          AcpiStatus = PlatformUpdateTables (CurrentTable, &TableVersion);
          if (!EFI_ERROR (AcpiStatus)) {
            //
            // Add the table
            //
            if (TableVersion != EFI_ACPI_TABLE_VERSION_NONE) {
              //
              // Install the table
              //
              AcpiStatus = AcpiTable->InstallAcpiTable (AcpiTable, CurrentTable, CurrentTable->Length, &TableHandle);
              if (EFI_ERROR (AcpiStatus)) {
                ASSERT_EFI_ERROR (AcpiStatus);
                return AcpiStatus;
              }
            }
          }
        }
      }
      //
      // Increment the instance
      //
      Instance++;
    }
  }

  //
  // Build any from-scratch ACPI tables. Halt on errors for debug builds, but
  // for release builds it is safe to continue.
  //
  Status = PlatformBuildTables ();
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}

/**
  Installs ACPI Platform tables that wasn't installed in the early phase

  @param None

  @retval EFI_SUCCESS -  Operation completed successfully.
**/
STATIC
EFI_STATUS
EFIAPI
AcpiPlatformLateAcpiTablesInstall (
  VOID
  )
{
  //
  // Install xHCI ACPI Table
  //
  InstallXhciAcpiTable ();

  return EFI_SUCCESS;
}


/**
  Installs ACPI PmSsdt tables

  @param None

  @retval EFI_SUCCESS -  Operation completed successfully.

**/
STATIC
EFI_STATUS
EFIAPI
PmSsdtEarlyAcpiTablesInstall (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    AcpiStatus;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTable;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FwVol;
  INTN                          Instance = 0;
  EFI_ACPI_COMMON_HEADER        *CurrentTable;
  EFI_ACPI_TABLE_VERSION        TableVersion;
  UINTN                         TableHandle;
  UINT32                        FvStatus;
  UINT32                        Size;

  //
  // Find the AcpiTable protocol
  //
  Status = LocateSupportProtocol (
            &gEfiAcpiTableProtocolGuid,
            gEfiPmSsdtTableStorageGuid,
            (VOID **) &AcpiTable,
            FALSE
            );

  ASSERT_EFI_ERROR (Status);

  //
  // Locate the firmware volume protocol
  //
  Status = LocateSupportProtocol (
            &gEfiFirmwareVolume2ProtocolGuid,
            gEfiPmSsdtTableStorageGuid,
            (VOID **) &FwVol,
            TRUE
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Read tables from the storage file.
  //
  while (!EFI_ERROR (Status)) {
    CurrentTable = NULL;
    TableVersion = EFI_ACPI_TABLE_VERSION_NONE;
    TableHandle = 0;

    Status = FwVol->ReadSection (
                      FwVol,
                      &gEfiPmSsdtTableStorageGuid,
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID **) &CurrentTable,
                      (UINTN *) &Size,
                      &FvStatus
                      );

    if (!EFI_ERROR (Status)) {
      //
      // Check if table should be processed or will be updated later
      //
      if (CurrentTable->Signature != NVDIMM_FW_INTERFACE_TABLE_SIGNATURE) {
        //
        // Allow platform specific code to reject the table or update it
        //
        AcpiStatus = AcpiPlatformHooksIsActiveTable (CurrentTable);
        if (!EFI_ERROR (AcpiStatus)) {
          //
          // Perform any table specific updates.
          //
          AcpiStatus = PlatformUpdateTables (CurrentTable, &TableVersion);
          if (!EFI_ERROR (AcpiStatus)) {
            //
            // Add the table
            //
            if (TableVersion != EFI_ACPI_TABLE_VERSION_NONE) {
              //
              // Install the table
              //
              AcpiStatus = AcpiTable->InstallAcpiTable (AcpiTable, CurrentTable, CurrentTable->Length, &TableHandle);
              if (!EFI_ERROR (AcpiStatus)) {
                ASSERT_EFI_ERROR (AcpiStatus);
                return AcpiStatus;
              }
            }
          }
        }
      }
      //
      // Increment the instance
      //
      Instance++;
    }
  }
  return EFI_SUCCESS;
} // PmSsdtEarlyAcpiTablesInstall()


/**
  Entry point for Acpi platform driver.

  @param ImageHandle  -  A handle for the image that is initializing this driver.
  @param SystemTable  -  A pointer to the EFI system table.

  @retval EFI_SUCCESS           -  Driver initialized successfully.
  @retval EFI_LOAD_ERROR        -  Failed to Initialize or has been loaded.
  @retval EFI_OUT_OF_RESOURCES  -  Could not allocate needed resources.
**/
EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *HobList;
  VOID        *RgstPtr;

  DYNAMIC_SI_LIBARY_PROTOCOL  *DynamicSiLibraryProtocol = NULL;

  Status = gBS->LocateProtocol (&gDynamicSiLibraryProtocolGuid, NULL, (VOID **) &DynamicSiLibraryProtocol);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  mFirstNotify = FALSE;
  //
  // Report Status Code to indicate Acpi Init
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_BS_ACPI_INIT)
    );

  Status = gBS->LocateProtocol (&gEfiIioUdsProtocolGuid, NULL, (VOID **) &mIioUds2);
  if (EFI_ERROR (Status)) {

    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  mCpuCsrAccessVarPtr = DynamicSiLibraryProtocol->GetSysCpuCsrAccessVar ();
  //
  // Update HOB variable for PCI resource information
  // Get the HOB list.  If it is not present, then ASSERT.
  //
  Status = EfiGetSystemConfigurationTable (&gEfiHobListGuid, &HobList);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize Platform Hooks
  //
  PlatformHookInit ();

  //
  // Install ACPI PLatform Tables
  //
  Status = AcpiPlatformEarlyAcpiTablesInstall ();
  ASSERT_EFI_ERROR (Status);

  //
  // Install ACPI PMSsdt Tables
  //
  Status = PmSsdtEarlyAcpiTablesInstall ();
  ASSERT_EFI_ERROR (Status);

  //
  // Install ACPI PLatform Tables that wasn't installed yet (NFIT/xHCI)
  //
  Status = AcpiPlatformLateAcpiTablesInstall ();
  ASSERT_EFI_ERROR (Status);

  //
  // Register ready to boot event handler
  //
  Status = EfiCreateEventReadyToBootEx (TPL_NOTIFY, AcpiOnReadyToBootCallback, NULL, &Event);
  ASSERT_EFI_ERROR (Status);

  //
  // Create PCI Enumeration Completed callback.
  //
  Event = EfiCreateProtocolNotifyEvent (&gEfiPciEnumerationCompleteProtocolGuid, TPL_CALLBACK,
                                        AcpiOnPciEnumCmplCallback, NULL, &RgstPtr);
  ASSERT (Event != NULL);

  //
  // Register EndOfDxe handler
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiOnEndOfDxeCallback,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register ExitBootServicesEvent handler
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiOnExitBootServicesCallback,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
} // AcpiPlatformEntryPoint()
