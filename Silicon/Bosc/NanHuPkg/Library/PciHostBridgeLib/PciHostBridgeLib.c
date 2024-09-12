/** @file
  PCI host bridge library instance for NanHuDev SOC.

  Copyright (C) 2020, Phytium Technology Co Ltd. All rights reserved.<BR>
  Copyright (c) 2024, Bosc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/IoLib.h>
#include <RiscVBitOp.h>

#include "PciHostBridgeLib.h"

#pragma pack(1)

typedef struct {
  PCI_DEVICE_PATH           PciDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;

#pragma pack ()

STATIC CONST EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath[] = {
  {
    PCI_DEVICE_PATH_NODE(0, 0),
    END_DEVICE_PATH_DEF
  },
};

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

STATIC PCI_ROOT_BRIDGE mRootBridge = {
  0,                                              // Segment
  0,                                              // Supports
  0,                                              // Attributes
  FALSE,                                           // DmaAbove4G
  FALSE,                                          // NoExtendedConfigSpace
  FALSE,                                          // ResourceAssigned
  0,           // AllocationAttributes
  {
    // Bus
    FixedPcdGet32 (PcdPciBusMin),
    FixedPcdGet32 (PcdPciBusMax)
  }, {
    // Io
    FixedPcdGet64 (PcdPciIoBase),
    FixedPcdGet64 (PcdPciIoBase) + FixedPcdGet64 (PcdPciIoSize) - 1
  }, {
    // Mem
    FixedPcdGet32 (PcdPciMmio32Base),
    FixedPcdGet32 (PcdPciMmio32Base) + (FixedPcdGet32 (PcdPciMmio32Size) - 1)
    //0x7FFFFFFF
  }, {
    // MemAbove4G
    FixedPcdGet64 (PcdPciMmio64Base),
    FixedPcdGet64 (PcdPciMmio64Base) + FixedPcdGet64 (PcdPciMmio64Size) - 1
  }, {
    // PMem
    MAX_UINT64,
    0
  }, {
    // PMemAbove4G
    MAX_UINT64,
    0
  },
  (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath
};

/**
  Return all the root bridge instances in an array.

  @param[out] Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.

**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  OUT UINTN     *Count
  )
{
  /* Enable the Bridge enable bit */
  UINT64 PciConfigBase = FixedPcdGet64 (PcdPciConfigBase);
  UINT32 Rpsc = MmioRead32 (PciConfigBase + XILINX_PCIE_REG_RPSC);
  MmioWrite32 (PciConfigBase + XILINX_PCIE_REG_RPSC, Rpsc | XILINX_PCIE_REG_RPSC_BEN);
  DEBUG ((DEBUG_INFO, "%a: PciConfigBase:0x%x Rpsc:0x%x\n", __func__, PciConfigBase));
  DEBUG ((DEBUG_INFO, "%a: Rpsc:0x%x \n", __func__, Rpsc));

  *Count = 1;
  return &mRootBridge;
}


/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges().

  @param[in] Bridges The root bridge instances array.
  @param[in] Count   The count of the array.

**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  IN PCI_ROOT_BRIDGE *Bridges,
  IN UINTN           Count
  )
{

}


/**
  Inform the platform that the resource conflict happens.

  @param[in] HostBridgeHandle Handle of the Host Bridge.
  @param[in] Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          SubmitResources().

**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  IN EFI_HANDLE                        HostBridgeHandle,
  IN VOID                              *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  BOOLEAN IsPrefetchable;

  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              ARRAY_SIZE (mPciHostBridgeLibAcpiAddressSpaceTypeStr));
      DEBUG ((DEBUG_INFO, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen,
              Descriptor->AddrRangeMax
              ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {

        IsPrefetchable = (Descriptor->SpecificFlag &
          EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE) != 0;

        DEBUG ((DEBUG_INFO, "     Granularity/SpecificFlag = %ld / %02x%s\n",
          Descriptor->AddrSpaceGranularity,
          Descriptor->SpecificFlag,
          (IsPrefetchable) ? L" (Prefetchable)" : L""
          ));
      }
    }
    //
    // Skip the end descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) (
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}
