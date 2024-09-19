/** @file

  This module installs Boot Progress Pei to report boot progress to SMpro.

  This module registers report status code listener to report boot progress
  to SMpro.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/AmpereCpuLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>
#include <Pi/PiStatusCode.h>
#include <Ppi/ReportStatusCodeHandler.h>

typedef enum  {
  BootNotStart = 0,
  BootStart,
  BootComplete,
  BootFailed,
  BootProgressStateMax
} BOOT_PROGRESS_STATE;

UINT32  PeiProgressStatusCode[] = {
  // Regular boot
  (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_PC_ENTRY_POINT),         // PEI Core is started
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_POWER_ON_INIT), // Pre-memory CPU initialization is started
  (EFI_SOFTWARE_PEI_SERVICE | EFI_SW_PS_PC_INSTALL_PEI_MEMORY),     // Memory Installed
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_PC_INIT_BEGIN),       // CPU post-memory initialization is started
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_CACHE_INIT),    // CPU post-memory initialization. Cache initialization
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_AP_INIT),       // CPU post-memory initialization. Application Processor(s) (AP) initialization
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_BSP_SELECT),    // CPU post-memory initialization. Boot Strap Processor (BSP) selection
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_SMM_INIT),      // CPU post-memory initialization. System Management Mode (SMM) initialization
  // DXE IPL is started
  (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_PC_HANDOFF_TO_NEXT), // DXE IPL is started
  // DXE Core is started
  (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ENTRY_POINT),     // DXE Core is started
  // Recovery
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_RECOVERY_AUTO),  // Recovery condition triggered by firmware (Auto recovery)
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_RECOVERY_USER),  // Recovery condition triggered by user (Forced recovery)
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_RECOVERY_BEGIN), // Recovery process started
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_CAPSULE_LOAD),   // Recovery firmware image is found
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_CAPSULE_START),  // Recovery firmware image is loaded
  // S3
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_S3_BOOT_SCRIPT), // S3 Boot Script execution
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_OS_WAKE),        // OS S3 wake vector call
  0                                                         // Must end with 0
};

UINT32  PeiErrorStatusCode[] = {
  // Regular boot
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_INVALID_TYPE),       // Memory initialization error. Invalid memory type
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_INVALID_SPEED),      // Memory initialization error. Incompatible memory speed
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_SPD_FAIL),           // Memory initialization error. SPD reading has failed
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_INVALID_SIZE),       // Memory initialization error. Invalid memory size
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_MISMATCH),           // Memory initialization error. Memory modules do not match
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_NONE_DETECTED),      // Memory initialization error. No usable memory detected
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_NONE_USEFUL),        // Memory initialization error. No usable memory detected
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_EC_NON_SPECIFIC),              // Unspecified memory initialization error.
  (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_EC_MEMORY_NOT_INSTALLED), // Memory not installed
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_INVALID_TYPE),   // Invalid CPU type
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_INVALID_SPEED),  // Invalid CPU Speed
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_MISMATCH),       // CPU mismatch
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST),      // CPU self test failed
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_CACHE),          // possible CPU cache error
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_INTERNAL),       // Internal CPU error
  (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_EC_NON_SPECIFIC),      // Internal CPU error
  (EFI_SOFTWARE_PEI_SERVICE | EFI_SW_PS_EC_RESET_NOT_AVAILABLE),     // reset PPI is not available
  // Recovery
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_RECOVERY_PPI_NOT_FOUND),     // Recovery PPI is not available
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_NO_RECOVERY_CAPSULE),        // Recovery capsule is not found
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_INVALID_CAPSULE_DESCRIPTOR), // Invalid recovery capsule
  // S3 Resume
  (EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_S3_RESUME_FAIL),     // S3 Resume Failed
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_RESUME_PPI_NOT_FOUND), // S3 Resume PPI not found
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_BOOT_SCRIPT_ERROR),    // S3 Resume Boot Script Error
  (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_OS_WAKE_ERROR),        // S3 OS Wake Error
  0
};

// Should always be BootStart when start
STATIC UINT8  mBootstate = BootStart;

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
  Report status code listener for PEI. This is used to record the boot progress info
  and report it to SMpro.

  @param[in]  PeiServices         An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
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
  @retval EFI_UNSUPPORTED         Status code not supported.

**/
EFI_STATUS
EFIAPI
BootProgressListenerPei (
  IN       CONST EFI_PEI_SERVICES  **PeiServices,
  IN       EFI_STATUS_CODE_TYPE    CodeType,
  IN       EFI_STATUS_CODE_VALUE   Value,
  IN       UINT32                  Instance,
  IN CONST EFI_GUID                *CallerId,
  IN CONST EFI_STATUS_CODE_DATA    *Data
  )
{
  BOOLEAN  IsProgress = FALSE;
  BOOLEAN  IsError    = FALSE;

  if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
    IsProgress = StatusCodeFilter (PeiProgressStatusCode, Value);
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
    IsError = StatusCodeFilter (PeiErrorStatusCode, Value);
  } else {
    return EFI_SUCCESS;
  }

  // No interested status found
  if (!IsProgress && !IsError) {
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_INFO,
    "BootProgressPeim: CodeType=0x%X Value=0x%X Instance=0x%X CallerIdGuid=%g Data=%p\n",
    CodeType,
    Value,
    Instance,
    CallerId,
    Data
    ));

  if (IsError) {
    mBootstate = BootFailed;
  }

  MailboxMsgSetBootProgress (MASTER_SOCKET, MAILBOX_BOOT_PROGRESS_STAGE_UEFI, mBootstate, Value);

  return EFI_SUCCESS;
}

/**
  Main entry for Boot Progress PEIM.

  This routine is to register report status code listener for Boot Progress PEIM.

  @param[in]  FileHandle              Handle of the file being invoked.
  @param[in]  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCCESS Report status code listener is registered successfully.

**/
EFI_STATUS
EFIAPI
BootProgressPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS               Status;
  EFI_PEI_RSC_HANDLER_PPI  *RscHandler;

  Status = PeiServicesLocatePpi (
             &gEfiPeiRscHandlerPpiGuid,
             0,
             NULL,
             (VOID **)&RscHandler
             );
  ASSERT_EFI_ERROR (Status);

  if (!EFI_ERROR (Status)) {
    Status = RscHandler->Register (BootProgressListenerPei);
  }

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
