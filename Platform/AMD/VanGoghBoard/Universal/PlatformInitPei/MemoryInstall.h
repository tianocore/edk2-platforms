/** @file
  Implements MemoryInstall.h
  Framework PEIM to initialize memory on an DDR2 SDRAM Memory Controller.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2016，Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MRC_WRAPPER_H_
#define MRC_WRAPPER_H_

//
// Maximum number of memory ranges supported by the memory controller
//
#define MAX_RANGES  (4 + 5)

//
// Min. of 48MB PEI phase
//
#define  PEI_MIN_MEMORY_SIZE           (8 * 0x800000)
#define  PEI_RECOVERY_MIN_MEMORY_SIZE  (8 * 0x800000)

//
// SMRAM Memory Range
//
#define PEI_MEMORY_RANGE_SMRAM       UINT32
#define PEI_MR_SMRAM_ALL             0xFFFFFFFF
#define PEI_MR_SMRAM_NONE            0x00000000
#define PEI_MR_SMRAM_CACHEABLE_MASK  0x80000000
#define PEI_MR_SMRAM_SEGTYPE_MASK    0x00FF0000
#define PEI_MR_SMRAM_ABSEG_MASK      0x00010000
#define PEI_MR_SMRAM_HSEG_MASK       0x00020000
#define PEI_MR_SMRAM_TSEG_MASK       0x00040000

//
// SMRAM range definitions
//
#define MC_ABSEG_HSEG_PHYSICAL_START  0x000A0000
#define MC_ABSEG_HSEG_LENGTH          0x00020000
#define MC_ABSEG_CPU_START            0x000A0000
#define MC_HSEG_CPU_START             0xFEDA0000

//
// If adding additional entries, SMRAM Size
// is a multiple of 128KB.
//
#define PEI_MR_SMRAM_SIZE_MASK        0x0000FFFF
#define PEI_MR_SMRAM_SIZE_128K_MASK   0x00000001
#define PEI_MR_SMRAM_SIZE_256K_MASK   0x00000002
#define PEI_MR_SMRAM_SIZE_512K_MASK   0x00000004
#define PEI_MR_SMRAM_SIZE_1024K_MASK  0x00000008
#define PEI_MR_SMRAM_SIZE_2048K_MASK  0x00000010
#define PEI_MR_SMRAM_SIZE_4096K_MASK  0x00000020
#define PEI_MR_SMRAM_SIZE_8192K_MASK  0x00000040

#define PEI_MR_SMRAM_ABSEG_128K_NOCACHE  0x00010001
#define PEI_MR_SMRAM_HSEG_128K_CACHE     0x80020001
#define PEI_MR_SMRAM_HSEG_128K_NOCACHE   0x00020001
#define PEI_MR_SMRAM_TSEG_128K_CACHE     0x80040001
#define PEI_MR_SMRAM_TSEG_128K_NOCACHE   0x00040001
#define PEI_MR_SMRAM_TSEG_256K_CACHE     0x80040002
#define PEI_MR_SMRAM_TSEG_256K_NOCACHE   0x00040002
#define PEI_MR_SMRAM_TSEG_512K_CACHE     0x80040004
#define PEI_MR_SMRAM_TSEG_512K_NOCACHE   0x00040004
#define PEI_MR_SMRAM_TSEG_1024K_CACHE    0x80040008
#define PEI_MR_SMRAM_TSEG_1024K_NOCACHE  0x00040008
#define PEI_MR_SMRAM_TSEG_2048K_CACHE    0x80040010
#define PEI_MR_SMRAM_TSEG_2048K_NOCACHE  0x00040010
#define PEI_MR_SMRAM_TSEG_4096K_CACHE    0x80040020
#define PEI_MR_SMRAM_TSEG_4096K_NOCACHE  0x00040020
#define PEI_MR_SMRAM_TSEG_8192K_CACHE    0x80040040
#define PEI_MR_SMRAM_TSEG_8192K_NOCACHE  0x00040040

//
// Pci Memory Hole
//
#define PEI_MEMORY_RANGE_PCI_MEMORY  UINT32

typedef enum {
  Ignore,
  Quick,
  Sparse,
  Extensive
} PEI_MEMORY_TEST_OP;

// Memory range types
//
typedef enum {
  DualChannelDdrMainMemory,
  DualChannelDdrSmramCacheable,
  DualChannelDdrSmramNonCacheable,
  DualChannelDdrGraphicsMemoryCacheable,
  DualChannelDdrGraphicsMemoryNonCacheable,
  DualChannelDdrReservedMemory,
  DualChannelDdrMaxMemoryRangeType
} PEI_DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE;

//
// Memory map range information
//
typedef struct {
  EFI_PHYSICAL_ADDRESS                      PhysicalAddress;
  EFI_PHYSICAL_ADDRESS                      CpuAddress;
  EFI_PHYSICAL_ADDRESS                      RangeLength;
  PEI_DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE    Type;
} PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE;

//
// This structure stores the base and size of the ACPI reserved memory used when
// resuming from S3.  This region must be allocated by the platform code.
//
typedef struct {
  UINT32    AcpiReservedMemoryBase;
  UINT32    AcpiReservedMemorySize;
  UINT32    SystemMemoryLength;
} RESERVED_ACPI_S3_RANGE;

#define RESERVED_ACPI_S3_RANGE_OFFSET  (EFI_PAGE_SIZE - sizeof (RESERVED_ACPI_S3_RANGE))

//
// ------------------------ TSEG Base
//
// ------------------------ RESERVED_CPU_S3_SAVE_OFFSET
// CPU S3 data
// ------------------------ RESERVED_ACPI_S3_RANGE_OFFSET
// S3 Memory base structure
// ------------------------ TSEG + 1 page

#define RESERVED_CPU_S3_SAVE_OFFSET  (RESERVED_ACPI_S3_RANGE_OFFSET - sizeof (SMM_S3_RESUME_STATE))

//
// Function prototypes.
//

/**

  This function installs memory.

  @param   PeiServices    PEI Services table.
  @param   BootMode       The specific boot path that is being followed
  @param   Mch            Pointer to the DualChannelDdrMemoryInit PPI
  @param   RowConfArray   Row configuration information for each row in the system.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters was invalid.
  @retval  EFI_ABORTED            An error occurred.

**/
EFI_STATUS
InstallEfiMemory (
  IN      EFI_PEI_SERVICES  **PeiServices,
  IN      EFI_BOOT_MODE     BootMode
  );

