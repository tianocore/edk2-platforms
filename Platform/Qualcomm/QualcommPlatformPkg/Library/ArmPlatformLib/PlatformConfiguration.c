/** @file
  Parser for UEFI platform configuration data.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RamPartitionTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiLib.h>
#include <Library/SortLib.h>

#include <PlatformConfiguration.h>

#define MAX_MEMORY_REGIONS  125

STATIC MEM_REGION_INFO  *mMemRegions        = NULL;
STATIC UINTN            mNumMemRegions      = 0;
STATIC UINTN            mNumMemoryMapRegion = 0;
STATIC UINT64           mMemMapLow          = 0xFFFFFFFFFFFFFFFFULL;
STATIC UINT64           mMemMapHigh         = 0;
STATIC BOOLEAN          mIsMemMapHighNoMap  = FALSE;

/**
  Allocates and zeros a buffer.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID*
EFIAPI
AllocateZeroPoolNoFree (
  IN UINTN  AllocationSize
  )
{
  VOID  *Memory;

  Memory = AllocatePages (EFI_SIZE_TO_PAGES (AllocationSize));
  if (Memory != NULL) {
    ZeroMem (Memory, AllocationSize);
  }

  return Memory;
}

/**
  Allocate memory without freeing to service multiple small allocation.
  This reduces number of HOBs.

  Memory is allocated from a static page-aligned pool and is never freed.

  @param[in]  Size  Number of bytes to allocate.

  @return  Pointer to allocated memory or NULL if allocation fails.

**/
STATIC VOID *
AllocateMemNoFree (
  UINTN  Size
  )
{
  STATIC UINT8  *FreeBufferPtr;
  STATIC UINT8  *EndPtr;
  UINT8         *AllocatedPtr;

  if (Size == 0) {
    return NULL;
  }

  if (Size >= EFI_PAGE_SIZE) {
    return AllocateZeroPoolNoFree (Size);
  }

  if (FreeBufferPtr == NULL) {
    if ((FreeBufferPtr = AllocateZeroPoolNoFree (EFI_PAGE_SIZE)) == NULL) {
      DEBUG ((DEBUG_WARN, "MemoryAlloc failed\n"));
      return NULL;
    }

    EndPtr = FreeBufferPtr + EFI_PAGE_SIZE;
  }

  Size = (Size + 7) & (~7);
  if (FreeBufferPtr + Size > EndPtr) {
    if ((FreeBufferPtr = AllocateZeroPoolNoFree (EFI_PAGE_SIZE)) == NULL) {
      DEBUG ((DEBUG_WARN, "MemoryAlloc failed\n"));
      ASSERT (FreeBufferPtr != NULL);
      return NULL;
    }

    EndPtr = FreeBufferPtr + EFI_PAGE_SIZE;
  }

  AllocatedPtr   = FreeBufferPtr;
  FreeBufferPtr += Size;
  return AllocatedPtr;
}

/**
  Get configuration memory map bounds.

  Calculates and stores the lowest and highest addresses from the
  configured memory map in global variables.

**/
STATIC VOID
GetConfigurationMemMapBounds (
  VOID
  )
{
  UINT64  Start;
  UINT64  End;
  UINTN   Index;

  for (Index = 0; Index < mNumMemRegions; Index++) {
    Start = mMemRegions[Index].MemBase;
    End   = Start + mMemRegions[Index].MemSize;
    if (Start < mMemMapLow) {
      mMemMapLow = Start;
    }

    if (End > mMemMapHigh) {
      mMemMapHigh        = End;
      mIsMemMapHighNoMap = FALSE;
      if (mMemRegions[Index].BuildHobOption == NoMap) {
        mIsMemMapHighNoMap = TRUE;
      }
    }
  }
}

