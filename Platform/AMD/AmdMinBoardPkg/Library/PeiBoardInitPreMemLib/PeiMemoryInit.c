/** @file

Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. All rights reserved
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMemoryInit.h"
#include "AmdMemoryInfoHob.h"

/// TPM2 MMIO region covers 5 localities (0-4), each 4 KB, per TCG PC Client spec.
#define TPM2_LOCALITY_COUNT  5u
#define TPM2_MMIO_SIZE       (TPM2_LOCALITY_COUNT * SIZE_4KB)

typedef struct {
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Size;
  CONST CHAR8             *Name;
  //
  // When TRUE, a Memory Allocation HOB is emitted so DXE Core pre-allocates
  // this range in the GCD before any DXE driver runs. Set TRUE for ranges
  // that must be protected from the PCI BAR allocator (e.g. IOAPIC, HPET).
  // Set FALSE when the owning DXE driver will call AllocateMemorySpace()
  // itself and pre-allocation from PEI is not required.
  //
  BOOLEAN                 PreAllocate;
} MMIO_CARVEOUT;

///
/// Table of MMIO ranges within the below-4GB MMIO region that must be
/// carved out of the plain MMIO resource HOB. Each entry produces its own
/// resource HOB so DXE Core registers it in the GCD separately. Entries
/// with PreAllocate = TRUE also get a Memory Allocation HOB so DXE Core
/// marks them as allocated before any DXE driver runs.
///
/// To add a new carved-out range, append one row before the sentinel -- no
/// other code changes are needed.
///
/// The { 0, 0, NULL, FALSE } sentinel MUST remain as the last entry. It
/// ensures the array is never zero-length (standard C requires at least one
/// initializer) and is silently skipped by the emit loop (Size == 0 guard).
///
STATIC CONST MMIO_CARVEOUT  mBelow4GbCarveouts[] = {
  // Base                               Size            Name      PreAllocate
  { FixedPcdGet32 (PcdIoApicAddress),   SIZE_4KB,       "IOAPIC", TRUE },
  { FixedPcdGet32 (PcdHpetBaseAddress), SIZE_4KB,       "HPET",   TRUE },
  { FixedPcdGet64 (PcdTpmBaseAddress),  TPM2_MMIO_SIZE, "TPM2",   TRUE },
  { 0,                                  0,              NULL,     FALSE}  // sentinel -- do not remove
};

/**
  Initialize SortedIdx[] to identity and bubble-sort it by ascending Base
  address of the corresponding mBelow4GbCarveouts entry.

  Sorting an index array (rather than the CONST data) avoids a copy and
  keeps the original table order intact for debugging.

  @param[in, out]  SortedIdx  Caller-allocated array of Count indices to
                               initialize and sort.
  @param[in]       Count      Number of elements in SortedIdx (must equal
                               ARRAY_SIZE (mBelow4GbCarveouts)).
**/
STATIC
VOID
SortCarveoutsByBase (
  IN OUT  UINTN  *SortedIdx,
  IN      UINTN  Count
  )
{
  UINTN  Idx;
  UINTN  Pass;
  UINTN  Tmp;

  for (Idx = 0; Idx < Count; Idx++) {
    SortedIdx[Idx] = Idx;
  }

  if (Count > 1) {
    for (Idx = 0; Idx < Count - 1; Idx++) {
      for (Pass = 0; Pass < Count - 1 - Idx; Pass++) {
        if (mBelow4GbCarveouts[SortedIdx[Pass]].Base >
            mBelow4GbCarveouts[SortedIdx[Pass + 1]].Base)
        {
          Tmp                 = SortedIdx[Pass];
          SortedIdx[Pass]     = SortedIdx[Pass + 1];
          SortedIdx[Pass + 1] = Tmp;
        }
      }
    }
  }
}

