/** @file
    PCI Host Bridge Library instance for pci-host-ecam-generic
    compatible RC implementations.

    Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH        AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevicePath;
} MY_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

MY_PCI_ROOT_BRIDGE_DEVICE_PATH  mRootBridgeDevicePathTemplate = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8)(sizeof (ACPI_HID_DEVICE_PATH) >> 8)
      }
    },
    EISA_PNP_ID (0x0A08), // PCI Express
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16  *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

//
// They depend on the 'reg' property of the default DT provided by qemu
//
#define GENERIC_ECAM_CONFIGURATION_BASE        0x30000000
#define GENERIC_ECAM_CONFIGURATION_SIZE        0x10000000

/**
  Apply EFI_MEMORY_UC attributees to the range [Base, Base + Size).

  @param  Base                  Range base.
  @param  Size                  Range size.

  @return EFI_STATUS            EFI_SUCCESS or others.
**/
STATIC
EFI_STATUS
EFIAPI
MapGcdMmioSpace (
  IN    UINT64  Base,
  IN    UINT64  Size
  )
{
  EFI_STATUS  Status;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  Base,
                  Size,
                  EFI_MEMORY_UC
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to add GCD memory space for region [0x%Lx+0x%Lx)\n",
      __func__,
      Base,
      Size
      ));
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (Base, Size, EFI_MEMORY_UC);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to set memory space attributes for region [0x%Lx+0x%Lx)\n",
      __func__,
      Base,
      Size
      ));
  }

  return Status;
}

/**
  Process a PCI_ROOT_BRIDGE by the default Device Tree.

  @param  DtIo                  For a pci-host-ecam-generic node.
  @param  Bridge                PCI_ROOT_BRIDGE to fill.

  @return EFI_STATUS            EFI_SUCCESS or others.
**/
STATIC
EFI_STATUS
EFIAPI
ProcessPciHost (
  IN  PCI_ROOT_BRIDGE     *Bridge
  )
{
  UINT64                    ConfigBase;
  UINT64                    ConfigSize;
  EFI_STATUS                Status;
  UINT32                    BusMin;
  UINT32                    BusMax;
  PCI_ROOT_BRIDGE_APERTURE  Io;
  PCI_ROOT_BRIDGE_APERTURE  Mem;
  PCI_ROOT_BRIDGE_APERTURE  MemAbove4G;
  PCI_ROOT_BRIDGE_APERTURE  PMem;
  PCI_ROOT_BRIDGE_APERTURE  PMemAbove4G;
  UINT64                    Attributes;
  UINT64                    AllocationAttributes;

  ZeroMem (&Io, sizeof (Io));
  ZeroMem (&Mem, sizeof (Mem));
  ZeroMem (&MemAbove4G, sizeof (MemAbove4G));
  ZeroMem (&PMem, sizeof (PMem));
  ZeroMem (&PMemAbove4G, sizeof (PMemAbove4G));

  PMem.Base = 1;
  PMemAbove4G.Base = MAX_UINT64;

  BusMin = PcdGet32 (PcdPciBusMin);
  BusMax = PcdGet32 (PcdPciBusMax);

  //
  // IO, MMIO32, MMIO64
  //
  Io.Base  = PcdGet64 (PcdPciIoBase);
  Io.Limit = PcdGet64 (PcdPciIoBase) + PcdGet64 (PcdPciIoSize) - 1;

  Mem.Base  = PcdGet32 (PcdPciMmio32Base);
  Mem.Limit = PcdGet32 (PcdPciMmio32Base)
              + PcdGet32 (PcdPciMmio32Size) - 1;

  MemAbove4G.Base  = PcdGet64 (PcdPciMmio64Base);
  MemAbove4G.Limit = PcdGet64 (PcdPciMmio64Base)
                     + PcdGet64 (PcdPciMmio64Size) - 1;

  //
  // Fetch the ECAM window
  //
  ConfigBase = GENERIC_ECAM_CONFIGURATION_BASE;
  ConfigSize = GENERIC_ECAM_CONFIGURATION_SIZE;

  DEBUG ((
    DEBUG_INFO,
    "%a: Config[0x%Lx+0x%Lx)\n",
    __func__,
    ConfigBase,
    ConfigSize
    ));

  //
  // Map the ECAM space in the GCD memory map
  //
  Status = MapGcdMmioSpace (ConfigBase, ConfigSize);
  if ((Status != EFI_SUCCESS) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MapGcdMmioSpace[0x%Lx-0x%Lx]: %r\n",
      ConfigBase,
      ConfigSize,
      Status
      ));
  }

  if (Io.Base <= Io.Limit) {
    //
    // Map the MMIO window that provides I/O access - the PCI host bridge code
    // is not aware of this translation and so it will only map the I/O view
    // in the GCD I/O map.
    //
    Status = MapGcdMmioSpace (Io.Base - Io.Translation, Io.Limit - Io.Base + 1);
    if ((Status != EFI_SUCCESS) && (Status != EFI_UNSUPPORTED)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: MapGcdMmioSpace[0x%Lx-0x%Lx]: %r\n",
        Status,
        Io.Base - Io.Translation,
        Io.Limit - Io.Base + 1
        ));
    }
  }

  Attributes = EFI_PCI_ATTRIBUTE_ISA_IO_16 |
               EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO |
               EFI_PCI_ATTRIBUTE_VGA_IO_16  |
               EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;

  AllocationAttributes = 0;
  if ((PMem.Base > PMem.Limit) &&
      (PMemAbove4G.Base > PMemAbove4G.Limit))
  {
    AllocationAttributes |= EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM;
  }

  if ((MemAbove4G.Base <= MemAbove4G.Limit) ||
      (PMemAbove4G.Limit <= PMemAbove4G.Limit))
  {
    AllocationAttributes |= EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
  }

  Bridge->DmaAbove4G            = TRUE;
  Bridge->Supports              = Attributes;
  Bridge->Attributes            = Attributes;
  Bridge->AllocationAttributes  = AllocationAttributes;
  Bridge->Bus.Base              = BusMin;
  Bridge->Bus.Limit             = BusMax;
  Bridge->NoExtendedConfigSpace = FALSE;

  CopyMem (&Bridge->Io, &Io, sizeof (Io));
  CopyMem (&Bridge->Mem, &Mem, sizeof (Mem));
  CopyMem (&Bridge->MemAbove4G, &MemAbove4G, sizeof (MemAbove4G));
  CopyMem (&Bridge->PMem, &PMem, sizeof (PMem));
  CopyMem (&Bridge->PMemAbove4G, &PMemAbove4G, sizeof (PMemAbove4G));

  return EFI_SUCCESS;
}