/**
  Add non-FD memory regions.

  Adds memory regions from RAM partition table that are not part of
  the firmware device (FD) region to the memory map.

  @param[in]  EntryCount  Number of entries in the entry table.
  @param[in]  EntryTable  Pointer to array of memory region entries.

  @retval  EFI_SUCCESS  Non-FD regions added successfully.

**/
EFI_STATUS
AddNonFdRegions (
  UINTN            EntryCount,
  MEM_REGION_INFO  *EntryTable
  )
{
  UINT64  RegionEndAddr;
  UINT64  DDRMemSize;
  UINTN   EntryIndex;

  DDRMemSize    = 0;

  for (EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++) {
    DDRMemSize += EntryTable[EntryIndex].MemSize;

    RegionEndAddr = EntryTable[EntryIndex].MemBase +  EntryTable[EntryIndex].MemSize;
    if ((EntryTable[EntryIndex].MemBase < mMemMapHigh) && (RegionEndAddr >= mMemMapLow)) {
      continue;
    }

    // Update segment properties which are common to all cases.
    AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, EntryTable[EntryIndex].Name);
    mMemRegions[mNumMemRegions].ResourceType      = EntryTable[EntryIndex].ResourceType;
    mMemRegions[mNumMemRegions].ResourceAttribute = EntryTable[EntryIndex].ResourceAttribute;
    mMemRegions[mNumMemRegions].MemoryType        = EntryTable[EntryIndex].MemoryType;
    mMemRegions[mNumMemRegions].CacheAttributes   = EntryTable[EntryIndex].CacheAttributes;
    mMemRegions[mNumMemRegions].MemBase           = EntryTable[EntryIndex].MemBase;
    mMemRegions[mNumMemRegions].MemSize           = EntryTable[EntryIndex].MemSize;
    mMemRegions[mNumMemRegions].BuildHobOption    = EntryTable[EntryIndex].BuildHobOption;

    mNumMemRegions++;
  }

  return EFI_SUCCESS;
}

/**
  Add remainder of non-FD region.

  Adds the remaining portion of a memory bank that extends beyond
  the configured memory map high address.

  @retval  EFI_SUCCESS        Remainder added successfully.
  @retval  EFI_LOAD_ERROR     Error occurred during addition.
  @retval  EFI_NOT_FOUND      Region not found (may be acceptable).

**/
EFI_STATUS
AddNonFdRegionRemainder (
  VOID
  )
{
  MEM_REGION_INFO  FdRegion;
  MEM_REGION_INFO  NonFdRegion;
  UINT64           RegionEndAddr;
  EFI_STATUS       Status;
  UINT64           UefiFdBase;

  RegionEndAddr = 0;
  UefiFdBase    = FixedPcdGet64 (PcdFdBaseAddress);

  Status = RamPartitionGetPartitionEntryByAddr (UefiFdBase, &FdRegion);
  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  Status = RamPartitionGetPartitionEntryByAddr(mMemMapHigh, &NonFdRegion);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) && mIsMemMapHighNoMap) {
      return EFI_SUCCESS;
    }

    return EFI_LOAD_ERROR;
  }

  if (FdRegion.MemBase == NonFdRegion.MemBase) {
    return EFI_SUCCESS;
  }

  RegionEndAddr = NonFdRegion.MemBase +  NonFdRegion.MemSize;

  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, NonFdRegion.Name);

  mMemRegions[mNumMemRegions].MemBase           = mMemMapHigh;
  mMemRegions[mNumMemRegions].MemSize           = (RegionEndAddr - mMemMapHigh);
  mMemRegions[mNumMemRegions].BuildHobOption    = NonFdRegion.BuildHobOption;
  mMemRegions[mNumMemRegions].ResourceType      = NonFdRegion.ResourceType;
  mMemRegions[mNumMemRegions].ResourceAttribute = NonFdRegion.ResourceAttribute;
  mMemRegions[mNumMemRegions].MemoryType        = NonFdRegion.MemoryType;
  mMemRegions[mNumMemRegions].CacheAttributes   = NonFdRegion.CacheAttributes;

  mNumMemRegions++;

  return EFI_SUCCESS;
}

