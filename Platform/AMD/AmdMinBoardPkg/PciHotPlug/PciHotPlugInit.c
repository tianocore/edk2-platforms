/** @file
  This file declares EFI PCI Hot Plug Init Protocol.

  This protocol provides the necessary functionality to initialize the Hot Plug
  Controllers (HPCs) and the buses that they control. This protocol also provides
  information regarding resource padding.

  @par Note:
    This source has the reference of OVMF PciHotPluginit.c and Intel platform PciHotPlug.c.

    This protocol is required only on platforms that support one or more PCI Hot
    Plug* slots or CardBus sockets.

  The EFI_PCI_HOT_PLUG_INIT_PROTOCOL provides a mechanism for the PCI bus enumerator
  to properly initialize the HPCs and CardBus sockets that require initialization.
  The HPC initialization takes place before the PCI enumeration process is complete.
  There cannot be more than one instance of this protocol in a system. This protocol
  is installed on its own separate handle.

  Because the system may include multiple HPCs, one instance of this protocol
  should represent all of them. The protocol functions use the device path of
  the HPC to identify the HPC. When the PCI bus enumerator finds a root HPC, it
  will call EFI_PCI_HOT_PLUG_INIT_PROTOCOL.InitializeRootHpc(). If InitializeRootHpc()
  is unable to initialize a root HPC, the PCI enumerator will ignore that root HPC
  and continue the enumeration process. If the HPC is not initialized, the devices
  that it controls may not be initialized, and no resource padding will be provided.

  From the standpoint of the PCI bus enumerator, HPCs are divided into the following
  two classes:

    - Root HPC:
        These HPCs must be initialized by calling InitializeRootHpc() during the
        enumeration process. These HPCs will also require resource padding. The
        platform code must have a priori knowledge of these devices and must know
        how to initialize them. There may not be any way to access their PCI
        configuration space before the PCI enumerator programs all the upstream
        bridges and thus enables the path to these devices. The PCI bus enumerator
        is responsible for determining the PCI bus address of the HPC before it
        calls InitializeRootHpc().
    - Nonroot HPC:
        These HPCs will not need explicit initialization during enumeration process.
        These HPCs will require resource padding. The platform code does not have
        to have a priori knowledge of these devices.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2016, Red Hat, Inc.<BR>
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This Protocol is defined in UEFI Platform Initialization Specification 1.2
  Volume 5: Standards

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/PciHotPlugInit.h>

//
// The protocol interface this driver produces.
//
STATIC EFI_PCI_HOT_PLUG_INIT_PROTOCOL  mPciHotPlugInit;

/**
  Returns a list of root Hot Plug Controllers (HPCs) that require initialization
  during the boot process.

  This procedure returns a list of root HPCs. The PCI bus driver must initialize
  these controllers during the boot process. The PCI bus driver may or may not be
  able to detect these HPCs. If the platform includes a PCI-to-CardBus bridge, it
  can be included in this list if it requires initialization.  The HpcList must be
  self consistent. An HPC cannot control any of its parent buses. Only one HPC can
  control a PCI bus. Because this list includes only root HPCs, no HPC in the list
  can be a child of another HPC. This policy must be enforced by the
  EFI_PCI_HOT_PLUG_INIT_PROTOCOL.   The PCI bus driver may not check for such
  invalid conditions.  The callee allocates the buffer HpcList

  @param[in]  This       Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL instance.
  @param[out] HpcCount   The number of root HPCs that were returned.
  @param[out] HpcList    The list of root HPCs. HpcCount defines the number of
                         elements in this list.

  @retval EFI_SUCCESS             HpcList was returned.
  @retval EFI_INVALID_PARAMETER   HpcCount is NULL or HpcList is NULL.

**/
EFI_STATUS
EFIAPI
GetRootHpcList (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL  *This,
  OUT UINTN                           *HpcCount,
  OUT EFI_HPC_LOCATION                **HpcList
  )
{
  if ((HpcCount == NULL) || (HpcList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Platform BIOS not doing any extra/special HPC initialization
  // Hence returning the HpcCount as zero and HpcList as NULL
  //
  *HpcCount = 0;
  *HpcList  = NULL;

  return EFI_SUCCESS;
}

/**
  Initializes one root Hot Plug Controller (HPC). This process may causes
  initialization of its subordinate buses.

  This function initializes the specified HPC. At the end of initialization,
  the hot-plug slots or sockets (controlled by this HPC) are powered and are
  connected to the bus. All the necessary registers in the HPC are set up. For
  a Standard (PCI) Hot Plug Controller (SHPC), the registers that must be set
  up are defined in the PCI Standard Hot Plug Controller and Subsystem
  Specification.

  @param[in]  This            Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL instance.
  @param[in]  HpcDevicePath   The device path to the HPC that is being initialized.
  @param[in]  HpcPciAddress   The address of the HPC function on the PCI bus.
  @param[in]  Event           The event that should be signaled when the HPC
                              initialization is complete.  Set to NULL if the
                              caller wants to wait until the entire initialization
                              process is complete.
  @param[out] HpcState        The state of the HPC hardware. The state is
                              EFI_HPC_STATE_INITIALIZED or EFI_HPC_STATE_ENABLED.

  @retval EFI_UNSUPPORTED         This instance of EFI_PCI_HOT_PLUG_INIT_PROTOCOL
                                  does not support the specified HPC.
  @retval EFI_INVALID_PARAMETER   HpcState is NULL.

**/
EFI_STATUS
EFIAPI
InitializeRootHpc (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL *HpcDevicePath,
  IN  UINT64 HpcPciAddress,
  IN  EFI_EVENT Event, OPTIONAL
  OUT EFI_HPC_STATE                   *HpcState
  )
{
  if (HpcState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // HPC is initialized by respective platform PI modules.
  // Hence no extra initialization required.
  //
  return EFI_UNSUPPORTED;
}

/**
  Returns the resource padding that is required by the PCI bus that is controlled
  by the specified Hot Plug Controller (HPC).

  This function returns the resource padding that is required by the PCI bus that
  is controlled by the specified HPC. This member function is called for all the
  root HPCs and nonroot HPCs that are detected by the PCI bus enumerator. This
  function will be called before PCI resource allocation is completed. This function
  must be called after all the root HPCs, with the possible exception of a
  PCI-to-CardBus bridge, have completed initialization.

  @param[in]  This            Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL instance.
  @param[in]  HpcDevicePath   The device path to the HPC.
  @param[in]  HpcPciAddress   The address of the HPC function on the PCI bus.
  @param[out] HpcState        The state of the HPC hardware.
  @param[out] Padding         The amount of resource padding that is required by the
                              PCI bus under the control of the specified HPC.
  @param[out] Attributes      Describes how padding is accounted for. The padding
                              is returned in the form of ACPI 2.0 resource descriptors.

  @retval EFI_SUCCESS             The resource padding was successfully returned.
  @retval EFI_INVALID_PARAMETER   HpcState or Padding or Attributes is NULL.
  @retval EFI_OUT_OF_RESOURCES    ACPI 2.0 resource descriptors for Padding
                                  cannot be allocated due to insufficient resources.

**/
EFI_STATUS
EFIAPI
GetResourcePadding (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL  *This,
  IN  EFI_DEVICE_PATH_PROTOCOL        *HpcDevicePath,
  IN  UINT64                          HpcPciAddress,
  OUT EFI_HPC_STATE                   *HpcState,
  OUT VOID                            **Padding,
  OUT EFI_HPC_PADDING_ATTRIBUTES      *Attributes
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *PaddingResource;

  if ((HpcState == NULL) || (Padding == NULL) || (Attributes == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Need total 5 resources
  // 1 - IO resource
  // 2 - Mem resource
  // 3 - PMem resource
  // 4 - Bus resource
  // 5 - end tag resource
  PaddingResource = AllocateZeroPool (4 * sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
  if (PaddingResource == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Padding = (VOID *)PaddingResource;

  //
  // Padding for bus
  //
  *Attributes = EfiPaddingPciBus;

  PaddingResource->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
  PaddingResource->Len  = (UINT16)(
                                   sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) -
                                   OFFSET_OF (
                                     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR,
                                     ResType
                                     )
                                   );
  PaddingResource->ResType      = ACPI_ADDRESS_SPACE_TYPE_BUS;
  PaddingResource->GenFlag      = 0x0;
  PaddingResource->SpecificFlag = 0;
  PaddingResource->AddrRangeMin = 0;
  PaddingResource->AddrRangeMax = 0;
  PaddingResource->AddrLen      = PcdGet8 (PcdPciHotPlugResourcePadBus);

  //
  // Padding for non-prefetchable memory
  //
  PaddingResource++;
  PaddingResource->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
  PaddingResource->Len  = (UINT16)(
                                   sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) -
                                   OFFSET_OF (
                                     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR,
                                     ResType
                                     )
                                   );
  PaddingResource->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
  PaddingResource->GenFlag              = 0x0;
  PaddingResource->AddrSpaceGranularity = 32;
  PaddingResource->SpecificFlag         = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_NON_CACHEABLE;
  PaddingResource->AddrRangeMin         = 0;
  PaddingResource->AddrLen              = (UINT64)PcdGet32 (PcdPciHotPlugResourcePadMem);
  PaddingResource->AddrRangeMax         = PaddingResource->AddrLen - 1;

  //
  // Padding for prefetchable memory
  //
  PaddingResource++;
  PaddingResource->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
  PaddingResource->Len  = (UINT16)(
                                   sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) -
                                   OFFSET_OF (
                                     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR,
                                     ResType
                                     )
                                   );
  PaddingResource->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
  PaddingResource->GenFlag              = 0x0;
  PaddingResource->AddrSpaceGranularity = 32;
  PaddingResource->SpecificFlag         = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
  PaddingResource->AddrLen              = (UINT64)PcdGet32 (PcdPciHotPlugResourcePadPMem);
  PaddingResource->AddrRangeMax         = PaddingResource->AddrLen - 1;

  //
  // Padding for I/O
  //
  PaddingResource++;
  PaddingResource->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
  PaddingResource->Len  = (UINT16)(
                                   sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) -
                                   OFFSET_OF (
                                     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR,
                                     ResType
                                     )
                                   );
  PaddingResource->ResType      = ACPI_ADDRESS_SPACE_TYPE_IO;
  PaddingResource->GenFlag      = 0x0;
  PaddingResource->SpecificFlag = 0;
  PaddingResource->AddrRangeMin = 0;
  PaddingResource->AddrLen      = (UINT64)PcdGet32 (PcdPciHotPlugResourcePadIo);
  PaddingResource->AddrRangeMax = PaddingResource->AddrLen - 1;

  //
  // Terminate the entries.
  //
  PaddingResource++;
  ((EFI_ACPI_END_TAG_DESCRIPTOR *)PaddingResource)->Desc     = ACPI_END_TAG_DESCRIPTOR;
  ((EFI_ACPI_END_TAG_DESCRIPTOR *)PaddingResource)->Checksum = 0x0;

  *HpcState = EFI_HPC_STATE_INITIALIZED | EFI_HPC_STATE_ENABLED;

  return EFI_SUCCESS;
}

/**
  Entry point for this driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to SystemTable.

  @retval EFI_SUCCESS      Driver has loaded successfully.
  @return                  Error codes from lower level functions.

**/
EFI_STATUS
EFIAPI
PciHotPlugInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mPciHotPlugInit.GetRootHpcList     = GetRootHpcList;
  mPciHotPlugInit.InitializeRootHpc  = InitializeRootHpc;
  mPciHotPlugInit.GetResourcePadding = GetResourcePadding;
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gEfiPciHotPlugInitProtocolGuid,
                &mPciHotPlugInit,
                NULL
                );
}
