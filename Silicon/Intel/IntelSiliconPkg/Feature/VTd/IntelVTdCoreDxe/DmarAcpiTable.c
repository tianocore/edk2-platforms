/** @file

  Copyright (c) 2017 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT32                       Entry;
} RSDT_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT64                       Entry;
} XSDT_TABLE;

#pragma pack()

EFI_ACPI_DMAR_HEADER  *mAcpiDmarTable = NULL;

/**
  Dump DMAR ACPI table.
**/
VOID
VtdDumpDmarTable (
  VOID
  )
{
  VtdLibDumpAcpiDmar (NULL, NULL, (EFI_ACPI_DMAR_HEADER *) (UINTN) mAcpiDmarTable);

  VTdLogAddDataEvent (VTDLOG_DXE_DMAR_TABLE, mAcpiDmarTable->Header.Length, (VOID *)mAcpiDmarTable, mAcpiDmarTable->Header.Length);
}

/**
  Get PCI device information from DMAR DevScopeEntry.

  @param[in]  Segment               The segment number.
  @param[in]  DmarDevScopeEntry     DMAR DevScopeEntry
  @param[out] Bus                   The bus number.
  @param[out] Device                The device number.
  @param[out] Function              The function number.

  @retval EFI_SUCCESS  The PCI device information is returned.
**/
EFI_STATUS
GetPciBusDeviceFunction (
  IN  UINT16                                      Segment,
  IN  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *DmarDevScopeEntry,
  OUT UINT8                                       *Bus,
  OUT UINT8                                       *Device,
  OUT UINT8                                       *Function
  )
{
  EFI_ACPI_DMAR_PCI_PATH                     *DmarPciPath;
  UINT8                                      MyBus;
  UINT8                                      MyDevice;
  UINT8                                      MyFunction;

  DmarPciPath = (EFI_ACPI_DMAR_PCI_PATH *)((UINTN)(DmarDevScopeEntry + 1));
  MyBus = DmarDevScopeEntry->StartBusNumber;
  MyDevice = DmarPciPath->Device;
  MyFunction = DmarPciPath->Function;

  switch (DmarDevScopeEntry->Type) {
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
    while ((UINTN)DmarPciPath + sizeof(EFI_ACPI_DMAR_PCI_PATH) < (UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length) {
      MyBus = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, MyBus, MyDevice, MyFunction, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
      DmarPciPath ++;
      MyDevice = DmarPciPath->Device;
      MyFunction = DmarPciPath->Function;
    }
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
    break;
  }

  *Bus = MyBus;
  *Device = MyDevice;
  *Function = MyFunction;

  return EFI_SUCCESS;
}

/**
  Process DMAR DRHD table.

  @param[in]  VtdIndex  The index of VTd engine.
  @param[in]  DmarDrhd  The DRHD table.

  @retval EFI_SUCCESS The DRHD table is processed.
**/
EFI_STATUS
ProcessDrhd (
  IN UINTN                      VtdIndex,
  IN EFI_ACPI_DMAR_DRHD_HEADER  *DmarDrhd
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDevScopeEntry;
  UINT8                                             Bus;
  UINT8                                             Device;
  UINT8                                             Function;
  UINT8                                             SecondaryBusNumber;
  EFI_STATUS                                        Status;
  VTD_SOURCE_ID                                     SourceId;

  mVtdUnitInformation[VtdIndex].PciDeviceInfo = AllocateZeroPool (sizeof (PCI_DEVICE_INFORMATION) + sizeof (PCI_DEVICE_DATA) * VTD_PCI_DATA_ALLOC_CHUNK);
  if (mVtdUnitInformation[VtdIndex].PciDeviceInfo == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  mVtdUnitInformation[VtdIndex].Segment            = DmarDrhd->SegmentNumber;
  mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress = (UINTN)DmarDrhd->RegisterBaseAddress;
  DEBUG ((DEBUG_INFO,"  VTD (%d) BaseAddress -  0x%016lx\n", VtdIndex, DmarDrhd->RegisterBaseAddress));

  mVtdUnitInformation[VtdIndex].PciDeviceInfo->Segment                = DmarDrhd->SegmentNumber;
  mVtdUnitInformation[VtdIndex].PciDeviceInfo->PciDeviceDataMaxNumber = VTD_PCI_DATA_ALLOC_CHUNK;

  if ((DmarDrhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL) != 0) {
    mVtdUnitInformation[VtdIndex].PciDeviceInfo->IncludeAllFlag = TRUE;
    DEBUG ((DEBUG_INFO,"  ProcessDrhd: with INCLUDE ALL\n"));

    Status = ScanAllPciBus((VOID *)VtdIndex, DmarDrhd->SegmentNumber, ScanBusCallbackRegisterPciDevice);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    mVtdUnitInformation[VtdIndex].PciDeviceInfo->IncludeAllFlag = FALSE;
    DEBUG ((DEBUG_INFO,"  ProcessDrhd: without INCLUDE ALL\n"));
  }

  DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarDrhd + 1));
  while ((UINTN)DmarDevScopeEntry < (UINTN)DmarDrhd + DmarDrhd->Header.Length) {

    Status = GetPciBusDeviceFunction (DmarDrhd->SegmentNumber, DmarDevScopeEntry, &Bus, &Device, &Function);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((DEBUG_INFO,"  ProcessDrhd: "));
    switch (DmarDevScopeEntry->Type) {
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
      DEBUG ((DEBUG_INFO,"PCI Endpoint"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
      DEBUG ((DEBUG_INFO,"PCI-PCI bridge"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
      DEBUG ((DEBUG_INFO,"IOAPIC"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
      DEBUG ((DEBUG_INFO,"MSI Capable HPET"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
      DEBUG ((DEBUG_INFO,"ACPI Namespace Device"));
      break;
    }
    DEBUG ((DEBUG_INFO," S%04x B%02x D%02x F%02x\n", DmarDrhd->SegmentNumber, Bus, Device, Function));

    SourceId.Bits.Bus = Bus;
    SourceId.Bits.Device = Device;
    SourceId.Bits.Function = Function;

    Status = RegisterPciDevice (VtdIndex, DmarDrhd->SegmentNumber, SourceId, DmarDevScopeEntry->Type, TRUE);
    if (EFI_ERROR (Status)) {
      //
      // There might be duplication for special device other than standard PCI device.
      //
      switch (DmarDevScopeEntry->Type) {
      case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
      case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
        return Status;
      }
    }

    switch (DmarDevScopeEntry->Type) {
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
      SecondaryBusNumber = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(DmarDrhd->SegmentNumber, Bus, Device, Function, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
      Status = ScanPciBus ((VOID *)VtdIndex, DmarDrhd->SegmentNumber, SecondaryBusNumber, ScanBusCallbackRegisterPciDevice);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      break;
    default:
      break;
    }

    DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length);
  }

  return EFI_SUCCESS;
}

/**
  Process DMAR RMRR table.

  @param[in]  DmarRmrr  The RMRR table.

  @retval EFI_SUCCESS The RMRR table is processed.
**/
EFI_STATUS
ProcessRmrr (
  IN EFI_ACPI_DMAR_RMRR_HEADER  *DmarRmrr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDevScopeEntry;
  UINT8                                             Bus;
  UINT8                                             Device;
  UINT8                                             Function;
  EFI_STATUS                                        Status;
  VTD_SOURCE_ID                                     SourceId;

  DEBUG ((DEBUG_INFO,"  RMRR (Base 0x%016lx, Limit 0x%016lx)\n", DmarRmrr->ReservedMemoryRegionBaseAddress, DmarRmrr->ReservedMemoryRegionLimitAddress));

  DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarRmrr + 1));
  while ((UINTN)DmarDevScopeEntry < (UINTN)DmarRmrr + DmarRmrr->Header.Length) {
    if (DmarDevScopeEntry->Type != EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) {
      DEBUG ((DEBUG_INFO,"RMRR DevScopeEntryType is not endpoint, type[0x%x] \n", DmarDevScopeEntry->Type));
      return EFI_DEVICE_ERROR;
    }

    Status = GetPciBusDeviceFunction (DmarRmrr->SegmentNumber, DmarDevScopeEntry, &Bus, &Device, &Function);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((DEBUG_INFO,"RMRR S%04x B%02x D%02x F%02x\n", DmarRmrr->SegmentNumber, Bus, Device, Function));

    SourceId.Bits.Bus = Bus;
    SourceId.Bits.Device = Device;
    SourceId.Bits.Function = Function;
    Status = SetAccessAttribute (
               DmarRmrr->SegmentNumber,
               SourceId,
               DmarRmrr->ReservedMemoryRegionBaseAddress,
               DmarRmrr->ReservedMemoryRegionLimitAddress + 1 - DmarRmrr->ReservedMemoryRegionBaseAddress,
               EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length);
  }

  return EFI_SUCCESS;
}

/**
  Get VTd engine number.
**/
UINTN
GetVtdEngineNumber (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdIndex;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      VtdIndex++;
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return VtdIndex ;
}

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  EFI_STATUS                                        Status;
  UINTN                                             VtdIndex;

  mVtdUnitNumber = GetVtdEngineNumber ();
  DEBUG ((DEBUG_INFO,"  VtdUnitNumber - %d\n", mVtdUnitNumber));
  ASSERT (mVtdUnitNumber > 0);
  if (mVtdUnitNumber == 0) {
    return EFI_DEVICE_ERROR;
  }

  mVtdUnitInformation = AllocateZeroPool (sizeof(*mVtdUnitInformation) * mVtdUnitNumber);
  ASSERT (mVtdUnitInformation != NULL);
  if (mVtdUnitInformation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      ASSERT (VtdIndex < mVtdUnitNumber);
      Status = ProcessDrhd (VtdIndex, (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      VtdIndex++;

      break;

    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  ASSERT (VtdIndex == mVtdUnitNumber);

  for (VtdIndex = 0; VtdIndex < mVtdUnitNumber; VtdIndex++) {
    VtdLibDumpPciDeviceInfo (NULL, NULL, mVtdUnitInformation[VtdIndex].PciDeviceInfo);

    VTdLogAddDataEvent (VTDLOG_DXE_PCI_DEVICE,
                        mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress,
                        mVtdUnitInformation[VtdIndex].PciDeviceInfo,
                        sizeof (PCI_DEVICE_INFORMATION) + sizeof (PCI_DEVICE_DATA) * mVtdUnitInformation[VtdIndex].PciDeviceInfo->PciDeviceDataNumber);
  }
  return EFI_SUCCESS ;
}

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableRmrr (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  EFI_STATUS                                        Status;

  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_RMRR:
      Status = ProcessRmrr ((EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return EFI_SUCCESS ;
}

/**
  Get the DMAR ACPI table.

  @retval EFI_SUCCESS           The DMAR ACPI table is got.
  @retval EFI_ALREADY_STARTED   The DMAR ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The DMAR ACPI table is not found.
**/
EFI_STATUS
GetDmarAcpiTable (
  VOID
  )
{
  if (mAcpiDmarTable != NULL) {
    return EFI_ALREADY_STARTED;
  }

  mAcpiDmarTable = (EFI_ACPI_DMAR_HEADER *) EfiLocateFirstAcpiTable (
                                              EFI_ACPI_4_0_DMA_REMAPPING_TABLE_SIGNATURE
                                              );
  if (mAcpiDmarTable == NULL) {
    return EFI_NOT_FOUND;
  }
  DEBUG ((DEBUG_INFO,"DMAR Table - 0x%08x\n", mAcpiDmarTable));
  VtdDumpDmarTable();

  return EFI_SUCCESS;
}