/**
  A Callback routine only AmdMemoryInfoHob is ready.

  @param[in]  PeiServices       General purpose services available to every PEIM.
  @param[in]  NotifyDescriptor  The descriptor for the notification event.
  @param[in]  Ppi               The context of the notification.

  @retval EFI_SUCCESS   Platform Pre Memory initialization is successful.
          EFI_STATUS    Various failure from underlying routine calls.
**/
EFI_STATUS
EFIAPI
EndofAmdMemoryInfoHobPpiGuidCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PEI_PLATFORM_MEMORY_SIZE_PPI    *PlatformMemorySizePpi;
  EFI_STATUS                      Status;
  UINT64                          MemorySize;
  AMD_MEMORY_INFO_HOB             *AmdMemoryInfoHob;
  AMD_MEMORY_RANGE_DESCRIPTOR     *AmdMemoryInfoRange;
  EFI_HOB_GUID_TYPE               *GuidHob;
  UINTN                           Index;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramHobDescriptorBlock;
  EFI_PHYSICAL_ADDRESS            SmramBaseAddress;
  UINT8                           SmramRanges;
  UINTN                           DataSize;
  EFI_PHYSICAL_ADDRESS            CurrentBase;
  EFI_PHYSICAL_ADDRESS            RangeEnd;
  UINTN                           SortedIdx[ARRAY_SIZE (mBelow4GbCarveouts)];
  UINTN                           CarveIdx;
  CONST MMIO_CARVEOUT             *Carve;

  SmramBaseAddress = 0;
  SmramRanges      = 0;

  // Locate AMD_MEMORY_INFO_HOB Guided HOB and retrieve data
  AmdMemoryInfoHob = NULL;
  GuidHob          = GetFirstGuidHob (&gAmdMemoryInfoHobGuid);
  if (GuidHob != NULL) {
    AmdMemoryInfoHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (AmdMemoryInfoHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: Could not locate AMD_MEMORY_INFO_HOB.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "AMD_MEMORY_INFO_HOB at 0x%X\n", AmdMemoryInfoHob));
  DEBUG ((DEBUG_INFO, "  Version: 0x%X\n", AmdMemoryInfoHob->Version));
  DEBUG ((DEBUG_INFO, "  NumberOfDescriptor: 0x%X\n", AmdMemoryInfoHob->NumberOfDescriptor));

  //
  // Build Descriptors
  //
  DEBUG ((DEBUG_INFO, "\nAMD HOB Descriptors:"));
  for (Index = 0; Index < AmdMemoryInfoHob->NumberOfDescriptor; Index++) {
    AmdMemoryInfoRange = (AMD_MEMORY_RANGE_DESCRIPTOR *)&(AmdMemoryInfoHob->Ranges[Index]);

    DEBUG ((DEBUG_INFO, "\n Index: %d\n", Index));
    DEBUG ((DEBUG_INFO, "   Base: 0x%lX\n", AmdMemoryInfoRange->Base));
    DEBUG ((DEBUG_INFO, "   Size: 0x%lX\n", AmdMemoryInfoRange->Size));
    DEBUG ((DEBUG_INFO, "   Attribute: 0x%X\n", AmdMemoryInfoRange->Attribute));

    switch (AmdMemoryInfoRange->Attribute) {
      case AMD_MEMORY_ATTRIBUTE_AVAILABLE:
        if (AmdMemoryInfoRange->Base < SIZE_4GB) {
          if (AmdMemoryInfoRange->Size < FixedPcdGet32 (PcdAmdSmramAreaSize)) {
            DEBUG ((
              DEBUG_ERROR,
              "%a: Skipping below-4GB range (Base=0x%lX, Size=0x%lX): "
              "too small to carve SMRAM (0x%X).\n",
              __func__,
              AmdMemoryInfoRange->Base,
              AmdMemoryInfoRange->Size,
              FixedPcdGet32 (PcdAmdSmramAreaSize)
              ));
            continue;
          }

          SmramRanges = 1u;
          // Set SMRAM base at heighest range below 4GB
          SmramBaseAddress = AmdMemoryInfoRange->Base + AmdMemoryInfoRange->Size - FixedPcdGet32 (PcdAmdSmramAreaSize);
          BuildResourceDescriptorHob (
            EFI_RESOURCE_MEMORY_RESERVED,
            (EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
            SmramBaseAddress,
            FixedPcdGet32 (PcdAmdSmramAreaSize)
            );
          DEBUG (
            (
             DEBUG_INFO,
             "SMRAM RESERVED_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
             SmramBaseAddress,
             FixedPcdGet32 (PcdAmdSmramAreaSize)
            )
            );

          if (AmdMemoryInfoRange->Size) {
            BuildResourceDescriptorHob (
              EFI_RESOURCE_SYSTEM_MEMORY,
              SYSTEM_MEMORY_ATTRIBUTES,
              AmdMemoryInfoRange->Base,
              AmdMemoryInfoRange->Size - FixedPcdGet32 (PcdAmdSmramAreaSize)
              );

            DEBUG (
              (
               DEBUG_INFO,
               "SYSTEM_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
               AmdMemoryInfoRange->Base,
               AmdMemoryInfoRange->Size - FixedPcdGet32 (PcdAmdSmramAreaSize)
              )
              );
          }

          break;
        }

        if (AmdMemoryInfoRange->Size > 0) {
          BuildResourceDescriptorHob (
            EFI_RESOURCE_SYSTEM_MEMORY,
            SYSTEM_MEMORY_ATTRIBUTES,
            AmdMemoryInfoRange->Base,
            AmdMemoryInfoRange->Size
            );

          DEBUG (
            (
             DEBUG_INFO,
             "SYSTEM_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
             AmdMemoryInfoRange->Base,
             AmdMemoryInfoRange->Size
            )
            );
        }

        break;

      case AMD_MEMORY_ATTRIBUTE_MMIO:
        //
        // For below-4GB MMIO, carve out entries in mBelow4GbCarveouts[] as
        // individually allocated regions. The Memory Allocation HOBs cause
        // DXE Core to mark these ranges as allocated in GCD (setting ImageHandle).
        //
        // To add a new carved-out range, append one row to mBelow4GbCarveouts[]
        // at the top of this file — no changes needed here.
        //
        if ((AmdMemoryInfoRange->Base < SIZE_4GB) &&
            (AmdMemoryInfoRange->Size <= (SIZE_4GB - AmdMemoryInfoRange->Base)))
        {
          RangeEnd    = AmdMemoryInfoRange->Base + AmdMemoryInfoRange->Size;
          CurrentBase = AmdMemoryInfoRange->Base;

          SortCarveoutsByBase (SortedIdx, ARRAY_SIZE (mBelow4GbCarveouts));

          DEBUG ((
            DEBUG_INFO,
            "MMIO Below 4GB: Base = 0x%lX, Size = 0x%lX\n",
            AmdMemoryInfoRange->Base,
            AmdMemoryInfoRange->Size
            ));

          //
          // Walk carveouts in sorted address order. Emit any MMIO gap before each
          // carveout as a plain resource HOB, then emit the carveout itself as both
          // a resource HOB and a memory allocation HOB.
          //
          for (CarveIdx = 0; CarveIdx < ARRAY_SIZE (mBelow4GbCarveouts); CarveIdx++) {
            Carve = &mBelow4GbCarveouts[SortedIdx[CarveIdx]];

            // Skip the sentinel entry (Size == 0) used to keep the array non-empty.
            if (Carve->Size == 0) {
              continue;
            }

            if ((Carve->Base < CurrentBase) || (Carve->Base >= RangeEnd)) {
              DEBUG ((
                DEBUG_ERROR,
                "  %a (0x%lX) outside MMIO range [0x%lX, 0x%lX) - skipping\n",
                Carve->Name,
                Carve->Base,
                AmdMemoryInfoRange->Base,
                RangeEnd
                ));
              continue;
            }

            if (Carve->Base + Carve->Size > RangeEnd) {
              DEBUG ((
                DEBUG_ERROR,
                "  %a end (0x%lX) exceeds MMIO RangeEnd (0x%lX) - skipping\n",
                Carve->Name,
                Carve->Base + Carve->Size,
                RangeEnd
                ));
              continue;
            }

            // Gap before this carveout
            if (Carve->Base > CurrentBase) {
              BuildResourceDescriptorHob (
                EFI_RESOURCE_MEMORY_MAPPED_IO,
                MEMORY_MAPPED_IO_ATTRIBUTES,
                CurrentBase,
                Carve->Base - CurrentBase
                );
              DEBUG ((
                DEBUG_INFO,
                "  MMIO: Base = 0x%lX, Size = 0x%lX\n",
                CurrentBase,
                Carve->Base - CurrentBase
                ));
            }

            BuildResourceDescriptorHob (
              EFI_RESOURCE_MEMORY_MAPPED_IO,
              MEMORY_MAPPED_IO_ATTRIBUTES,
              Carve->Base,
              Carve->Size
              );
            if (Carve->PreAllocate) {
              BuildMemoryAllocationHob (
                Carve->Base,
                Carve->Size,
                EfiMemoryMappedIO
                );
              DEBUG ((
                DEBUG_INFO,
                "  %a MMIO (Allocated): Base = 0x%lX, Size = 0x%lX\n",
                Carve->Name,
                Carve->Base,
                Carve->Size
                ));
            } else {
              DEBUG ((
                DEBUG_INFO,
                "  %a MMIO (Resource only): Base = 0x%lX, Size = 0x%lX\n",
                Carve->Name,
                Carve->Base,
                Carve->Size
                ));
            }

            CurrentBase = Carve->Base + Carve->Size;
          }

          // Remaining gap after the last carveout
          if (CurrentBase < RangeEnd) {
            BuildResourceDescriptorHob (
              EFI_RESOURCE_MEMORY_MAPPED_IO,
              MEMORY_MAPPED_IO_ATTRIBUTES,
              CurrentBase,
              RangeEnd - CurrentBase
              );
            DEBUG ((
              DEBUG_INFO,
              "  MMIO: Base = 0x%lX, Size = 0x%lX\n",
              CurrentBase,
              RangeEnd - CurrentBase
              ));
          }
        } else {
          BuildResourceDescriptorHob (
            EFI_RESOURCE_MEMORY_MAPPED_IO,
            MEMORY_MAPPED_IO_ATTRIBUTES,
            AmdMemoryInfoRange->Base,
            AmdMemoryInfoRange->Size
            );
          DEBUG ((
            DEBUG_INFO,
            "MMIO: Base = 0x%lX, Size = 0x%lX\n",
            AmdMemoryInfoRange->Base,
            AmdMemoryInfoRange->Size
            ));
        }

        break;

      case AMD_MEMORY_ATTRIBUTE_GPU_SP:
        BuildResourceDescriptorHob (
          EFI_RESOURCE_SYSTEM_MEMORY,
          (EFI_RESOURCE_ATTRIBUTE_PRESENT |
           EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
           EFI_RESOURCE_ATTRIBUTE_TESTED |
           EFI_RESOURCE_ATTRIBUTE_SPECIAL_PURPOSE),
          AmdMemoryInfoRange->Base,
          AmdMemoryInfoRange->Size
          );

        DEBUG (
          (
           DEBUG_INFO,
           "HBM: Base = 0x%lX, Size = 0x%lX\n",
           AmdMemoryInfoRange->Base,
           AmdMemoryInfoRange->Size
          )
          );
        break;

      case AMD_MEMORY_ATTRIBUTE_PERSISTENT_MEMORY:
        BuildResourceDescriptorHob (
          EFI_RESOURCE_SYSTEM_MEMORY,
          EFI_RESOURCE_ATTRIBUTE_PERSISTENT | EFI_RESOURCE_ATTRIBUTE_PERSISTABLE,
          AmdMemoryInfoRange->Base,
          AmdMemoryInfoRange->Size
          );

        DEBUG (
          (
           DEBUG_INFO,
           "PERSISTENT_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
           AmdMemoryInfoRange->Base,
           AmdMemoryInfoRange->Size
          )
          );
        break;

      case AMD_MEMORY_ATTRIBUTE_RESERVED:
      case AMD_MEMORY_ATTRIBUTE_RESERVED_X4_PUSH_WRITE:
      case AMD_MEMORY_ATTRIBUTE_UMA:
      default:
        BuildResourceDescriptorHob (
          EFI_RESOURCE_MEMORY_RESERVED,
          0,
          AmdMemoryInfoRange->Base,
          AmdMemoryInfoRange->Size
          );

        DEBUG (
          (
           DEBUG_INFO,
           "RESERVED_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
           AmdMemoryInfoRange->Base,
           AmdMemoryInfoRange->Size
          )
          );
        break;
    }
  }

  if (SmramRanges < 1) {
    DEBUG ((DEBUG_ERROR, "%a: Error(%r): No SMRAM range found.\n", __func__, EFI_NOT_FOUND));
    return EFI_NOT_FOUND;
  }

  DataSize = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK);

  DataSize += ((SmramRanges - (UINT8)1) * sizeof (EFI_SMRAM_DESCRIPTOR));

  SmramHobDescriptorBlock = BuildGuidHob (
                              &gEfiSmmSmramMemoryGuid,
                              DataSize
                              );
  if (SmramHobDescriptorBlock == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Error: Failed to build SMRAM HOB.\n", __func__));
    return EFI_NOT_FOUND;
  }

  SmramHobDescriptorBlock->NumberOfSmmReservedRegions  = SmramRanges;
  SmramHobDescriptorBlock->Descriptor[0].PhysicalStart = SmramBaseAddress;
  SmramHobDescriptorBlock->Descriptor[0].CpuStart      = SmramBaseAddress;
  SmramHobDescriptorBlock->Descriptor[0].PhysicalSize  = FixedPcdGet32 (PcdAmdSmramAreaSize);
  SmramHobDescriptorBlock->Descriptor[0].RegionState   = EFI_SMRAM_CLOSED | EFI_CACHEABLE;

  Status = PeiServicesLocatePpi (
             &gPeiPlatformMemorySizePpiGuid,
             0,
             NULL,
             (VOID **)&PlatformMemorySizePpi
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PlatformMemorySizePpi->GetPlatformMemorySize (
                                    PeiServices,
                                    PlatformMemorySizePpi,
                                    &MemorySize
                                    );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "%a: Error(%r) in getting Platform Memory size.\n",
       __func__,
       Status
      )
      );
    return Status;
  }

  DEBUG (
    (
     DEBUG_INFO,
     "Installing PeiMemory, BaseAddress = 0x%x, Size = 0x%x\n",
     BASE_1MB,
     MemorySize
    )
    );
  /// The region below 1MB is used for PeiMpLib usage.
  /// WakeupBufferStart = 0xFF000, WakeupBufferEnd = 0x100000.
  /// Therefore, PeiMemory is installed starting above 1MB.
  Status = PeiServicesInstallPeiMemory (BASE_1MB, MemorySize);
  return Status;
}
