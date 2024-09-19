/** @file

   Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>

   SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Uefi.h>

#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PcieHotPlugLib.h>
#include <Library/PcieHotPlugPortMapLib.h>

#define END_PORT_MAP_ENTRY  0xFF

// SPM takes up to 4 arguments as value for SPCI call (Args.SpciParam1->Args.SpciParam4).
#define MAX_MSG_CMD_ARGS  4

UINT32  HandleId;

STATIC
EFI_STATUS
SpciStatusMap (
  UINTN  SpciStatus
  )
{
  switch (SpciStatus) {
    case SPCI_SUCCESS:
      return EFI_SUCCESS;

    case SPCI_NOT_SUPPORTED:
      return EFI_UNSUPPORTED;

    case SPCI_INVALID_PARAMETER:
      return EFI_INVALID_PARAMETER;

    case SPCI_NO_MEMORY:
      return EFI_OUT_OF_RESOURCES;

    case SPCI_BUSY:
    case SPCI_QUEUED:
      return EFI_NOT_READY;

    case SPCI_DENIED:
      return EFI_ACCESS_DENIED;

    case SPCI_NOT_PRESENT:
      return EFI_NOT_FOUND;

    default:
      return EFI_DEVICE_ERROR;
  }
}

EFI_STATUS
EFIAPI
SpciServiceHandleOpen (
  UINT16    ClientId,
  UINT32    *HandleId,
  EFI_GUID  Guid
  )
{
  ARM_SMC_ARGS  SmcArgs;
  EFI_STATUS    Status;
  UINT32        X1;
  UINT64        Uuid1, Uuid2, Uuid3, Uuid4;

  if (HandleId == NULL) {
    DEBUG ((DEBUG_ERROR, "%a HandleId is NULL \n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Uuid1 = Guid.Data1;
  Uuid2 = Guid.Data3 << 16 | Guid.Data2;
  Uuid3 = Guid.Data4[3] << 24 | Guid.Data4[2] << 16 | Guid.Data4[1] << 8 | Guid.Data4[0];
  Uuid4 = Guid.Data4[7] << 24 | Guid.Data4[6] << 16 | Guid.Data4[5] << 8 | Guid.Data4[4];

  SmcArgs.Arg0 = SPCI_SERVICE_HANDLE_OPEN;
  SmcArgs.Arg1 = Uuid1;
  SmcArgs.Arg2 = Uuid2;
  SmcArgs.Arg3 = Uuid3;
  SmcArgs.Arg4 = Uuid4;
  SmcArgs.Arg5 = 0;
  SmcArgs.Arg6 = 0;
  SmcArgs.Arg7 = ClientId;
  ArmCallSmc (&SmcArgs);

  Status = SpciStatusMap (SmcArgs.Arg0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  X1 = SmcArgs.Arg1;

  if ((X1 & 0x0000FFFF) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: SpciServiceHandleOpen returned X1 = 0x%08x\n",
      __func__,
      X1
      ));
    return EFI_DEVICE_ERROR;
  }

  // Combine of returned handle and clientid
  *HandleId = (UINT32)X1 | ClientId;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpciServiceHandleClose (
  UINT32  HandleId
  )
{
  ARM_SMC_ARGS  SmcArgs;
  EFI_STATUS    Status;

  SmcArgs.Arg0 = SPCI_SERVICE_HANDLE_CLOSE;
  SmcArgs.Arg1 = HandleId;
  ArmCallSmc (&SmcArgs);

  Status = SpciStatusMap (SmcArgs.Arg0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpciServiceRequestStart (
  AMPERE_SPCI_ARGS  *Args
  )
{
  ARM_SMC_ARGS  SmcArgs;
  EFI_STATUS    Status;

  if (Args == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  SmcArgs.Arg0 = SPCI_SERVICE_REQUEST_START_AARCH64;
  SmcArgs.Arg1 = Args->SpciCommand;
  SmcArgs.Arg2 = Args->SpciParam1;
  SmcArgs.Arg3 = Args->SpciParam2;
  SmcArgs.Arg4 = Args->SpciParam3;
  SmcArgs.Arg5 = Args->SpciParam4;
  SmcArgs.Arg6 = Args->SpciParam5;
  SmcArgs.Arg7 = (UINT64)Args->HandleId;
  ArmCallSmc (&SmcArgs);

  Status = SpciStatusMap (SmcArgs.Arg0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Return Token
  Args->Token = SmcArgs.Arg1;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpciServiceRequestBlocking (
  AMPERE_SPCI_ARGS  *Args
  )
{
  ARM_SMC_ARGS  SmcArgs;
  EFI_STATUS    Status;

  if (Args == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  SmcArgs.Arg0 = SPCI_SERVICE_REQUEST_BLOCKING_AARCH64;
  SmcArgs.Arg1 = Args->SpciCommand;
  SmcArgs.Arg2 = Args->SpciParam1;
  SmcArgs.Arg3 = Args->SpciParam2;
  SmcArgs.Arg4 = Args->SpciParam3;
  SmcArgs.Arg5 = Args->SpciParam4;
  SmcArgs.Arg6 = Args->SpciParam5;
  SmcArgs.Arg7 = (UINT64)Args->HandleId;
  ArmCallSmc (&SmcArgs);

  Status = SpciStatusMap (SmcArgs.Arg0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Args->SpciCommand = SmcArgs.Arg1;
  Args->SpciParam1  = SmcArgs.Arg2;
  Args->SpciParam2  = SmcArgs.Arg3;

  return EFI_SUCCESS;
}

/**
  Set GPIO pins used for PCIe reset. This command
  limits the number of GPIO[16:21] for reset purpose.
**/
VOID
PcieHotPlugSetGpioMap (
  VOID
  )
{
  AMPERE_SPCI_ARGS  Args;
  EFI_STATUS        Status;

  Args.HandleId    = HandleId;
  Args.SpciCommand = PCIE_HOT_PLUG_SPCI_CMD_GPIO_MAP;
  Args.SpciParam1  = (UINTN)PcdGet8 (PcdPcieHotPlugGpioResetMap);

  Status = SpciServiceRequestBlocking (&Args);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SPM HotPlug GPIO reset map failed. Returned: %r\n", Status));
  }
}

