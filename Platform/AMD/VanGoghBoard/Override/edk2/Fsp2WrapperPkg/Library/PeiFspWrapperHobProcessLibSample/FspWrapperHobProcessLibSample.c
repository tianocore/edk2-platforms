/** @file
  Sample to provide FSP wrapper hob process related function.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"
#include <Ppi/AmdPspFtpmPpi.h>
#include <Ppi/Reset2.h>
#include <FspEas/FspApi.h>
#include <FspmUpd.h>
#include <Library/PeiServicesLib.h>
#include <FspMemoryRegionHob.h>
#include <FspExportedInterfaceHob.h>

#define MTRR_LIB_CACHE_MTRR_ENABLED  0x800

extern EFI_GUID  gAmdResourceSizeForEachRbGuid;

EFI_MEMORY_TYPE_INFORMATION  mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,       0x40   },
  { EfiACPIMemoryNVS,           0x20   },
  { EfiReservedMemoryType,      0x10   },
  { EfiMemoryMappedIO,          0      },
  { EfiMemoryMappedIOPortSpace, 0      },
  { EfiPalCode,                 0      },
  { EfiRuntimeServicesCode,     0x80   },
  { EfiRuntimeServicesData,     0x40   },
  { EfiLoaderCode,              0      },
  { EfiLoaderData,              0      },
  { EfiBootServicesCode,        0x400  },
  { EfiBootServicesData,        0x1000 },
  { EfiConventionalMemory,      0x4    },
  { EfiUnusableMemory,          0      },
  { EfiMaxMemoryType,           0      }
};

#pragma pack (push,4) // AGESA BUG
typedef struct _APOBLIB_INFO {
  BOOLEAN    Supported;              ///<  Specify if APOB supported
  UINT32     ApobSize;               ///<  ApobSize
  UINT64     ApobAddr;               ///<  The Address of APOB
} APOBLIB_INFO;
#pragma pack (pop)
STATIC_ASSERT (sizeof (APOBLIB_INFO) == 16, "APOB Hob not aligned as MSVC behavior!");

/**

  This function returns the memory ranges to be enabled, along with information
  describing how the range should be used.

  @param  PeiServices   PEI Services Table.
  @param  MemoryMap     Buffer to record details of the memory ranges tobe enabled.
  @param  NumRanges     On input, this contains the maximum number of memory ranges that can be described
                        in the MemoryMap buffer.

  @return MemoryMap     The buffer will be filled in
          NumRanges     will contain the actual number of memory ranges that are to be anabled.
          EFI_SUCCESS   The function completed successfully.

**/
EFI_STATUS
GetMemoryMap (
  IN     EFI_PEI_SERVICES                       **PeiServices,
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges,
  IN     VOID                                   *FspHobList
  )
{
  EFI_PHYSICAL_ADDRESS                   MemorySize;
  EFI_PHYSICAL_ADDRESS                   RowLength;
  PEI_MEMORY_RANGE_SMRAM                 SmramMask;
  PEI_MEMORY_RANGE_SMRAM                 TsegMask;
  UINT32                                 BlockNum;
  UINT8                                  ExtendedMemoryIndex;
  UINT8                                  Index;
  UINT64                                 SmRamTsegBase;
  UINT64                                 SmRamTsegLength;
  UINT64                                 SmRamTsegMask;
  UINT64                                 LowMemoryLength;
  PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  TemMemoryMap[MAX_RANGES];
  UINT8                                  TemNumRanges;

  if ((*NumRanges) < MAX_RANGES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Get the Memory Map
  //
  TemNumRanges    = MAX_RANGES;
  LowMemoryLength = 0;
  *NumRanges      = 0;
  ZeroMem (TemMemoryMap, sizeof (PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE) * MAX_RANGES);

  GetAvailableMemoryRanges (TemMemoryMap, &TemNumRanges, FspHobList);

  for (Index = 0; Index < TemNumRanges; Index++) {
    if (TemMemoryMap[Index].CpuAddress < SIZE_4GB) {
      LowMemoryLength += TemMemoryMap[Index].RangeLength;
    } else {
      //
      // Memory Map information Upper than 4G
      //
      MemoryMap[*NumRanges].PhysicalAddress = TemMemoryMap[Index].PhysicalAddress;
      MemoryMap[*NumRanges].CpuAddress      = TemMemoryMap[Index].CpuAddress;
      MemoryMap[*NumRanges].RangeLength     = TemMemoryMap[Index].RangeLength;
      MemoryMap[*NumRanges].Type            = DualChannelDdrMainMemory;
      (*NumRanges)++;
    }
  }

  TemNumRanges = MAX_RANGES;
  ZeroMem (TemMemoryMap, sizeof (PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE) * MAX_RANGES);

  GetReservedMemoryRanges (TemMemoryMap, &TemNumRanges, FspHobList);
  for (Index = 0; Index < TemNumRanges; Index++) {
    MemoryMap[*NumRanges].PhysicalAddress = TemMemoryMap[Index].PhysicalAddress;
    MemoryMap[*NumRanges].CpuAddress      = TemMemoryMap[Index].CpuAddress;
    MemoryMap[*NumRanges].RangeLength     = TemMemoryMap[Index].RangeLength;
    MemoryMap[*NumRanges].Type            = DualChannelDdrReservedMemory;
    (*NumRanges)++;
  }

  //
  // Choose regions to reserve for SMM use (AB/H SEG and TSEG). Size is in 128K blocks
  //
  SmramMask = PEI_MR_SMRAM_ABSEG_128K_NOCACHE | PEI_MR_SMRAM_TSEG_4096K_CACHE;

  //
  // Generate Memory ranges for the memory map.
  //
  MemorySize = 0;

  RowLength = LowMemoryLength;

  //
  // Add memory below 640KB to the memory map. Make sure memory between
  // 640KB and 1MB are reserved, even if not used for SMRAM
  //
  MemoryMap[*NumRanges].PhysicalAddress = MemorySize;
  MemoryMap[*NumRanges].CpuAddress      = MemorySize;
  MemoryMap[*NumRanges].RangeLength     = 0xA0000;
  MemoryMap[*NumRanges].Type            = DualChannelDdrMainMemory;
  (*NumRanges)++;

  // Reserve ABSEG or HSEG SMRAM if needed
  //
  if (SmramMask & (PEI_MR_SMRAM_ABSEG_MASK | PEI_MR_SMRAM_HSEG_MASK)) {
    MemoryMap[*NumRanges].PhysicalAddress = MC_ABSEG_HSEG_PHYSICAL_START;
    MemoryMap[*NumRanges].RangeLength     = MC_ABSEG_HSEG_LENGTH;
    MemoryMap[*NumRanges].CpuAddress      = (SmramMask & PEI_MR_SMRAM_ABSEG_MASK) ?
                                            MC_ABSEG_CPU_START : MC_HSEG_CPU_START;
    //
    // Chipset only supports cacheable SMRAM
    //
    MemoryMap[*NumRanges].Type = DualChannelDdrSmramCacheable;
  } else {
    //
    // Just mark this range reserved
    //
    MemoryMap[*NumRanges].PhysicalAddress = 0xA0000;
    MemoryMap[*NumRanges].CpuAddress      = 0xA0000;
    MemoryMap[*NumRanges].RangeLength     = 0x60000;
    MemoryMap[*NumRanges].Type            = DualChannelDdrReservedMemory;
  }

  (*NumRanges)++;
  RowLength -= (0x100000 - MemorySize);
  MemorySize = 0x100000;

  //
  // Add remaining memory to the memory map
  //
  MemoryMap[*NumRanges].PhysicalAddress = MemorySize;
  MemoryMap[*NumRanges].CpuAddress      = MemorySize;
  MemoryMap[*NumRanges].RangeLength     = RowLength;
  MemoryMap[*NumRanges].Type            = DualChannelDdrMainMemory;
  (*NumRanges)++;
  MemorySize += RowLength;

  ExtendedMemoryIndex = (UINT8)(*NumRanges - 1);

  // See if we need to trim TSEG out of the highest memory range
  //
  if (SmramMask & PEI_MR_SMRAM_TSEG_MASK) {
    // pcd
    //
    // Create the new range for TSEG and remove that range from the previous SdrDdrMainMemory range
    //
    TsegMask = (SmramMask & PEI_MR_SMRAM_SIZE_MASK);

    BlockNum = 1;
    while (TsegMask) {
      TsegMask >>= 1;
      BlockNum <<= 1;
    }

    BlockNum >>= 1;

    if (BlockNum) {
      SmRamTsegLength                             = (BlockNum * 128 * 1024);
      MemoryMap[*NumRanges].RangeLength           = SmRamTsegLength;
      MemorySize                                 -= MemoryMap[*NumRanges].RangeLength;
      MemoryMap[*NumRanges].PhysicalAddress       = MemorySize;
      MemoryMap[*NumRanges].CpuAddress            = MemorySize;
      SmRamTsegBase                               = MemorySize;
      MemoryMap[ExtendedMemoryIndex].RangeLength -= MemoryMap[*NumRanges].RangeLength;

      //
      // Turn On Smram
      //
      SmRamTsegMask = (0x0000010000000000L-SmRamTsegLength) & 0xFFFFFFFE0000UL; // TSegMask[47:17]
      AsmWriteMsr64 (0xC0010112, SmRamTsegBase);
      AsmWriteMsr64 (0xC0010113, SmRamTsegMask); // enable
    }

    //
    // Chipset only supports non-cacheable SMRAM
    //
    MemoryMap[*NumRanges].Type = DualChannelDdrSmramNonCacheable;

    (*NumRanges)++;
  }

  return EFI_SUCCESS;
}

/**

  This function installs memory.

  @param   PeiServices    PEI Services table.
  @param   BootMode       The specific boot path that is being followed
  @param   Mch            Pointer to the DualChannelDdrMemoryInit PPI
  @param   RowConfArray   Row configuration information for each row in the system.

  @return  EFI_SUCCESS            The function completed successfully.
           EFI_INVALID_PARAMETER  One of the input parameters was invalid.
           EFI_ABORTED            An error occurred.

**/
EFI_STATUS
InstallEfiMemory (
  IN      EFI_PEI_SERVICES  **PeiServices,
  IN      EFI_BOOT_MODE     BootMode,
  IN      VOID              *FspHobList
  )
{
  EFI_PHYSICAL_ADDRESS                   PeiMemoryBaseAddress;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK         *SmramHobDescriptorBlock;
  EFI_STATUS                             Status;
  EFI_PEI_HOB_POINTERS                   Hob;
  PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  MemoryMap[MAX_RANGES];
  UINT8                                  Index;
  UINT8                                  NumRanges;
  UINT8                                  SmramIndex;
  UINT8                                  SmramRanges;
  UINT64                                 PeiMemoryLength;
  UINTN                                  BufferSize;
  UINTN                                  PeiMemoryIndex;
  EFI_RESOURCE_ATTRIBUTE_TYPE            Attribute;
  EFI_SMRAM_DESCRIPTOR                   DescriptorAcpiVariable;
  VOID                                   *CapsuleBuffer;
  UINTN                                  CapsuleBufferLength;
  EFI_PEI_CAPSULE_PPI                    *Capsule;
  VOID                                   *LargeMemRangeBuf;
  UINTN                                  LargeMemRangeBufLen;

  //
  // Get the Memory Map
  //
  NumRanges = MAX_RANGES;

  ZeroMem (MemoryMap, sizeof (PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE) * NumRanges);

  Status = GetMemoryMap (
             PeiServices,
             (PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE *)MemoryMap,
             &NumRanges,
             FspHobList
             );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "NumRanges: %d\n", NumRanges));

  DEBUG ((DEBUG_INFO, "GetMemoryMap:\n"));
  for (Index = 0; Index < NumRanges; Index++) {
    DEBUG ((DEBUG_INFO, "Index: %d ", Index));
    DEBUG ((DEBUG_INFO, "RangeLength: 0x%016lX\t", MemoryMap[Index].RangeLength));
    DEBUG ((DEBUG_INFO, "PhysicalAddress: 0x%016lX\t", MemoryMap[Index].PhysicalAddress));
    DEBUG ((DEBUG_INFO, "CpuAddress: 0x%016lX\t", MemoryMap[Index].CpuAddress));
    DEBUG ((DEBUG_INFO, "Type: %d\n", MemoryMap[Index].Type));
  }

  //
  // Find the highest memory range in processor native address space to give to
  // PEI. Then take the top.
  //
  PeiMemoryBaseAddress = 0;

  //
  // Query the platform for the minimum memory size
  //

  Status = GetPlatformMemorySize (
             PeiServices,
             BootMode,
             &PeiMemoryLength
             );
  ASSERT_EFI_ERROR (Status);
  PeiMemoryLength = (PeiMemoryLength > PEI_MIN_MEMORY_SIZE) ? PeiMemoryLength : PEI_MIN_MEMORY_SIZE;
  //

  PeiMemoryIndex = 0;

  for (Index = 0; Index < NumRanges; Index++) {
    DEBUG ((DEBUG_INFO, "Found 0x%lx bytes at ", MemoryMap[Index].RangeLength));
    DEBUG ((DEBUG_INFO, "0x%lx.\t", MemoryMap[Index].PhysicalAddress));
    DEBUG ((DEBUG_INFO, "Type: %d.\n", MemoryMap[Index].Type));

    if ((MemoryMap[Index].Type == DualChannelDdrMainMemory) &&
        (MemoryMap[Index].PhysicalAddress + MemoryMap[Index].RangeLength < MAX_ADDRESS) &&
        (MemoryMap[Index].PhysicalAddress >= PeiMemoryBaseAddress) &&
        (MemoryMap[Index].RangeLength >= PeiMemoryLength))
    {
      PeiMemoryBaseAddress = MemoryMap[Index].PhysicalAddress +
                             MemoryMap[Index].RangeLength -
                             PeiMemoryLength;
      PeiMemoryIndex = Index;
    }
  }

  //
  // Find the largest memory range excluding that given to PEI.
  //
  LargeMemRangeBuf    = NULL;
  LargeMemRangeBufLen = 0;
  for (Index = 0; Index < NumRanges; Index++) {
    if ((MemoryMap[Index].Type == DualChannelDdrMainMemory) &&
        (MemoryMap[Index].PhysicalAddress + MemoryMap[Index].RangeLength < MAX_ADDRESS))
    {
      if (Index != PeiMemoryIndex) {
        if (MemoryMap[Index].RangeLength > LargeMemRangeBufLen) {
          LargeMemRangeBuf    = (VOID *)((UINTN)MemoryMap[Index].PhysicalAddress);
          LargeMemRangeBufLen = (UINTN)MemoryMap[Index].RangeLength;
        }
      } else {
        if ((MemoryMap[Index].RangeLength - PeiMemoryLength) >= LargeMemRangeBufLen) {
          LargeMemRangeBuf    = (VOID *)((UINTN)MemoryMap[Index].PhysicalAddress);
          LargeMemRangeBufLen = (UINTN)(MemoryMap[Index].RangeLength - PeiMemoryLength);
        }
      }
    }
  }

  Capsule             = NULL;
  CapsuleBuffer       = NULL;
  CapsuleBufferLength = 0;
  if (BootMode == BOOT_ON_FLASH_UPDATE) {
    Status = PeiServicesLocatePpi (
               &gEfiPeiCapsulePpiGuid,  // GUID
               0,                       // INSTANCE
               NULL,                    // EFI_PEI_PPI_DESCRIPTOR
               (VOID **)&Capsule        // PPI
               );
    ASSERT_EFI_ERROR (Status);

    if (Status == EFI_SUCCESS) {
      CapsuleBuffer       = LargeMemRangeBuf;
      CapsuleBufferLength = LargeMemRangeBufLen;
      DEBUG ((DEBUG_INFO, "CapsuleBuffer: %x, CapsuleBufferLength: %x\n", CapsuleBuffer, CapsuleBufferLength));

      //
      // Call the Capsule PPI Coalesce function to coalesce the capsule data.
      //
      Status = Capsule->Coalesce (
                          PeiServices,
                          &CapsuleBuffer,
                          &CapsuleBufferLength
                          );
      //
      // If it failed, then NULL out our capsule PPI pointer so that the capsule
      // HOB does not get created below.
      //
      if (Status != EFI_SUCCESS) {
        Capsule = NULL;
      }
    }
  }

  //
  // Carve out the top memory reserved for PEI
  //
  Status = PeiServicesInstallPeiMemory (PeiMemoryBaseAddress, PeiMemoryLength);
  ASSERT_EFI_ERROR (Status);
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,                      // MemoryType,
    (
     EFI_RESOURCE_ATTRIBUTE_PRESENT |
     EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
     EFI_RESOURCE_ATTRIBUTE_TESTED |
     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
    ),
    PeiMemoryBaseAddress,                            // MemoryBegin
    PeiMemoryLength                                  // MemoryLength
    );
  // Report first 640KB of system memory
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    (
     EFI_RESOURCE_ATTRIBUTE_PRESENT |
     EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
     EFI_RESOURCE_ATTRIBUTE_TESTED |
     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
    ),
    (EFI_PHYSICAL_ADDRESS)(0),
    (UINT64)(0xA0000)
    );

  //
  // Install physical memory descriptor hobs for each memory range.
  //
  SmramRanges = 0;
  for (Index = 0; Index < NumRanges; Index++) {
    Attribute = 0;
    if (MemoryMap[Index].Type == DualChannelDdrMainMemory) {
      if (Index == PeiMemoryIndex) {
        //
        // This is a partially tested Main Memory range, give it to EFI
        //
        BuildResourceDescriptorHob (
          EFI_RESOURCE_SYSTEM_MEMORY,
          (
           EFI_RESOURCE_ATTRIBUTE_PRESENT |
           EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
           EFI_RESOURCE_ATTRIBUTE_TESTED     |
           EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
           EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
           EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
           EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
          ),
          MemoryMap[Index].PhysicalAddress,
          MemoryMap[Index].RangeLength - PeiMemoryLength
          );
      } else {
        //
        // This is an untested Main Memory range, give it to EFI
        //
        BuildResourceDescriptorHob (
          EFI_RESOURCE_SYSTEM_MEMORY,       // MemoryType,
          (
           EFI_RESOURCE_ATTRIBUTE_PRESENT |
           EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
           EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
           EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
           EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
           EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
          ),
          MemoryMap[Index].PhysicalAddress, // MemoryBegin
          MemoryMap[Index].RangeLength      // MemoryLength
          );
      }
    } else {
      //
      // Only report TSEG range to align AcpiVariableHobOnSmramReserveHobThunk
      //
      if ((MemoryMap[Index].PhysicalAddress > 0x100000) &&
          ((MemoryMap[Index].Type == DualChannelDdrSmramCacheable) ||
           (MemoryMap[Index].Type == DualChannelDdrSmramNonCacheable)))
      {
        SmramRanges++;
      }

      //
      // AMD CPU has different flow to SMM and normal mode cache attribute.
      // SmmIPL will set TSEG and HSEG as UC when exit SMM.
      // the Attribute only support 0 then it will fail to set them to UC
      // otherwise the SmmIPL will hang at set memory attribute.
      //
      if (MemoryMap[Index].Type == DualChannelDdrGraphicsMemoryNonCacheable) {
        Attribute |= EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
      }

      if (MemoryMap[Index].Type == DualChannelDdrGraphicsMemoryCacheable) {
        Attribute |= EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
      }

      if (MemoryMap[Index].Type == DualChannelDdrReservedMemory) {
        Attribute |= EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
                     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
      }

      //
      // Make sure non-system memory is marked as reserved
      //
      BuildResourceDescriptorHob (
        EFI_RESOURCE_MEMORY_RESERVED,     // MemoryType,
        Attribute,                        // MemoryAttribute
        MemoryMap[Index].PhysicalAddress, // MemoryBegin
        MemoryMap[Index].RangeLength      // MemoryLength
        );
    }
  }

  //
  // Allocate one extra EFI_SMRAM_DESCRIPTOR to describe a page of SMRAM memory that contains a pointer
  // to the SMM Services Table that is required on the S3 resume path
  //
  ASSERT (SmramRanges > 0);
  BufferSize = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK);
  if (SmramRanges > 0) {
    BufferSize += ((SmramRanges) * sizeof (EFI_SMRAM_DESCRIPTOR));
  }

  Hob.Raw = BuildGuidHob (
              &gEfiSmmPeiSmramMemoryReserveGuid,
              BufferSize
              );
  ASSERT (Hob.Raw);

  SmramHobDescriptorBlock                             = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)(Hob.Raw);
  SmramHobDescriptorBlock->NumberOfSmmReservedRegions = SmramRanges + 1;

  SmramIndex = 0;
  for (Index = 0; Index < NumRanges; Index++) {
    if ((MemoryMap[Index].PhysicalAddress > 0x100000) &&
        ((MemoryMap[Index].Type == DualChannelDdrSmramCacheable) ||
         (MemoryMap[Index].Type == DualChannelDdrSmramNonCacheable)))
    {
      //
      // This is an SMRAM range, create an SMRAM descriptor
      //
      SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalStart = MemoryMap[Index].PhysicalAddress;
      SmramHobDescriptorBlock->Descriptor[SmramIndex].CpuStart      = MemoryMap[Index].CpuAddress;
      SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize  = MemoryMap[Index].RangeLength;
      if (MemoryMap[Index].Type == DualChannelDdrSmramCacheable) {
        SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState = EFI_SMRAM_CLOSED | EFI_CACHEABLE;
      } else {
        SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState = EFI_SMRAM_CLOSED;
      }

      if ( SmramIndex ==  SmramRanges - 1) {
        //
        // one extra EFI_SMRAM_DESCRIPTOR for a page of SMRAM memory
        //
        SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize = EFI_PAGE_SIZE;
        SmramIndex++;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalStart  = MemoryMap[Index].PhysicalAddress + EFI_PAGE_SIZE;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].CpuStart       = MemoryMap[Index].CpuAddress + EFI_PAGE_SIZE;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize   = MemoryMap[Index].RangeLength - EFI_PAGE_SIZE;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState    = SmramHobDescriptorBlock->Descriptor[SmramIndex-1].RegionState;
        SmramHobDescriptorBlock->Descriptor[SmramIndex-1].RegionState |= EFI_ALLOCATED;
      }

      SmramIndex++;
    }
  }

  //
  // Build a HOB with the location of the reserved memory range.
  //
  CopyMem (&DescriptorAcpiVariable, &SmramHobDescriptorBlock->Descriptor[SmramRanges-1], sizeof (EFI_SMRAM_DESCRIPTOR));
  DescriptorAcpiVariable.CpuStart += RESERVED_CPU_S3_SAVE_OFFSET;
  DEBUG ((DEBUG_INFO, "gEfiAcpiVariableGuid CpuStart: 0x%X\n", (UINTN)DescriptorAcpiVariable.CpuStart));
  BuildGuidDataHob (
    &gEfiAcpiVariableGuid,
    &DescriptorAcpiVariable,
    sizeof (EFI_SMRAM_DESCRIPTOR)
    );

  //
  // If we found the capsule PPI (and we didn't have errors), then
  // call the capsule PEIM to allocate memory for the capsule.
  //
  if (Capsule != NULL) {
    Status = Capsule->CreateState (PeiServices, CapsuleBuffer, CapsuleBufferLength);
  }

  return EFI_SUCCESS;
}

