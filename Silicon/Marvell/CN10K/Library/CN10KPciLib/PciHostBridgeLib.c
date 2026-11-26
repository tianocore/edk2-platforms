/** @file

  PCI Host Bridge Library instance for Marvell CN10K SoCs

  Returns the root bridge apertures of all CN10K PCIe root complexes:
    - segments 0-2: ECAM root complexes (on-chip PCIe controllers),
    - segments 3-8: PEM controllers (external PCIe slots).

  Every root bridge exposes a single 64-bit MMIO window located above 4 GB;
  there are no IO or 32-bit MMIO windows on cn10ka. The generic ARM CPU I/O
  2 implementation (ArmPciCpuIo2Dxe) restricts IO accesses to the first
  64 KB, which cannot reach the PEM IO windows mapped above 4 GB, so no IO
  aperture is exposed to the enumerator.

  The MMIO window of the ECAM segments is CPU-address-only (no translation)
  while the PEM windows are translated: the CPU address equals the PCI
  address shifted by the controller-specific base (CpuMemBase) used by the
  legacy Cavium driver.

  Copyright (c) 2026, Marvell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>

#include <IndustryStandard/Pci22.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH        AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC CONST EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  mEfiPciRootBridgeDevicePathTemplate = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID (0x0A08), // PCI Express
    0                     // AcpiDevicePath.UID
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

typedef struct {
  UINT64    Mem64Base;     /* Device address of the MMIO window base.  */
  UINT64    Mem64Size;     /* Size of the MMIO window.                 */
  UINT64    Mem64CpuBase;  /* Host - device address offset.            */
} CN10K_ROOT_BRIDGE_DATA;

//
// Memory window is shared by all CN10K ECAM root complexes. PEM controllers
// have their own MMIO window and CPU-side mapping.
//
STATIC CONST CN10K_ROOT_BRIDGE_DATA  mRootBridgeData[] = {
  /* ECAM0 - segment 0 */
  { 0x800000000000,  0x0E0000000000, 0              },
  /* ECAM1 - segment 1 */
  { 0x800000000000,  0x0E0000000000, 0              },
  /* ECAM2 - segment 2 */
  { 0x800000000000,  0x0E0000000000, 0              },
  /* PEM0 - segment 3 */
  { 0x0010000000000, 0x001000000000, 0x883000000000 },
  /* PEM1 - segment 4 */
  { 0x0010000000000, 0x001000000000, 0x88B000000000 },
  /* PEM2 - segment 5 */
  { 0x0010000000000, 0x001000000000, 0x893000000000 },
  /* PEM3 - segment 6 */
  { 0x0010000000000, 0x001000000000, 0x89B000000000 },
  /* PEM4 - segment 7 */
  { 0x0010000000000, 0x001000000000, 0x8A3000000000 },
  /* PEM5 - segment 8 */
  { 0x0010000000000, 0x001000000000, 0x8AB000000000 },
};