/**
  Lock current Portmap table.
**/
VOID
PcieHotPlugSetLockPortMap (
  VOID
  )
{
  AMPERE_SPCI_ARGS  Args;
  EFI_STATUS        Status;

  Args.HandleId    = HandleId;
  Args.SpciCommand = PCIE_HOT_PLUG_SPCI_CMD_PORT_MAP_LOCK;

  Status = SpciServiceRequestBlocking (&Args);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SPM HotPlug port map lock failed. Returned: %r\n", Status));
  }
}

/**
  Start Hot plug service.
**/
VOID
PcieHotPlugSetStart (
  VOID
  )
{
  AMPERE_SPCI_ARGS  Args;
  EFI_STATUS        Status;

  Args.HandleId    = HandleId;
  Args.SpciCommand = PCIE_HOT_PLUG_SPCI_CMD_START;

  Status = SpciServiceRequestBlocking (&Args);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SPM HotPlug start failed. Returned: %r\n", Status));
  }
}

/**
  Clear current configuration of Portmap table.
**/
VOID
PcieHotPlugSetClear (
  VOID
  )
{
  AMPERE_SPCI_ARGS  Args;
  EFI_STATUS        Status;

  Args.HandleId    = HandleId;
  Args.SpciCommand = PCIE_HOT_PLUG_SPCI_CMD_PORT_MAP_CLR;

  Status = SpciServiceRequestBlocking (&Args);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SPM HotPlug clear port map failed. Returned: %r\n", Status));
  }
}

/**
  Set configuration for Portmap table.
**/
VOID
PcieHotPlugSetPortMap (
  VOID
  )
{
  AMPERE_SPCI_ARGS  Args;
  EFI_STATUS        Status;

  PCIE_HOT_PLUG_PORT_MAP_TABLE  *PortMapTable;
  PCIE_HOT_PLUG_PORT_MAP_ENTRY  PortMapEntry;
  UINT8                         Index;
  UINT8                         PortMapEntryIndex;
  BOOLEAN                       IsEndPortMapEntry;
  UINTN                         ConfigValue;
  UINTN                         *ConfigLegacy;

  // Retrieves PCD of Portmap table.
  PortMapTable = (PCIE_HOT_PLUG_PORT_MAP_TABLE *)PcdGetPtr (PcdPcieHotPlugPortMapTable);

  //
  // Check whether specific platform configuration is used?
  // Otherwise, keep default configuration (Mt. Jade 2U).
  //
  if (!PortMapTable->UseDefaultConfig) {
    IsEndPortMapEntry = FALSE;
    PortMapEntryIndex = 0;

    // Clear old Port Map table first.
    PcieHotPlugSetClear ();

    while (!IsEndPortMapEntry) {
      ZeroMem (&Args, sizeof (Args));
      Args.HandleId    = HandleId;
      Args.SpciCommand = PCIE_HOT_PLUG_SPCI_CMD_PORT_MAP_SET;

      // Pointer will get configuration value for Args.SpciParam1->Args.SpciParam5
      ConfigLegacy = &Args.SpciParam1;

      for (Index = 0; Index < MAX_MSG_CMD_ARGS; Index++) {
        PortMapEntry  = *((PCIE_HOT_PLUG_PORT_MAP_ENTRY *)PortMapTable->PortMap[PortMapEntryIndex]);
        ConfigValue   = PCIE_HOT_PLUG_GET_CONFIG_VALUE (PortMapEntry);
        *ConfigLegacy = ConfigValue;

        if (PortMapTable->PortMap[PortMapEntryIndex][0] == END_PORT_MAP_ENTRY) {
          IsEndPortMapEntry = TRUE;
          break;
        }

        PortMapEntryIndex++;
        ConfigLegacy++;
      }

      Status = SpciServiceRequestBlocking (&Args);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "SPM HotPlug set port map failed. Returned: %r\n", Status));
      }
    }
  }
}

/**
  This function will start Hotplug service after following steps:
  - Open handle to make a SPCI call.
  - Set GPIO pins for PCIe reset.
  - Set configuration for Portmap table.
  - Lock current Portmap table.
  - Start Hot plug service.
  - Close handle.
**/
VOID
PcieHotPlugStart (
  VOID
  )
{
  EFI_STATUS  Status;

  // Open handle
  Status = SpciServiceHandleOpen (SPCI_CLIENT_ID, &HandleId, gPcieHotPlugGuid);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SPM failed to return invalid handle. Returned: %r\n",
      Status
      ));

    return;
  }

  // Set GPIO pins for PCIe reset
  PcieHotPlugSetGpioMap ();

  // Set Portmap table
  PcieHotPlugSetPortMap ();

  // Lock current Portmap table
  PcieHotPlugSetLockPortMap ();

  // Start Hot plug service
  PcieHotPlugSetStart ();

  // Close handle
  Status = SpciServiceHandleClose (HandleId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SPM HotPlug close handle failed. Returned: %r\n", Status));
  }
}