/**

  Find memory that is reserved so PEI has some to use.

  @param  PeiServices      PEI Services table.
  @param  VariableSevices  Variable PPI instance.

  @retval EFI_SUCCESS  The function completed successfully.
                       Error value from LocatePpi()

**/
EFI_STATUS
InstallS3Memory (
  IN      EFI_PEI_SERVICES  **PeiServices,
  IN      EFI_BOOT_MODE     BootMode
  );

/**

  This function returns the memory ranges to be enabled, along with information
  describing how the range should be used.

  @param  PeiServices   PEI Services Table.
  @param  MemoryMap     Buffer to record details of the memory ranges tobe enabled.
  @param  NumRanges     On input, this contains the maximum number of memory ranges that can be described
                        in the MemoryMap buffer.

  @retval MemoryMap     The buffer will be filled in
  @retval NumRanges     will contain the actual number of memory ranges that are to be anabled.
  @retval EFI_SUCCESS   The function completed successfully.

**/
EFI_STATUS
GetMemoryMap (
  IN     EFI_PEI_SERVICES                       **PeiServices,
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges
  );

/**

  This function Get Platform Memory Size.

  @param   PeiServices    PEI Services table.
  @param   BootMode       The specific boot path that is being followed
  @param   MemorySize     Size of Memory used by Platform

  @retval  EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
GetPlatformMemorySize (
  IN      EFI_PEI_SERVICES  **PeiServices,
  IN      EFI_BOOT_MODE     BootMode,
  IN OUT  UINT64            *MemorySize
  );

#endif
