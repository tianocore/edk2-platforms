/** @file
Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Guid/HobList.h>
#include <Guid/SmramMemoryReserve.h>

#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MmServicesTableLib.h>

extern EFI_GUID  gEfiBootloaderMmHobListGuid;

//
// Cache copy of HobList pointer.
//
STATIC VOID  *gHobList = NULL;

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.
  If the pointer to the HOB list is NULL, then ASSERT().

  @retval      The pointer to the HOB list.
  @retval NULL The pointer to the HobList is not valid.

**/
VOID *
EFIAPI
GetHobList (
  VOID
  )
{
  UINTN  Index;

  //
  // If other libraries uses hoblib Apis in its constructor, GetHobList may be called before HobLibConstructor.
  // Try to get Bootloader Hoblist from configuration table if it has been installed to relieve this problem.
  //
  if (gHobList == NULL) {
    for (Index = 0; Index < gMmst->NumberOfTableEntries; Index++) {
      if (CompareGuid (&gEfiBootloaderMmHobListGuid, &gMmst->MmConfigurationTable[Index].VendorGuid)) {
        gHobList = gMmst->MmConfigurationTable[Index].VendorTable;
        break;
      }
    }
  }

  ASSERT (gHobList != NULL);
  return gHobList;
}

/**
  Returns the next instance of a HOB type from the starting HOB.

  This function searches the first instance of a HOB type from the starting HOB pointer.
  If there does not exist such HOB type from the starting HOB pointer, it will return NULL.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.

  If HobStart is NULL, then ASSERT().

  @param  Type          The HOB type to return.
  @param  HobStart      The starting HOB pointer to search from.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetNextHob (
  IN UINT16      Type,
  IN CONST VOID  *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *)HobStart;
  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == Type) {
      return Hob.Raw;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return NULL;
}

/**
  Returns the first instance of a HOB type among the whole HOB list.

  This function searches the first instance of a HOB type among the whole HOB list.
  If there does not exist such HOB type in the HOB list, it will return NULL.

  If the pointer to the HOB list is NULL, then ASSERT().

  @param  Type          The HOB type to return.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetFirstHob (
  IN UINT16  Type
  )
{
  VOID  *HobList;

  HobList = GetHobList ();
  return GetNextHob (Type, HobList);
}

/**
  Returns the next instance of the matched GUID HOB from the starting HOB.

  This function searches the first instance of a HOB from the starting HOB pointer.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION, and its GUID Name equals to the input Guid.
  If such a HOB from the starting HOB pointer does not exist, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size information, respectively.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.

  If Guid is NULL, then ASSERT().
  If HobStart is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.
  @param  HobStart      A pointer to a Guid.

  @return The next instance of the matched GUID HOB from the starting HOB.

**/
VOID *
EFIAPI
GetNextGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN CONST VOID      *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;

  GuidHob.Raw = (UINT8 *)HobStart;
  while ((GuidHob.Raw = GetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, GuidHob.Raw)) != NULL) {
    if (CompareGuid (Guid, &GuidHob.Guid->Name)) {
      break;
    }

    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }

  if ((GuidHob.Raw == NULL) && (HobStart == GetHobList ())) {
    DEBUG ((DEBUG_WARN, "Bootloader MM: GUIDed HOB [%g] not found in the entire HOB list.\n", Guid));
  }

  return GuidHob.Raw;
}

/**
  Returns the first instance of the matched GUID HOB among the whole HOB list.

  This function searches the first instance of a HOB among the whole HOB list.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If such a HOB from the starting HOB pointer does not exist, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size information, respectively.

  If the pointer to the HOB list is NULL, then ASSERT().
  If Guid is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.

  @return The first instance of the matched GUID HOB among the whole HOB list.

**/
VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID  *Guid
  )
{
  VOID  *HobList;

  HobList = GetHobList ();
  return GetNextGuidHob (Guid, HobList);
}

/**
  Get the system boot mode from the HOB list.

  This function returns the system boot mode information from the
  PHIT HOB in HOB list.

  If the pointer to the HOB list is NULL, then ASSERT().

  @param  VOID

  @return The Boot Mode.

**/
EFI_BOOT_MODE
EFIAPI
GetBootModeHob (
  VOID
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HandOffHob;

  HandOffHob = (EFI_HOB_HANDOFF_INFO_TABLE *)GetHobList ();

  return HandOffHob->BootMode;
}

/**
  Builds a HOB for a loaded PE32 module.

  This function builds a HOB for a loaded PE32 module.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If ModuleName is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().

  @param  ModuleName              The GUID File Name of the module.
  @param  MemoryAllocationModule  The 64 bit physical address of the module.
  @param  ModuleLength            The length of the module in bytes.
  @param  EntryPoint              The 64 bit physical address of the module entry point.

**/
VOID
EFIAPI
BuildModuleHob (
  IN CONST EFI_GUID        *ModuleName,
  IN EFI_PHYSICAL_ADDRESS  MemoryAllocationModule,
  IN UINT64                ModuleLength,
  IN EFI_PHYSICAL_ADDRESS  EntryPoint
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB that describes a chunk of system memory.

  This function builds a HOB that describes a chunk of system memory.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If there is no additional space for HOB creation, then ASSERT().

  @param  ResourceType        The type of resource described by this HOB.
  @param  ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param  PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param  NumberOfBytes       The length of the memory described by this HOB in bytes.

**/
VOID
EFIAPI
BuildResourceDescriptorHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
}

