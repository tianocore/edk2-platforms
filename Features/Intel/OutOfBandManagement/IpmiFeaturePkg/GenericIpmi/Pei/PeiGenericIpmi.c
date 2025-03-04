/** @file
  Generic IPMI stack during PEI phase

  @copyright
  Copyright 2017 - 2021 Intel Corporation. <BR>
  Copyright (c) 1985 - 2023, American Megatrends International LLC. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Ipmi.h>
#include "PeiGenericIpmi.h"
#include <Library/ReportStatusCodeLib.h>
#include <Library/IpmiPlatformHookLib.h>
#include <Library/BmcCommonInterfaceLib.h>
#include <Library/PeiServicesLib.h>

///////////////////////////////////////////////////////////////////////////////

static EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiPeiMemoryDiscoveredPpiGuid,
  UpdateIpmiInstancePtr
};

/**
    Initialize the API and parameters for IPMI Transport2 Instance

    @param[in] IpmiInstance         Pointer to IPMI Instance

    @return VOID

**/
VOID
InitIpmiTransport2 (
  IN  PEI_IPMI_BMC_INSTANCE_DATA  *IpmiInstance
  )
{
  IpmiInstance->IpmiTransport2Ppi.InterfaceType           = FixedPcdGet8 (PcdDefaultSystemInterface);
  IpmiInstance->IpmiTransport2Ppi.IpmiTransport2BmcStatus = BmcStatusOk;
  IpmiInstance->IpmiTransport2Ppi.IpmiSubmitCommand2      = PeiIpmiSendCommand2;
  IpmiInstance->IpmiTransport2Ppi.IpmiSubmitCommand2Ex    = PeiIpmiSendCommand2Ex;

  if (FixedPcdGet8 (PcdBtInterfaceSupport) == 1) {
    if (!EFI_ERROR (PlatformIpmiIoRangeSet (FixedPcdGet16 (PcdBtControlPort)))) {
      InitBtInterfaceData (&IpmiInstance->IpmiTransport2Ppi);
    }
  }

  if (FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) {
    InitSsifInterfaceData (&IpmiInstance->IpmiTransport2Ppi);
  }

  if (FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) {
    InitIpmbInterfaceData (&IpmiInstance->IpmiTransport2Ppi);
  }
}

/*++

Routine Description:
  Notify callback function for interfaces.

Arguments:
  PeiServices      - Describes the list of possible PEI Services.
  NotifyDescriptor - Pointer to notify descriptor.
  Ppi              - Pointer to Ppi.

Returns:
  Status

--*/
EFI_STATUS
EFIAPI
NotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                  Status;
  PEI_IPMI_BMC_INSTANCE_DATA  *IpmiInstance;
  PEI_IPMI_DATA_HOB           *IpmiInstancePtrHob;
  EFI_HOB_GUID_TYPE           *GuidHob;
  IPMI_INTERFACE_STATE        InterfaceState;

  InterfaceState = IpmiInterfaceNotReady;

  GuidHob = GetFirstGuidHob (&gPeiIpmiHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  IpmiInstancePtrHob = (PEI_IPMI_DATA_HOB *)GET_GUID_HOB_DATA (GuidHob);
  IpmiInstance       = (PEI_IPMI_BMC_INSTANCE_DATA *)IpmiInstancePtrHob->IpmiInstance;

  if (FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) {
    InitSsifInterfaceData (&IpmiInstance->IpmiTransport2Ppi);

    if (IpmiInstance->IpmiTransport2Ppi.Interface.Ssif.InterfaceState == IpmiInterfaceInitialized) {
      InterfaceState = IpmiInterfaceInitialized;
    }
  }

  if (FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) {
    InitIpmbInterfaceData (&IpmiInstance->IpmiTransport2Ppi);

    if (IpmiInstance->IpmiTransport2Ppi.Interface.Ipmb.InterfaceState == IpmiInterfaceInitialized) {
      InterfaceState = IpmiInterfaceInitialized;
    }
  }

  // Default Interface data should be initialized to install Ipmi Transport2 Protocol.
  if (InterfaceState != IpmiInterfaceInitialized) {
    return EFI_UNSUPPORTED;
  }

  Status = PeiServicesInstallPpi (&IpmiInstance->PeiIpmi2BmcDataDesc);
  return Status;
}

