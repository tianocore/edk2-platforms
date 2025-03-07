/** @file

  Implements AMD Turin Platform SoC Library.
  Provides interface to Get Set platform specific data.
  
  Copyright (C) 2023 -2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Acpi65.h>
#include <Library/DebugLib.h>
#include <Protocol/AmdNbioPcieServicesProtocol.h>
#include <GnbDxio.h>
#include <Guid/GnbPcieInfoHob.h>
#include <Register/AmdIoApic.h>
#include <GnbRegistersBRH.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/NbioHandleLib.h>
#include <Library/SmnAccessLib.h>
#include <Library/PcieConfigLib.h>
#include <Library/AmdPlatformSocLib.h>
#include <Protocol/AmdPciResourcesProtocol.h>
#include <Protocol/AmdCxlServicesProtocol.h>
#include <Library/NbioCommonLibDxe.h>
#include <Protocol/FabricNumaServices2.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/AmdCpmTableProtocol/AmdCpmTableProtocol.h>

#define MAX_IOAPIC_NUM  0x20

/**
  Obtains proximity domain for given PciAddress,
  provided in BDF format.

  Calls AGESA fabric service to obtain domain information.

  @param[in]      SocketID    - SocketID of the provided PciAddress
  @param[in]      PciAddress  - PCI Address of the device

  @retval         SocketID, if fails to get data from fabric service, else
                            PXM value.
**/
UINTN
GetPxmDomain (
  IN UINT8     SocketId,
  IN PCI_ADDR  PciAddress
  )
{
  FABRIC_NUMA_SERVICES2_PROTOCOL  *FabricNumaServices;
  EFI_STATUS                      Status;
  PXM_DOMAIN_INFO                 PxmDomainInfo;

  Status = gBS->LocateProtocol (&gAmdFabricNumaServices2ProtocolGuid, NULL, (VOID **)&FabricNumaServices);
  if (EFI_ERROR (Status)) {
    return SocketId;
  }

  ZeroMem ((VOID *)&PxmDomainInfo, sizeof (PxmDomainInfo));
  Status = FabricNumaServices->GetPxmDomainInfo (FabricNumaServices, PciAddress, &PxmDomainInfo);
  if (EFI_ERROR (Status)) {
    return SocketId;
  }

  ASSERT (PxmDomainInfo.Count == 1);
  return PxmDomainInfo.Domain[0];
}