/**
  Builds a GUID HOB with a certain data length.

  This function builds a customized HOB tagged with a GUID for identification
  and returns the start address of GUID HOB data so that caller can fill the customized data.
  The HOB Header and Name field is already stripped.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If Guid is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           DataLength
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  Copies a data buffer to a newly-built HOB.

  This function builds a customized HOB tagged with a GUID for identification,
  copies the input data to the HOB data field and returns the start address of the GUID HOB data.
  The HOB Header and Name field is already stripped.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If Guid is NULL, then ASSERT().
  If Data is NULL and DataLength > 0, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  Data          The data to be copied into the data field of the GUID HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidDataHob (
  IN CONST EFI_GUID  *Guid,
  IN VOID            *Data,
  IN UINTN           DataLength
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  Builds a Firmware Volume HOB.

  This function builds a Firmware Volume HOB.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.

**/
VOID
EFIAPI
BuildFvHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
}

/**
  Builds a EFI_HOB_TYPE_FV2 HOB.

  This function builds a EFI_HOB_TYPE_FV2 HOB.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.
  @param  FvName       The name of the Firmware Volume.
  @param  FileName      The name of the file.

**/
VOID
EFIAPI
BuildFv2Hob (
  IN          EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN          UINT64                Length,
  IN CONST    EFI_GUID              *FvName,
  IN CONST    EFI_GUID              *FileName
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the CPU.

  This function builds a HOB for the CPU.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If there is no additional space for HOB creation, then ASSERT().

  @param  SizeOfMemorySpace   The maximum physical memory addressability of the processor.
  @param  SizeOfIoSpace       The maximum physical I/O addressability of the processor.

**/
VOID
EFIAPI
BuildCpuHob (
  IN UINT8  SizeOfMemorySpace,
  IN UINT8  SizeOfIoSpace
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the memory allocation.

  This function builds a HOB for the memory allocation.
  It can only be invoked by Standalone MM Core.
  For Standalone MM drivers, it will ASSERT() since HOB is read only for Standalone MM drivers.

  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the memory.
  @param  Length        The length of the memory allocation in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildMemoryAllocationHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN EFI_MEMORY_TYPE       MemoryType
  )
{
  //
  // HOB is read only for Standalone MM drivers
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB that describes a chunk of system memory with Owner GUID.

  This function builds a HOB that describes a chunk of system memory.
  If there is no additional space for HOB creation, then ASSERT().

  @param  ResourceType        The type of resource described by this HOB.
  @param  ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param  PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param  NumberOfBytes       The length of the memory described by this HOB in bytes.
  @param  OwnerGUID           GUID for the owner of this resource.

**/
VOID
EFIAPI
BuildResourceDescriptorWithOwnerHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes,
  IN EFI_GUID                     *OwnerGUID
  )
{
  ASSERT (FALSE);
}

/**
  Builds a Capsule Volume HOB.

  This function builds a Capsule Volume HOB.
  If the platform does not support Capsule Volume HOBs, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Capsule Volume.
  @param  Length        The size of the Capsule Volume in bytes.

**/
VOID
EFIAPI
BuildCvHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  ASSERT (FALSE);
}

