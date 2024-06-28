/** @file
  Source code file for Platform Init PEI module

Copyright (c) 2017 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <IndustryStandard/Pci30.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MpServices2.h>

#include <Guid/FirmwareFileSystem2.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/TimerLib.h>
#include <Library/BoardInitLib.h>
#include <Library/TestPointCheckLib.h>
#include <Library/SetCacheMtrrLib.h>
#include <Library/SmmRelocationLib.h>

EFI_STATUS
EFIAPI
PlatformInitEndOfPei (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Notification function called when EDKII_PEI_MP_SERVICES2_PPI becomes available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @return  Status of the notification. The status code returned from this
           function is ignored.
**/
STATIC
EFI_STATUS
EFIAPI
OnMpServices2Available (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

static EFI_PEI_NOTIFY_DESCRIPTOR  mEndOfPeiNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  (EFI_PEIM_NOTIFY_ENTRY_POINT) PlatformInitEndOfPei
};

//
// Notification object for registering the callback, for when
// EFI_PEI_MP_SERVICES_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR  mMpServices2Notify = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK |   // Flags
  EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEdkiiPeiMpServices2PpiGuid,              // Guid
  OnMpServices2Available                     // Notify
};

/**
  This function handles PlatformInit task at the end of PEI

  @param[in]  PeiServices  Pointer to PEI Services Table.
  @param[in]  NotifyDesc   Pointer to the descriptor for the Notification event that
                           caused this function to execute.
  @param[in]  Ppi          Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully
  @retval     others
**/
EFI_STATUS
EFIAPI
PlatformInitEndOfPei (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                    Status;

  Status = BoardInitAfterSiliconInit ();
  ASSERT_EFI_ERROR (Status);

  TestPointEndOfPeiSystemResourceFunctional ();

  TestPointEndOfPeiPciBusMasterDisabled ();

  Status = SetCacheMtrrAfterEndOfPei ();
  ASSERT_EFI_ERROR (Status);

  TestPointEndOfPeiMtrrFunctional ();

  return Status;
}

/**
  Notification function called when EDKII_PEI_MP_SERVICES2_PPI becomes available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @return  Status of the notification. The status code returned from this
           function is ignored.
**/
STATIC
EFI_STATUS
EFIAPI
OnMpServices2Available (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EDKII_PEI_MP_SERVICES2_PPI  *MpServices2;
  EFI_STATUS                  Status;

  MpServices2 = Ppi;

  //
  // Smm Relocation Initialize.
  //
  Status = SmmRelocationInit (MpServices2);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OnMpServices2Available: Not able to execute Smm Relocation Init.  Status: %r\n", Status));
  }

  return EFI_SUCCESS;
}

/**
  Platform Init PEI module entry point

  @param[in]  FileHandle           Not used.
  @param[in]  PeiServices          General purpose services available to every PEIM.

  @retval     EFI_SUCCESS          The function completes successfully
  @retval     EFI_OUT_OF_RESOURCES Insufficient resources to create database
**/
EFI_STATUS
EFIAPI
PlatformInitPostMemEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                       Status;

  Status = BoardInitBeforeSiliconInit ();
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesNotifyPpi (&mMpServices2Notify);
  ASSERT_EFI_ERROR (Status);

  //
  // Performing PlatformInitEndOfPei after EndOfPei PPI produced
  //
  Status = PeiServicesNotifyPpi (&mEndOfPeiNotifyList);

  return Status;
}