/*++

Routine Description:
  Registers callback for Ppi.

Arguments:
  PeiServices      - Describes the list of possible PEI Services.
  PpiGuid          - Pointer to Ppi guid to register call back.

Returns:
  Status

--*/
EFI_STATUS
RegisterPpiCallback (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN       EFI_GUID          *PpiGuid
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_NOTIFY_DESCRIPTOR  *PpiNotifyDesc;

  if ((PpiGuid == NULL) ||
      ((PpiGuid != NULL) && IsZeroBuffer (PpiGuid, sizeof (EFI_GUID))))
  {
    return EFI_INVALID_PARAMETER;
  }

  PpiNotifyDesc = (EFI_PEI_NOTIFY_DESCRIPTOR *)AllocateZeroPool (sizeof (EFI_PEI_NOTIFY_DESCRIPTOR));
  if (PpiNotifyDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PpiNotifyDesc->Flags  = EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  PpiNotifyDesc->Guid   = PpiGuid;
  PpiNotifyDesc->Notify = NotifyCallback;

  Status = (*PeiServices)->NotifyPpi (PeiServices, PpiNotifyDesc);
  return Status;
}

/*++

Routine Description:
  After memory is discovered, update the IPMI Instance pointer in Hob.

Arguments:
  PeiServices      - Describes the list of possible PEI Services.
  NotifyDescriptor - PPointer to notify descriptor..
  Ppi              - Pointer to Ppi.

Returns:
  Status

--*/
EFI_STATUS
EFIAPI
UpdateIpmiInstancePtr (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PEI_IPMI_BMC_INSTANCE_DATA  *IpmiInstance;
  PEI_IPMI_DATA_HOB           *IpmiInstancePtrHob;
  EFI_HOB_GUID_TYPE           *GuidHob;

  GuidHob = GetFirstGuidHob (&gPeiIpmiHobGuid);

  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  IpmiInstancePtrHob = (PEI_IPMI_DATA_HOB *)GET_GUID_HOB_DATA (GuidHob);
  IpmiInstance       = (PEI_IPMI_BMC_INSTANCE_DATA *)IpmiInstancePtrHob->IpmiInstance;

  if (!IpmiInstance) {
    return EFI_ABORTED;
  }

  DEBUG ((DEBUG_INFO, "IpmiInstance Signature in CAR Memory: %x\n", IpmiInstance->Signature));

  if ((UINTN)IpmiInstancePtrHob >= (UINTN)IpmiInstancePtrHob->PreMemIpmiDataHobPtr) {
    IpmiInstance = (PEI_IPMI_BMC_INSTANCE_DATA *)((UINT8 *)IpmiInstance + \
                                                  (UINTN)((UINTN)IpmiInstancePtrHob - (UINTN)IpmiInstancePtrHob->PreMemIpmiDataHobPtr));
  } else {
    IpmiInstance = (PEI_IPMI_BMC_INSTANCE_DATA *)((UINT8 *)IpmiInstance - \
                                                  (UINTN)((UINTN)IpmiInstancePtrHob->PreMemIpmiDataHobPtr - (UINTN)IpmiInstancePtrHob));
  }
  //
  //Migrate function pointer after module reload
  //
  IpmiInstance->IpmiTransportPpi.IpmiSubmitCommand = PeiIpmiSendCommand;
  IpmiInstance->IpmiTransportPpi.GetBmcStatus      = PeiGetIpmiBmcStatus;
  IpmiInstance->PeiIpmiBmcDataDesc.Guid  = &gPeiIpmiTransportPpiGuid;
  IpmiInstance->PeiIpmiBmcDataDesc.Ppi   = &IpmiInstance->IpmiTransportPpi;

  IpmiInstancePtrHob->IpmiInstance = (UINTN)IpmiInstance;
  DEBUG ((DEBUG_INFO, "IpmiInstance Signature after Permanent Memory discovered: %x\n", IpmiInstance->Signature));
  return EFI_SUCCESS;
}

/*****************************************************************************
 @brief
  Internal function

 @param[in] PeiServices          General purpose services available to every PEIM.

 @retval EFI_SUCCESS             Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
PeiInitializeIpmiKcsPhysicalLayer (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                  Status;
  PEI_IPMI_BMC_INSTANCE_DATA  *mIpmiInstance;
  PEI_IPMI_DATA_HOB           *IpmiInstancePtrHob;
  IPMI_INTERFACE_STATE        InterfaceState;
  UINT8                       Index;

  mIpmiInstance  = NULL;
  InterfaceState = IpmiInterfaceNotReady;

  //
  // Send Pre-Boot signal to BMC
  //
  if (PcdGetBool (PcdSignalPreBootToBmc)) {
    Status = SendPreBootSignaltoBmc (PeiServices);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Enable OEM specific southbridge SIO KCS I/O address range 0xCA0 to 0xCAF at here
  // if the the I/O address range has not been enabled.
  //
  Status = PlatformIpmiIoRangeSet (PcdGet16 (PcdIpmiIoBaseAddress));
  DEBUG ((DEBUG_INFO, "IPMI Peim:PlatformIpmiIoRangeSet - %r!\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mIpmiInstance = AllocateZeroPool (sizeof (PEI_IPMI_BMC_INSTANCE_DATA));
  if (mIpmiInstance == NULL) {
    DEBUG ((DEBUG_ERROR, "IPMI Peim:EFI_OUT_OF_RESOURCES of memory allocation\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if ((FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) || (FixedPcdGet8 (PcdSsifInterfaceSupport) == 1)) {
    // Create Guided hob to pass IPMI Instance data pointer to notify functions.
    IpmiInstancePtrHob = BuildGuidHob (&gPeiIpmiHobGuid, sizeof (PEI_IPMI_DATA_HOB));
    if (IpmiInstancePtrHob == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed to create Hob guid for IPMI Instance!!!\n"));
      FreePool (mIpmiInstance);
      return EFI_OUT_OF_RESOURCES;
    }

    IpmiInstancePtrHob->IpmiInstance         = (UINTN)mIpmiInstance;
    IpmiInstancePtrHob->PreMemIpmiDataHobPtr = IpmiInstancePtrHob;

    // Check for Memory initialization
    Status = (*PeiServices)->LocatePpi (
                                        PeiServices,
                                        &gEfiPeiMemoryDiscoveredPpiGuid,
                                        0,
                                        NULL,
                                        NULL
                                        );

    if (EFI_ERROR (Status)) {
      /* Register Memory discovered PPI call back to update HOB with new
      IpmiInstance pointer.*/
      Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyList);
      if (EFI_ERROR (Status)) {
        FreePool (mIpmiInstance);
        return Status;
      }
    }
  }

  //
  // Calibrate TSC Counter.  Stall for 10ms, then multiply the resulting number of
  // ticks in that period by 100 to get the number of ticks in a 1 second timeout
  //
  DEBUG ((DEBUG_INFO, "IPMI Peim:IPMI STACK Initialization\n"));
  mIpmiInstance->KcsTimeoutPeriod = (BMC_KCS_TIMEOUT_PEI *1000*1000) / KCS_DELAY_UNIT_PEI;
  DEBUG ((DEBUG_INFO, "IPMI Peim:KcsTimeoutPeriod = 0x%x\n", mIpmiInstance->KcsTimeoutPeriod));

  //
  // Initialize IPMI IO Base.
  //
  mIpmiInstance->IpmiIoBase = PcdGet16 (PcdIpmiIoBaseAddress);
  DEBUG ((DEBUG_INFO, "IPMI Peim:IpmiIoBase=0x%x\n", mIpmiInstance->IpmiIoBase));
  mIpmiInstance->Signature                          = SM_IPMI_BMC_SIGNATURE;
  mIpmiInstance->SlaveAddress                       = BMC_SLAVE_ADDRESS;
  mIpmiInstance->BmcStatus                          = BMC_NOTREADY;
  mIpmiInstance->IpmiTransportPpi.IpmiSubmitCommand = PeiIpmiSendCommand;
  mIpmiInstance->IpmiTransportPpi.GetBmcStatus      = PeiGetIpmiBmcStatus;

  mIpmiInstance->PeiIpmiBmcDataDesc.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  mIpmiInstance->PeiIpmiBmcDataDesc.Guid  = &gPeiIpmiTransportPpiGuid;
  mIpmiInstance->PeiIpmiBmcDataDesc.Ppi   = &mIpmiInstance->IpmiTransportPpi;

  if (FixedPcdGet8 (PcdKcsInterfaceSupport) == 1) {
    //
    // Get the Device ID and check if the system is in Force Update mode.
    //
    Status = GetDeviceId (mIpmiInstance);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "IPMI Peim:Get BMC Device Id Failed. Status=%r\n", Status));
    }

    //
    // Do not continue initialization if the BMC is in Force Update Mode.
    //
    if ((mIpmiInstance->BmcStatus != BMC_UPDATE_IN_PROGRESS) && (mIpmiInstance->BmcStatus != BMC_HARDFAIL)) {
      Status = PeiServicesInstallPpi (&mIpmiInstance->PeiIpmiBmcDataDesc);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  InitIpmiTransport2 (mIpmiInstance);

  // Check interface data initialized successfully else register notify protocol.
  for (Index = SysInterfaceKcs; Index < SysInterfaceMax; Index++) {
    switch (Index) {
      case SysInterfaceKcs:
        if (FixedPcdGet8 (PcdKcsInterfaceSupport) == 1) {
          if ((mIpmiInstance->BmcStatus != BMC_HARDFAIL) && (mIpmiInstance->BmcStatus != BMC_UPDATE_IN_PROGRESS)) {
            BMC_INTERFACE_STATUS  BmcStatus;
            mIpmiInstance->IpmiTransport2Ppi.Interface.KcsInterfaceState = IpmiInterfaceInitialized;
            Status                                                       = CheckSelfTestByInterfaceType (
                                                                                                         &mIpmiInstance->IpmiTransport2Ppi,
                                                                                                         &BmcStatus,
                                                                                                         SysInterfaceKcs
                                                                                                         );
            if (!EFI_ERROR (Status) && (BmcStatus != BmcStatusHardFail)) {
              InterfaceState = IpmiInterfaceInitialized;
            } else {
              mIpmiInstance->IpmiTransport2Ppi.Interface.KcsInterfaceState = IpmiInterfaceInitError;
            }
          }
        }
        break;

      case SysInterfaceBt:
        if (FixedPcdGet8 (PcdBtInterfaceSupport) == 1) {
          if (mIpmiInstance->IpmiTransport2Ppi.Interface.Bt.InterfaceState == IpmiInterfaceInitialized) {
            InterfaceState = IpmiInterfaceInitialized;
          }
        }
        break;

      case SysInterfaceSsif:
        if (FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) {
          if (mIpmiInstance->IpmiTransport2Ppi.Interface.Ssif.InterfaceState == IpmiInterfaceInitialized) {
            InterfaceState = IpmiInterfaceInitialized;
          } else if (mIpmiInstance->IpmiTransport2Ppi.Interface.Ssif.InterfaceState == IpmiInterfaceInitError) {
            // Register protocol notify for SMBUS Protocol.
            Status = RegisterPpiCallback (PeiServices, &mIpmiInstance->IpmiTransport2Ppi.Interface.Ssif.SsifInterfaceApiGuid);
          }
        }
        break;

      case SysInterfaceIpmb:
        if (FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) {
          if (mIpmiInstance->IpmiTransport2Ppi.Interface.Ipmb.InterfaceState == IpmiInterfaceInitialized) {
            InterfaceState = IpmiInterfaceInitialized;
          } else if (mIpmiInstance->IpmiTransport2Ppi.Interface.Ipmb.InterfaceState == IpmiInterfaceInitError) {
            // Register protocol notify for SMBUS Protocol.
            Status = RegisterPpiCallback (PeiServices, &mIpmiInstance->IpmiTransport2Ppi.Interface.Ipmb.IpmbInterfaceApiGuid);
          }
        }
        break;

      default:
        break;
    }
  }

  // Any one of the Interface data should be initialized to install Ipmi Transport2 Protocol.
  if (InterfaceState != IpmiInterfaceInitialized) {
    DEBUG ((DEBUG_INFO, "Interface not ready yet. \n"));
    return EFI_SUCCESS;
  }

  mIpmiInstance->PeiIpmi2BmcDataDesc.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  mIpmiInstance->PeiIpmi2BmcDataDesc.Guid  = &gPeiIpmiTransport2PpiGuid;
  mIpmiInstance->PeiIpmi2BmcDataDesc.Ppi   = &mIpmiInstance->IpmiTransport2Ppi;

  Status = PeiServicesInstallPpi (&mIpmiInstance->PeiIpmi2BmcDataDesc);
  return Status;
}

