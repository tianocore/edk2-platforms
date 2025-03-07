/** @file
  Implements MemoryPeim.C
  Tiano PEIM to provide the platform support functionality.
  This file implements the Platform Memory Range PPI

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2004  - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

#define MTRR_LIB_CACHE_MTRR_ENABLED  0x800
#define SYS_CFG                      0xC0010010ul

VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64  *MtrrValidBitsMask,
  OUT UINT64  *MtrrValidAddressMask
  );

/**

  This function set different memory range cache attribute.

  @param  PeiServices         Pointer to the PEI Service Table.

**/
EFI_STATUS
EFIAPI
SetPeiCacheMode (
  IN  CONST EFI_PEI_SERVICES  **PeiServices
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
    NULL
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
    MsrData  = AsmReadMsr64 (SYS_CFG);
    MsrData |= BIT22;
    AsmWriteMsr64 (SYS_CFG, MsrData);
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

  AsmWriteMsr64 (SYS_CFG, (AsmReadMsr64 (SYS_CFG) | (1 << 19)));
  MtrrSetAllMtrrs (&MtrrSetting);
  AsmWriteMsr64 (SYS_CFG, (AsmReadMsr64 (SYS_CFG) & (~(1 << 19))));
  //
  // Dump MTRR Setting
  //
  MtrrDebugPrintAllMtrrs ();
  (VOID)Status;
  return EFI_SUCCESS;
}

