/** @file
  This driver publishes a protocol that is used by the ACPI SMM Platform
  driver to notify the BMC of Power State transitions.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BmcAcpiSwChild.h"

//
// Global variables
//
EFI_BMC_ACPI_SW_CHILD_POLICY_PROTOCOL  mBmcAcpiSwChild;

/**
  This function initializes the BMC ACPI SW Child protocol.

  @retval EFI_SUCCESS - If all services discovered.
  @retval Other       - Failure in constructor.

**/
EFI_STATUS
InitializeBmcAcpiSwChild (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Status = EFI_SUCCESS;

  mBmcAcpiSwChild.SetACPIPowerStateInBMC = (EFI_SET_ACPI_POWER_STATE_IN_BMC)SetACPIPowerStateInBMC;

  //
  // Install protocol
  //
  Handle = NULL;
  Status = gMmst->MmInstallProtocolInterface (
                    &Handle,
                    &gEfiBmcAcpiSwChildPolicyProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mBmcAcpiSwChild
                    );

  return Status;
}

/**
  Send the time to the BMC, in the UNIX 32 bit format.

  @retval Status          - result of sending the time stamp

**/
EFI_STATUS
SyncTimeStamp (
  VOID
  )
{
  EFI_STATUS                   Status;
  UINT32                       ResponseDataSize;
  IPMI_ADD_SEL_ENTRY_REQUEST   TimeStampEvtRecord;
  IPMI_ADD_SEL_ENTRY_RESPONSE  AddSelEntryResponse;
  IPMI_SET_SEL_TIME_REQUEST    SelTimeReq;
  UINT8                        SelTimeResponse;

  TimeStampEvtRecord.RecordData.RecordId     = 0;                                    // Record Id
  TimeStampEvtRecord.RecordData.RecordType   = IPMI_SEL_SYSTEM_RECORD;               // Record Type
  TimeStampEvtRecord.RecordData.TimeStamp    = 0;                                    // Time stamp
  TimeStampEvtRecord.RecordData.GeneratorId  = 0x0003;                               // GenID:BIOS
  TimeStampEvtRecord.RecordData.EvMRevision  = IPMI_EVM_REVISION;                    // EVM REV
  TimeStampEvtRecord.RecordData.SensorType   = 0x12;                                 // Sensor Type
  TimeStampEvtRecord.RecordData.SensorNumber = 0x83;                                 // Sensor No
  TimeStampEvtRecord.RecordData.EventDirType = IPMI_SENSOR_TYPE_EVENT_CODE_DISCRETE; // Event Dir
  TimeStampEvtRecord.RecordData.OEMEvData1   = 05;                                   // Sensor specific Offset for Timestamp Clock Synch Event.
  TimeStampEvtRecord.RecordData.OEMEvData2   = 00;                                   // ED2
  TimeStampEvtRecord.RecordData.OEMEvData3   = 0xFF;                                 // ED3

  //
  // Log Timestamp Clock Synch Event 1st pair.
  //
  ResponseDataSize = sizeof (AddSelEntryResponse);
  Status           = IpmiSubmitCommand (
                       IPMI_NETFN_STORAGE,
                       IPMI_STORAGE_ADD_SEL_ENTRY,
                       (UINT8 *)&TimeStampEvtRecord,
                       sizeof (TimeStampEvtRecord),
                       (UINT8 *)&AddSelEntryResponse,
                       &ResponseDataSize
                       );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "IpmiSubmitCommand Storage Add SEL Entry Failed %r\n", Status));
    return Status;
  }

  Status = EfiSmGetTimeStamp (&SelTimeReq.Timestamp);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ResponseDataSize = sizeof (SelTimeResponse);
  Status           = IpmiSubmitCommand (
                       IPMI_NETFN_STORAGE,
                       IPMI_STORAGE_SET_SEL_TIME,
                       (UINT8 *)&SelTimeReq,
                       sizeof (SelTimeReq),
                       (UINT8 *)&SelTimeResponse,
                       &ResponseDataSize
                       );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "IpmiSubmitCommand Storage Set SEL Time Failed %r\n", Status));
    return Status;
  }

  //
  // Log Timestamp Clock Sync Event 2nd pair.
  //
  TimeStampEvtRecord.RecordData.OEMEvData2 = 0x80;
  ResponseDataSize                         = sizeof (AddSelEntryResponse);
  Status                                   = IpmiSubmitCommand (
                                               IPMI_NETFN_STORAGE,
                                               IPMI_STORAGE_ADD_SEL_ENTRY,
                                               (UINT8 *)&TimeStampEvtRecord,
                                               sizeof (TimeStampEvtRecord),
                                               (UINT8 *)&AddSelEntryResponse,
                                               &ResponseDataSize
                                               );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "IpmiSubmitCommand Storage Add SEL Entry Failed %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Send a command to BMC to set the present power state.

  @param This
  @param PowerState
  @param DeviceState

  @retval EFI_SUCCESS               if successful
  @retval Other than EFI_SUCCESS    if not successful

**/
EFI_STATUS
SetACPIPowerStateInBMC (
  IN EFI_BMC_ACPI_SW_CHILD_POLICY_PROTOCOL  *This,
  IN UINT8                                  PowerState,
  IN UINT8                                  DeviceState
  )
{
  EFI_STATUS                         Status;
  IPMI_SET_ACPI_POWER_STATE_REQUEST  AcpiPowerStateRequest;
  UINT8                              AcpiPowerStateResponse;
  UINT32                             ResponseDataSize;

  AcpiPowerStateRequest.SystemPowerState.Bits.PowerState  = PowerState;
  AcpiPowerStateRequest.SystemPowerState.Bits.StateChange = 1;
  AcpiPowerStateRequest.DevicePowerState.Bits.PowerState  = DeviceState;
  AcpiPowerStateRequest.DevicePowerState.Bits.StateChange = 1;

  ResponseDataSize =  sizeof (AcpiPowerStateResponse);

  Status = IpmiSubmitCommand (
             IPMI_NETFN_APP,
             IPMI_APP_SET_ACPI_POWERSTATE,
             (UINT8 *)&AcpiPowerStateRequest,
             sizeof (AcpiPowerStateRequest),
             (UINT8 *)&AcpiPowerStateResponse,
             &ResponseDataSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "IpmiSubmitCommand App Set Acpi Power State Failed %r\n", Status));
  }

  return Status;
}