/**
  Populate and return all the root bridge instances in an array.

  @param  Count  Where to store the number of returned PCI_ROOT_BRIDGE structs.

  @return A Count-sized array of PCI_ROOT_BRIDGE on success. NULL otherwise.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  OUT UINTN  *Count
  )
{
  EFI_STATUS          Status;
  PCI_ROOT_BRIDGE     *Bridges;
  MY_PCI_ROOT_BRIDGE_DEVICE_PATH  *DevicePath;

  //
  // Only support one RootBridge with single PcdPciExpressBaseAddress
  //
  *Count = 1;

  Bridges = AllocateZeroPool (sizeof (PCI_ROOT_BRIDGE) * (*Count));
  if (Bridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, EFI_OUT_OF_RESOURCES));
    return NULL;
  }

  DevicePath = AllocateCopyPool (
                 sizeof mRootBridgeDevicePathTemplate,
                 &mRootBridgeDevicePathTemplate
                 );
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, EFI_OUT_OF_RESOURCES));
    return NULL;
  }

  DevicePath->AcpiDevicePath.UID  = 0;
  Bridges[0].Segment    = 0;
  Bridges[0].DevicePath = (VOID *)DevicePath;

  Status = ProcessPciHost (&Bridges[0]);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ProcessPciHost[0]: %r\n",
      __func__,
      Status
      ));
    FreePool (DevicePath);
  }

  return Bridges;
}

/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE  *Bridges,
  UINTN            Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; Index++, Bridges++) {
    FreePool (Bridges->DevicePath);
  }

  FreePool (Bridges);
}

/**
  Inform the platform that a resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE  HostBridgeHandle,
  VOID        *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  UINTN                              RootBridgeIndex;

  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor      = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for ( ; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (
        Descriptor->ResType <
        ARRAY_SIZE (mPciHostBridgeLibAcpiAddressSpaceTypeStr)
        );
      DEBUG ((
        DEBUG_ERROR,
        " %s: Length/Alignment = 0x%lx / 0x%lx\n",
        mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
        Descriptor->AddrLen,
        Descriptor->AddrRangeMax
        ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((
          DEBUG_ERROR,
          "     Granularity/SpecificFlag = %ld / %02x%s\n",
          Descriptor->AddrSpaceGranularity,
          Descriptor->SpecificFlag,
          ((Descriptor->SpecificFlag &
            EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
            ) != 0) ? L" (Prefetchable)" : L""
          ));
      }
    }

    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                                                       (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                                                       );
  }
}
