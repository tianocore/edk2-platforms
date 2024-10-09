/** @file
 *
 *  Static SMBIOS tables for the SbsaQemu platform.
 *
 *  Note: Some tables are provided by ArmPkg. The tables that are not provided by
 *  ArmPkg, but are required by SBBR, are as follows:
 *    Physical Memory Array (Type 16)
 *    Memory Device (Type 17) - For each socketed system-memory Device
 *    Memory Array Mapped Address (Type 19) - One per contiguous block per Physical Memory Array
 *
 *  Copyright (c) 2022, ARM Limited. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Base.h>
#include <Uefi/UefiSpec.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Guid/SmBios.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

/**
  A global variable to store the SMBIOS handle for table Type 16. This variable should
  only be modified by calling PhyMemArrayInfoUpdateSmbiosType16.
**/
STATIC SMBIOS_HANDLE  mPhyMemArrayInfoType16Handle = SMBIOS_HANDLE_PI_RESERVED;

/**
  SMBIOS data definition, TYPE16, Physical Memory Array
**/
SMBIOS_TABLE_TYPE16  mPhyMemArrayInfoType16 = {
  { EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, sizeof (SMBIOS_TABLE_TYPE16), 0 },
  MemoryArrayLocationSystemBoard,  // Location; (system board)
  MemoryArrayUseSystemMemory,      // Use; (system memory)
  MemoryErrorCorrectionUnknown,    // MemoryErrorCorrection; (unknown).
  0x80000000,                      // MaximumCapacity; (capacity is represented in the ExtendedMaximumCapacity field)
  0xFFFE,                          // MemoryErrorInformationHandle; (not provided)
  1,                               // NumberOfMemoryDevices
  0x0000080000000000ULL            // ExtendedMaximumCapacity; (fixed at 8 TiB for SbsaQemu)
};
CHAR8                *mPhyMemArrayInfoType16Strings[] = {
  NULL
};

/**
  SMBIOS data definition, TYPE17, Physical Memory Array
**/
SMBIOS_TABLE_TYPE17  mMemDevInfoType17 = {
  { EFI_SMBIOS_TYPE_MEMORY_DEVICE, sizeof (SMBIOS_TABLE_TYPE17), 0 },
  0,                        // MemoryArrayHandle; should match SMBIOS_TABLE_TYPE16.Handle,
                            // initialized at runtime, refer to MemDevInfoUpdateSmbiosType17
  0xFFFE,                   // MemoryErrorInformationHandle; (not provided)
  0xFFFF,                   // TotalWidth; (unknown)
  0xFFFF,                   // DataWidth; (unknown)
  0x0,                      // Size; initialized at runtime, refer to MemDevInfoUpdateSmbiosType17
  MemoryFormFactorUnknown,  // FormFactor; (unknown)
  0,                        // DeviceSet; (not part of a set)
  0,                        // DeviceLocator String
  0,                        // BankLocator String
  MemoryTypeUnknown,        // MemoryType; (unknown)
  {     // TypeDetail;
    0,  // Reserved        :1;
    0,  // Other           :1;
    1,  // Unknown         :1;
    0,  // FastPaged       :1;
    0,  // StaticColumn    :1;
    0,  // PseudoStatic    :1;
    0,  // Rambus          :1;
    0,  // Synchronous     :1;
    0,  // Cmos            :1;
    0,  // Edo             :1;
    0,  // WindowDram      :1;
    0,  // CacheDram       :1;
    0,  // Nonvolatile     :1;
    0,  // Registered      :1;
    0,  // Unbuffered      :1;
    0,  // Reserved1       :1;
  },
  0,                        // Speed; (unknown)
  0,                        // Manufacturer String
  0,                        // SerialNumber String
  0,                        // AssetTag String
  0,                        // PartNumber String
  0,                        // Attributes; (unknown rank)
  0,                        // ExtendedSize; potentially initialized at runtime if size is >= 32 GiB - 1 MiB,
                            // refer to MemDevInfoUpdateSmbiosType17
  0,                        // ConfiguredMemoryClockSpeed; (unknown)
  0,                        // MinimumVoltage; (unknown)
  0,                        // MaximumVoltage; (unknown)
  0,                        // ConfiguredVoltage; (unknown)
  MemoryTechnologyDram,     // MemoryTechnology; (DRAM)
  {                         // MemoryOperatingModeCapability
    {
      0, // Reserved                        :1;
      0, // Other                           :1;
      0, // Unknown                         :1;
      1, // VolatileMemory                  :1;
      0, // ByteAccessiblePersistentMemory  :1;
      0, // BlockAccessiblePersistentMemory :1;
      0  // Reserved                        :10;
    }
  },
  0, // FirwareVersion
  0, // ModuleManufacturerID (unknown)
  0, // ModuleProductID (unknown)
  0, // MemorySubsystemControllerManufacturerID (unknown)
  0, // MemorySubsystemControllerProductID (unknown)
  0, // NonVolatileSize
  0, // VolatileSize; initialized at runtime, refer to MemDevInfoUpdateSmbiosType17
  0, // CacheSize
  0, // LogicalSize (since MemoryType is not MemoryTypeLogicalNonVolatileDevice)
  0, // ExtendedSpeed
  0, // ExtendedConfiguredMemorySpeed
  0, // Pmic0ManufacturerID
  0, // Pmic0RevisionNumber
  0, // RcdManufacturerID
  0, // RcdRevisionNumber
};
CHAR8                *mMemDevInfoType17Strings[] = {
  NULL
};

