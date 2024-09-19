/** @file
  PCI Host Bridge Library instance for Ampere Altra-based platforms.

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/RootComplexInfoHob.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Platform/Ac01.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

GLOBAL_REMOVE_IF_UNREFERENCED
STATIC CHAR16 CONST *CONST  mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH        AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  mEfiPciRootBridgeDevicePath = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID (0x0A08), // PCIe
    0
  },  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

STATIC PCI_ROOT_BRIDGE  mRootBridgeTemplate = {
  0,                               // Segment
  0,                               // Supports
  0,                               // Attributes
  TRUE,                            // DmaAbove4G
  FALSE,                           // NoExtendedConfigSpace
  FALSE,                           // ResourceAssigned
  EFI_PCI_HOST_BRIDGE_MEM64_DECODE,
  {
    // Bus
    0,
    0xFF,
    0
  },                                {
    // Io
    0,
    0,
    0
  },                                {
    // Mem
    MAX_UINT64,
    0,
    0
  },                                {
    // MemAbove4G
    MAX_UINT64,
    0,
    0
  },                                {
    // PMem
    MAX_UINT64,
    0,
    0
  },                                {
    // PMemAbove4G
    MAX_UINT64,
    0,
    0
  },
  (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath
};

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN  *Count
  )
{
  AC01_ROOT_COMPLEX                *RootComplex;
  AC01_ROOT_COMPLEX                *RootComplexList;
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  *DevicePath;
  PCI_ROOT_BRIDGE                  *RootBridge;
  PCI_ROOT_BRIDGE                  *RootBridges;
  UINT8                            Index;
  UINT8                            RootBridgeCount = 0;
  VOID                             *Hob;

  Hob = GetFirstGuidHob (&gRootComplexInfoHobGuid);
  if (Hob == NULL) {
    return NULL;
  }

  RootComplexList = (AC01_ROOT_COMPLEX *)GET_GUID_HOB_DATA (Hob);

  RootBridges = AllocatePool (AC01_PCIE_MAX_ROOT_COMPLEX * sizeof (PCI_ROOT_BRIDGE));
  if (RootBridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate RootBridges\n", __func__));
    return NULL;
  }

  for (Index = 0; Index < AC01_PCIE_MAX_ROOT_COMPLEX; Index++) {
    RootComplex = &RootComplexList[Index];
    if (!RootComplex->Active) {
      continue;
    }

    RootBridge = &RootBridges[RootBridgeCount];
    CopyMem (RootBridge, &mRootBridgeTemplate, sizeof (PCI_ROOT_BRIDGE));

    if (RootComplex->Mmio32Base != 0) {
      RootBridge->Mem.Base   = RootComplex->Mmio32Base;
      RootBridge->Mem.Limit  = RootComplex->Mmio32Base + RootComplex->Mmio32Size - 1;
      RootBridge->PMem.Base  = RootBridge->Mem.Base;
      RootBridge->PMem.Limit = RootBridge->Mem.Limit;
      RootBridge->Io.Base    = RootComplex->Mmio32Base + RootComplex->Mmio32Size - AC01_PCIE_IO_SIZE;
      RootBridge->Io.Limit   = RootBridge->Mem.Limit;
    }

    if (RootComplex->MmioBase != 0) {
      RootBridge->PMemAbove4G.Base  = RootComplex->MmioBase;
      RootBridge->PMemAbove4G.Limit = RootComplex->MmioBase + RootComplex->MmioSize - 1;
    }

    RootBridge->Segment = RootComplex->Logical;

    DevicePath = AllocateCopyPool (
                   sizeof (EFI_PCI_ROOT_BRIDGE_DEVICE_PATH),
                   (VOID *)&mEfiPciRootBridgeDevicePath
                   );
    if (DevicePath == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to allocate device path\n", __func__));
      return NULL;
    }

    //
    // Embedded the Root Complex Index into the DevicePath
    // This will be used later by the platform NotifyPhase()
    //
    DevicePath->AcpiDevicePath.UID = Index;

    RootBridge->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
    RootBridgeCount++;
  }

  *Count = RootBridgeCount;
  return RootBridges;
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
  //
  // Unsupported
  //
}

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOUrce_ALLOCATION_PROTOCOL
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
        (sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr) /
         sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr[0])
        )
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
