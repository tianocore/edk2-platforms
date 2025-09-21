/** @file
  IPMI common hook functions

  @copyright
  Copyright 1999 - 2021 Intel Corporation. <BR>
  Copyright (c) 1985 - 2023, American Megatrends International LLC. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <ServerManagement.h>
#include <PeiIpmiHooks.h>

EFI_STATUS
EFIAPI
PeiIpmiSendCommand (
  IN      PEI_IPMI_TRANSPORT_PPI  *This,
  IN      UINT8                   NetFunction,
  IN      UINT8                   Lun,
  IN      UINT8                   Command,
  IN      UINT8                   *CommandData,
  IN      UINT32                  CommandDataSize,
  IN OUT  UINT8                   *ResponseData,
  IN OUT  UINT32                  *ResponseDataSize
  )

/*++

Routine Description:

  Send Ipmi Command in the right mode: HECI or KCS,  to the
  appropiate device, ME or BMC.

Arguments:

  This              - Pointer to IPMI protocol instance
  NetFunction       - Net Function of command to send
  Lun               - LUN of command to send
  Command           - IPMI command to send
  CommandData       - Pointer to command data buffer, if needed
  CommandDataSize   - Size of command data buffer
  ResponseData      - Pointer to response data buffer
  ResponseDataSize  - Pointer to response data buffer size

Returns:

  EFI_INVALID_PARAMETER - One of the input values is bad
  EFI_DEVICE_ERROR      - IPMI command failed
  EFI_BUFFER_TOO_SMALL  - Response buffer is too small
  EFI_UNSUPPORTED       - Command is not supported by BMC
  EFI_SUCCESS           - Command completed successfully

--*/
{
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // This Will be unchanged ( BMC/KCS style )
  //
  return PeiIpmiSendCommandToBmc (
                                  This,
                                  NetFunction,
                                  Lun,
                                  Command,
                                  CommandData,
                                  (UINT8)CommandDataSize,
                                  ResponseData,
                                  (UINT8 *)ResponseDataSize,
                                  NULL
                                  );
} // IpmiSendCommand()

EFI_STATUS
EFIAPI
PeiIpmiSendCommand2 (
  IN      IPMI_TRANSPORT2  *This,
  IN      UINT8            NetFunction,
  IN      UINT8            Lun,
  IN      UINT8            Command,
  IN      UINT8            *CommandData,
  IN      UINT32           CommandDataSize,
  IN OUT  UINT8            *ResponseData,
  IN OUT  UINT32           *ResponseDataSize
  )

/*++

Routine Description:

  This API use the default interface (PcdDefaultSystemInterface) to send IPMI command
  in the right mode to the appropiate device, ME or BMC.

Arguments:

  This              - Pointer to IPMI protocol instance
  NetFunction       - Net Function of command to send
  Lun               - LUN of command to send
  Command           - IPMI command to send
  CommandData       - Pointer to command data buffer, if needed
  CommandDataSize   - Size of command data buffer
  ResponseData      - Pointer to response data buffer
  ResponseDataSize  - Pointer to response data buffer size

Returns:

  EFI_INVALID_PARAMETER - One of the input values is bad
  EFI_DEVICE_ERROR      - IPMI command failed
  EFI_BUFFER_TOO_SMALL  - Response buffer is too small
  EFI_UNSUPPORTED       - Command is not supported by BMC
  EFI_SUCCESS           - Command completed successfully

--*/
{
  PEI_IPMI_BMC_INSTANCE_DATA  *PeiIpmiInstance;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PeiIpmiInstance = INSTANCE_FROM_PEI_IPMI_TRANSPORT2_THIS (This);

  if ((FixedPcdGet8 (PcdKcsInterfaceSupport) == 1) &&
      ((PeiIpmiInstance->IpmiTransport2Ppi.InterfaceType == SysInterfaceKcs) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.KcsInterfaceState == IpmiInterfaceInitialized)))
  {
    return PeiIpmiSendCommand (
                               &PeiIpmiInstance->IpmiTransportPpi,
                               NetFunction,
                               Lun,
                               Command,
                               CommandData,
                               CommandDataSize,
                               ResponseData,
                               ResponseDataSize
                               );
  }

  if ((FixedPcdGet8 (PcdBtInterfaceSupport) == 1) &&
      ((PeiIpmiInstance->IpmiTransport2Ppi.InterfaceType == SysInterfaceBt) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.Bt.InterfaceState == IpmiInterfaceInitialized)))
  {
    return IpmiBtSendCommandToBmc (
                                   &PeiIpmiInstance->IpmiTransport2Ppi,
                                   NetFunction,
                                   Lun,
                                   Command,
                                   CommandData,
                                   (UINT8)CommandDataSize,
                                   ResponseData,
                                   (UINT8 *)ResponseDataSize,
                                   NULL
                                   );
  }

  if ((FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) &&
      ((PeiIpmiInstance->IpmiTransport2Ppi.InterfaceType == SysInterfaceSsif) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.Ssif.InterfaceState == IpmiInterfaceInitialized)))
  {
    return IpmiSsifSendCommandToBmc (
                                     &PeiIpmiInstance->IpmiTransport2Ppi,
                                     NetFunction,
                                     Lun,
                                     Command,
                                     CommandData,
                                     (UINT8)CommandDataSize,
                                     ResponseData,
                                     (UINT8 *)ResponseDataSize,
                                     NULL
                                     );
  }

  if ((FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) &&
      ((PeiIpmiInstance->IpmiTransport2Ppi.InterfaceType == SysInterfaceIpmb) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.Ipmb.InterfaceState == IpmiInterfaceInitialized)))
  {
    return IpmiIpmbSendCommandToBmc (
                                     &PeiIpmiInstance->IpmiTransport2Ppi,
                                     NetFunction,
                                     Lun,
                                     Command,
                                     CommandData,
                                     (UINT8)CommandDataSize,
                                     ResponseData,
                                     (UINT8 *)ResponseDataSize,
                                     NULL
                                     );
  }

  return EFI_UNSUPPORTED;
} // IpmiSendCommand()

