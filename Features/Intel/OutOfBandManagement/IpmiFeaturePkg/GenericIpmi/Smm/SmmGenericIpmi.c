/** @file
  Generic SMM IPMI stack driver

  @copyright
  Copyright 1999 - 2021 Intel Corporation. <BR>
  Copyright (c) 1985 - 2023, American Megatrends International LLC. <BR>
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
#include <Library/SmmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IpmiBaseLib.h>
#include <SmStatusCodes.h>
#include "IpmiHooks.h"
#include "IpmiBmcCommon.h"
#include "IpmiBmc.h"
#include <Library/TimerLib.h>
#include <Library/BmcCommonInterfaceLib.h>

IPMI_BMC_INSTANCE_DATA  *mIpmiInstance;
EFI_HANDLE              mImageHandle;

EFI_STATUS
GetDeviceId (
  IN      IPMI_BMC_INSTANCE_DATA  *IpmiInstance,
  IN      EFI_STATUS_CODE_VALUE   StatusCodeValue[],
  IN OUT  UINT8                   *ErrorCount
  )

/*++

Routine Description:
  Execute the Get Device ID command to determine whether or not the BMC is in Force Update
  Mode.  If it is, then report it to the error manager.

Arguments:
  IpmiInstance    - Data structure describing BMC variables and used for sending commands
  StatusCodeValue - An array used to accumulate error codes for later reporting.
  ErrorCount      - Counter used to keep track of error codes in StatusCodeValue

Returns:
  Status

--*/
{
  EFI_STATUS    Status;
  UINT32        DataSize;
  SM_CTRL_INFO  *ControllerInfo;
  UINT8         TimeOut;
  UINT8         Retries;
  UINT8         TempData[MAX_TEMP_DATA];

  TimeOut = 0;
  Retries = PcdGet8 (PcdIpmiBmcReadyDelayTimer);

  do {
    //
    // Get the device ID information for the BMC.
    //
    DataSize = MAX_TEMP_DATA;
    Status   = IpmiSendCommand (
                                &IpmiInstance->IpmiTransport,
                                IPMI_NETFN_APP,
                                0,
                                IPMI_APP_GET_DEVICE_ID,
                                NULL,
                                0,
                                TempData,
                                &DataSize
                                );
    if (Status == EFI_SUCCESS) {
      DEBUG ((DEBUG_INFO, "IPMI: SendCommand success!\n"));
      break;
    } else {
      //
      // Display message and retry.
      //
      DEBUG (
             (DEBUG_WARN | DEBUG_INFO,
              "IPMI: Waiting for BMC (KCS 0x%x)...\n",
              IpmiInstance->IpmiIoBase)
             );
      MicroSecondDelay (500 * 1000);
      TimeOut++;
    }
  } while (TimeOut < Retries);

  //
  // If there is no error then proceed to check the data returned by the BMC
  //
  if (!EFI_ERROR (Status)) {
    ControllerInfo = (SM_CTRL_INFO *)TempData;
    //
    // If the controller is in Update Mode and the maximum number of errors has not been exceeded, then
    // save the error code to the StatusCode array and increment the counter.  Set the BMC Status to indicate
    // the BMC is in force update mode.
    //
    if (ControllerInfo->UpdateMode != 0) {
      IpmiInstance->BmcStatus = BMC_UPDATE_IN_PROGRESS;
      if (*ErrorCount < MAX_SOFT_COUNT) {
        StatusCodeValue[*ErrorCount] = EFI_COMPUTING_UNIT_FIRMWARE_PROCESSOR | CU_FP_EC_FORCE_UPDATE_MODE;
        (*ErrorCount)++;
      }
    }
  }

  return Status;
}

/*++

Routine Description:
  Initialize the API and parameters for IPMI Transport2 Instance

Arguments:
  IpmiInstance    - Pointer to IPMI Instance.

Returns:
  VOID            - Nothing.

--*/
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

/*++

Routine Description:
  Notify call back to initialize the interfaces and install SMM IPMI
  protocol.

Arguments:
  Protocol    - Pointer to the protocol guid.
  Interface   - Pointer to the protocol instance.
  Handle      - Handle on which the protocol is installed.

Returns:
  Status of Notify call back.

--*/
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
    Handle = NULL;
    Status = gSmst->SmmInstallProtocolInterface (
                                                 &Handle,
                                                 &gSmmIpmiTransport2ProtocolGuid,
                                                 EFI_NATIVE_INTERFACE,
                                                 &mIpmiInstance->IpmiTransport2
                                                 );
  }
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}

/*++

Routine Description:
  Registers Protocol call back.

Arguments:
  ProtocolGuid    - Pointer to Protocol GUID to register call back.

Returns:
  Status.

--*/
EFI_STATUS
SmmRegisterProtocolCallback (
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

  Status = gSmst->SmmRegisterProtocolNotify (
                                             ProtocolGuid,
                                             SmmNotifyCallback,
                                             &Registration
                                             );
  return Status;
}

EFI_STATUS
SmmInitializeIpmiKcsPhysicalLayer (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

/*++

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

--*/
{
  EFI_STATUS             Status;
  UINT8                  ErrorCount;
  EFI_HANDLE             Handle;
  EFI_STATUS_CODE_VALUE  StatusCodeValue[MAX_SOFT_COUNT];
  IPMI_INTERFACE_STATE   InterfaceState;
  UINT8                  Index;

  DEBUG ((DEBUG_INFO, "SmmInitializeIpmiKcsPhysicalLayer entry \n"));
  ErrorCount     = 0;
  mImageHandle   = ImageHandle;
  InterfaceState = IpmiInterfaceNotReady;

  mIpmiInstance = AllocateZeroPool (sizeof (IPMI_BMC_INSTANCE_DATA));
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
    mIpmiInstance->IpmiIoBase                      = PcdGet16 (PcdIpmiSmmIoBaseAddress);
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
      // If we want to improve performance, we're going to comment it.
      //
      Status = GetDeviceId (
                            mIpmiInstance,
                            StatusCodeValue,
                            &ErrorCount
                            );
      ASSERT_EFI_ERROR (Status);
      Handle = NULL;
      Status = gSmst->SmmInstallProtocolInterface (
                                                   &Handle,
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
              Status = SmmRegisterProtocolCallback (&mIpmiInstance->IpmiTransport2.Interface.Ssif.SsifInterfaceApiGuid);
            }
          }
          break;

        case SysInterfaceIpmb:
          if (FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) {
            if (mIpmiInstance->IpmiTransport2.Interface.Ipmb.InterfaceState == IpmiInterfaceInitialized) {
              InterfaceState = IpmiInterfaceInitialized;
            } else if (mIpmiInstance->IpmiTransport2.Interface.Ipmb.InterfaceState == IpmiInterfaceInitError) {
              // Register protocol notify for SMBUS Protocol.
              Status = SmmRegisterProtocolCallback (&mIpmiInstance->IpmiTransport2.Interface.Ipmb.IpmbInterfaceApiGuid);
            }
          }
          break;

        default:
          break;
      }
    }

    // Default Interface data should be initialized to install Ipmi Transport2 Protocol.
    if (InterfaceState == IpmiInterfaceInitialized) {
      Handle = NULL;
      Status = gSmst->SmmInstallProtocolInterface (
                                                   &Handle,
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

EFI_STATUS
EFIAPI
InitializeSmmGenericIpmi (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SmmInitializeIpmiKcsPhysicalLayer (ImageHandle, SystemTable);
  return EFI_SUCCESS;
}