/**

  This function returns the different avaiable memory length.

  @param  PeiServices         Pointer to the PEI Service Table.
  @param  LowMemoryLength     Avaiable memory length below 4G address.
  @param  HighMemoryLength    Avaiable memory length above 4G address.
  @param  GraphicMemoryBase   Avaiable UMA base address.
  @param  GraphicMemoryLength Avaiable UMA length.

  @retval  LowMemoryLength     Avaiable memory length below 4G address.
  @retval  HighMemoryLength    Avaiable memory length above 4G address.
  @retval  GraphicMemoryBase   Avaiable UMA base address.
  @retval  GraphicMemoryLength Avaiable UMA length.

**/
EFI_STATUS
GetMemorySize (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  OUT UINT64                  *LowMemoryLength,
  OUT UINT64                  *HighMemoryLength,
  OUT UINT64                  *GraphicMemoryBase OPTIONAL,
  OUT UINT64                  *GraphicMemoryLength OPTIONAL
  )
{
  AMD_MEMORY_INFO_HOB          *AmdMemoryInfoHob;
  AMD_MEMORY_RANGE_DESCRIPTOR  *Range;
  UINT32                       Index;

  if (HighMemoryLength != NULL) {
    *HighMemoryLength = 0;
  }

  if (LowMemoryLength != NULL) {
    *LowMemoryLength = 0x100000;
  }

  // Get Pointer to HOB
  AmdMemoryInfoHob = GetFirstGuidHob (&gAmdMemoryInfoHobGuid);
  ASSERT (AmdMemoryInfoHob != NULL);
  // Get HOB Data
  AmdMemoryInfoHob = GET_GUID_HOB_DATA (AmdMemoryInfoHob);
  if (AmdMemoryInfoHob != NULL) {
    for (Index = 0; Index < AmdMemoryInfoHob->NumberOfDescriptor; Index++) {
      Range = (AMD_MEMORY_RANGE_DESCRIPTOR *)&AmdMemoryInfoHob->Ranges[Index];
      if (Range->Attribute == AMD_MEMORY_ATTRIBUTE_AVAILABLE) {
        if (Range->Base < SIZE_4GB) {
          if (LowMemoryLength != NULL) {
            *LowMemoryLength = Range->Size;
          }
        } else if (Range->Base >= SIZE_4GB) {
          if (HighMemoryLength != NULL) {
            *HighMemoryLength = Range->Size;
          }
        }
      } else if (Range->Attribute == AMD_MEMORY_ATTRIBUTE_UMA) {
        if (GraphicMemoryBase != NULL) {
          *GraphicMemoryBase = Range->Base;
        }

        if (GraphicMemoryLength != NULL) {
          *GraphicMemoryLength = Range->Size;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**

  This function returns the memory ranges to be enabled, along with information
  describing how the range should be used.

  @param  MemoryMap     Buffer to record details of the memory ranges.
  @param  NumRanges     On input, this contains the maximum number of memory ranges that can be described
                        in the MemoryMap buffer.

  @retval MemoryMap     The buffer will be filled in
  @retval NumRanges     will contain the actual number of memory ranges that are to be anabled.
  @retval EFI_SUCCESS   The function completed successfully.

**/
EFI_STATUS
GetAvailableMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges
  )
{
  AMD_MEMORY_INFO_HOB          *AmdMemoryInfoHob;
  AMD_MEMORY_RANGE_DESCRIPTOR  *Range;
  UINT32                       Index;

  DEBUG ((DEBUG_INFO, "GetAvailableMemoryRanges++\n"));
  if ((*NumRanges) < MAX_RANGES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  *NumRanges = 0;

  // Get Pointer to HOB
  AmdMemoryInfoHob = GetFirstGuidHob (&gAmdMemoryInfoHobGuid);
  ASSERT (AmdMemoryInfoHob != NULL);
  // Get HOB Data
  AmdMemoryInfoHob = GET_GUID_HOB_DATA (AmdMemoryInfoHob);
  if (AmdMemoryInfoHob != NULL) {
    for (Index = 0; Index < AmdMemoryInfoHob->NumberOfDescriptor; Index++) {
      Range = (AMD_MEMORY_RANGE_DESCRIPTOR *)&AmdMemoryInfoHob->Ranges[Index];
      if (Range->Attribute == AMD_MEMORY_ATTRIBUTE_AVAILABLE) {
        MemoryMap[*NumRanges].PhysicalAddress = Range->Base;
        MemoryMap[*NumRanges].CpuAddress      = Range->Base;
        MemoryMap[*NumRanges].RangeLength     = Range->Size;
        MemoryMap[*NumRanges].Type            = DualChannelDdrMainMemory;
        (*NumRanges)++;
        DEBUG ((DEBUG_INFO, " Base:0x%016lX, Size: 0x%016lX\n", Range->Base, Range->Size));
      }
    }
  }

  return EFI_SUCCESS;
}

/**

  This function returns the memory ranges to be reserved, along with information
  describing how the range should be used.

  @param  MemoryMap     Buffer to record details of the memory ranges.
  @param  NumRanges     On input, this contains the maximum number of memory ranges that can be described
                        in the MemoryMap buffer.

  @retval MemoryMap     The buffer will be filled in
  @retval NumRanges     will contain the actual number of memory ranges that are to be reserved.
  @retval EFI_SUCCESS   The function completed successfully.

**/
EFI_STATUS
GetReservedMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges
  )
{
  AMD_MEMORY_INFO_HOB          *AmdMemoryInfoHob;
  AMD_MEMORY_RANGE_DESCRIPTOR  *Range;
  UINT32                       Index;

  DEBUG ((DEBUG_INFO, "GetReservedMemoryRanges\n"));
  if ((*NumRanges) < MAX_RANGES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  *NumRanges = 0;

  // Get Pointer to HOB
  AmdMemoryInfoHob = GetFirstGuidHob (&gAmdMemoryInfoHobGuid);
  ASSERT (AmdMemoryInfoHob != NULL);
  // Get HOB Data
  AmdMemoryInfoHob = GET_GUID_HOB_DATA (AmdMemoryInfoHob);
  if (AmdMemoryInfoHob != NULL) {
    for (Index = 0; Index < AmdMemoryInfoHob->NumberOfDescriptor; Index++) {
      Range = (AMD_MEMORY_RANGE_DESCRIPTOR *)&AmdMemoryInfoHob->Ranges[Index];
      if (Range->Base > SIZE_4GB) {
        if (Range->Attribute == AMD_MEMORY_ATTRIBUTE_RESERVED) {
          MemoryMap[*NumRanges].PhysicalAddress = Range->Base;
          MemoryMap[*NumRanges].CpuAddress      = Range->Base;
          MemoryMap[*NumRanges].RangeLength     = Range->Size;
          MemoryMap[*NumRanges].Type            = DualChannelDdrReservedMemory;
          (*NumRanges)++;
          DEBUG ((DEBUG_INFO, " GetReservedMemoryRanges Base:0x%016lX, Size: 0x%016lX\n", Range->Base, Range->Size));
        }

        if (Range->Attribute == AMD_MEMORY_ATTRIBUTE_UMA) {
          MemoryMap[*NumRanges].PhysicalAddress = Range->Base;
          MemoryMap[*NumRanges].CpuAddress      = Range->Base;
          MemoryMap[*NumRanges].RangeLength     = Range->Size;
          MemoryMap[*NumRanges].Type            = DualChannelDdrReservedMemory;
          (*NumRanges)++;
          DEBUG ((DEBUG_INFO, " GetReservedMemoryRanges Base:0x%016lX, Size: 0x%016lX\n", Range->Base, Range->Size));
        }
      }
    }
  }

  return EFI_SUCCESS;
}