SMBIOS_TABLE_TYPE19  mMemArrMapInfoType19 = {
  { EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, sizeof (SMBIOS_TABLE_TYPE19), 0 },
  0,  // StartingAddress; initialized at runtime, refer to MemArrMapInfoUpdateSmbiosType19
  0,  // EndingAddress; initialized at runtime, refer to MemArrMapInfoUpdateSmbiosType19
  0,  // MemoryArrayHandle; should match SMBIOS_TABLE_TYPE16.Handle,
      // initialized at runtime, refer to MemDevInfoUpdateSmbiosType17
  1,  // PartitionWidth
  0,  // ExtendedStartingAddress; potentially initialized at runtime,
      // refer to MemDevInfoUpdateSmbiosType17
  0,  // ExtendedEndingAddress; potentially initialized at runtime,
      // refer to MemDevInfoUpdateSmbiosType17
};
CHAR8                *mMemArrMapInfoType19Strings[] = {
  NULL
};

/**
  Create an SMBIOS record.

  Converts a fixed SMBIOS structure and an array of pointers to strings into
  an SMBIOS record where the strings are cat'ed on the end of the fixed record
  and terminated via a double NULL and add to SMBIOS table.

  @param  Template          Fixed SMBIOS structure, required.
  @param  StringPack        Array of strings to convert to an SMBIOS string pack.
                            NULL is OK.
  @param  DataSmbiosHandle  The new SMBIOS record handle.
                            NULL is OK.

  @return  EFI_SUCCESS on success, other values on error.
**/
STATIC
EFI_STATUS
EFIAPI
LogSmbiosData (
  IN  EFI_SMBIOS_TABLE_HEADER  *Template,
  IN  CHAR8                    **StringPack,
  OUT EFI_SMBIOS_HANDLE        *DataSmbiosHandle
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_PROTOCOL      *Smbios;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  UINTN                    Index;
  UINTN                    StringSize;
  UINTN                    Size;
  CHAR8                    *Str;

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate SMBIOS protocol: %r\n", Status));
    return Status;
  }

  //
  // Calculate the size of the fixed record and optional string pack
  //
  Size = Template->Length;
  if (StringPack == NULL) {
    //
    // If there are no strings, allow for a double-NULL
    //
    Size += 2;
  } else {
    for (Index = 0; StringPack[Index] != NULL; Index++) {
      StringSize = AsciiStrSize (StringPack[Index]);
      Size      += StringSize;
    }

    if (StringPack[0] == NULL) {
      //
      // If the only string is NULL, include it in size calculation
      //
      Size += 1;
    }

    //
    // Add an additional NULL for a terminating double NULL
    //
    Size += 1;
  }

  //
  // Copy over the template
  //
  Record = (EFI_SMBIOS_TABLE_HEADER *)AllocateZeroPool (Size);
  if (Record == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for SMBIOS table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Record, Template, Template->Length);

  //
  // Append the string pack
  //
  Str = ((CHAR8 *)Record) + Record->Length;
  for (Index = 0; StringPack[Index] != NULL; Index++) {
    StringSize = AsciiStrSize (StringPack[Index]);
    CopyMem (Str, StringPack[Index], StringSize);
    Str += StringSize;
  }

  //
  // Add an additional NULL for a terminating double NULL
  //
  *Str = 0;

  //
  // Add the table to SMBIOS
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status       = Smbios->Add (
                           Smbios,
                           gImageHandle,
                           &SmbiosHandle,
                           Record
                           );

  if ((Status == EFI_SUCCESS) && (DataSmbiosHandle != NULL)) {
    *DataSmbiosHandle = SmbiosHandle;
  }

  ASSERT_EFI_ERROR (Status);
  FreePool (Record);
  return Status;
}

/**
  Updates SMBIOS table Type 16 and creates an SMBIOS record for it.

  Stores the SMBIOS handle for table Type 16 in mPhyMemArrayInfoType16Handle.
**/
STATIC
VOID
PhyMemArrayInfoUpdateSmbiosType16 (
  VOID
  )
{
  LogSmbiosData (
    (EFI_SMBIOS_TABLE_HEADER *)&mPhyMemArrayInfoType16,
    mPhyMemArrayInfoType16Strings,
    &mPhyMemArrayInfoType16Handle
    );
}

