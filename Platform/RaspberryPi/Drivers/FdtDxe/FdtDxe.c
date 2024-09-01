/** @file
 *
 *  Copyright (c) 2017, Andrey Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) 2016, Linaro, Ltd. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <libfdt.h>
#include <Protocol/RpiFirmware.h>
#include <Guid/Fdt.h>
#include <ConfigVars.h>

STATIC VOID                             *mFdtImage;

STATIC RASPBERRY_PI_FIRMWARE_PROTOCOL   *mFwProtocol;

STATIC
EFI_STATUS
FixEthernetAliases (
  VOID
)
{
  INTN          Aliases;
  CONST CHAR8   *Ethernet;
  CONST CHAR8   *Ethernet0;
  CONST CHAR8   *Alias;
  UINTN         CopySize;
  CHAR8         *Copy;
  INTN          Retval;
  EFI_STATUS    Status;

  //
  // Look up the 'ethernet[0]' aliases
  //
  Aliases = fdt_path_offset (mFdtImage, "/aliases");
  if (Aliases < 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate '/aliases'\n", __func__));
    return EFI_NOT_FOUND;
  }
  Ethernet = fdt_getprop (mFdtImage, Aliases, "ethernet", NULL);
  Ethernet0 = fdt_getprop (mFdtImage, Aliases, "ethernet0", NULL);
  Alias = Ethernet ? Ethernet : Ethernet0;
  if (!Alias) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate 'ethernet[0]' alias\n", __func__));
    return EFI_NOT_FOUND;
  }

  //
  // Create copy for fdt_setprop
  //
  CopySize = AsciiStrSize (Alias);
  Copy = AllocateCopyPool (CopySize, Alias);
  if (!Copy) {
    DEBUG ((DEBUG_ERROR, "%a: failed to copy '%a'\n", __func__, Alias));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create missing aliases
  //
  Status = EFI_SUCCESS;
  if (!Ethernet) {
    Retval = fdt_setprop (mFdtImage, Aliases, "ethernet", Copy, CopySize);
    if (Retval != 0) {
      Status = EFI_NOT_FOUND;
      DEBUG ((DEBUG_ERROR, "%a: failed to create 'ethernet' alias (%d)\n",
        __func__, Retval));
    }
    DEBUG ((DEBUG_INFO, "%a: created 'ethernet' alias '%a'\n", __func__, Copy));
  }
  if (!Ethernet0) {
    Retval = fdt_setprop (mFdtImage, Aliases, "ethernet0", Copy, CopySize);
    if (Retval != 0) {
      Status = EFI_NOT_FOUND;
      DEBUG ((DEBUG_ERROR, "%a: failed to create 'ethernet0' alias (%d)\n",
        __func__, Retval));
    }
    DEBUG ((DEBUG_INFO, "%a: created 'ethernet0' alias '%a'\n", __func__, Copy));
  }

  FreePool (Copy);
  return Status;
}

STATIC
EFI_STATUS
UpdateMacAddress (
  VOID
  )
{
  INTN          Node;
  INTN          Retval;
  EFI_STATUS    Status;
  UINT8         MacAddress[6];

  //
  // Locate the node that the 'ethernet' alias refers to
  //
  Node = fdt_path_offset (mFdtImage, "ethernet");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate 'ethernet' alias\n", __func__));
    return EFI_NOT_FOUND;
  }

  //
  // Get the MAC address from the firmware
  //
  Status = mFwProtocol->GetMacAddress (MacAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to retrieve MAC address\n", __func__));
    return Status;
  }

  Retval = fdt_setprop (mFdtImage, Node, "mac-address", MacAddress,
    sizeof MacAddress);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create 'mac-address' property (%d)\n",
      __func__, Retval));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "%a: setting MAC address to %02x:%02x:%02x:%02x:%02x:%02x\n",
    __func__, MacAddress[0], MacAddress[1], MacAddress[2], MacAddress[3],
    MacAddress[4], MacAddress[5]));
  return EFI_SUCCESS;
}

//
// Add "bcm2835-usb" to the USB compatible property list, if not present.
// Required because some Linux kernels can't handle USB devices otherwise.
//
STATIC
EFI_STATUS
AddUsbCompatibleProperty (
  VOID
  )
{
  CONST CHAR8   Prop[]    = "brcm,bcm2708-usb";
  CONST CHAR8   NewProp[] = "brcm,bcm2835-usb";
  CONST CHAR8   *List;
  CHAR8         *NewList;
  INT32         ListSize;
  INTN          Node;
  INTN          Retval;

  // Locate the node that the 'usb' alias refers to
  Node = fdt_path_offset (mFdtImage, "usb");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate 'usb' alias\n", __func__));
    return EFI_NOT_FOUND;
  }

  // Get the property list. This is a list of NUL terminated strings.
  List = fdt_getprop (mFdtImage, Node, "compatible", &ListSize);
  if (List == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate properties\n", __func__));
    return EFI_NOT_FOUND;
  }

  // Check if the compatible value we plan to add is already present
  if (fdt_stringlist_contains (List, ListSize, NewProp)) {
    DEBUG ((DEBUG_INFO, "%a: property '%a' is already set.\n",
      __func__, NewProp));
    return EFI_SUCCESS;
  }

  // Make sure the compatible device is what we expect
  if (!fdt_stringlist_contains (List, ListSize, Prop)) {
    DEBUG ((DEBUG_ERROR, "%a: property '%a' is missing!\n",
      __func__, Prop));
    return EFI_NOT_FOUND;
  }

  // Add the new NUL terminated entry to our list
  DEBUG ((DEBUG_INFO, "%a: adding '%a' to the properties\n",
    __func__, NewProp));

  NewList = AllocatePool (ListSize + sizeof (NewProp));
  if (NewList == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;;
  }
  CopyMem (NewList, List, ListSize);
  CopyMem (&NewList[ListSize], NewProp, sizeof (NewProp));

  Retval = fdt_setprop (mFdtImage, Node, "compatible", NewList,
             ListSize + sizeof (NewProp));
  FreePool (NewList);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to update properties (%d)\n",
      __func__, Retval));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CleanMemoryNodes (
  VOID
  )
{
  INTN Node;
  INT32 Retval;

  Node = fdt_path_offset (mFdtImage, "/memory");
  if (Node < 0) {
    return EFI_SUCCESS;
  }

  /*
   * Remove bogus memory nodes which can make the booted
   * OS go crazy and ignore the UEFI map.
   */
  DEBUG ((DEBUG_INFO, "Removing bogus /memory\n"));
  Retval = fdt_del_node (mFdtImage, Node);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "Failed to remove /memory\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SanitizePSCI (
  VOID
  )
{
  INTN Node;
  INTN Root;
  INT32 Retval;

  Root = fdt_path_offset (mFdtImage, "/");
  ASSERT (Root >= 0);
  if (Root < 0) {
    return EFI_NOT_FOUND;
  }

  Node = fdt_path_offset (mFdtImage, "/psci");
  if (Node < 0) {
    Node = fdt_add_subnode (mFdtImage, Root, "psci");
  }

  ASSERT (Node >= 0);
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "Couldn't find/create /psci\n"));
    return EFI_NOT_FOUND;
  }

  Retval = fdt_setprop_string (mFdtImage, Node, "compatible", "arm,psci-1.0");
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "Couldn't set /psci compatible property\n"));
    return EFI_NOT_FOUND;
  }

  Retval = fdt_setprop_string (mFdtImage, Node, "method", "smc");
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "Couldn't set /psci method property\n"));
    return EFI_NOT_FOUND;
  }

  Root = fdt_path_offset (mFdtImage, "/cpus");
  if (Root < 0) {
    DEBUG ((DEBUG_ERROR, "No CPUs to update with PSCI enable-method?\n"));
    return EFI_NOT_FOUND;
  }

  Node = fdt_first_subnode (mFdtImage, Root);
  while (Node >= 0) {
    if (fdt_setprop_string (mFdtImage, Node, "enable-method", "psci") != 0) {
      DEBUG ((DEBUG_ERROR, "Failed to update enable-method for a CPU\n"));
      return EFI_NOT_FOUND;
    }

    fdt_delprop (mFdtImage, Node, "cpu-release-addr");
    Node = fdt_next_subnode (mFdtImage, Node);
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CleanSimpleFramebuffer (
  VOID
  )
{
  INTN Node;
  INT32 Retval;

  /*
   * Should look for nodes by kind and remove aliases
   * by matching against device.
   */
  Node = fdt_path_offset (mFdtImage, "display0");
  if (Node < 0) {
    return EFI_SUCCESS;
  }

  /*
   * Remove bogus GPU-injected simple-framebuffer, which
   * doesn't reflect the framebuffer built by UEFI.
   */
  DEBUG ((DEBUG_INFO, "Removing bogus display0\n"));
  Retval = fdt_del_node (mFdtImage, Node);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "Failed to remove display0\n"));
    return EFI_NOT_FOUND;
  }

  Node = fdt_path_offset (mFdtImage, "/aliases");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "Couldn't find /aliases to remove display0\n"));
    return EFI_NOT_FOUND;
  }

  Retval = fdt_delprop (mFdtImage, Node, "display0");
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "Failed to remove display0 alias\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SyncPcie (
  VOID
  )
{
#if (RPI_MODEL == 4)
  INTN          Node;
  INTN          Retval;
  UINT32        DmaRanges[7];

  Node = fdt_path_offset (mFdtImage, "pcie0");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate 'pcie0' alias\n", __func__));
    return EFI_NOT_FOUND;
  }

  // non translated 32-bit DMA window with a limit of 0xc0000000
  DmaRanges[0] = cpu_to_fdt32 (0x02000000);
  DmaRanges[1] = cpu_to_fdt32 (0x00000000);
  DmaRanges[2] = cpu_to_fdt32 (0x00000000);
  DmaRanges[3] = cpu_to_fdt32 (0x00000000);
  DmaRanges[4] = cpu_to_fdt32 (0x00000000);
  DmaRanges[5] = cpu_to_fdt32 (0x00000000);
  DmaRanges[6] = cpu_to_fdt32 (0xc0000000);

  DEBUG ((DEBUG_INFO, "%a: Updating PCIe dma-ranges\n",  __func__));

  /*
   * Match dma-ranges with the EDK2+ACPI setup we are using.  This works
   * around a failure in Linux and OpenBSD to reset the PCIe/XHCI correctly
   * when in DT mode.
   */
  Retval = fdt_setprop (mFdtImage, Node, "dma-ranges",
                        DmaRanges,  sizeof DmaRanges);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate PCIe 'dma-ranges' property (%d)\n",
      __func__, Retval));
    return EFI_NOT_FOUND;
  }

  // move the MMIO window too
  DmaRanges[0] = cpu_to_fdt32 (0x02000000); // non prefetchable 32-bit
  DmaRanges[1] = cpu_to_fdt32 (FixedPcdGet64 (PcdBcm27xxPciBusMmioAdr) >> 32); // bus addr @ 0x0f8000000
  DmaRanges[2] = cpu_to_fdt32 (FixedPcdGet64 (PcdBcm27xxPciBusMmioAdr) & MAX_UINT32);
  DmaRanges[3] = cpu_to_fdt32 (FixedPcdGet64 (PcdBcm27xxPciCpuMmioAdr) >> 32); // cpu addr @ 0x600000000
  DmaRanges[4] = cpu_to_fdt32 (FixedPcdGet64 (PcdBcm27xxPciCpuMmioAdr) & MAX_UINT32);
  DmaRanges[5] = cpu_to_fdt32 (0x00000000);
  DmaRanges[6] = cpu_to_fdt32 (FixedPcdGet32 (PcdBcm27xxPciBusMmioLen) + 1); // len = 0x4000 0000

  DEBUG ((DEBUG_INFO, "%a: Updating PCIe ranges\n",  __func__));

  /*
   * Match ranges (BAR/MMIO) with the EDK2+ACPI setup we are using.
   */
  Retval = fdt_setprop (mFdtImage, Node, "ranges",
                        DmaRanges,  sizeof DmaRanges);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate PCIe MMIO 'ranges' property (%d)\n",
      __func__, Retval));
    return EFI_NOT_FOUND;
  }

  if (PcdGet32 (PcdXhciReload) != 1) {
    return EFI_SUCCESS;
  }

  /*
   * Now that we are always running without DMA translation, and with a 3G
   * limit, there shouldn't be a need to reset/reload the XHCI. The
   * possible problem is that the PCIe root port is itself being reset (by
   * Linux+DT). The RPi foundation claims this is needed as a pre-req to
   * reloading the XHCI firmware, which also implies that a PCI fundamental
   * reset should cause the XHCI itself to reset.  This isn't happening
   * fully, otherwise reloading the firmware would be mandatory. As it is,
   * recent kernels actually start to have problems following the XHCI
   * reset notification mailbox!  Instead lets stop the kernel from
   * triggering the mailbox by removing the node.
   */

  Node = fdt_path_offset (mFdtImage, "/scb/pcie@7d500000/pci");
  if (Node < 0) {
    // This can happen on CM4/etc which doesn't have an onboard XHCI
    DEBUG ((DEBUG_INFO, "%a: failed to locate /scb/pcie@7d500000/pci\n", __func__));
  } else {
    Retval = fdt_del_node (mFdtImage, Node);
    if (Retval != 0) {
      DEBUG ((DEBUG_ERROR, "Failed to remove /scb/pcie@7d500000/pci\n"));
      return EFI_NOT_FOUND;
    }
  }