/*****************************************************************************
 @bref
  PRE-BOOT signal will be sent in very early PEI phase, to enable necessary KCS access for host boot.

  @param[in] PeiServices          General purpose services available to every PEIM.

  @retval EFI_SUCCESS   Indicates that the signal is sent successfully.
**/
EFI_STATUS
SendPreBootSignaltoBmc (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS          Status;
  EFI_PEI_CPU_IO_PPI  *CpuIoPpi;
  UINT32              ProvisionPort = 0;
  UINT8               PreBoot       = 0;

  //
  // Locate CpuIo service
  //
  CpuIoPpi      = (**PeiServices).CpuIo;
  ProvisionPort = PcdGet32 (PcdSioMailboxBaseAddress) + MBXDAT_B;
  PreBoot       = 0x01;// PRE-BOOT

  Status = CpuIoPpi->Io.Write (
                               PeiServices,
                               CpuIoPpi,
                               EfiPeiCpuIoWidthUint8,
                               ProvisionPort,
                               1,
                               &PreBoot
                               );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SendPreBootSignaltoBmc () Write PRE-BOOT Status=%r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/*****************************************************************************
 @bref
  The entry point of the Ipmi PEIM. Instals Ipmi PPI interface.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   Indicates that Ipmi initialization completed successfully.
**/
EFI_STATUS
PeimIpmiInterfaceInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  //
  //Register this moudule for Shadow.
  //
  Status = PeiServicesRegisterForShadow (FileHandle);
  DEBUG ((DEBUG_INFO, "%a %d register for Shwdow Result %r \n", __FUNCTION__, __LINE__,Status));

  //
  // Performing Ipmi KCS physical layer initialization
  //
  Status = PeiInitializeIpmiKcsPhysicalLayer (PeiServices);

  return Status;
} // PeimIpmiInterfaceInit()