/**
  Add remainder of FD region.

  Adds the portions of the memory bank containing the firmware device
  region that are outside the configured memory map bounds.

  @retval  EFI_SUCCESS     FD remainder added successfully.
  @retval  EFI_LOAD_ERROR  Error occurred during addition.

**/
EFI_STATUS
AddFdRegionRemainder (
  VOID
  )
{
  MEM_REGION_INFO  FdRegion;
  UINT64           RegionEndAddr;
  EFI_STATUS       Status;
  UINT64           UefiFdBase;

  UefiFdBase    = FixedPcdGet64 (PcdFdBaseAddress);

  Status = RamPartitionGetPartitionEntryByAddr (UefiFdBase, &FdRegion);
  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  RegionEndAddr = FdRegion.MemBase +  FdRegion.MemSize;

  if (mMemMapLow > FdRegion.MemBase) {
    AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, FdRegion.Name);

    mMemRegions[mNumMemRegions].MemBase           = FdRegion.MemBase;
    mMemRegions[mNumMemRegions].MemSize           = (mMemMapLow - FdRegion.MemBase);
    mMemRegions[mNumMemRegions].BuildHobOption    = FdRegion.BuildHobOption;
    mMemRegions[mNumMemRegions].ResourceType      = FdRegion.ResourceType;
    mMemRegions[mNumMemRegions].ResourceAttribute = FdRegion.ResourceAttribute;
    mMemRegions[mNumMemRegions].MemoryType        = FdRegion.MemoryType;
    mMemRegions[mNumMemRegions].CacheAttributes   = FdRegion.CacheAttributes;

    mNumMemRegions++;
  }

  if (mMemMapHigh < RegionEndAddr) {
    AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, FdRegion.Name);

    mMemRegions[mNumMemRegions].MemBase           = mMemMapHigh;
    mMemRegions[mNumMemRegions].MemSize           = (RegionEndAddr - mMemMapHigh);
    mMemRegions[mNumMemRegions].BuildHobOption    = FdRegion.BuildHobOption;
    mMemRegions[mNumMemRegions].ResourceType      = FdRegion.ResourceType;
    mMemRegions[mNumMemRegions].ResourceAttribute = FdRegion.ResourceAttribute;
    mMemRegions[mNumMemRegions].MemoryType        = FdRegion.MemoryType;
    mMemRegions[mNumMemRegions].CacheAttributes   = FdRegion.CacheAttributes;

    mNumMemRegions++;
  }

  return EFI_SUCCESS;
}