/**
  Builds a HOB for the BSP store.

  This function builds a HOB for BSP store.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the BSP.
  @param  Length        The length of the BSP store in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildBspStoreHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN EFI_MEMORY_TYPE       MemoryType
  )
{
  ASSERT (FALSE);
}

/**
  Builds a HOB for the Stack.

  This function builds a HOB for the stack.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
EFIAPI
BuildStackHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  ASSERT (FALSE);
}

/**
  The constructor function caches the pointer to HOB list.

  The constructor function gets the start address of HOB list from system configuration table.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle     The firmware allocated handle for the image.
  @param  MmSystemTable   A pointer to the MM System Table.

  @retval EFI_SUCCESS     The constructor successfully gets HobList.
  @retval Other value     The constructor can't get HobList.

**/
EFI_STATUS
EFIAPI
HobLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  VOID                  *MmCoreHobList;
  EFI_PEI_HOB_POINTERS  Hob;
  VOID                  *DstHobPtr;
  UINT64                HobListSize;
  EFI_HOB_GUID_TYPE     *GuidHob;
  EFI_PHYSICAL_ADDRESS  BlContextDataAddr;
  UINT16                BlContextDataSize;
  EFI_PHYSICAL_ADDRESS  Buffer;

  MmCoreHobList     = NULL;
  HobListSize       = 0;
  BlContextDataAddr = 0;
  BlContextDataSize = 0;

  if (gMmst == NULL) {
    ASSERT (gMmst != NULL);
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < gMmst->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiBootloaderMmHobListGuid, &gMmst->MmConfigurationTable[Index].VendorGuid)) {
      gHobList = gMmst->MmConfigurationTable[Index].VendorTable;
      break;
    }
  }

  //
  // Directly return if the Bootloader MM Hoblist has been initialized.
  //
  if (gHobList != NULL) {
    return EFI_SUCCESS;
  }

  //
  // Get Mm Hoblist from Mm service table.
  //
  for (Index = 0; Index < gMmst->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiHobListGuid, &gMmst->MmConfigurationTable[Index].VendorGuid)) {
      MmCoreHobList = gMmst->MmConfigurationTable[Index].VendorTable;
      break;
    }
  }

  if (MmCoreHobList == NULL) {
    ASSERT (MmCoreHobList != NULL);
    return EFI_UNSUPPORTED;
  }

  //
  // Locate the Bootloader FV Context Data HOB from FSP MM HobList.
  //
  GuidHob = GetNextGuidHob (&gFspSmmBootLoaderFvContextHobGuid, MmCoreHobList);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    BlContextDataAddr = (EFI_PHYSICAL_ADDRESS)GET_GUID_HOB_DATA (GuidHob);
    BlContextDataSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
    HobListSize      += BlContextDataSize;
  }

  //
  // Collect the size of memory required for memory allocation hobs.
  //
  Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, MmCoreHobList);
  while (Hob.Raw != NULL) {
    if (!IsZeroGuid (&Hob.MemoryAllocation->AllocDescriptor.Name)) {
      HobListSize += Hob.Header->HobLength;

      Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (Hob));
    }
  }

  //
  // Collect the size of memory required for resource descriptor hobs.
  //
  Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, MmCoreHobList);
  while (Hob.Raw != NULL) {
    HobListSize += Hob.Header->HobLength;
    Hob.Raw      = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, GET_NEXT_HOB (Hob));
  }

  //
  // If can't locate Bootloader context data or its data is 0, we need to add a END_OF_HOB_LIST hob.
  //
  if ((BlContextDataAddr == 0) || (BlContextDataSize == 0)) {
    HobListSize += ALIGN_VALUE (sizeof (EFI_HOB_GENERIC_HEADER), 8);
  }

  //
  // Allocate and initialize Bootloader MM HobList based on Bootloader MM context data.
  //
  Status = gMmst->MmAllocatePages (
                    AllocateAnyPages,
                    EfiRuntimeServicesData,
                    EFI_SIZE_TO_PAGES (HobListSize),
                    &Buffer
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  gHobList = (VOID *)(UINTN)Buffer;

  //
  // 1. Locate and copy the the memory allocation hobs that used by Bootloader MM modules.
  //
  DstHobPtr = (VOID *)(UINTN)Buffer;
  Hob.Raw   = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, MmCoreHobList);
  while (Hob.Raw != NULL) {
    if (!IsZeroGuid (&Hob.MemoryAllocation->AllocDescriptor.Name)) {
      CopyMem (DstHobPtr, Hob.Raw, Hob.Header->HobLength);
      DstHobPtr = (VOID *)((UINT8 *)DstHobPtr + Hob.Header->HobLength);

      Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (Hob));
    }
  }

  //
  // 2. Locate and copy the the Resouce descriptor hobs that used by Bootloader MM modules.
  //
  Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, MmCoreHobList);
  while (Hob.Raw != NULL) {
    CopyMem (DstHobPtr, Hob.Raw, Hob.Header->HobLength);
    DstHobPtr = (VOID *)((UINT8 *)DstHobPtr + Hob.Header->HobLength);
    Hob.Raw   = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, GET_NEXT_HOB (Hob));
  }

  //
  // 3. Copy the Bootloader Context Data that used by Bootloader MM modules.
  //
  if ((BlContextDataAddr == 0) || (BlContextDataSize == 0)) {
    //
    // Build the End of Hob List if BlContextData is not provided by bootloader.
    //
    ((EFI_HOB_GENERIC_HEADER *)((UINT8 *)DstHobPtr))->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
    ((EFI_HOB_GENERIC_HEADER *)((UINT8 *)DstHobPtr))->HobLength = (UINT16)ALIGN_VALUE (sizeof (EFI_HOB_GENERIC_HEADER), 8);
    ((EFI_HOB_GENERIC_HEADER *)((UINT8 *)DstHobPtr))->Reserved  = 0;
  } else {
    CopyMem (DstHobPtr, (VOID *)(UINTN)BlContextDataAddr, BlContextDataSize);
  }

  gMmst->MmInstallConfigurationTable (gMmst, &gEfiBootloaderMmHobListGuid, gHobList, HobListSize);

  return EFI_SUCCESS;
}
