/** @file

  This module installs Boot Progress Dxe that report boot progress to SMpro.

  This module registers report status code listener to report boot progress
  to SMpro.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/AmpereCpuLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Pi/PiStatusCode.h>
#include <Protocol/ReportStatusCodeHandler.h>

typedef struct {
  UINT8                    Byte;
  EFI_STATUS_CODE_VALUE    Value;
} STATUS_CODE_TO_CHECKPOINT;

typedef enum {
  BootNotStart = 0,
  BootStart,
  BootComplete,
  BootFailed,
  BootProgressStateMax
} BOOT_PROGRESS_STATE;

UINT32  DxeProgressCode[] = {
  (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ENTRY_POINT),                     // DXE Core is started
  (EFI_COMPUTING_UNIT_CHIPSET | EFI_CHIPSET_PC_DXE_HB_INIT),                    // PCI host bridge initialization
  (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT),                 // Boot Device Selection (BDS) phase is started 
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS),     // Driver connecting is started
  (EFI_IO_BUS_PCI | EFI_IOB_PC_INIT),                                           // PCI Bus initialization is started
  (EFI_IO_BUS_PCI | EFI_IOB_PCI_HPC_INIT),                                      // PCI Bus Hot Plug Controller Initialization
  (EFI_IO_BUS_PCI | EFI_IOB_PCI_BUS_ENUM),                                      // PCI Bus Enumeration
  (EFI_IO_BUS_PCI | EFI_IOB_PCI_RES_ALLOC),                                     // PCI Bus Request Resources
  (EFI_IO_BUS_PCI | EFI_IOB_PC_ENABLE),                                         // PCI Bus Assign Resources
  (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_INIT),                               // Console Output devices connect
  (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_INIT),                                    // Console input devices connect
  (EFI_IO_BUS_LPC | EFI_IOB_PC_INIT),                                           // Super IO Initialization
  (EFI_IO_BUS_USB | EFI_IOB_PC_INIT),                                           // USB initialization is started
  (EFI_IO_BUS_USB | EFI_IOB_PC_RESET),                                          // USB Reset
  (EFI_IO_BUS_USB | EFI_IOB_PC_DETECT),                                         // USB Detect
  (EFI_IO_BUS_USB | EFI_IOB_PC_ENABLE),                                         // USB Enable
  (EFI_IO_BUS_SCSI | EFI_IOB_PC_INIT),                                          // SCSI initialization is started
  (EFI_IO_BUS_SCSI | EFI_IOB_PC_RESET),                                         // SCSI Reset
  (EFI_IO_BUS_SCSI | EFI_IOB_PC_DETECT),                                        // SCSI Detect
  (EFI_IO_BUS_SCSI | EFI_IOB_PC_ENABLE),                                        // SCSI Enable
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_VERIFYING_PASSWORD),           // Setup Verifying Password
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP),                          // Start of Setup
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT),                          // Setup Input Wait
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT),          // Ready To Boot event
  (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES),            // Exit Boot Services event
  (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_RS_PC_SET_VIRTUAL_ADDRESS_MAP),    // Runtime Set Virtual Address MAP Begin
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_VIRTUAL_ADDRESS_CHANGE_EVENT), // Runtime Set Virtual Address MAP End
  (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_RS_PC_RESET_SYSTEM),               // System Reset
  (EFI_IO_BUS_USB | EFI_IOB_PC_HOTPLUG),                                        // USB hot plug
  (EFI_IO_BUS_PCI | EFI_IOB_PC_HOTPLUG),                                        // PCI bus hot plug
  0                                                                             // Must ended by 0
};

UINT32  DxeErrorCode[] = {
  (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_EC_NO_ARCH),                   // Some of the Architectural Protocols are not available
  (EFI_IO_BUS_PCI | EFI_IOB_EC_RESOURCE_CONFLICT),                        // PCI resource allocation error. Out of Resources
  (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_NOT_DETECTED),                 // No Console Output Devices are found
  (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED),                      // No Console Input Devices are found
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_INVALID_PASSWORD),       // Invalid password
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR), // Error loading Boot Option (LoadImage returned error)
  (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED),     // Boot Option is failed (StartImage returned error)
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UPDATE_FAIL),             // Flash update is failed
  (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_PS_EC_RESET_NOT_AVAILABLE),  // Reset protocol is not available
  0                                                                       // Must end by 0
};

EFI_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol = NULL;

STATIC UINT8  mBootstate = BootStart;

STATIC BOOLEAN  mEndOfDxe = FALSE;

STATIC
BOOLEAN
StatusCodeFilter (
  UINT32                 *Map,
  EFI_STATUS_CODE_VALUE  Value
  )
{
  UINTN  Index = 0;

  while (Map[Index] != 0) {
    if (Map[Index] == Value) {
      return TRUE;
    }

    Index++;
  }

  return FALSE;
}

/**
  Report status code listener of Boot Progress Dxe.

  @param[in]  CodeType            Indicates the type of status code being reported.
  @param[in]  Value               Describes the current status of a hardware or software entity.
                                  This included information about the class and subclass that is used to
                                  classify the entity as well as an operation.
  @param[in]  Instance            The enumeration of a hardware or software entity within
                                  the system. Valid instance numbers start with 1.
  @param[in]  CallerId            This optional parameter may be used to identify the caller.
                                  This parameter allows the status code driver to apply different rules to
                                  different callers.
  @param[in]  Data                This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS             Status code is what we expected.
  @retval Otherwise               Errors returned from the MailboxMsgSetBootProgress() functions.

**/
EFI_STATUS
EFIAPI
BootProgressListenerDxe (
  IN EFI_STATUS_CODE_TYPE   CodeType,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN EFI_GUID               *CallerId,
  IN EFI_STATUS_CODE_DATA   *Data
  )
{
  EFI_STATUS  Status;
  UINT8       BootStage;
  BOOLEAN     IsProgress = FALSE;
  BOOLEAN     IsError    = FALSE;

  if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
    IsProgress = StatusCodeFilter (DxeProgressCode, Value);
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
    IsError = StatusCodeFilter (DxeErrorCode, Value);
  } else {
    return EFI_SUCCESS;
  }

  if (!IsProgress && !IsError) {
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_INFO,
    "BootProgressDxe: CodeType=0x%X Value=0x%X Instance=0x%X CallerIdGuid=%g Data=%p\n",
    CodeType,
    Value,
    Instance,
    CallerId,
    Data
    ));

  if (IsError) {
    mBootstate = BootFailed;
  } else if (  (Value == (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES))
            || (Value == (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT)))
  {
    /* Set boot complete when reach to Exit Boot Service event or DXE Core Handoff To Next */
    mBootstate = BootComplete;
  } else {
    mBootstate = BootStart;
  }

  BootStage = mEndOfDxe ? MAILBOX_BOOT_PROGRESS_STAGE_OS : MAILBOX_BOOT_PROGRESS_STAGE_UEFI;

  Status = MailboxMsgSetBootProgress (MASTER_SOCKET, BootStage, mBootstate, Value);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MailboxMsgSetBootProgress failed - %r\n", Status));
    return Status;
  }

  // Set mEndOfDxe flag to indicate that UEFI firmware booting ended
  if (Value == (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT)) {
    mEndOfDxe = TRUE;
  }

  if ((Value == (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES)) &&
      (mRscHandlerProtocol != NULL))
  {
    mRscHandlerProtocol->Unregister (BootProgressListenerDxe);
  }

  return EFI_SUCCESS;
}

/**
  The module Entry Point of the Firmware Performance Data Table DXE driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BootProgressDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Get Report Status Code Handler Protocol.
  //
  Status = gBS->LocateProtocol (&gEfiRscHandlerProtocolGuid, NULL, (VOID **)&mRscHandlerProtocol);
  ASSERT_EFI_ERROR (Status);

  //
  // Register report status code listener for OS Loader load and start.
  //
  if (!EFI_ERROR (Status)) {
    Status = mRscHandlerProtocol->Register (BootProgressListenerDxe, TPL_HIGH_LEVEL);
  }

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