/**
  Update dynamic memory regions.

  Updates memory regions marked as AddDynamicMem based on RAM partition
  table information, converting them to conventional memory where appropriate.

  @retval  EFI_SUCCESS  Dynamic regions updated successfully.
  @retval  Other        Error occurred during update.

**/
STATIC EFI_STATUS EFIAPI
UpdateDynamicMemoryRegions (
  VOID
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  RamPartTableEntry[RAM_NUM_PART_ENTRIES];
  UINTN            RamPartTableEntryCount;
  UINT32           MemRgnCnt;

  RamPartTableEntryCount = RAM_NUM_PART_ENTRIES;
  Status = RamPartitionGetRamPartitions (&RamPartTableEntryCount, RamPartTableEntry);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  /* Parse through the memory region and check for the region with AddDynamicMem one */
  for (MemRgnCnt = 0; MemRgnCnt < mNumMemRegions; MemRgnCnt++) {
    BUILD_HOB_OPTION_TYPE  HobValue;

    HobValue = mMemRegions[MemRgnCnt].BuildHobOption;
    if (HobValue == AddDynamicMem) {
      /* Check if the RamPartitiontable hole is less than carved out */
      UINT32  RamPartIndex;

      for (RamPartIndex = 0; RamPartIndex < RamPartTableEntryCount; RamPartIndex++) {
        UINT64  RamPartitionEntryEndAddress;

        RamPartitionEntryEndAddress = RamPartTableEntry[RamPartIndex].MemBase + RamPartTableEntry[RamPartIndex].MemSize;
        if ((RamPartitionEntryEndAddress >= mMemRegions[MemRgnCnt].MemBase) &&
            (RamPartitionEntryEndAddress <= mMemRegions[MemRgnCnt].MemBase + mMemRegions[MemRgnCnt].MemSize))
        {
          /* Update if requred pMemRegion size and allocate rest of the reserved memory space as Conventional memory */
          UINT64  HoleSize;

          HoleSize = mMemRegions[MemRgnCnt].MemBase + mMemRegions[MemRgnCnt].MemSize - RamPartitionEntryEndAddress;
          if (HoleSize < mMemRegions[MemRgnCnt].MemSize) {
            /* Update MemRegion size and attributes */
            mMemRegions[MemRgnCnt].MemBase           = mMemRegions[MemRgnCnt].MemBase;
            mMemRegions[MemRgnCnt].MemSize           = mMemRegions[MemRgnCnt].MemSize - HoleSize;
            mMemRegions[MemRgnCnt].BuildHobOption    = (BUILD_HOB_OPTION_TYPE)AddMem;
            mMemRegions[MemRgnCnt].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
            mMemRegions[MemRgnCnt].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
            mMemRegions[MemRgnCnt].MemoryType        = (EFI_MEMORY_TYPE)EfiConventionalMemory;
            mMemRegions[MemRgnCnt].CacheAttributes   = (ARM_MEMORY_REGION_ATTRIBUTES)ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
          }

          break;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Identify and report upper RAM partitions to the HOB list.

  Add the highest RAM aprtition table entry as avilable memory.

  @retval EFI_SUCCESS           Upper memory partitions were successfully
                                identified and reported.
  @retval EFI_NOT_FOUND         No RAM partitions were found in the
                                platform configuration.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources for HOB creation.

**/
EFI_STATUS
AddUpperMemoryFromRamPartitions (
  VOID
  )
{
  MEM_REGION_INFO  EntryList[RAM_NUM_PART_ENTRIES];
  UINTN            EntryCount;
  UINT64           HighestBase;
  UINT64           HighestSize;
  UINTN            Index;
  UINTN            HighestIndex;
  EFI_STATUS       Status;

  EntryCount = RAM_NUM_PART_ENTRIES;

  Status = RamPartitionGetRamPartitions (&EntryCount, &EntryList[0]);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /* Find the uppermost RAM partition entry */
  HighestBase  = 0;
  HighestSize  = 0;
  HighestIndex = EntryCount; /* sentinel: no match */

  for (Index = 0; Index < EntryCount; Index++) {
    if (EntryList[Index].MemBase > HighestBase) {
      HighestBase  = EntryList[Index].MemBase;
      HighestSize  = EntryList[Index].MemSize;
      HighestIndex = Index;
    }
  }

  if (HighestIndex == EntryCount) {
    return EFI_NOT_FOUND;
  }

  if (mNumMemRegions >= MAX_MEMORY_REGIONS) {
    ASSERT (mNumMemRegions < MAX_MEMORY_REGIONS);
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, EntryList[HighestIndex].Name);
  mMemRegions[mNumMemRegions].MemBase           = HighestBase;
  mMemRegions[mNumMemRegions].MemSize           = HighestSize;
  mMemRegions[mNumMemRegions].BuildHobOption    = NoBuildHob;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
  mMemRegions[mNumMemRegions].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
  mMemRegions[mNumMemRegions].MemoryType        = (EFI_MEMORY_TYPE)EfiConventionalMemory;
  mMemRegions[mNumMemRegions].CacheAttributes   = (ARM_MEMORY_REGION_ATTRIBUTES)ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  mNumMemRegions++;

  /* Manually build the ResourceDescriptorHob so DXE discovers this memory,
 * without a MemoryAllocationHob that would expose it to the PEI allocator */
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES,
    HighestBase,
    HighestSize
    );

  return EFI_SUCCESS;
}

/**
  Update system memory regions.

  Adds all banks from RAM partition table, as well as remainder of
  bank containing FD region to the memory map table.

  @retval  EFI_SUCCESS  System memory regions updated successfully.
  @retval  Other        Error occurred during update.

**/
EFI_STATUS
EFIAPI
UpdateSystemMemoryRegions (
  VOID
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  EntryList[RAM_NUM_PART_ENTRIES];
  UINTN            EntryCount;

  EntryCount = RAM_NUM_PART_ENTRIES;

  Status = RamPartitionGetRamPartitions (&EntryCount, &EntryList[0]);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = AddNonFdRegions (EntryCount, &EntryList[0]);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = AddFdRegionRemainder ();
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = AddNonFdRegionRemainder ();
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = UpdateDynamicMemoryRegions ();
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  return Status;
}

/**
  Load and parse platform configuration.

  Parses UEFI platform configuration data (memory map, register map,
  and configuration parameters) from device tree and stores the
  information for access by other UEFI modules.

  @retval  EFI_SUCCESS     Configuration loaded and parsed successfully.
  @retval  EFI_LOAD_ERROR  Error occurred during parsing.

**/
EFI_STATUS
EFIAPI
LoadAndParsePlatformCfg (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Load static platform configuration.

  Populates the memory region table with a minimal static memory map
  using PCD values when device tree is not available. The UEFI FD region
  is added as the sole memory-map entry (used for RAM partition validation
  and memory bounds calculation), followed by peripheral regions for SMEM,
  UART, and IMEM cookies.

  @retval  EFI_SUCCESS          Static configuration loaded successfully.
  @retval  EFI_OUT_OF_RESOURCES Memory allocation failed.

**/
EFI_STATUS
EFIAPI
LoadStaticPlatformCfg (
  VOID
  )
{
  mMemRegions = (MEM_REGION_INFO *)AllocateMemNoFree (sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS);
  if (mMemRegions == NULL) {
    DEBUG ((DEBUG_ERROR, "LoadStaticPlatformCfg: failed to allocate memory table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (mMemRegions, sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS, 0);
  mNumMemRegions = 0;

  /* ------------------------------------------------------------------- */
  /* Memory-map region: UEFI FD (system memory where firmware lives)     */
  /*                                                                     */
  /* ArmPlatformSetupDebugBuffer() has already built an                  */
  /* EFI_RESOURCE_MEMORY_RESERVED HOB for the T32 DDR buffer at          */
  /* [PcdTrace32DdrBase, PcdTrace32DdrBase + PcdTrace32DdrSize).         */
  /* That region falls within the UEFI FD range, so we must NOT emit a   */
  /* single system-memory descriptor covering the whole UEFI region.     */
  /* ------------------------------------------------------------------- */

  /* Lower portion: FD base up to (but not including) the T32 buffer */
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES,
    FixedPcdGet64 (PcdFdBaseAddress),
    FixedPcdGet64 (PcdTrace32DdrBase) - FixedPcdGet64 (PcdFdBaseAddress)
    );

  /* Upper portion: above the T32 buffer to the end of the UEFI region */
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES,
    FixedPcdGet64 (PcdTrace32DdrBase) + FixedPcdGet64 (PcdTrace32DdrSize),
    (FixedPcdGet64 (PcdFdBaseAddress) + FixedPcdGet64 (PcdSystemMemorySize))
    - (FixedPcdGet64 (PcdTrace32DdrBase) + FixedPcdGet64 (PcdTrace32DdrSize))
    );

  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "UEFI FD");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdFdBaseAddress);
  mMemRegions[mNumMemRegions].MemSize           = FixedPcdGet64 (PcdSystemMemorySize);
  mMemRegions[mNumMemRegions].BuildHobOption    = NoBuildHob;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
  mMemRegions[mNumMemRegions].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
  mMemRegions[mNumMemRegions].MemoryType        = EfiBootServicesData;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  mNumMemRegions++;

  /* mNumMemoryMapRegion tracks only memory-map entries (not register/peripheral
   * entries) so that ValidateParsedMemoryRegions() only checks RAM regions. */
  mNumMemoryMapRegion = mNumMemRegions;

  /* Calculate mMemMapLow / mMemMapHigh from the memory-map entries only.
   * This must be done before appending peripheral entries so that
   * UpdateSystemMemoryRegions() uses the correct FD region bounds. */
  GetConfigurationMemMapBounds ();

  /* ------------------------------------------------------------------ */
  /* Register/peripheral regions (not validated against RAM partitions) */
  /* ------------------------------------------------------------------ */

  /* SMEM - shared memory (uncached) */
  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "SMEM");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdSmemBaseAddress);
  mMemRegions[mNumMemRegions].MemSize           = FixedPcdGet64 (PcdSmemSize);
  mMemRegions[mNumMemRegions].BuildHobOption    = AddPeripheral;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_MEMORY_MAPPED_IO;
  mMemRegions[mNumMemRegions].ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
  mMemRegions[mNumMemRegions].MemoryType        = EfiMemoryMappedIO;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
  mNumMemRegions++;

  /* Serial Port */
  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "UART");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdSerialRegisterBase);
  mMemRegions[mNumMemRegions].MemSize           = EFI_PAGE_SIZE;
  mMemRegions[mNumMemRegions].BuildHobOption    = AddPeripheral;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_MEMORY_MAPPED_IO;
  mMemRegions[mNumMemRegions].ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
  mMemRegions[mNumMemRegions].MemoryType        = EfiMemoryMappedIO;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  mNumMemRegions++;

  /* IMEM Cookies */
  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "IMEM");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdIMemCookiesBase);
  mMemRegions[mNumMemRegions].MemSize           = FixedPcdGet64 (PcdIMemCookiesSize);
  mMemRegions[mNumMemRegions].BuildHobOption    = AddPeripheral;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_MEMORY_MAPPED_IO;
  mMemRegions[mNumMemRegions].ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
  mMemRegions[mNumMemRegions].MemoryType        = EfiMemoryMappedIO;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  mNumMemRegions++;

  DEBUG ((
    DEBUG_INFO,
    "LoadStaticPlatformCfg: %d regions (FD base=0x%lx size=0x%lx)\n",
    mNumMemRegions,
    FixedPcdGet64 (PcdFdBaseAddress),
    (UINT64)PcdGet32 (PcdSystemMemoryUefiRegionSize)
    ));

  return EFI_SUCCESS;
}