/**
  @brief Get the PCIe CXL2 Info object

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  CxlPortInfo   The CXL port information
  @param[in, out]  CxlCount      Number of CXL port present

  @retval EFI_SUCCESS             Successfully retrieve the CXL port information.
          EFI_INVALID_PARAMETERS  Incorrect parameters provided.
          EFI_UNSUPPORTED         Platform do not support this function.
          Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
GetPcieCxl2Info (
  IN OUT AMD_CXL_PORT_INFO  **CxlPortInfo,
  IN OUT UINTN              *CxlCount
  )
{
  UINTN                 CxlRbSupportCount;
  EFI_STATUS            Status;
  PCIE_PLATFORM_CONFIG  *Pcie;
  GNB_HANDLE            *GnbHandle;
  AMD_CXL_PORT_INFO     *LocalCxlPortInfoHead;
  AMD_CXL_PORT_INFO     *LocalCxlPortInfo;

  if ((CxlPortInfo == NULL) || (CxlCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CxlRbSupportCount    = 0;
  LocalCxlPortInfoHead = NULL;

  // Collecting Pcie information from Hob
  Status = PcieGetPcieDxe (&Pcie);
  if (!EFI_ERROR (Status)) {
    GnbHandle = NbioGetHandle (Pcie);
    while (GnbHandle != NULL) {
      if ((GnbHandle->Header.DescriptorFlags & SILICON_CXL_CAPABLE) == SILICON_CXL_CAPABLE) {
        CxlRbSupportCount++;
      }

      GnbHandle = GnbGetNextHandle (GnbHandle);
    }

    if (CxlRbSupportCount > 0) {
      LocalCxlPortInfoHead = AllocateZeroPool (sizeof (AMD_CXL_PORT_INFO) * CxlRbSupportCount);
      if (LocalCxlPortInfoHead == NULL) {
        ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
        return EFI_OUT_OF_RESOURCES;
      }

      GnbHandle = NbioGetHandle (Pcie);
      //
      // The code below will get all the root bridges with CXL 2.0 support
      // We need to add the CXL ACPI method to all of them
      //
      LocalCxlPortInfo = LocalCxlPortInfoHead;
      while (GnbHandle != NULL) {
        if ((GnbHandle->Header.DescriptorFlags & SILICON_CXL_CAPABLE) == SILICON_CXL_CAPABLE) {
          LocalCxlPortInfo->EndPointBDF.AddressValue = GnbHandle->Address.AddressValue;
          LocalCxlPortInfo->IsCxl2                   = TRUE;
          LocalCxlPortInfo++;
        }

        GnbHandle = GnbGetNextHandle (GnbHandle);
      }
    }

    *CxlCount    = CxlRbSupportCount;
    *CxlPortInfo = LocalCxlPortInfoHead;
    if (CxlRbSupportCount == 0) {
      return EFI_NOT_FOUND;
    }
  } else {
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  @brief Get the Pcie Cxl Info object

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  CxlPortInfo   The CXL port information
  @param[in, out]  CxlCount      Number of CXL port present

  @retval EFI_SUCCESS             Successfully retrieve the CXL port information.
          EFI_INVALID_PARAMETERS  Incorrect parameters provided.
          EFI_UNSUPPORTED         Platform do not support this function.
          Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
EFIAPI
GetPcieCxlInfo (
  IN OUT AMD_CXL_PORT_INFO  **CxlPortInfo,
  IN OUT UINTN              *CxlCount
  )
{
  EFI_STATUS                      Status;
  AMD_NBIO_CXL_SERVICES_PROTOCOL  *AmdNbioCxlServicesProtocol;
  UINT8                           Index;
  AMD_CXL_PORT_INFO               *CxlPortInfoHead;
  AMD_CXL_PORT_INFO               *LocalCxlPortInfo;
  AMD_CXL_PORT_INFO_STRUCT        NbioPortInfo;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __func__));

  // Check for CXL 2.0 first
  Status = GetPcieCxl2Info (CxlPortInfo, CxlCount);
  if (!EFI_ERROR (Status) && (*CxlCount > 0)) {
    return Status;
  }

  AmdNbioCxlServicesProtocol = NULL;

  Status = gBS->LocateProtocol (
                  &gAmdNbioCxlServicesProtocolGuid,
                  NULL,
                  (VOID **)&AmdNbioCxlServicesProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: Failed to locate AmdNbioCxlServices Protocol: %r\n", __func__, Status));
    Status = EFI_SUCCESS;
    return Status;
  }

  CxlPortInfoHead = AllocateZeroPool (sizeof (AMD_CXL_PORT_INFO) * AmdNbioCxlServicesProtocol->CxlCount);
  if (CxlPortInfoHead == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Populate the data structure for the CXL devices in the system to add to
  // the ACPI Table
  //
  for (Index = 0, LocalCxlPortInfo = CxlPortInfoHead; Index < AmdNbioCxlServicesProtocol->CxlCount; Index++, LocalCxlPortInfo++) {
    Status = AmdNbioCxlServicesProtocol->CxlGetRootPortInformation (
                                           AmdNbioCxlServicesProtocol,
                                           Index,
                                           &NbioPortInfo
                                           );
    if (Status != EFI_SUCCESS) {
      break;
    }

    LocalCxlPortInfo->EndPointBDF.AddressValue = NbioPortInfo.EndPointBDF.AddressValue;
    if (NbioPortInfo.DsRcrb == 0) {
      LocalCxlPortInfo->IsCxl2 = TRUE;
    } else {
      LocalCxlPortInfo->IsCxl2 = FALSE;
    }
  }

  *CxlPortInfo = CxlPortInfoHead;
  *CxlCount    = AmdNbioCxlServicesProtocol->CxlCount;
  return EFI_SUCCESS;
}

/**
  Get the platform specific IOAPIC information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  IoApicInfo  The IOAPIC information
  @param[in, out]  IoApicCount Number of IOAPIC present

  @retval EFI_SUCCESS             Successfully retrieve the IOAPIC information.
          EFI_INVALID_PARAMETERS  Incorrect parameters provided.
          EFI_UNSUPPORTED         Platform do not support this function.
          Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
EFIAPI
GetIoApicInfo (
  IN OUT EFI_ACPI_6_5_IO_APIC_STRUCTURE  **IoApicInfo,
  IN OUT UINT8                           *IoApicCount
  )
{
  EFI_STATUS                           Status;
  DXE_AMD_NBIO_PCIE_SERVICES_PROTOCOL  *PcieServicesProtocol;
  PCIE_PLATFORM_CONFIG                 *Pcie;
  GNB_HANDLE                           *GnbHandle;
  GNB_PCIE_INFORMATION_DATA_HOB        *PciePlatformConfigHobData;
  UINT32                               Value32;
  EFI_ACPI_6_5_IO_APIC_STRUCTURE       *IoApic;
  UINT8                                LocalIoApicCount;
  IO_APIC_IDENTIFICATION_REGISTER      IoApicIdentificationRegister;
  UINT32                               GlobalSystemInterruptBase;
  IO_APIC_VERSION_REGISTER             IoApicVersionRegister;

  if ((IoApicCount == NULL) || (IoApicInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IoApic = AllocateZeroPool (sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE) * MAX_IOAPIC_NUM);
  if (IoApic == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Not enough memory to allocate EFI_ACPI_6_5_IO_APIC_STRUCTURE\n",
      __func__,
      __LINE__
      ));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  // FCH IO APIC
  GlobalSystemInterruptBase = 0;
  MmioWrite8 (
    PcdGet32 (PcdIoApicAddress) + IOAPIC_INDEX_OFFSET,
    IO_APIC_VERSION_REGISTER_INDEX
    );
  IoApicVersionRegister.Uint32 = MmioRead32 (PcdGet32 (PcdIoApicAddress) + IOAPIC_DATA_OFFSET);
  GlobalSystemInterruptBase   += IoApicVersionRegister.Bits.MaximumRedirectionEntry + 1;
  Status                       = gBS->LocateProtocol (
                                        &gAmdNbioPcieServicesProtocolGuid,
                                        NULL,
                                        (VOID **)&PcieServicesProtocol
                                        );
  if (!EFI_ERROR (Status)) {
    PcieServicesProtocol->PcieGetTopology (PcieServicesProtocol, (UINT32 **)&PciePlatformConfigHobData);
    Pcie                                               = &(PciePlatformConfigHobData->PciePlatformConfigHob);
    GnbHandle                                          = NbioGetHandle (Pcie);
    LocalIoApicCount                                   = 0;
    IoApic[LocalIoApicCount].Type                      = EFI_ACPI_6_5_IO_APIC;
    IoApic[LocalIoApicCount].Length                    = sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE);
    IoApic[LocalIoApicCount].IoApicId                  = PcdGet8 (PcdIoApicId);
    IoApic[LocalIoApicCount].Reserved                  = 0;
    IoApic[LocalIoApicCount].IoApicAddress             = PcdGet32 (PcdIoApicAddress);
    IoApic[LocalIoApicCount].GlobalSystemInterruptBase = 0;
    LocalIoApicCount++;
    while (GnbHandle != NULL) {
      // Fill the header
      IoApic[LocalIoApicCount].Type     = EFI_ACPI_6_5_IO_APIC;
      IoApic[LocalIoApicCount].Length   = sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE);
      IoApic[LocalIoApicCount].Reserved = 0;
      // Read IOAPIC Address
      if (GnbHandle->RBIndex < 4) {
        SmnRegisterReadS (
          GnbHandle->Address.Address.Segment,
          GnbHandle->Address.Address.Bus,
          NBIO_SPACE (GnbHandle, SMN_IOHUB0NBIO0_IOAPIC_BASE_ADDR_LO_ADDRESS),
          &Value32
          );
      } else {
        SmnRegisterReadS (
          GnbHandle->Address.Address.Segment,
          GnbHandle->Address.Address.Bus,
          NBIO_SPACE (GnbHandle, SMN_IOHUB1NBIO0_IOAPIC_BASE_ADDR_LO_ADDRESS),
          &Value32
          );
      }

      IoApic[LocalIoApicCount].IoApicAddress =  Value32 & IOAPIC_BASE_ADDR_LO_IOAPIC_BASE_ADDR_LO_MASK;

      // Set APIC ID
      MmioWrite8 (
        IoApic[LocalIoApicCount].IoApicAddress + IOAPIC_INDEX_OFFSET,
        IO_APIC_IDENTIFICATION_REGISTER_INDEX
        );
      IoApicIdentificationRegister.Uint32 = MmioRead32 (IoApic[LocalIoApicCount].IoApicAddress + IOAPIC_DATA_OFFSET);
      IoApic[LocalIoApicCount].IoApicId   = (UINT8)IoApicIdentificationRegister.Bits.Identification;

      // Get Read the number of redirection entries in this IOAPIC
      MmioWrite8 (
        IoApic[LocalIoApicCount].IoApicAddress + IOAPIC_INDEX_OFFSET,
        IO_APIC_VERSION_REGISTER_INDEX
        );
      IoApicVersionRegister.Uint32 = MmioRead32 (
                                       IoApic[LocalIoApicCount].IoApicAddress + IOAPIC_DATA_OFFSET
                                       );
      // Set Global System Interrupt Base
      IoApic[LocalIoApicCount].GlobalSystemInterruptBase = GlobalSystemInterruptBase;
      GlobalSystemInterruptBase                         += IoApicVersionRegister.Bits.MaximumRedirectionEntry + 1;

      LocalIoApicCount++;
      GnbHandle = GnbGetNextHandle (GnbHandle);
    }
  }

  *IoApicInfo  = IoApic;
  *IoApicCount = LocalIoApicCount;
  return EFI_SUCCESS;
}

/**
  Get the platform PCIe configuration information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  RootBridge              The root bridge information
  @param[in, out]  RootBridgeCount         Number of root bridges present

  @retval EFI_SUCCESS             Successfully retrieve the root bridge information.
          EFI_INVALID_PARAMETERS  Incorrect parameters provided.
          EFI_UNSUPPORTED         Platform do not support this function.
          Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
EFIAPI
GetPcieInfo (
  IN OUT AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  **RootBridge,
  IN OUT UINTN                                *RootBridgeCount
  )
{
  EFI_STATUS                           Status;
  UINTN                                NumberOfRootBridges;
  UINTN                                RbIndex;
  UINTN                                Index;
  UINTN                                CxlIndex;
  AMD_PCI_RESOURCES_PROTOCOL           *AmdPciResources;
  AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *LocalRootBridgeArray; // do not free
  EFI_ACPI_6_5_IO_APIC_STRUCTURE       *IoApicInfo;
  UINT8                                IoApicCount;
  AMD_CXL_PORT_INFO                    *CxlPortInfoHead;
  AMD_CXL_PORT_INFO                    *CxlPortInfo;
  UINTN                                CxlCount;
  PCI_ADDR                             PciAddr;

  IoApicInfo  = NULL;
  IoApicCount = 0;
  Status      = GetIoApicInfo (&IoApicInfo, &IoApicCount);
  if (EFI_ERROR (Status) || (IoApicInfo == NULL) || (IoApicCount == 0)) {
    DEBUG ((DEBUG_ERROR, "%a:%d Cannot obtain NBIO IOAPIC information.\n", __func__, __LINE__));
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gAmdPciResourceProtocolGuid,
                  NULL,
                  (VOID **)&AmdPciResources
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to locate AMD PCIe Resource Protocol: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = AmdPciResources->AmdPciResourcesGetNumberOfRootBridges (
                              AmdPciResources,
                              &NumberOfRootBridges
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Number Of Root Bridges: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  LocalRootBridgeArray = AllocateZeroPool (sizeof (AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE) * NumberOfRootBridges);
  if (LocalRootBridgeArray == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  // Collect CXL info
  CxlPortInfoHead = NULL;
  CxlCount        = 0;
  Status          = GetPcieCxlInfo (&CxlPortInfoHead, &CxlCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[INFO] Cannot find CXL device.\n"));
  }

  // Collect Root Bridges to be sorted
  for (RbIndex = 1; RbIndex <= NumberOfRootBridges; RbIndex++) {
    Status = AmdPciResources->AmdPciResourcesGetRootBridgeInfo (AmdPciResources, RbIndex, (PCI_ROOT_BRIDGE_OBJECT **)&LocalRootBridgeArray[RbIndex - 1].Object);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get Root Bridges information : %r\n",
        __func__,
        Status
        ));
      FreePool (LocalRootBridgeArray);
      if (CxlPortInfoHead != NULL) {
        FreePool (CxlPortInfoHead);
      }

      *RootBridge      = NULL;
      *RootBridgeCount = 0;
      return Status;
    }

    // Assign GSI values
    LocalRootBridgeArray[RbIndex - 1].GlobalInterruptStart = IoApicInfo[RbIndex].GlobalSystemInterruptBase;

    // Get PXM info
    ZeroMem ((VOID *)&PciAddr, sizeof (PciAddr));
    PciAddr.Address.Bus                         = (UINT32)LocalRootBridgeArray[RbIndex - 1].Object->BaseBusNumber;
    PciAddr.Address.Segment                     = (UINT32)LocalRootBridgeArray[RbIndex - 1].Object->Segment;
    LocalRootBridgeArray[RbIndex - 1].PxmDomain = GetPxmDomain (LocalRootBridgeArray[RbIndex - 1].Object->SocketId, PciAddr);

    // check for CXL port
    if (CxlCount > 0) {
      for (CxlIndex = 0, CxlPortInfo = CxlPortInfoHead; CxlIndex < CxlCount; CxlIndex++, CxlPortInfo++) {
        if ((CxlPortInfo->EndPointBDF.Address.Segment ==  LocalRootBridgeArray[RbIndex - 1].Object->Segment) &&
            (CxlPortInfo->EndPointBDF.Address.Bus == LocalRootBridgeArray[RbIndex - 1].Object->BaseBusNumber))
        {
          LocalRootBridgeArray[RbIndex - 1].CxlCount                             = 1;
          LocalRootBridgeArray[RbIndex - 1].CxlPortInfo.IsCxl2                   = CxlPortInfo->IsCxl2;
          LocalRootBridgeArray[RbIndex - 1].CxlPortInfo.EndPointBDF.AddressValue = CxlPortInfo->EndPointBDF.AddressValue;
          break;
        }
      }
    }

    Status = AmdPciResources->AmdPciResourcesGetNumberOfRootPorts (
                                AmdPciResources,
                                LocalRootBridgeArray[RbIndex - 1].Object->Index,
                                &LocalRootBridgeArray[RbIndex - 1].RootPortCount
                                );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: ERROR: GetNumberOfRootPorts Failed: %r\n",
        __func__,
        Status
        ));
      FreePool (LocalRootBridgeArray);
      *RootBridge      = NULL;
      *RootBridgeCount = 0;
      return Status;
    }

    for (Index = 1; Index <= LocalRootBridgeArray[RbIndex - 1].RootPortCount; Index++) {
      Status = AmdPciResources->AmdPciResourcesGetRootPortInfo (
                                  AmdPciResources,
                                  LocalRootBridgeArray[RbIndex - 1].Object->Index,
                                  Index,
                                  (PCI_ROOT_PORT_OBJECT **)&LocalRootBridgeArray[RbIndex - 1].RootPort[Index]
                                  );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: ERROR: AmdPciResourcesGetRootPortInfo Failed: %r\n",
          __func__,
          Status
          ));
        FreePool (LocalRootBridgeArray);
        *RootBridge      = NULL;
        *RootBridgeCount = 0;
        return Status;
      }
    }
  }

  FreePool (IoApicInfo);
  if (CxlPortInfoHead != NULL) {
    FreePool (CxlPortInfoHead);
  }

  *RootBridge      = LocalRootBridgeArray;
  *RootBridgeCount = NumberOfRootBridges;
  return Status;
}

/**
  This function returns SBDF information for a given slot number.

  @param[in]  SlotNumInfo                Slot number to be provided.
  @param[out] SegInfo                    Segment number.
  @param[out] BusInfo                    Bus number.
  @param[out] DevFunInfo                 Bits 0-2 corresponds to function number & bits 3-7 corresponds
                                         to device number.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_INVALID_PARAMETER      One or many parameters are invalid.
  @retval EFI_NOT_FOUND              SBDF information is not found for the given slot number.

**/
EFI_STATUS
SlotBdfInfo (
  IN  UINT16  *SlotNumInfo,
  OUT UINT16  *SegInfo,
  OUT UINT8   *BusInfo,
  OUT UINT8   *DevFunInfo
  )
{
  EFI_STATUS            Status;
  PCIE_PLATFORM_CONFIG  *Pcie;
  PCIE_COMPLEX_CONFIG   *ComplexList;
  PCIE_SILICON_CONFIG   *SiliconList;
  PCIE_WRAPPER_CONFIG   *WrapperList;
  PCIE_ENGINE_CONFIG    *EngineList;

  if ((SlotNumInfo == NULL) || (SegInfo == NULL) || (BusInfo == NULL) || (DevFunInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Pcie   = NULL;
  Status = PcieGetPcieDxe (&Pcie);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ComplexList = (PCIE_COMPLEX_CONFIG *)PcieConfigGetChild (DESCRIPTOR_COMPLEX, &Pcie->Header);

  while (ComplexList != NULL) {
    SiliconList = PcieConfigGetChildSilicon (ComplexList);
    while (SiliconList != NULL) {
      WrapperList = PcieConfigGetChildWrapper (SiliconList);
      while (WrapperList != NULL) {
        EngineList = PcieConfigGetChildEngine (WrapperList);
        while (EngineList != NULL) {
          if (EngineList->Type.Port.PortData.SlotNum == *SlotNumInfo) {
            *SegInfo    = EngineList->Type.Port.Address.Address.Segment & 0xFFFF;
            *BusInfo    = EngineList->Type.Port.Address.Address.Bus & 0xFF;
            *DevFunInfo = (((EngineList->Type.Port.Address.Address.Device) & 0x1F) << 3) |
                          ((EngineList->Type.Port.Address.Address.Function) & 0x7);
            return EFI_SUCCESS;
          }

          EngineList = PcieLibGetNextDescriptor (EngineList);
        }

        WrapperList = PcieLibGetNextDescriptor (WrapperList);
      }

      SiliconList = PcieLibGetNextDescriptor (SiliconList);
    }

    if ((ComplexList->Header.DescriptorFlags & DESCRIPTOR_TERMINATE_TOPOLOGY) == 0) {
      ComplexList++;
    } else {
      ComplexList = NULL;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function allocates and populate system slot smbios record (Type 9).

  @param  DxioPortPtr              Pointer to DXIO port descriptor.
  @param  SmbiosRecordPtr          Pointer to smbios type 9 record.

  @retval EFI_SUCCESS              All parameters were valid.
  @retval EFI_INVALID_PARAMETER    One or many parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     Resource not available.

**/
EFI_STATUS
CreateSmbiosSystemSlotRecord (
  IN DXIO_PORT_DESCRIPTOR    *DxioPortPtr,
  IN OUT SMBIOS_TABLE_TYPE9  **SmbiosRecordPtr
  )
{
  EFI_STATUS          Status;
  SMBIOS_TABLE_TYPE9  *SmbiosRecord;
  UINT16              SlotNumInfo;
  UINT16              SegInfo;
  UINT8               BusInfo;
  UINT8               DevFunInfo;
  UINT8               PortWidth;

  Status     = EFI_SUCCESS;
  SegInfo    = 0xFFFF;
  BusInfo    = 0xFF;
  DevFunInfo = 0xFF;

  if ((DxioPortPtr == NULL) || (SmbiosRecordPtr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SmbiosRecord = NULL;
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE9));
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  } else {
    // Currently only map PCIE slots in system slot table.
    if (DxioPortPtr->EngineData.EngineType == DxioPcieEngine) {
      switch (DxioPortPtr->Port.LinkSpeedCapability) {
        case DxioGenMaxSupported:
          SmbiosRecord->SlotType = SlotTypePCIExpressGen5;
          break;
        case DxioGen1:
          SmbiosRecord->SlotType = SlotTypePciExpress;
          break;
        case DxioGen2:
          SmbiosRecord->SlotType = SlotTypePciExpressGen2;
          break;
        case DxioGen3:
          SmbiosRecord->SlotType = SlotTypePciExpressGen3;
          break;
        case DxioGen4:
          SmbiosRecord->SlotType = SlotTypePciExpressGen4;
          break;
        case DxioGen5:
          SmbiosRecord->SlotType = SlotTypePCIExpressGen5;
          break;
        default:
          SmbiosRecord->SlotType = SlotTypePCIExpressGen5;
          break;
      }
    } else {
      SmbiosRecord->SlotType = SlotTypeOther;
    }

    if (DxioPortPtr->EngineData.EndLane >= DxioPortPtr->EngineData.StartLane) {
      PortWidth = DxioPortPtr->EngineData.EndLane - DxioPortPtr->EngineData.StartLane + 1;
    } else {
      PortWidth = DxioPortPtr->EngineData.StartLane - DxioPortPtr->EngineData.EndLane + 1;
    }

    switch (PortWidth)
    {
      case 16:
        SmbiosRecord->SlotDataBusWidth = SlotDataBusWidth16X;
        SmbiosRecord->DataBusWidth     = 16;
        break;
      case 8:
        SmbiosRecord->SlotDataBusWidth = SlotDataBusWidth8X;
        SmbiosRecord->DataBusWidth     = 8;
        break;
      case 4:
        SmbiosRecord->SlotDataBusWidth = SlotDataBusWidth4X;
        SmbiosRecord->DataBusWidth     = 4;
        break;
      case 2:
        SmbiosRecord->SlotDataBusWidth = SlotDataBusWidth2X;
        SmbiosRecord->DataBusWidth     = 2;
        break;
      default:
        SmbiosRecord->SlotDataBusWidth = SlotDataBusWidth1X;
        SmbiosRecord->DataBusWidth     = 1;
        break;
    }

    if (DxioPortPtr->Port.EndpointStatus == (DXIO_ENDPOINT_STATUS)EndpointDetect) {
      SmbiosRecord->CurrentUsage = SlotUsageInUse;
    } else if (DxioPortPtr->Port.EndpointStatus == (DXIO_ENDPOINT_STATUS)EndpointNotPresent) {
      SmbiosRecord->CurrentUsage = SlotUsageAvailable;
    } else {
      SmbiosRecord->CurrentUsage = SlotUsageUnknown;
    }

    SlotNumInfo = DxioPortPtr->Port.SlotNum;
    Status      = SlotBdfInfo (
                    &SlotNumInfo,
                    &SegInfo,
                    &BusInfo,
                    &DevFunInfo
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Could not get SBDF information %r\n", Status));
    }

    SmbiosRecord->SlotLength        = SlotLengthLong;
    SmbiosRecord->SlotID            = DxioPortPtr->Port.SlotNum;
    SmbiosRecord->SegmentGroupNum   = SegInfo;
    SmbiosRecord->BusNum            = BusInfo;
    SmbiosRecord->DevFuncNum        = DevFunInfo;
    SmbiosRecord->PeerGroupingCount = 0;

    *SmbiosRecordPtr = SmbiosRecord;
  }

  return Status;
}

/**
  Parse the port parameters and update their slot numbers

  @param[in, out]  PortDescriptor    Port descriptor to update

  @retval VOID
**/
VOID
GetSlotNumber (
  IN OUT     DXIO_PORT_DESCRIPTOR      *PortDescriptor
  )
{
  PORT_PARAM  *PortParam;

  PortParam = (PORT_PARAM *) &(PortDescriptor->PortParams);
  while (PortParam != NULL && PortParam->ParamType != 0) {
    if (PortParam->ParamType == PP_SLOT_NUM) {
      PortDescriptor->Port.SlotNum = PortParam->ParamValue;
    }
    PortParam++;
  }
}

/**
  Parse the DXIO topology table for PcieEngine's and update
  their slot number

  @param[in, out]  DxioTopologyTablePtr    DXIO Topology Table pointer

  @retval VOID
**/
VOID
UpdateSlotNum (
  IN OUT AMD_CPM_DXIO_TOPOLOGY_TABLE       *DxioTopologyTablePtr
  )
{
  UINTN Count;

  for (Count = 0; Count < AMD_DXIO_PORT_DESCRIPTOR_SIZE; Count++) {
    if (DxioTopologyTablePtr->Port[Count].Flags == DESCRIPTOR_TERMINATE_LIST) {
      break;
    }
    if (!(DxioTopologyTablePtr->Port[Count].EngineData.EngineType == DxioPcieEngine &&
        DxioTopologyTablePtr->Port[Count].Port.PortPresent != DxioPortDisabled)) {
      continue;
    }
    GetSlotNumber (&DxioTopologyTablePtr->Port[Count]);
  }
}

/**
  Get the platform specific System Slot information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  SystemSlotInfo          The System Slot information
  @param[in, out]  SystemSlotCount         Number of System Slot present

  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
GetSystemSlotInfo (
  IN OUT SMBIOS_TABLE_TYPE9  **SystemSlotInfo,
  IN OUT UINTN               *SystemSlotCount
  )
{
  AMD_CPM_TABLE_PROTOCOL       *CpmTableProtocolPtr;
  AMD_CPM_DXIO_TOPOLOGY_TABLE  *DxioTopologyTablePtr2[2];
  AMD_CPM_DXIO_TOPOLOGY_TABLE  *DxioTopologyTableCopyPtr[2];
  EFI_STATUS                   Status;
  UINTN                        SocketIdx;
  UINTN                        DxioPortIdx;
  UINTN                        SlotCount;
  SMBIOS_TABLE_TYPE9           *SlotInfo;
  SMBIOS_TABLE_TYPE9           *SmbiosRecord;

  if ((SystemSlotInfo == NULL) || (SystemSlotCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (
                  &gAmdCpmTableProtocolGuid,
                  NULL,
                  (VOID **)&CpmTableProtocolPtr
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate AmdCpmTableProtocol: %r\n", Status));
    return Status;
  }

  DxioTopologyTablePtr2[0] = NULL;
  DxioTopologyTablePtr2[0] = CpmTableProtocolPtr->CommonFunction.GetTablePtr2 (
                                                                   CpmTableProtocolPtr,
                                                                   CPM_SIGNATURE_DXIO_TOPOLOGY
                                                                   );

  DxioTopologyTablePtr2[1] = NULL;
  DxioTopologyTablePtr2[1] = CpmTableProtocolPtr->CommonFunction.GetTablePtr2 (
                                                                   CpmTableProtocolPtr,
                                                                   CPM_SIGNATURE_DXIO_TOPOLOGY_S1
                                                                 );

  //Shouldn't modify CPM table, so make a copy that we can edit
  DxioTopologyTableCopyPtr[0] = AllocateZeroPool (sizeof(AMD_CPM_DXIO_TOPOLOGY_TABLE));
  DxioTopologyTableCopyPtr[1] = AllocateZeroPool (sizeof(AMD_CPM_DXIO_TOPOLOGY_TABLE));
                
  CopyMem (DxioTopologyTableCopyPtr[0], (VOID*) DxioTopologyTablePtr2[0], sizeof (AMD_CPM_DXIO_TOPOLOGY_TABLE));
  CopyMem (DxioTopologyTableCopyPtr[1], (VOID*) DxioTopologyTablePtr2[1], sizeof (AMD_CPM_DXIO_TOPOLOGY_TABLE));

  //Update Slot Numbers from port params
  for (SocketIdx = 0; SocketIdx < FixedPcdGet32 (PcdAmdNumberOfPhysicalSocket); SocketIdx++ ) {
    if (DxioTopologyTableCopyPtr[SocketIdx] != NULL) {
      UpdateSlotNum (DxioTopologyTableCopyPtr[SocketIdx]);
    }
  }

  // Add Smbios System Slot information for all sockets present.
  SlotCount = 0;
  for (SocketIdx = 0; SocketIdx < FixedPcdGet32 (PcdAmdNumberOfPhysicalSocket); SocketIdx++ ) {
    if (DxioTopologyTableCopyPtr[SocketIdx] != NULL) {
      for (DxioPortIdx = 0; DxioPortIdx < AMD_DXIO_PORT_DESCRIPTOR_SIZE;
           DxioPortIdx++)
      {
        // Check if Slot is present
        if ((DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx].Port.SlotNum > 0) &&
            (DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx].Port.PortPresent == 1))
        {
          SlotCount++;
        }

        // Terminate if last port found.
        if ((DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx].Flags & DESCRIPTOR_TERMINATE_LIST)) {
          break;
        }
      }
    }
  }

  if (SlotCount == 0) {
    return EFI_NOT_FOUND;
  }

  SlotInfo = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE9) * SlotCount);
  if (SlotInfo == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  SlotCount = 0;
  for (SocketIdx = 0; SocketIdx < FixedPcdGet32 (PcdAmdNumberOfPhysicalSocket); SocketIdx++ ) {
    if (DxioTopologyTableCopyPtr[SocketIdx] != NULL) {
      for (DxioPortIdx = 0; DxioPortIdx < AMD_DXIO_PORT_DESCRIPTOR_SIZE;
           DxioPortIdx++)
      {
        // Check if Slot is present
        if ((DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx].Port.SlotNum > 0) &&
            (DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx].Port.PortPresent == 1))
        {
          SmbiosRecord = NULL;
          Status       = CreateSmbiosSystemSlotRecord (
                           &DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx],
                           &SmbiosRecord
                           );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_VERBOSE, "Slot (%d) information not found. Status = %r\n",
              DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx].Port.SlotNum,
              Status
              ));
          } else {
            CopyMem (&SlotInfo[SlotCount], SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE9));
            SlotCount++;
          }
        }

        // Terminate if last port found.
        if ((DxioTopologyTableCopyPtr[SocketIdx]->Port[DxioPortIdx].Flags & 0x80000000)) {
          break;
        }
      }
    }
  }

  FreePool (DxioTopologyTableCopyPtr[0]);
  FreePool (DxioTopologyTableCopyPtr[1]);

  *SystemSlotInfo  = SlotInfo;
  *SystemSlotCount = SlotCount;
  return EFI_SUCCESS;
}