/**
  Return all the CN10K root bridge instances.

  Allocates one PCI_ROOT_BRIDGE per entry in mRootBridgeData, assigns the
  segment number and fixed bus/MMIO apertures, and allocates a separate ACPI
  device path for each bridge. The caller owns the returned array and must
  release it with PciHostBridgeFreeRootBridges().

  @param[out]  Count  Receives the number of root bridge instances.

  @return  The root bridge array, or NULL if the array could not be allocated.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  OUT UINTN  *Count
  )
{
  PCI_ROOT_BRIDGE                  *PciRootBridges;
  PCI_ROOT_BRIDGE                  *RootBridge;
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  *DevicePath;
  CONST CN10K_ROOT_BRIDGE_DATA     *BridgeData;
  UINTN                            Index;

  *Count = 0;

  PciRootBridges = AllocateZeroPool (
                     ARRAY_SIZE (mRootBridgeData) *
                     sizeof (PCI_ROOT_BRIDGE)
                     );
  if (PciRootBridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate root bridges\n", __func__));
    return NULL;
  }

  *Count     = ARRAY_SIZE (mRootBridgeData);
  RootBridge = PciRootBridges;

  for (Index = 0; Index < ARRAY_SIZE (mRootBridgeData); Index++, RootBridge++) {
    DevicePath = AllocateCopyPool (
                   sizeof (EFI_PCI_ROOT_BRIDGE_DEVICE_PATH),
                   &mEfiPciRootBridgeDevicePathTemplate
                   );
    if (DevicePath == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to allocate device path\n", __func__));
      continue;
    }

    DevicePath->AcpiDevicePath.UID = Index;

    BridgeData = &mRootBridgeData[Index];

    RootBridge->Segment    = Index;
    RootBridge->Supports   = EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE | EFI_PCI_DEVICE_ENABLE;
    RootBridge->Attributes = RootBridge->Supports;

    RootBridge->DmaAbove4G            = TRUE;
    RootBridge->NoExtendedConfigSpace = FALSE;
    RootBridge->ResourceAssigned      = FALSE;

    /*
     * The CN10K PCIe root bridges do not implement any BAR registers:
     * their memory window is fixed in hardware and always accessible, so
     * there is nothing for the resource allocator to reprogram.
     */
    RootBridge->AllocationAttributes = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
                                       EFI_PCI_HOST_BRIDGE_MEM64_DECODE;

    RootBridge->Bus.Base        = 0;
    RootBridge->Bus.Limit       = 0xFF;
    RootBridge->Bus.Translation = 0;

    /* No IO aperture - see the header comment. */
    RootBridge->Io.Base        = MAX_UINT64;
    RootBridge->Io.Limit       = 0;
    RootBridge->Io.Translation = 0;

    /* No 32-bit MMIO */
    RootBridge->Mem.Base        = MAX_UINT64;
    RootBridge->Mem.Limit       = 0;
    RootBridge->Mem.Translation = 0;

    RootBridge->MemAbove4G.Base  = BridgeData->Mem64Base;
    RootBridge->MemAbove4G.Limit = BridgeData->Mem64Base +
                                   BridgeData->Mem64Size - 1;

    /*
     * Host address = Device address - Translation, hence
     * Translation = -(Host - Device) = MAX_UINT64 - Mem64CpuBase + 1.
     */
    RootBridge->MemAbove4G.Translation =
      MAX_UINT64 - BridgeData->Mem64CpuBase + 1;

    /* Prefetchable and non-prefetchable ranges are combined. */
    RootBridge->PMem.Base               = MAX_UINT64;
    RootBridge->PMem.Limit              = 0;
    RootBridge->PMem.Translation        = 0;
    RootBridge->PMemAbove4G.Base        = MAX_UINT64;
    RootBridge->PMemAbove4G.Limit       = 0;
    RootBridge->PMemAbove4G.Translation = 0;

    RootBridge->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
  }

  return PciRootBridges;
}

/**
  Free the root bridge instances array returned from
  PciHostBridgeGetRootBridges().

  Frees the device path allocated for each bridge and then the array itself.

  @param[in]  Bridges  The root bridge instances array. May be NULL.
  @param[in]  Count    The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  IN PCI_ROOT_BRIDGE  *Bridges,
  IN UINTN            Count
  )
{
  UINTN  Index;

  if (Bridges == NULL) {
    return;
  }

  /* Each root bridge carries a separately allocated device path. */
  for (Index = 0; Index < Count; Index++) {
    if (Bridges[Index].DevicePath != NULL) {
      FreePool (Bridges[Index].DevicePath);
    }
  }

  FreePool (Bridges);
}

/**
  Report a PCI resource conflict to the debug output.

  Walks the ACPI resource descriptor list for every root bridge and logs the
  length, alignment, granularity, and specific flags of each conflicting
  memory or IO descriptor. Each root bridge's descriptors end with an ACPI end
  tag, which is skipped before moving to the next bridge.

  @param[in]  HostBridgeHandle  Handle of the host bridge.
  @param[in]  Configuration     Pointer to the ACPI resource descriptors for
                                all root bridges.
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  IN EFI_HANDLE  HostBridgeHandle,
  IN VOID        *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  CHAR16                             *MemoryType;
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
        if (Descriptor->SpecificFlag &
            EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE)
        {
          MemoryType = L"(Prefetchable)";
        } else {
          MemoryType = L"";
        }

        DEBUG ((
          DEBUG_ERROR,
          "     Granularity/SpecificFlag = %ld / %02x%s\n",
          Descriptor->AddrSpaceGranularity,
          Descriptor->SpecificFlag,
          MemoryType
          ));
      }
    }

    /* Skip the END descriptor for the root bridge. */
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)((EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1);
  }
}