/**
  Get memory region configuration information.

  Gets the Memory Map that was parsed from the platform cfg file.

  @param[out]  MemoryRegions     Pointer to receive memory regions array.
  @param[out]  NumMemoryRegions  Pointer to receive number of memory regions.

  @retval  EFI_SUCCESS             Memory region info retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS EFIAPI
GetMemRegionCfgInfo (
  MEM_REGION_INFO  **MemoryRegions,
  UINTN            *NumMemoryRegions
  )
{
  if ((MemoryRegions == NULL) || (NumMemoryRegions == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *MemoryRegions    = mMemRegions;
  *NumMemoryRegions = mNumMemRegions;

  return EFI_SUCCESS;
}

/**
  Validate parsed memory regions.

  Check if there is a region defined in uefiplat that overlaps with a
  hole in rampartition table.

  @retval  EFI_SUCCESS             Memory regions validated successfully.
  @retval  EFI_INVALID_PARAMETER   Overlapping regions found.

**/
EFI_STATUS EFIAPI
ValidateParsedMemoryRegions (
  VOID
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  RamPartTableEntry[RAM_NUM_PART_ENTRIES];
  UINTN            RamPartTableEntryCount;
  BOOLEAN          NoOverlap;
  UINT32           Index;
  UINT32           RamPartitionIndex;

  RamPartitionIndex = 0;

  RamPartTableEntryCount = RAM_NUM_PART_ENTRIES;
  Status = RamPartitionGetRamPartitions (&RamPartTableEntryCount, RamPartTableEntry);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  for (Index = 0; Index < mNumMemoryMapRegion; Index++) {
    NoOverlap = FALSE;

    /* This first cond added for the hole that is carved out in memmap.dtsi
     * for converting it to Conventional Memory and NoMap is not needed to
     * be checked for overlap in RamPartitionTable */
    if ((mMemRegions[Index].BuildHobOption == AddDynamicMem) ||
        (mMemRegions[Index].BuildHobOption == NoMap) ||
        (mMemRegions[Index].BuildHobOption == NoBuildHob))
    {
      continue;
    }

    for (RamPartitionIndex = 0; RamPartitionIndex < RamPartTableEntryCount; RamPartitionIndex++) {
      if ((mMemRegions[Index].MemBase >= RamPartTableEntry[RamPartitionIndex].MemBase) &&
          ((mMemRegions[Index].MemBase +  mMemRegions[Index].MemSize-1) >= RamPartTableEntry[RamPartitionIndex].MemBase) &&
          (mMemRegions[Index].MemBase < (RamPartTableEntry[RamPartitionIndex].MemBase + RamPartTableEntry[RamPartitionIndex].MemSize)) &&
          ((mMemRegions[Index].MemBase + mMemRegions[Index].MemSize-1) < (RamPartTableEntry[RamPartitionIndex].MemBase + RamPartTableEntry[RamPartitionIndex].MemSize))
          )
      {
        /* The region lies in the usable rampartition table range and there is
         * no overlap in the hole region defined by RamPartition table */
        NoOverlap = TRUE;
        break;
      }
    }

    if (!NoOverlap) {
      DEBUG ((
        DEBUG_ERROR,
        "Memory region %a carved out in a hole defined by RAM partition table !\n",
        mMemRegions[Index].Name
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}
