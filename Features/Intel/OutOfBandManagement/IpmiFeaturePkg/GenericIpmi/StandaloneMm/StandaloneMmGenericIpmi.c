/** @file
  Generic StandaloneMm IPMI stack driver

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
#include <Library/TimerLib.h>
#include <Library/BmcCommonInterfaceLib.h>
#include <SmStatusCodes.h>
#include "IpmiHooks.h"
#include "IpmiBmcCommon.h"
#include "IpmiBmc.h"

IPMI_BMC_INSTANCE_DATA  *mIpmiInstance;
EFI_HANDLE              mIpmiTransportHandle;
EFI_HANDLE              mIpmiTransport2Handle;

/**

Routine Description:
  Initialize the API and parameters for IPMI Transport2 Instance

Arguments:
  IpmiInstance    - Pointer to IPMI Instance.

Returns:
  VOID            - Nothing.

**/
VOID
InitIpmiTransport2 (
  IN  IPMI_BMC_INSTANCE_DATA  *IpmiInstance
  )
{
  IpmiInstance->IpmiTransport2.InterfaceType           = FixedPcdGet8 (PcdDefaultSystemInterface);
  IpmiInstance->IpmiTransport2.IpmiTransport2BmcStatus = BmcStatusOk;
  IpmiInstance->IpmiTransport2.IpmiSubmitCommand2      = IpmiSendCommand2;
  IpmiInstance->IpmiTransport2.IpmiSubmitCommand2Ex    = IpmiSendCommand2Ex;

  if (FixedPcdGet8 (PcdBtInterfaceSupport) == 1) {
    InitBtInterfaceData (&IpmiInstance->IpmiTransport2);
  }

  if (FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) {
    InitSsifInterfaceData (&IpmiInstance->IpmiTransport2);
  }

  if (FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) {
    InitIpmbInterfaceData (&IpmiInstance->IpmiTransport2);
  }
}

/**

Routine Description:
  Notify call back to initialize the interfaces and install SMM IPMI
  protocol.

Arguments:
  Protocol    - Pointer to the protocol guid.
  Interface   - Pointer to the protocol instance.
  Handle      - Handle on which the protocol is installed.

Returns:
  Status of Notify call back.

**/
EFI_STATUS
EFIAPI
SmmNotifyCallback (
  IN CONST  EFI_GUID    *Protocol,
  IN        VOID        *Interface,
  IN        EFI_HANDLE  Handle
  )
{
  EFI_STATUS            Status;
  IPMI_INTERFACE_STATE  InterfaceState;

  InterfaceState = IpmiInterfaceNotReady;

  if (FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) {
    InitSsifInterfaceData (&mIpmiInstance->IpmiTransport2);

    if (mIpmiInstance->IpmiTransport2.Interface.Ssif.InterfaceState == IpmiInterfaceInitialized) {
      InterfaceState = IpmiInterfaceInitialized;
    }
  }

  if (FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) {
    InitIpmbInterfaceData (&mIpmiInstance->IpmiTransport2);
  }

  if (mIpmiInstance->IpmiTransport2.Interface.Ipmb.InterfaceState == IpmiInterfaceInitialized) {
    InterfaceState = IpmiInterfaceInitialized;
  }

  if (InterfaceState != IpmiInterfaceInitialized) {
    return EFI_SUCCESS;
  }

  // Default Interface data should be initialized to install Ipmi Transport2 Protocol.
  if (InterfaceState == IpmiInterfaceInitialized) {
    mIpmiTransport2Handle = NULL;
    Status                = gMmst->MmInstallProtocolInterface (
                                     &mIpmiTransport2Handle,
                                     &gSmmIpmiTransport2ProtocolGuid,
                                     EFI_NATIVE_INTERFACE,
                                     &mIpmiInstance->IpmiTransport2
                                     );
  }

  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}

