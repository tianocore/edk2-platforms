/** @file
  Shared helpers for detecting and accessing the Simics UEFI
  PCI config device.

  Copyright (c) 2026, Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SimicsUefiDeviceLib.h>
#include <Protocol/PciRootBridgeIo.h>

static EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *mPciIo      = NULL;
static UINT64                            mConfigAddr  = 0;
static BOOLEAN                           mInitDone   = FALSE;

/**
  Locate the PCI Root Bridge I/O protocol and build the config
  address once, caching both for subsequent calls.

  @param[out] PciIo        Cached protocol pointer.
  @param[out] ConfigAddr   Cached config address.

  @retval EFI_SUCCESS      Protocol located and address built.
  @retval other            LocateProtocol failure.
**/
static
EFI_STATUS
EnsureInitialized (
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  **PciIo,
  OUT UINT64                           *ConfigAddr
  )
{
  EFI_STATUS  Status;

  if (!mInitDone) {
    Status = gBS->LocateProtocol (
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    (VOID **)&mPciIo
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SimicsUefiDeviceLib: Could not locate PciRootBridgeIo.\n"));
      return Status;
    }

    mConfigAddr = ((UINT64)PcdGet8 (PcdUefiDeviceBus)      << 24) |
                  ((UINT64)PcdGet8 (PcdUefiDeviceDevice)    << 16) |
                  ((UINT64)PcdGet8 (PcdUefiDeviceFunction)  <<  8);
    mInitDone = TRUE;
  }

  *PciIo      = mPciIo;
  *ConfigAddr = mConfigAddr;
  return EFI_SUCCESS;
}

/**
  Check for the presence of the Simics UEFI PCI config device.

  @param[out] PciRootBridgeIo  On success, the PCI Root Bridge I/O instance.
  @param[out] ConfigAddress    On success, the assembled PCI config address.

  @retval EFI_SUCCESS    Simics UEFI device found.
  @retval EFI_NOT_FOUND  Device not present or IDs do not match.
  @retval other          Protocol locate or PCI read failure.
**/
EFI_STATUS
EFIAPI
SimicsUefiDeviceCheck (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciIo;
  UINT64                           Addr;
  UINT16                           DevValues[4];

  Status = EnsureInitialized (&PciIo, &Addr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PciIo->Pci.Read (PciIo, EfiPciWidthUint16, Addr + 0x00, 1, &DevValues[0]);
  PciIo->Pci.Read (PciIo, EfiPciWidthUint16, Addr + 0x02, 1, &DevValues[1]);
  PciIo->Pci.Read (PciIo, EfiPciWidthUint16, Addr + 0x2c, 1, &DevValues[2]);
  PciIo->Pci.Read (PciIo, EfiPciWidthUint16, Addr + 0x2e, 1, &DevValues[3]);

  //
  // The Simics UEFI device is a hidden paravirtualization device.
  // It uses VID/DID = 0xFFFF ("no device present" to normal PCI
  // enumeration) but is identified by its Subsystem VID/ID pair
  // of 0x8086/0x8086.  This ensures normal OS and firmware PCI
  // probing ignores the device while Simics-aware code can still
  // find it.
  //
  if ((DevValues[0] == 0xFFFF) &&
      (DevValues[1] == 0xFFFF) &&
      (DevValues[2] == 0x8086) &&
      (DevValues[3] == 0x8086))
  {
    DEBUG ((DEBUG_INFO, "Simics UEFI device detected.\n"));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "Simics UEFI device NOT detected.\n"));
  return EFI_NOT_FOUND;
}

/**
  Read from PCI config space of the Simics UEFI device.

  @param[in]  Offset  Byte offset within PCI config space.
  @param[in]  Width   Access width in bytes (1, 2, 4, or 8).
  @param[out] Buffer  Destination buffer.

  @retval EFI_SUCCESS            Read succeeded.
  @retval EFI_INVALID_PARAMETER  Width is not 1, 2, 4, or 8.
  @retval other                  Device not found or read failed.
**/
EFI_STATUS
EFIAPI
SimicsUefiDeviceRead (
  IN  UINT64  Offset,
  IN  UINT8   Width,
  OUT VOID    *Buffer
  )
{
  EFI_STATUS                       Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciIo;
  UINT64                           Addr;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  PciWidth;

  switch (Width) {
    case 1:  PciWidth = EfiPciWidthUint8;   break;
    case 2:  PciWidth = EfiPciWidthUint16;  break;
    case 4:  PciWidth = EfiPciWidthUint32;  break;
    case 8:  PciWidth = EfiPciWidthUint64;  break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  Status = EnsureInitialized (&PciIo, &Addr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return PciIo->Pci.Read (PciIo, PciWidth, Addr + Offset, 1, Buffer);
}

/**
  Write to PCI config space of the Simics UEFI device.

  @param[in] Offset  Byte offset within PCI config space.
  @param[in] Width   Access width in bytes (1, 2, 4, or 8).
  @param[in] Buffer  Source data.

  @retval EFI_SUCCESS            Write succeeded.
  @retval EFI_INVALID_PARAMETER  Width is not 1, 2, 4, or 8.
  @retval other                  Device not found or write failed.
**/
EFI_STATUS
EFIAPI
SimicsUefiDeviceWrite (
  IN UINT64  Offset,
  IN UINT8   Width,
  IN VOID    *Buffer
  )
{
  EFI_STATUS                       Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciIo;
  UINT64                           Addr;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  PciWidth;

  switch (Width) {
    case 1:  PciWidth = EfiPciWidthUint8;   break;
    case 2:  PciWidth = EfiPciWidthUint16;  break;
    case 4:  PciWidth = EfiPciWidthUint32;  break;
    case 8:  PciWidth = EfiPciWidthUint64;  break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  Status = EnsureInitialized (&PciIo, &Addr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return PciIo->Pci.Write (PciIo, PciWidth, Addr + Offset, 1, Buffer);
}