EFI_STATUS
PeiGetIpmiBmcStatus (
  IN      PEI_IPMI_TRANSPORT_PPI  *This,
  OUT BMC_STATUS                  *BmcStatus,
  OUT SM_COM_ADDRESS              *ComAddress
  )

/*++

Routine Description:

  Updates the BMC status and returns the Com Address

Arguments:

  This        - Pointer to IPMI protocol instance
  BmcStatus   - BMC status
  ComAddress  - Com Address

Returns:

  EFI_SUCCESS - Success

--*/
{
  return PeiIpmiBmcStatus (
                           This,
                           BmcStatus,
                           ComAddress,
                           NULL
                           );
}

EFI_STATUS
GetDeviceId (
  IN      PEI_IPMI_BMC_INSTANCE_DATA  *mIpmiInstance
  )

/*++

Routine Description:
  Execute the Get Device ID command to determine whether or not the BMC is in Force Update
  Mode.  If it is, then report it to the error manager.

Arguments:
  mIpmiInstance   - Data structure describing BMC variables and used for sending commands
  StatusCodeValue - An array used to accumulate error codes for later reporting.
  ErrorCount      - Counter used to keep track of error codes in StatusCodeValue

Returns:
  Status

--*/
{
  EFI_STATUS    Status;
  UINT32        DataSize;
  SM_CTRL_INFO  *pBmcInfo;
  UINTN         Retries;
  UINT8         TempData[MAX_TEMP_DATA];

  //
  // Set up a loop to retry for up to PcdIpmiBmcReadyDelayTimer seconds. Calculate retries not timeout
  // so that in case KCS is not enabled and IpmiSendCommand() returns
  // immediately we will not wait all the PcdIpmiBmcReadyDelayTimer seconds.
  //
  Retries = PcdGet8 (PcdIpmiBmcReadyDelayTimer);
  //
  // Get the device ID information for the BMC.
  //
  DataSize = sizeof (TempData);
  while (EFI_ERROR (
                    Status = PeiIpmiSendCommand (
                                                 &mIpmiInstance->IpmiTransportPpi,
                                                 IPMI_NETFN_APP,
                                                 0,
                                                 IPMI_APP_GET_DEVICE_ID,
                                                 NULL,
                                                 0,
                                                 TempData,
                                                 &DataSize
                                                 )
                    ))
  {
    DEBUG (
           (DEBUG_WARN, "[IPMI] BMC does not respond (status: %r), %d retries left\n",
            Status, Retries)
           );

    if (Retries-- == 0) {
      ReportStatusCode (EFI_ERROR_CODE | EFI_ERROR_MAJOR, EFI_COMPUTING_UNIT_FIRMWARE_PROCESSOR | EFI_CU_FP_EC_COMM_ERROR);
      mIpmiInstance->BmcStatus = BMC_HARDFAIL;
      return Status;
    }

    //
    // Handle the case that BMC FW still not enable KCS channel after AC cycle. just stall 1 second
    //
    MicroSecondDelay (1*1000*1000);
  }

  pBmcInfo = (SM_CTRL_INFO *)&TempData[0];
  DEBUG (
         (DEBUG_INFO, "[IPMI PEI] BMC Device ID: 0x%02X, firmware version: %d.%02X UpdateMode:%x\n",
          pBmcInfo->DeviceId, pBmcInfo->MajorFirmwareRev, pBmcInfo->MinorFirmwareRev, pBmcInfo->UpdateMode)
         );
  //
  // In OpenBMC, UpdateMode: the bit 7 of byte 4 in get device id command is used for the BMC status:
  // 0 means BMC is ready, 1 means BMC is not ready.
  // At the very beginning of BMC power on, the status is 1 means BMC is in booting process and not ready. It is not the flag for force update mode.
  //
  if (pBmcInfo->UpdateMode == BMC_READY) {
    mIpmiInstance->BmcStatus = BMC_OK;
    return EFI_SUCCESS;
  } else {
    //
    // Updatemode = 1 mean BMC is not ready, continue waiting.
    //
    while (Retries-- != 0) {
      MicroSecondDelay (1*1000*1000); // delay 1 seconds
      DEBUG ((DEBUG_INFO, "[IPMI PEI] UpdateMode Retries:%x \n", Retries));
      Status = PeiIpmiSendCommand (
                                   &mIpmiInstance->IpmiTransportPpi,
                                   IPMI_NETFN_APP,
                                   0,
                                   IPMI_APP_GET_DEVICE_ID,
                                   NULL,
                                   0,
                                   TempData,
                                   &DataSize
                                   );
      if (!EFI_ERROR (Status)) {
        pBmcInfo = (SM_CTRL_INFO *)&TempData[0];
        DEBUG ((DEBUG_INFO, "[IPMI PEI] UpdateMode Retries:%x   pBmcInfo->UpdateMode:%x\n", Retries, pBmcInfo->UpdateMode));
        if (pBmcInfo->UpdateMode == BMC_READY) {
          mIpmiInstance->BmcStatus = BMC_OK;
          return EFI_SUCCESS;
        }
      }
    }
  }

  mIpmiInstance->BmcStatus = BMC_HARDFAIL;
  return Status;
} // GetDeviceId()
