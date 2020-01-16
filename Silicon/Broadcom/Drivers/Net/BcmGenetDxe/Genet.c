/** @file

  Copyright (c) 2019, Jeremy Linton All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  This driver acts like a stub to set the Broadcom
  Genet MAC address, until the actual network driver
  is in place.

  Currently, this only supports querying the MAC on a
  Raspberry Pi platform, through a VideoCore call (exposed
  through the RpiFirmware interface), but can be extented
  to other platforms MAC initialization.

**/

#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Genet.h>
#include <PiDxe.h>
#ifdef GENET_MAC_USE_RPI_INIT
#include <Protocol/RpiFirmware.h>
#endif

static
VOID
RMWRegister (
  UINT32                Offset,
  UINT32                Mask,
  UINT32                In
  )
{
  EFI_PHYSICAL_ADDRESS  Addr;
  UINT32                Data;
  UINT32                Shift;

  Addr = GENET_BASE_ADDRESS + Offset;
  Data = 0;
  Shift = 1;
  if (In) {
    while (!(Mask & Shift))
      Shift <<= 1;
    Data = (MmioRead32 (Addr) & ~Mask) | ((In * Shift) & Mask);
  } else {
    Data = MmioRead32 (Addr) & ~Mask;
  }

  MmioWrite32 (Addr, Data);

  ArmDataMemoryBarrier ();
}

STATIC
VOID
WdRegister (
  UINT32                Offset,
  UINT32                In
  )
{
  EFI_PHYSICAL_ADDRESS  Base = GENET_BASE_ADDRESS;

  MmioWrite32 (Base + Offset, In);

  ArmDataMemoryBarrier ();
}

STATIC
VOID
GenetMacInit (UINT8 *addr)
{

  // Bring the umac out of reset
  RMWRegister (GENET_SYS_RBUF_FLUSH_CTRL, 0x2, 1);
  gBS->Stall (10);
  RMWRegister (GENET_SYS_RBUF_FLUSH_CTRL, 0x2, 0);

  // Update the MAC
  WdRegister (GENET_UMAC_MAC0, (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3]);
  WdRegister (GENET_UMAC_MAC1, (addr[4] << 8) | addr[5]);
}

/**
  The entry point of Genet UEFI Driver.

  @param  ImageHandle                The image handle of the UEFI Driver.
  @param  SystemTable                A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The Driver or UEFI Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than
                                     SystemTable->Hdr.Revision.

**/
EFI_STATUS
EFIAPI
GenetEntryPoint (
  IN  EFI_HANDLE          ImageHandle,
  IN  EFI_SYSTEM_TABLE    *SystemTable
  )
{
  UINT8 MacAddr[6];

#ifdef GENET_MAC_USE_RPI_INIT
  // This part is specific to the Raspberry Pi platform, as
  // we query the dedicated VideoCore to obtain the Mac address.
  // Other platforms should add their own Mac initilialization protocol.
  RASPBERRY_PI_FIRMWARE_PROTOCOL   *mFwProtocol;
  EFI_STATUS Status;


  DEBUG ((DEBUG_INFO, "GENET:%a(): Init\n", __FUNCTION__));

  Status = gBS->LocateProtocol (&gRaspberryPiFirmwareProtocolGuid, NULL,
                  (VOID**)&mFwProtocol);
  ASSERT_EFI_ERROR(Status);

  // Get the MAC address from the firmware
  Status = mFwProtocol->GetMacAddress (MacAddr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to retrieve MAC address\n", __FUNCTION__));
    return Status;
  }
#else
  DEBUG ((DEBUG_ERROR, "%a: no method defined to retrieve MAC address\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
#endif

  // Write it to the hardware
  GenetMacInit (MacAddr);

  return EFI_SUCCESS;
}