EFI_STATUS
EFIAPI
PeiIpmiSendCommand2Ex (
  IN      IPMI_TRANSPORT2        *This,
  IN      UINT8                  NetFunction,
  IN      UINT8                  Lun,
  IN      UINT8                  Command,
  IN      UINT8                  *CommandData,
  IN      UINT32                 CommandDataSize,
  IN OUT  UINT8                  *ResponseData,
  IN OUT  UINT32                 *ResponseDataSize,
  IN      SYSTEM_INTERFACE_TYPE  InterfaceType
  )
{
  /*++
  Routine Description:

    This API use the specific interface type to send IPMI command
    in the right mode to the appropiate device, ME or BMC.

  Arguments:

    This              - Pointer to IPMI protocol instance
    NetFunction       - Net Function of command to send
    Lun               - LUN of command to send
    Command           - IPMI command to send
    CommandData       - Pointer to command data buffer, if needed
    CommandDataSize   - Size of command data buffer
    ResponseData      - Pointer to response data buffer
    ResponseDataSize  - Pointer to response data buffer size
    InterfaceType     - BMC Interface type.

  Returns:

    EFI_INVALID_PARAMETER - One of the input values is bad
    EFI_DEVICE_ERROR      - IPMI command failed
    EFI_BUFFER_TOO_SMALL  - Response buffer is too small
    EFI_UNSUPPORTED       - Command is not supported by BMC
    EFI_SUCCESS           - Command completed successfully

  --*/

  PEI_IPMI_BMC_INSTANCE_DATA  *PeiIpmiInstance;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PeiIpmiInstance = INSTANCE_FROM_PEI_IPMI_TRANSPORT2_THIS (This);

  if ((FixedPcdGet8 (PcdKcsInterfaceSupport) == 1) &&
      ((InterfaceType == SysInterfaceKcs) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.KcsInterfaceState == IpmiInterfaceInitialized)))
  {
    return PeiIpmiSendCommand (
                               &PeiIpmiInstance->IpmiTransportPpi,
                               NetFunction,
                               Lun,
                               Command,
                               CommandData,
                               CommandDataSize,
                               ResponseData,
                               ResponseDataSize
                               );
  }

  if ((FixedPcdGet8 (PcdBtInterfaceSupport) == 1) &&
      ((InterfaceType == SysInterfaceBt) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.Bt.InterfaceState == IpmiInterfaceInitialized)))
  {
    return IpmiBtSendCommandToBmc (
                                   &PeiIpmiInstance->IpmiTransport2Ppi,
                                   NetFunction,
                                   Lun,
                                   Command,
                                   CommandData,
                                   (UINT8)CommandDataSize,
                                   ResponseData,
                                   (UINT8 *)ResponseDataSize,
                                   NULL
                                   );
  }

  if ((FixedPcdGet8 (PcdSsifInterfaceSupport) == 1) &&
      ((InterfaceType == SysInterfaceSsif) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.Ssif.InterfaceState == IpmiInterfaceInitialized)))
  {
    return IpmiSsifSendCommandToBmc (
                                     &PeiIpmiInstance->IpmiTransport2Ppi,
                                     NetFunction,
                                     Lun,
                                     Command,
                                     CommandData,
                                     (UINT8)CommandDataSize,
                                     ResponseData,
                                     (UINT8 *)ResponseDataSize,
                                     NULL
                                     );
  }

  if ((FixedPcdGet8 (PcdIpmbInterfaceSupport) == 1) &&
      ((InterfaceType == SysInterfaceIpmb) &&
       (PeiIpmiInstance->IpmiTransport2Ppi.Interface.Ipmb.InterfaceState == IpmiInterfaceInitialized)))
  {
    return IpmiIpmbSendCommandToBmc (
                                     &PeiIpmiInstance->IpmiTransport2Ppi,
                                     NetFunction,
                                     Lun,
                                     Command,
                                     CommandData,
                                     (UINT8)CommandDataSize,
                                     ResponseData,
                                     (UINT8 *)ResponseDataSize,
                                     NULL
                                     );
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
PeiIpmiBmcStatus (
  IN  PEI_IPMI_TRANSPORT_PPI  *This,
  OUT BMC_STATUS              *BmcStatus,
  OUT SM_COM_ADDRESS          *ComAddress,
  IN  VOID                    *Context
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
  if ((This == NULL) || (BmcStatus == NULL) || (ComAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return IpmiBmcStatus (
                        This,
                        BmcStatus,
                        ComAddress,
                        NULL
                        );
}