/**

Routine Description:
  Registers Protocol call back.

Arguments:
  ProtocolGuid    - Pointer to Protocol GUID to register call back.

Returns:
  Status.

**/
EFI_STATUS
MmRegisterProtocolCallback (
  IN  EFI_GUID  *ProtocolGuid
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  if ((ProtocolGuid == NULL) ||
      ((ProtocolGuid != NULL) && IsZeroBuffer (ProtocolGuid, sizeof (EFI_GUID))))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = gMmst->MmRegisterProtocolNotify (
                    ProtocolGuid,
                    SmmNotifyCallback,
                    &Registration
                    );
  return Status;
}

EFI_STATUS
SmmInitializeIpmiKcsPhysicalLayer (
  VOID
  )

/**

Routine Description:
  Setup and initialize the BMC for the DXE phase.  In order to verify the BMC is functioning
  as expected, the BMC Selftest is performed.  The results are then checked and any errors are
  reported to the error manager.  Errors are collected throughout this routine and reported
  just prior to installing the driver.  If there are more errors than MAX_SOFT_COUNT, then they
  will be ignored.

Arguments:
  ImageHandle - Handle of this driver image
  SystemTable - Table containing standard EFI services

Returns:
  EFI_SUCCESS - Successful driver initialization

**/
{
  EFI_STATUS            Status;
  IPMI_INTERFACE_STATE  InterfaceState;
  UINT8                 Index;

  DEBUG ((DEBUG_INFO, "SmmInitializeIpmiKcsPhysicalLayer entry \n"));
  InterfaceState = IpmiInterfaceNotReady;

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (IPMI_BMC_INSTANCE_DATA),
                    (VOID **)&mIpmiInstance
                    );
  ASSERT (mIpmiInstance != NULL);
  if (mIpmiInstance == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR!! Null Pointer returned by AllocateZeroPool ()\n"));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  } else {
    //
    // Initialize the KCS transaction timeout. Assume delay unit is 1000 us.
    //
    mIpmiInstance->KcsTimeoutPeriod = (BMC_KCS_TIMEOUT * 1000*1000) / KCS_DELAY_UNIT;

    //
    // Initialize IPMI IO Base, we still use SMS IO base to get device ID and Seltest result since SMM IF may have different cmds supported
    //
    mIpmiInstance->IpmiIoBase                      = FixedPcdGet16 (PcdIpmiSmmIoBaseAddress);
    mIpmiInstance->Signature                       = SM_IPMI_BMC_SIGNATURE;
    mIpmiInstance->SlaveAddress                    = BMC_SLAVE_ADDRESS;
    mIpmiInstance->BmcStatus                       = BMC_NOTREADY;
    mIpmiInstance->IpmiTransport.IpmiSubmitCommand = IpmiSendCommand;
    mIpmiInstance->IpmiTransport.GetBmcStatus      = IpmiGetBmcStatus;

    if (FixedPcdGet8 (PcdKcsInterfaceSupport) == 1) {
      DEBUG ((DEBUG_INFO, "IPMI: Waiting for Getting BMC DID in SMM \n"));
      //
      // Get the Device ID and check if the system is in Force Update mode.
      //
      // Just obey the Spec..
      // To improve performance, we're going to comment it.
      //

      mIpmiTransportHandle = NULL;
      Status               = gMmst->MmInstallProtocolInterface (
                                      &mIpmiTransportHandle,
                                      &gSmmIpmiTransportProtocolGuid,
                                      EFI_NATIVE_INTERFACE,
                                      &mIpmiInstance->IpmiTransport
                                      );
      ASSERT_EFI_ERROR (Status);
    }

    InitIpmiTransport2 (mIpmiInstance);

    // Check interface data initialized successfully else register notify protocol.
    for (Index = SysInterfaceKcs; Index < SysInterfaceMax; Index++) {
      switch (Index) {
        case SysInterfaceKcs:
          if (FixedPcdGet8 (PcdKcsInterfaceSupport) == 1) {
            if ((mIpmiInstance->BmcStatus != BMC_HARDFAIL) && (mIpmiInstance->BmcStatus != BMC_UPDATE_IN_PROGRESS)) {
              BMC_INTERFACE_STATUS  BmcStatus;
              mIpmiInstance->IpmiTransport2.Interface.KcsInterfaceState = IpmiInterfaceInitialized;
              Status                                                    = CheckSelfTestByInterfaceType (
                                                                            &mIpmiInstance->IpmiTransport2,
                                                                            &BmcStatus,
                                                                            SysInterfaceKcs
                                                                            );
              if (!EFI_ERROR (Status) && (BmcStatus != BmcStatusHardFail)) {
                InterfaceState = IpmiInterfaceInitialized;
              } else {
                mIpmiInstance->IpmiTransport2.Interface.KcsInterfaceState = IpmiInterfaceInitError;
              }
            }
          }

          break;

        case SysInterfaceBt:
          if (FixedPcdGet8 (PcdBtInterfaceSupport) == 1) {
            if (mIpmiInstance->IpmiTransport2.Interface.Bt.InterfaceState == IpmiInterfaceInitialized) {
              InterfaceState = IpmiInterfaceInitialized;
            }
          }

          break;

        case SysInterfaceSsif:
          if (FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) {
            if (mIpmiInstance->IpmiTransport2.Interface.Ssif.InterfaceState == IpmiInterfaceInitialized) {
              InterfaceState = IpmiInterfaceInitialized;
            } else if (mIpmiInstance->IpmiTransport2.Interface.Ssif.InterfaceState == IpmiInterfaceInitError) {
              // Register protocol notify for SMBUS Protocol.
              Status = MmRegisterProtocolCallback (&mIpmiInstance->IpmiTransport2.Interface.Ssif.SsifInterfaceApiGuid);
            }
          }

          break;

        case SysInterfaceIpmb:
          if (FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) {
            if (mIpmiInstance->IpmiTransport2.Interface.Ipmb.InterfaceState == IpmiInterfaceInitialized) {
              InterfaceState = IpmiInterfaceInitialized;
            } else if (mIpmiInstance->IpmiTransport2.Interface.Ipmb.InterfaceState == IpmiInterfaceInitError) {
              // Register protocol notify for SMBUS Protocol.
              Status = MmRegisterProtocolCallback (&mIpmiInstance->IpmiTransport2.Interface.Ipmb.IpmbInterfaceApiGuid);
            }
          }

          break;

        default:
          break;
      }
    }

    // Default Interface data should be initialized to install Ipmi Transport2 Protocol.
    if (InterfaceState == IpmiInterfaceInitialized) {
      mIpmiTransport2Handle = NULL;
      Status                = gMmst->MmInstallProtocolInterface (
                                       &mIpmiTransport2Handle,
                                       &gSmmIpmiTransport2ProtocolGuid,
                                       EFI_NATIVE_INTERFACE,
                                       &mIpmiInstance->IpmiTransport2
                                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "IPMI Transport2 protocol install Status = %r \n", Status));
      }
    }

    DEBUG ((DEBUG_INFO, "SmmInitializeIpmiKcsPhysicalLayer exit \n"));

    return EFI_SUCCESS;
  }
}

/**
  The module Entry Point of driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the MM System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeGenericIpmiStandaloneMm (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
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
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  if (mIpmiInstance != NULL) {
    if (mIpmiTransportHandle != NULL) {
      Status = gMmst->MmUninstallProtocolInterface (
                        mIpmiTransportHandle,
                        &gSmmIpmiTransportProtocolGuid,
                        &mIpmiInstance->IpmiTransport
                        );
      ASSERT_EFI_ERROR (Status);
    }

    if (mIpmiTransport2Handle != NULL) {
      Status = gMmst->MmUninstallProtocolInterface (
                        mIpmiTransport2Handle,
                        &gSmmIpmiTransport2ProtocolGuid,
                        &mIpmiInstance->IpmiTransport2
                        );
      ASSERT_EFI_ERROR (Status);
    }

    gMmst->MmFreePool (mIpmiInstance);
  }

  return Status;
}