/**

  Find memory that is reserved so PEI has some to use.

  @param  PeiServices      PEI Services table.
  @param  VariableSevices  Variable PPI instance.

  @return EFI_SUCCESS  The function completed successfully.
                       Error value from LocatePpi()

**/
EFI_STATUS
InstallS3Memory (
  IN      EFI_PEI_SERVICES  **PeiServices,
  IN      EFI_BOOT_MODE     BootMode,
  IN      VOID              *FspHobList
  )
{
  EFI_STATUS                             Status;
  UINTN                                  S3MemoryBase;
  UINTN                                  S3MemorySize;
  UINT8                                  SmramRanges;
  UINT8                                  NumRanges;
  UINT8                                  Index;
  UINT8                                  SmramIndex;
  UINTN                                  BufferSize;
  EFI_PEI_HOB_POINTERS                   Hob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK         *SmramHobDescriptorBlock;
  PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  MemoryMap[MAX_RANGES];
  RESERVED_ACPI_S3_RANGE                 *S3MemoryRangeData;
  EFI_SMRAM_DESCRIPTOR                   DescriptorAcpiVariable;

  //
  // Get the Memory Map
  //
  NumRanges = MAX_RANGES;

  ZeroMem (MemoryMap, sizeof (PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE) * NumRanges);

  Status = GetMemoryMap (
             PeiServices,
             (PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE *)MemoryMap,
             &NumRanges,
             FspHobList
             );
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "NumRanges = 0x%x\n", NumRanges));

  //
  // Install physical memory descriptor hobs for each memory range.
  //
  SmramRanges = 0;
  DEBUG ((DEBUG_INFO, "GetMemoryMap:\n"));
  for (Index = 0; Index < NumRanges; Index++) {
    DEBUG ((DEBUG_INFO, "Index: %d ", Index));
    DEBUG ((DEBUG_INFO, "RangeLength: 0x%016lX\t", MemoryMap[Index].RangeLength));
    DEBUG ((DEBUG_INFO, "PhysicalAddress: 0x%016lX\t", MemoryMap[Index].PhysicalAddress));
    DEBUG ((DEBUG_INFO, "CpuAddress: 0x%016lX\t", MemoryMap[Index].CpuAddress));
    DEBUG ((DEBUG_INFO, "Type: %d\n", MemoryMap[Index].Type));
    if ((MemoryMap[Index].PhysicalAddress > 0x100000) &&
        ((MemoryMap[Index].Type == DualChannelDdrSmramCacheable) ||
         (MemoryMap[Index].Type == DualChannelDdrSmramNonCacheable)))
    {
      SmramRanges++;
    }
  }

  ASSERT (SmramRanges > 0);
  DEBUG ((DEBUG_INFO, "SmramRanges = 0x%x\n", SmramRanges));

  //
  // Allocate one extra EFI_SMRAM_DESCRIPTOR to describe a page of SMRAM memory that contains a pointer
  // to the SMM Services Table that is required on the S3 resume path
  //
  BufferSize = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK);
  if (SmramRanges > 0) {
    BufferSize += ((SmramRanges) * sizeof (EFI_SMRAM_DESCRIPTOR));
  }

  DEBUG ((DEBUG_INFO, "BufferSize = 0x%x\n", BufferSize));

  Hob.Raw = BuildGuidHob (
              &gEfiSmmPeiSmramMemoryReserveGuid,
              BufferSize
              );
  ASSERT (Hob.Raw);
  DEBUG ((DEBUG_INFO, "gEfiSmmPeiSmramMemoryReserveGuid/SmramHobDescriptorBlock: 0x%X \n", (UINTN)Hob.Raw));

  SmramHobDescriptorBlock                             = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)(Hob.Raw);
  SmramHobDescriptorBlock->NumberOfSmmReservedRegions = SmramRanges + 1;

  SmramIndex = 0;
  for (Index = 0; Index < NumRanges; Index++) {
    DEBUG ((DEBUG_INFO, "Index: 0x%X \t", Index));
    DEBUG ((DEBUG_INFO, "SmramIndex: 0x%X \n", SmramIndex));
    if ((MemoryMap[Index].PhysicalAddress > 0x100000) &&
        ((MemoryMap[Index].Type == DualChannelDdrSmramCacheable) ||
         (MemoryMap[Index].Type == DualChannelDdrSmramNonCacheable))
        )
    {
      //
      // This is an SMRAM range, create an SMRAM descriptor
      //
      SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalStart = MemoryMap[Index].PhysicalAddress;
      SmramHobDescriptorBlock->Descriptor[SmramIndex].CpuStart      = MemoryMap[Index].CpuAddress;
      SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize  = MemoryMap[Index].RangeLength;
      if (MemoryMap[Index].Type == DualChannelDdrSmramCacheable) {
        SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState = EFI_SMRAM_CLOSED | EFI_CACHEABLE;
      } else {
        SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState = EFI_SMRAM_CLOSED;
      }

      DEBUG ((DEBUG_INFO, "SmramIndex: 0x%X \n", SmramIndex));
      DEBUG ((DEBUG_INFO, "PhysicalStart: 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalStart));
      DEBUG ((DEBUG_INFO, "CpuStart     : 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].CpuStart));
      DEBUG ((DEBUG_INFO, "PhysicalSize : 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize));
      DEBUG ((DEBUG_INFO, "RegionState  : 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState));
      if ( SmramIndex ==  SmramRanges - 1) {
        //
        // one extra EFI_SMRAM_DESCRIPTOR for a page of SMRAM memory
        //
        SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize = EFI_PAGE_SIZE;
        SmramIndex++;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalStart  = MemoryMap[Index].PhysicalAddress + EFI_PAGE_SIZE;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].CpuStart       = MemoryMap[Index].CpuAddress + EFI_PAGE_SIZE;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize   = MemoryMap[Index].RangeLength - EFI_PAGE_SIZE;
        SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState    = SmramHobDescriptorBlock->Descriptor[SmramIndex-1].RegionState;
        SmramHobDescriptorBlock->Descriptor[SmramIndex-1].RegionState |= EFI_ALLOCATED;
        DEBUG ((DEBUG_INFO, "SmramIndex: 0x%X \n", SmramIndex));
        DEBUG ((DEBUG_INFO, "PhysicalStart: 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalStart));
        DEBUG ((DEBUG_INFO, "CpuStart     : 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].CpuStart));
        DEBUG ((DEBUG_INFO, "PhysicalSize : 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].PhysicalSize));
        DEBUG ((DEBUG_INFO, "RegionState  : 0x%X\n\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex].RegionState));

        DEBUG ((DEBUG_INFO, "PhysicalSize : 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex-1].PhysicalSize));
        DEBUG ((DEBUG_INFO, "RegionState  : 0x%X\n", (UINTN)SmramHobDescriptorBlock->Descriptor[SmramIndex-1].RegionState));
      }

      SmramIndex++;
    }
  }

  //
  // Build a HOB with the location of the reserved memory range.
  //
  CopyMem (&DescriptorAcpiVariable, &SmramHobDescriptorBlock->Descriptor[SmramRanges-1], sizeof (EFI_SMRAM_DESCRIPTOR));
  DescriptorAcpiVariable.CpuStart += RESERVED_CPU_S3_SAVE_OFFSET;
  DEBUG ((DEBUG_INFO, "gEfiAcpiVariableGuid CpuStart: 0x%X\n", (UINTN)DescriptorAcpiVariable.CpuStart));
  BuildGuidDataHob (
    &gEfiAcpiVariableGuid,
    &DescriptorAcpiVariable,
    sizeof (EFI_SMRAM_DESCRIPTOR)
    );

  //
  // Get the location and size of the S3 memory range in the reserved page and
  // install it as PEI Memory.
  //

  DEBUG ((DEBUG_INFO, "TSEG Base = 0x%08x\n", SmramHobDescriptorBlock->Descriptor[SmramRanges].PhysicalStart));
  DEBUG ((DEBUG_INFO, "SmramRanges = 0x%x\n", SmramRanges));
  S3MemoryRangeData = (RESERVED_ACPI_S3_RANGE *)(UINTN)
                      (SmramHobDescriptorBlock->Descriptor[SmramRanges].PhysicalStart + RESERVED_ACPI_S3_RANGE_OFFSET);
  DEBUG ((DEBUG_INFO, "S3MemoryRangeData = 0x%08x\n", (UINTN)S3MemoryRangeData));

  DEBUG ((DEBUG_INFO, "S3MemoryRangeData->AcpiReservedMemoryBase = 0x%X\n", (UINTN)S3MemoryRangeData->AcpiReservedMemoryBase));
  DEBUG ((DEBUG_INFO, "S3MemoryRangeData->AcpiReservedMemorySize = 0x%X\n", (UINTN)S3MemoryRangeData->AcpiReservedMemorySize));
  DEBUG ((DEBUG_INFO, "S3MemoryRangeData->SystemMemoryLength = 0x%X\n", (UINTN)S3MemoryRangeData->SystemMemoryLength));

  S3MemoryBase = (UINTN)(S3MemoryRangeData->AcpiReservedMemoryBase);
  DEBUG ((DEBUG_INFO, "S3MemoryBase = 0x%08x\n", S3MemoryBase));
  S3MemorySize = (UINTN)(S3MemoryRangeData->AcpiReservedMemorySize);
  DEBUG ((DEBUG_INFO, "S3MemorySize = 0x%08x\n", S3MemorySize));

  Status = PeiServicesInstallPeiMemory (S3MemoryBase, S3MemorySize);
  ASSERT_EFI_ERROR (Status);

  //
  // Retrieve the system memory length and build memory hob for the system
  // memory above 1MB. So Memory Callback can set cache for the system memory
  // correctly on S3 boot path, just like it does on Normal boot path.
  //
  ASSERT ((S3MemoryRangeData->SystemMemoryLength - 0x100000) > 0);
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    (
     EFI_RESOURCE_ATTRIBUTE_PRESENT |
     EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
    ),
    0x100000,
    S3MemoryRangeData->SystemMemoryLength - 0x100000
    );

  DEBUG ((DEBUG_INFO, "MemoryBegin: 0x%lX, MemoryLength: 0x%lX\n", 0x100000, S3MemoryRangeData->SystemMemoryLength - 0x100000));

  for (Index = 0; Index < NumRanges; Index++) {
    if ((MemoryMap[Index].Type == DualChannelDdrMainMemory) &&
        (MemoryMap[Index].PhysicalAddress + MemoryMap[Index].RangeLength < 0x100000))
    {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        (
         EFI_RESOURCE_ATTRIBUTE_PRESENT |
         EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
         EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
        ),
        MemoryMap[Index].PhysicalAddress,
        MemoryMap[Index].RangeLength
        );
      DEBUG ((DEBUG_INFO, "MemoryBegin: 0x%lX, MemoryLength: 0x%lX\n", MemoryMap[Index].PhysicalAddress, MemoryMap[Index].RangeLength));

      DEBUG ((DEBUG_INFO, "Build resource HOB for Legacy Region on S3 patch :"));
      DEBUG ((DEBUG_INFO, " Memory Base:0x%lX Length:0x%lX\n", MemoryMap[Index].PhysicalAddress, MemoryMap[Index].RangeLength));
    }
  }

  AsmMsrOr64 (0xC0010113, 0x4403); // Enable SMRAM
  return EFI_SUCCESS;
}

EFI_STATUS
GetPlatformMemorySize (
  IN       EFI_PEI_SERVICES  **PeiServices,
  IN       EFI_BOOT_MODE     BootMode,
  IN OUT   UINT64            *MemorySize
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Variable;
  UINTN                            DataSize;
  EFI_MEMORY_TYPE_INFORMATION      MemoryData[EfiMaxMemoryType + 1];
  UINTN                            Index;

  DataSize = sizeof (MemoryData);

  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    //
    // // Treat recovery as if variable not found (eg 1st boot).
    //
    Status = EFI_NOT_FOUND;
  } else {
    Status = PeiServicesLocatePpi (
               &gEfiPeiReadOnlyVariable2PpiGuid,
               0,
               NULL,
               (VOID **)&Variable
               );

    ASSERT_EFI_ERROR (Status);

    DataSize = sizeof (MemoryData);
    Status   = Variable->GetVariable (
                           Variable,
                           EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                           &gEfiMemoryTypeInformationGuid,
                           NULL,
                           &DataSize,
                           &MemoryData
                           );
  }

  //
  // Accumulate maximum amount of memory needed
  //
  if (EFI_ERROR (Status)) {
    //
    // Start with minimum memory
    //
    *MemorySize = PEI_MIN_MEMORY_SIZE;

    for (Index = 0; Index < sizeof (mDefaultMemoryTypeInformation) / sizeof (EFI_MEMORY_TYPE_INFORMATION); Index++) {
      *MemorySize += mDefaultMemoryTypeInformation[Index].NumberOfPages * EFI_PAGE_SIZE;
    }

    //
    // Build the GUID'd HOB for DXE
    //
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      mDefaultMemoryTypeInformation,
      sizeof (mDefaultMemoryTypeInformation)
      );
  } else {
    //
    // Start with at least PEI_MIN_MEMORY_SIZE pages of memory for the DXE Core and the DXE Stack
    //

    *MemorySize = PEI_MIN_MEMORY_SIZE;
    for (Index = 0; Index < DataSize / sizeof (EFI_MEMORY_TYPE_INFORMATION); Index++) {
      DEBUG ((DEBUG_INFO, "Index %d, Page: %d\n", Index, MemoryData[Index].NumberOfPages));
      *MemorySize += MemoryData[Index].NumberOfPages * EFI_PAGE_SIZE;
    }

    //
    // Build the GUID'd HOB for DXE
    //
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      MemoryData,
      DataSize
      );
  }

  DEBUG ((DEBUG_INFO, "GetPlatformMemorySize, MemorySize: 0x%lX\n", *MemorySize));
  return EFI_SUCCESS;
}

/**
  Post FSP-M HOB process for Memory Resource Descriptor.

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
PostFspmHobProcess (
  IN VOID  *FspHobList
  )
{
  EFI_STATUS        Status;
  EFI_BOOT_MODE     BootMode;
  EFI_PEI_SERVICES  **PeiServices;

  PreFspmHobProcess (FspHobList);

  PeiServices = (EFI_PEI_SERVICES **)GetPeiServicesTablePointer ();
  //
  // Get boot mode
  //
  SetPeiCacheMode ((const EFI_PEI_SERVICES **)PeiServices, FspHobList);

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  if (BootMode == BOOT_ON_S3_RESUME) {
    DEBUG ((DEBUG_INFO, "Following BOOT_ON_S3_RESUME boot path.\n"));

    Status = InstallS3Memory (PeiServices, BootMode, FspHobList);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      PeiServicesResetSystem ();
    }

    return EFI_SUCCESS;
  }

  Status = InstallEfiMemory (PeiServices, BootMode, FspHobList);
  return Status;
}

/**
  Process FSP HOB list

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

**/
VOID
ProcessFspHobList (
  IN VOID  *FspHobList
  )
{
  EFI_PEI_HOB_POINTERS  FspHob;

  FspHob.Raw = FspHobList;

  //
  // Add all the HOBs from FSP binary to FSP wrapper
  //
  while (!END_OF_HOB_LIST (FspHob)) {
    if (FspHob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {
      //
      // Skip FSP binary creates PcdDataBaseHobGuid
      //
      if (!CompareGuid (&FspHob.Guid->Name, &gPcdDataBaseHobGuid)) {
        BuildGuidDataHob (
          &FspHob.Guid->Name,
          GET_GUID_HOB_DATA (FspHob),
          GET_GUID_HOB_DATA_SIZE (FspHob)
          );
      }
    }

    FspHob.Raw = GET_NEXT_HOB (FspHob);
  }
}

/**
  Post FSP-S HOB process (not Memory Resource Descriptor).

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
PostFspsHobProcess (
  IN VOID  *FspHobList
  )
{
  //
  // PostFspsHobProcess () will be called in both FSP API and Dispatch modes to
  // align the same behavior and support a variety of boot loader implementations.
  // Boot loader provided library function is recommended to support both API and
  // Dispatch modes by checking PcdFspModeSelection.
  //
  if (PcdGet8 (PcdFspModeSelection) == 1) {
    //
    // Only in FSP API mode the wrapper has to build hobs basing on FSP output data.
    // In this case FspHobList cannot be NULL.
    //
    ASSERT (FspHobList != NULL);
    ProcessFspHobList (FspHobList);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetAvailableMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges,
  IN VOID                                       *FspHobList
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  DEBUG ((DEBUG_INFO, "GetAvailableMemoryRanges++\n"));
  if ((*NumRanges) < MAX_RANGES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  *NumRanges = 0;

  // Get Pointer to HOB
  Hob.Raw = (UINT8 *)(UINTN)FspHobList;
  ASSERT (Hob.Raw != NULL);
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw)) != NULL) {
    if ((Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY)) {
      MemoryMap[*NumRanges].PhysicalAddress =  Hob.ResourceDescriptor->PhysicalStart;
      MemoryMap[*NumRanges].CpuAddress      =  Hob.ResourceDescriptor->PhysicalStart;
      MemoryMap[*NumRanges].RangeLength     =  Hob.ResourceDescriptor->ResourceLength;
      MemoryMap[*NumRanges].Type            =  DualChannelDdrReservedMemory;
      (*NumRanges)++;
      DEBUG ((
        DEBUG_INFO,
        " GetAvailableMemoryRanges Base:0x%016lX, Size: 0x%016lX\n", \
        Hob.ResourceDescriptor->PhysicalStart, \
        Hob.ResourceDescriptor->ResourceLength
        ));
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetReservedMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges,
  IN VOID                                       *FspHobList
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  DEBUG ((DEBUG_INFO, "GetReservedMemoryRanges\n"));
  if ((*NumRanges) < MAX_RANGES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  *NumRanges = 0;

  // Get Pointer to HOB
  Hob.Raw = (UINT8 *)(UINTN)FspHobList;
  ASSERT (Hob.Raw != NULL);
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw)) != NULL) {
    if ((Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED)) {
      MemoryMap[*NumRanges].PhysicalAddress =  Hob.ResourceDescriptor->PhysicalStart;
      MemoryMap[*NumRanges].CpuAddress      =  Hob.ResourceDescriptor->PhysicalStart;
      MemoryMap[*NumRanges].RangeLength     =  Hob.ResourceDescriptor->ResourceLength;
      MemoryMap[*NumRanges].Type            =  DualChannelDdrReservedMemory;
      (*NumRanges)++;
      DEBUG ((
        DEBUG_INFO,
        " GetReservedMemoryRanges Base:0x%016lX, Size: 0x%016lX\n", \
        Hob.ResourceDescriptor->PhysicalStart, \
        Hob.ResourceDescriptor->ResourceLength
        ));
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PreFspmHobProcess (
  IN VOID  *FspHobList
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINT64                FspMemorySize;
  EFI_PHYSICAL_ADDRESS  FspMemoryBase;
  BOOLEAN               FoundFspMemHob;
  UINT64                SpaceAfterFSPReservedMemory = 0;

  FspMemorySize  = 0;
  FspMemoryBase  = 0;
  FoundFspMemHob = FALSE;

  //
  // Parse the hob list from fsp
  // Report all the resource hob except the memory between 1M and 4G
  //
  Hob.Raw = (UINT8 *)(UINTN)FspHobList;
  DEBUG ((DEBUG_INFO, "FspHobList - 0x%x\n", FspHobList));

  // In HOBs returned by FSP-M, there is an unintended memory region overlap:
  // e.g.:
  // 0x00000000 - 0x80000000 Empty Memory
  // 0x80000000 - 0xFFFFFFFF Reserved
  // ......
  // 0x7F000000 - 0x80000000 Taken by FSP
  // Since AMD's current FSP implementation doesn't FIX empty memory size, so we do it here.

  // Firstly we pick up FSP Memory information.
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw)) != NULL) {
    if (  (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED) // Found the low memory length below 4G
       && (Hob.ResourceDescriptor->PhysicalStart >= BASE_1MB)
       && (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength <= BASE_4GB)
       && (CompareGuid (&Hob.ResourceDescriptor->Owner, &gFspReservedMemoryResourceHobGuid)))
    {
      FoundFspMemHob = TRUE;
      FspMemoryBase  = Hob.ResourceDescriptor->PhysicalStart;
      FspMemorySize  = Hob.ResourceDescriptor->ResourceLength;
      DEBUG ((DEBUG_INFO, "Find fsp mem hob, base 0x%llx, len 0x%llx\n", FspMemoryBase, FspMemorySize));
      FSP_MEMORY_REGION_HOB  *FspRegion = BuildGuidHob (&gFspReservedMemoryResourceHobGuid, sizeof (FSP_MEMORY_REGION_HOB));
      FspRegion->BeginAddress = Hob.ResourceDescriptor->PhysicalStart;
      FspRegion->Length       = Hob.ResourceDescriptor->ResourceLength;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  Hob.Raw = (UINT8 *)(UINTN)FspHobList;

  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw)) != NULL) {
    DEBUG ((DEBUG_INFO, "\nResourceType: 0x%x\n", Hob.ResourceDescriptor->ResourceType));
    if ((Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) ||
        (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED))
    {
      DEBUG ((DEBUG_INFO, "ResourceAttribute: 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
      DEBUG ((DEBUG_INFO, "PhysicalStart: 0x%llx\n", Hob.ResourceDescriptor->PhysicalStart));
      DEBUG ((DEBUG_INFO, "ResourceLength: 0x%llx\n", Hob.ResourceDescriptor->ResourceLength));
      DEBUG ((DEBUG_INFO, "Owner: %g\n\n", &Hob.ResourceDescriptor->Owner));
    }

    if (  (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) // Found the low memory length below 4G
       && (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength <= BASE_4GB))
    {
      // Now we fix the FSP memory overlap issue.
      if (  (Hob.ResourceDescriptor->PhysicalStart < FspMemoryBase)
         && (Hob.ResourceDescriptor->PhysicalStart+Hob.ResourceDescriptor->ResourceLength >= FspMemoryBase+FspMemorySize))
      {
        DEBUG ((DEBUG_ERROR, "Found overlap! Adjusting (%llx->%llx)\n", Hob.ResourceDescriptor->ResourceLength, Hob.ResourceDescriptor->ResourceLength-FspMemorySize));
        SpaceAfterFSPReservedMemory             = (Hob.ResourceDescriptor->PhysicalStart+Hob.ResourceDescriptor->ResourceLength)-(FspMemoryBase+FspMemorySize);
        Hob.ResourceDescriptor->ResourceLength -= (FspMemorySize+SpaceAfterFSPReservedMemory);
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
      continue;
    }

    if (  (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED) // Found the low memory length below 4G
       && (Hob.ResourceDescriptor->PhysicalStart >= BASE_1MB)
       && (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength <= BASE_4GB)
       && (CompareGuid (&Hob.ResourceDescriptor->Owner, &gFspReservedMemoryResourceHobGuid)))
    {
      FoundFspMemHob = TRUE;
      FspMemoryBase  = Hob.ResourceDescriptor->PhysicalStart;
      FspMemorySize  = Hob.ResourceDescriptor->ResourceLength;
      DEBUG ((DEBUG_INFO, "Find fsp mem hob, base 0x%llx, len 0x%llx\n", FspMemoryBase, FspMemorySize));
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  if (!FoundFspMemHob) {
    DEBUG ((DEBUG_INFO, "Didn't find the fsp used memory information.\n"));
    // ASSERT(FALSE);
  }

  if (SpaceAfterFSPReservedMemory) {
    DEBUG ((DEBUG_INFO, "Left some space after FSP. Creating space for them.\n"));
  }

  DEBUG ((DEBUG_INFO, "FspMemoryBase: 0x%x.\n", FspMemoryBase));
  DEBUG ((DEBUG_INFO, "FspMemorySize: 0x%x.\n", FspMemorySize));
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,                     // MemoryType,
    (
     EFI_RESOURCE_ATTRIBUTE_PRESENT |
     EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
     EFI_RESOURCE_ATTRIBUTE_TESTED |
     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
    ),
    FspMemoryBase,                            // MemoryBegin
    FspMemorySize                             // MemoryLength
    );
  BuildMemoryAllocationHob (FspMemoryBase, FspMemorySize, EfiRuntimeServicesCode);

  Hob.Raw = GetNextGuidHob (&gFspExportedInterfaceHobGuid, FspHobList);
  FSP_EXPORTED_INTERFACE_HOB  *ExportedInterfaceHob = GET_GUID_HOB_DATA (Hob.Raw);
  EFI_PEI_PPI_DESCRIPTOR      *FspProvidedPpiList   = AllocatePool (sizeof (EFI_PEI_PPI_DESCRIPTOR)*2);

  if (FspProvidedPpiList != NULL) {
    FspProvidedPpiList[0].Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI);
    FspProvidedPpiList[0].Guid  = &gAmdPspFtpmPpiGuid;
    FspProvidedPpiList[0].Ppi   = ExportedInterfaceHob->PspFtpmPpi;
    FspProvidedPpiList[1].Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    FspProvidedPpiList[1].Guid  = &gEfiPeiReset2PpiGuid;
    FspProvidedPpiList[1].Ppi   = ExportedInterfaceHob->Reset2Ppi;
  }

  PeiServicesInstallPpi (FspProvidedPpiList);

  Hob.Raw = GetNextGuidHob (&gAmdPspApobHobGuid, FspHobList);
  APOBLIB_INFO  *ApobHob = GET_GUID_HOB_DATA (Hob.Raw);

  if (ApobHob->Supported) {
    DEBUG ((DEBUG_INFO, "FSP-M Wrapper: Reserving APOB region %p+%x\n", (UINTN)ApobHob->ApobAddr, ApobHob->ApobSize));
    BuildMemoryAllocationHob (ApobHob->ApobAddr, ApobHob->ApobSize, EfiACPIMemoryNVS);
  }

  return EFI_SUCCESS;
}

VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64  *MtrrValidBitsMask,
  OUT UINT64  *MtrrValidAddressMask
  );

EFI_STATUS
EFIAPI
SetPeiCacheMode (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  VOID                    *FspHobList
  )
{
  EFI_STATUS  Status;

  EFI_BOOT_MODE  BootMode;
  UINT64         MemoryLength;
  UINT64         MemOverflow;
  UINT64         MemoryLengthUc;
  UINT64         MaxMemoryLength;
  UINT64         LowMemoryLength;
  UINT64         HighMemoryLength;
  UINT8          Index;
  MTRR_SETTINGS  MtrrSetting;
  UINT64         MsrData;
  UINT64         MtrrValidBitsMask;
  UINT64         MtrrValidAddressMask;

  MtrrLibInitializeMtrrMask (
    &MtrrValidBitsMask,
    &MtrrValidAddressMask
    );

  //
  // Variable initialization
  //
  LowMemoryLength  = 0;
  HighMemoryLength = 0;
  MemoryLengthUc   = 0;

  Status = (*PeiServices)->GetBootMode (
                             PeiServices,
                             &BootMode
                             );

  //
  // Determine memory usage
  //
  GetMemorySize (
    PeiServices,
    &LowMemoryLength,
    &HighMemoryLength,
    NULL,
    NULL,
    FspHobList
    );

  MaxMemoryLength = LowMemoryLength;

  //
  // Round up to nearest 256MB with high memory and 64MB w/o high memory
  //
  if (HighMemoryLength != 0 ) {
    MemOverflow = (LowMemoryLength & 0x0fffffff);
    if (MemOverflow != 0) {
      MaxMemoryLength = LowMemoryLength + (0x10000000 - MemOverflow);
    }
  } else {
    MemOverflow = (LowMemoryLength & 0x03ffffff);
    if (MemOverflow != 0) {
      MaxMemoryLength = LowMemoryLength + (0x4000000 - MemOverflow);
    }
  }

  ZeroMem (&MtrrSetting, sizeof (MTRR_SETTINGS));
  for (Index = 0; Index < 2; Index++) {
    MtrrSetting.Fixed.Mtrr[Index] = 0x1E1E1E1E1E1E1E1Eul;
  }

  // 0xA0000-0xBFFFF used for ASEG which cache type is controlled by bit 10:8 of SMMMask(MSR 0xC0010113)
  for (Index = 3; Index < 11; Index++) {
    MtrrSetting.Fixed.Mtrr[Index] = 0x1C1C1C1C1C1C1C1Cul;
  }

  //
  // Cache the flash area to improve the boot performance in PEI phase
  //
  Index                                  = 0;
  MtrrSetting.Variables.Mtrr[Index].Base = FixedPcdGet32 (PcdFlashAreaBaseAddress) | CacheWriteProtected;
  MtrrSetting.Variables.Mtrr[Index].Mask = ((~((UINT64)(FixedPcdGet32 (PcdFlashAreaSize) - 1))) & MtrrValidBitsMask) | MTRR_LIB_CACHE_MTRR_ENABLED;

  Index++;

  MemOverflow = 0;
  while (MaxMemoryLength > MemOverflow) {
    MtrrSetting.Variables.Mtrr[Index].Base = (MemOverflow & MtrrValidAddressMask) | CacheWriteBack;
    MemoryLength                           = MaxMemoryLength - MemOverflow;
    MemoryLength                           = GetPowerOfTwo64 (MemoryLength);
    MtrrSetting.Variables.Mtrr[Index].Mask = ((~(MemoryLength - 1)) & MtrrValidBitsMask) | MTRR_LIB_CACHE_MTRR_ENABLED;

    MemOverflow += MemoryLength;
    Index++;
  }

  MemoryLength = LowMemoryLength;

  while (MaxMemoryLength != MemoryLength) {
    MemoryLengthUc = GetPowerOfTwo64 (MaxMemoryLength - MemoryLength);

    MtrrSetting.Variables.Mtrr[Index].Base = ((MaxMemoryLength - MemoryLengthUc) & MtrrValidAddressMask) | CacheUncacheable;
    MtrrSetting.Variables.Mtrr[Index].Mask = ((~(MemoryLengthUc   - 1)) & MtrrValidBitsMask) | MTRR_LIB_CACHE_MTRR_ENABLED;
    MaxMemoryLength                       -= MemoryLengthUc;
    Index++;
  }

  if (HighMemoryLength > 0) {
    MsrData  = AsmReadMsr64 (0xC0010010ul);
    MsrData |= BIT22;
    AsmWriteMsr64 (0xC0010010ul, MsrData);
  }

  for (Index = 0; Index < MTRR_NUMBER_OF_VARIABLE_MTRR; Index++) {
    if (MtrrSetting.Variables.Mtrr[Index].Base == 0) {
      break;
    }

    DEBUG ((DEBUG_INFO, "Base=%lx, Mask=%lx\n", MtrrSetting.Variables.Mtrr[Index].Base, MtrrSetting.Variables.Mtrr[Index].Mask));
  }

  //
  // set FE/E bits for IA32_MTRR_DEF_TYPE
  //
  MtrrSetting.MtrrDefType |=  3 <<10;

  AsmWriteMsr64 (0xC0010010ul, (AsmReadMsr64 (0xC0010010ul) | (1 << 19)));
  MtrrSetAllMtrrs (&MtrrSetting);
  AsmWriteMsr64 (0xC0010010ul, (AsmReadMsr64 (0xC0010010ul) & (~(1 << 19))));
  //
  // Dump MTRR Setting
  //
  MtrrDebugPrintAllMtrrs ();

  return Status;
}

EFI_STATUS
GetMemorySize (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  OUT UINT64                  *LowMemoryLength,
  OUT UINT64                  *HighMemoryLength,
  OUT UINT64                  *GraphicMemoryBase OPTIONAL,
  OUT UINT64                  *GraphicMemoryLength OPTIONAL,
  IN VOID                     *FspHobList
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PEI_HOB_POINTERS  FspHob;

  *HighMemoryLength = 0;
  *LowMemoryLength  = 0x100000;
  // We don't support getting UMA information from FSP hob for now.
  if ((GraphicMemoryBase != NULL) || (GraphicMemoryLength != NULL)) {
    return EFI_UNSUPPORTED;
  }

  // Get HOB Data
  Hob.Raw    = (UINT8 *)(UINTN)FspHobList;
  FspHob.Raw = NULL;
  ASSERT (Hob.Raw != NULL);
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw)) != NULL) {
    if (CompareGuid (&Hob.ResourceDescriptor->Owner, &gFspReservedMemoryResourceHobGuid)) {
      FspHob.Raw = Hob.Raw;
    }

    if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      if (Hob.ResourceDescriptor->PhysicalStart < SIZE_4GB) {
        if (LowMemoryLength != NULL) {
          *LowMemoryLength = Hob.ResourceDescriptor->ResourceLength;
        }
      } else if (Hob.ResourceDescriptor->PhysicalStart >= SIZE_4GB) {
        if (HighMemoryLength != NULL) {
          *HighMemoryLength = Hob.ResourceDescriptor->ResourceLength;
        }
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  if ((FspHob.Raw != NULL) && (*LowMemoryLength == FspHob.ResourceDescriptor->PhysicalStart)) {
    // FSP should also be cached.
    *LowMemoryLength += FspHob.ResourceDescriptor->ResourceLength;
    DEBUG ((DEBUG_INFO, "Patching cache region for FSP area!\n"));
  }

  return EFI_SUCCESS;
}
