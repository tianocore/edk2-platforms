/** @file
  Generic StandaloneMm IPMI stack driver

  @copyright
  Copyright 2023 Intel Corporation. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

//
// Statements that include other files
//
#include <IndustryStandard/Ipmi.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/SmmLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IpmiBaseLib.h>
#include <SmStatusCodes.h>
#include "IpmiHooks.h"
#include "IpmiBmcCommon.h"
#include "IpmiBmc.h"
#include <Library/TimerLib.h>

IPMI_BMC_INSTANCE_DATA             *mIpmiInstance;
EFI_HANDLE                         mHandle;

EFI_STATUS
SmmInitializeIpmiKcsPhysicalLayer (
  VOID
  )
/*++

Routine Description:
  Setup and initialize the BMC for the SMM phase.  In order to verify the BMC is functioning
  as expected, the BMC Selftest is performed.  The results are then checked and any errors are
  reported to the error manager.  Errors are collected throughout this routine and reported
  just prior to installing the driver.  If there are more errors than MAX_SOFT_COUNT, then they
  will be ignored.

Arguments:
  ImageHandle - Handle of this driver image
  SystemTable - Table containing standard EFI services

Returns:
  EFI_SUCCESS - Successful driver initialization

--*/
{
  EFI_STATUS                       Status;

  DEBUG ((DEBUG_INFO,"SmmInitializeIpmiKcsPhysicalLayer entry \n"));

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (IPMI_BMC_INSTANCE_DATA),
                    (VOID **)&mIpmiInstance);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "mIpmiInstance mem alloc failed - 0x%x\n", Status));
    return Status;
  } else {

    //
    // Initialize the KCS transaction timeout. Assume delay unit is 1000 us.
    //
    mIpmiInstance->KcsTimeoutPeriod = (BMC_KCS_TIMEOUT * 1000*1000) / KCS_DELAY_UNIT;

    //
    // Initialize IPMI IO Base, we still use SMS IO base to get device ID and Seltest result since SMM IF may have different cmds supported
    //
    mIpmiInstance->IpmiIoBase                       = FixedPcdGet16 (PcdIpmiSmmIoBaseAddress);
    mIpmiInstance->Signature                        = SM_IPMI_BMC_SIGNATURE;
    mIpmiInstance->SlaveAddress                     = BMC_SLAVE_ADDRESS;
    mIpmiInstance->BmcStatus                        = BMC_NOTREADY;
    mIpmiInstance->IpmiTransport.IpmiSubmitCommand  = IpmiSendCommand;
    mIpmiInstance->IpmiTransport.GetBmcStatus       = IpmiGetBmcStatus;

    mHandle = NULL;
    Status = gMmst->MmInstallProtocolInterface (
                      &mHandle,
                      &gSmmIpmiTransportProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &mIpmiInstance->IpmiTransport
                      );
    ASSERT_EFI_ERROR (Status);

    DEBUG ((DEBUG_INFO,"SmmInitializeIpmiKcsPhysicalLayer exit \n"));

    return EFI_SUCCESS;
  }
}

/**
  The module Entry Point of driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the MM System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
InitializeGenericIpmiStandaloneMm (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_MM_SYSTEM_TABLE    *SystemTable
  )
{
  SmmInitializeIpmiKcsPhysicalLayer ();
  return EFI_SUCCESS;
}

/**
  Unloads an image.

  @param[in] ImageHandle        Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
GenericIpmiStandaloneMmUnload (
  IN EFI_HANDLE             ImageHandle
  )
{
  EFI_STATUS                Status;

  Status = EFI_SUCCESS;
  if (mIpmiInstance != NULL) {
    if (mHandle != NULL) {
      Status = gMmst->MmUninstallProtocolInterface (
                        mHandle,
                        &gSmmIpmiTransportProtocolGuid,
                        &mIpmiInstance->IpmiTransport
                        );
      ASSERT_EFI_ERROR (Status);
    }
    gMmst->MmFreePool (mIpmiInstance);
  }

  return Status;
}