#endif
  return EFI_SUCCESS;
}

/**
  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
FdtDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  INT32      Retval;
  EFI_STATUS Status;
  UINTN      FdtSize;
  VOID       *FdtImage = NULL;

  if (PcdGet32 (PcdSystemTableMode) != SYSTEM_TABLE_MODE_BOTH &&
      PcdGet32 (PcdSystemTableMode) != SYSTEM_TABLE_MODE_DT) {
    DEBUG ((DEBUG_INFO, "Device Tree disabled per user configuration\n"));
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gRaspberryPiFirmwareProtocolGuid, NULL,
                  (VOID**)&mFwProtocol);
  ASSERT_EFI_ERROR (Status);

  FdtImage = (VOID*)(UINTN)PcdGet32 (PcdFdtBaseAddress);
  Retval = fdt_check_header (FdtImage);
  if (Retval != 0) {
    /*
     * Any one of:
     * - Invalid config.txt device_tree_address (not PcdFdtBaseAddress)
     * - Missing FDT for your Pi variant (if not overriding via device_tree=)
     */
    DEBUG ((DEBUG_ERROR, "No devicetree passed via config.txt\n"));
    return EFI_NOT_FOUND;
  }

  FdtSize = fdt_totalsize (FdtImage);
  DEBUG ((DEBUG_INFO, "Devicetree passed via config.txt (0x%lx bytes)\n", FdtSize));

  /*
   * Probably overkill.
   */
  FdtSize += EFI_PAGE_SIZE * 2;
  Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (FdtSize), (EFI_PHYSICAL_ADDRESS*)&mFdtImage);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate devicetree: %r\n", Status));
    goto out;
  }

  Retval = fdt_open_into (FdtImage, mFdtImage, FdtSize);
  if (Retval != 0) {
     DEBUG ((DEBUG_ERROR, "fdt_open_into failed: %d\n", Retval));
     goto out;
  }

  /*
   * These are all best-effort.
   */

  Status = SanitizePSCI ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to sanitize PSCI: %r\n", Status);
  }

  Status = CleanMemoryNodes ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to clean memory nodes: %r\n", Status);
  }

  Status = CleanSimpleFramebuffer ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to clean frame buffer: %r\n", Status);
  }

  Status = FixEthernetAliases ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to fix ethernet aliases: %r\n", Status);
  }

  Status = UpdateMacAddress ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to update MAC address: %r\n", Status);
  }

  Status = AddUsbCompatibleProperty ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to update USB compatible properties: %r\n", Status);
  }

  SyncPcie ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to update PCIe address ranges: %r\n", Status);
  }

  DEBUG ((DEBUG_INFO, "Installed devicetree at address %p\n", mFdtImage));
  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, mFdtImage);
  if (EFI_ERROR (Status)) {
     DEBUG ((DEBUG_ERROR, "Couldn't register devicetree: %r\n", Status));
     goto out;
  }

out:
  if (EFI_ERROR(Status)) {
    if (mFdtImage != NULL) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS) mFdtImage, EFI_SIZE_TO_PAGES (FdtSize));
    }
  }
  return Status;
}