/**
  Updates SMBIOS table Type 17 and creates an SMBIOS record for it.

  Must be called after PhyMemArrayInfoUpdateSmbiosType16.
**/
STATIC
VOID
MemDevInfoUpdateSmbiosType17 (
  VOID
  )
{
  UINT64  MemorySize;

  //
  // PhyMemArrayInfoUpdateSmbiosType16 must be called before MemDevInfoUpdateSmbiosType17
  //
  if (mPhyMemArrayInfoType16Handle == SMBIOS_HANDLE_PI_RESERVED) {
    DEBUG ((DEBUG_ERROR, "%a: mPhyMemArrayInfoType16Handle is not initialized\n", __func__));
    return;
  }

  mMemDevInfoType17.MemoryArrayHandle = mPhyMemArrayInfoType16Handle;

  MemorySize = PcdGet64 (PcdSystemMemorySize);
  if (MemorySize >= SIZE_32GB - SIZE_1MB) {
    //
    // If memory size is >= 32 GiB - 1 MiB, we need to use ExtendedSize to represent it.
    // In this case, Size is set to 0x7FFF.
    //
    mMemDevInfoType17.Size         = 0x7FFF;
    mMemDevInfoType17.ExtendedSize = MemorySize / SIZE_1MB;
  } else {
    //
    // If memory size is < 32 GiB - 1 MiB, we use just Size.
    //
    mMemDevInfoType17.Size         = MemorySize / SIZE_1MB;
    mMemDevInfoType17.ExtendedSize = 0;
  }

  //
  // All memory on SbsaQemu is volatile
  //
  mMemDevInfoType17.VolatileSize = MemorySize;

  LogSmbiosData (
    (EFI_SMBIOS_TABLE_HEADER *)&mMemDevInfoType17,
    mMemDevInfoType17Strings,
    NULL
    );
}

/**
  Updates SMBIOS table Type 19 and creates an SMBIOS record for it.

  Must be called after PhyMemArrayInfoUpdateSmbiosType16.
**/
VOID
MemArrMapInfoUpdateSmbiosType19 (
  VOID
  )
{
  UINT64  MemorySize;
  UINT64  MemoryBase;
  UINT64  StartingAddressKiB;
  UINT64  EndingAddressKiB;

  //
  // PhyMemArrayInfoUpdateSmbiosType16 must be called before MemDevInfoUpdateSmbiosType17
  //
  if (mPhyMemArrayInfoType16Handle == SMBIOS_HANDLE_PI_RESERVED) {
    DEBUG ((DEBUG_ERROR, "%a: mPhyMemArrayInfoType16Handle is not initialized\n", __func__));
    return;
  }

  mMemArrMapInfoType19.MemoryArrayHandle = mPhyMemArrayInfoType16Handle;

  MemorySize         = PcdGet64 (PcdSystemMemorySize);
  MemoryBase         = PcdGet64 (PcdSystemMemoryBase);
  StartingAddressKiB = MemoryBase / SIZE_1KB;
  EndingAddressKiB   = StartingAddressKiB + (MemorySize / SIZE_1KB) - 1;
  if (EndingAddressKiB > MAX_UINT32) {
    //
    // If the ending address cannot fit in a 32-bit unsigned integer, we use
    // ExtendedStartingAddress and ExtendedEndingAddress fields, which are 64 bits.
    // In this case, StartingAddress and EndingAddress fields are set to 0xFFFFFFFF.
    // Note that, since always starting address < ending address, if the ending address
    // can fit into 32 bits, then so can the starting address.
    //
    mMemArrMapInfoType19.StartingAddress         = 0xFFFFFFFF;
    mMemArrMapInfoType19.EndingAddress           = 0xFFFFFFFF;
    mMemArrMapInfoType19.ExtendedStartingAddress = StartingAddressKiB;
    mMemArrMapInfoType19.ExtendedEndingAddress   = EndingAddressKiB;
  } else {
    //
    // If the starting address and the ending address can fit into 32 bits, we just use
    // StartingAddress and EndingAddress fields.
    //
    mMemArrMapInfoType19.StartingAddress         = StartingAddressKiB;
    mMemArrMapInfoType19.EndingAddress           = EndingAddressKiB;
    mMemArrMapInfoType19.ExtendedStartingAddress = 0;
    mMemArrMapInfoType19.ExtendedEndingAddress   = 0;
  }

  LogSmbiosData (
    (EFI_SMBIOS_TABLE_HEADER *)&mMemArrMapInfoType19,
    mMemArrMapInfoType19Strings,
    NULL
    );
}

/**
  Driver entry point.
**/
EFI_STATUS
EFIAPI
SbsaQemuSmbiosDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  PhyMemArrayInfoUpdateSmbiosType16 ();
  MemDevInfoUpdateSmbiosType17 ();
  MemArrMapInfoUpdateSmbiosType19 ();

  return EFI_SUCCESS;
}
