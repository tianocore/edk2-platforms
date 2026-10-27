/** @file
  Platform SMBIOS driver for the Marvell CN10K SoC.

  This file contains the driver entry point, the board/DIMM configuration
  gathering and the helpers shared by the per-type SMBIOS installers.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/Smbios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CN10KConfigLib.h>
#include <Library/FdtLib.h>

#include "SmbiosPlatformDxe.h"

/**
  Return whether the given product ID is a supported CN10K/CNF10K variant.

  @param[in]  CpuType  Product ID read from the SMMU IIDR register.

  @retval TRUE   The product ID is supported.
  @retval FALSE  The product ID is not supported.
**/
BOOLEAN
CpuTypeIsSupported (
  IN UINT8  CpuType
  )
{
  switch (CpuType) {
    case Cn10kaProductId:
    case Cnf10kaProductId:
    case Cnf10kbProductId:
    case Cn10kbProductId:
      return TRUE;
    default:
      return FALSE;
  }
}

/**
  Decode the primary SDRAM data width from the SPD data.

  DDR5 modules report the per-channel width and the number of channels in
  byte 235; DDR4 modules report the width in byte 13. The result is scaled by
  the number of channels per DIMM for DDR5.

  @param[in]  Spd  Pointer to the raw SPD data.

  @return  Total primary data bus width in bits, or 0 for an unknown encoding.
**/
STATIC
UINT16
GetBusDataWidth (
  IN CONST UINT8  *Spd
  )
{
  UINT8   PrimaryBusWidth;
  UINT16  RetVal;

  if (Spd[2] == CN10K_SPD_MEMORY_TYPE_DDR5) {
    // DDR5
    PrimaryBusWidth = Spd[235] & 0x7;
  } else {
    PrimaryBusWidth = Spd[13] & 0x7;
  }

  switch (PrimaryBusWidth) {
    case 0x00:
      RetVal = 8;
      break;
    case 0x01:
      RetVal = 16;
      break;
    case 0x02:
      RetVal = 32;
      break;
    case 0x03:
      RetVal = 64;
      break;
    default:
      RetVal = 0;
  }

  if (Spd[2] == CN10K_SPD_MEMORY_TYPE_DDR5) {
    // DDR5: width per channel * channels per DIMM = total width
    RetVal *= ((Spd[235] >> 5) & 0x3) + 1; // Multiply by number of channels per DIMM
  }

  return RetVal;
}

/**
  Decode the SDRAM extension bus width from the SPD data.

  DDR5 modules report the width and channel count in byte 235; DDR4 modules
  report the width in byte 13. The result is scaled by the number of channels
  per DIMM for DDR5.

  @param[in]  Spd  Pointer to the raw SPD data.

  @return  Extension bus width in bits, or 0 for an unknown encoding.
**/
STATIC
UINT8
GetBusExtWidth (
  IN CONST UINT8  *Spd
  )
{
  UINT8  ExtBusWidth;
  UINT8  RetVal;

  if (Spd[2] == CN10K_SPD_MEMORY_TYPE_DDR5) {
    // DDR5
    ExtBusWidth = (Spd[235] >> 3) & 0x3;

    switch (ExtBusWidth) {
      case 0x00:
        RetVal = 0;
        break;
      case 0x01:
        RetVal = 4;
        break;
      case 0x02:
        RetVal = 8;
        break;
      default:
        RetVal = 0;
    }

    RetVal *= ((Spd[235] >> 5) & 0x3) + 1; // Multiply by number of channels per DIMM
  } else {
    // DDR4
    ExtBusWidth = (Spd[13] >> 3) & 0x3;

    switch (ExtBusWidth) {
      case 0x00:
        RetVal = 0;
        break;
      case 0x01:
        RetVal = 4;
        break;
      default:
        RetVal = 0;
    }
  }

  return RetVal;
}

/**
  Return the total SDRAM bus width from the SPD data.

  @param[in]  Spd  Pointer to the raw SPD data.

  @return  Primary plus extension bus width in bits.
**/
STATIC
UINT16
GetBusTotalWidth (
  IN CONST UINT8  *Spd
  )
{
  UINT16  DataWidth = GetBusDataWidth (Spd);
  UINT8   ExtWidth  = GetBusExtWidth (Spd);

  return DataWidth + ExtWidth;
}

/**
  Decode the SDRAM data rate from the SPD minimum cycle time.

  DDR5 uses the tCKAVGmin field in bytes 20-21 and the DDR5 SPD speed grade
  table. DDR4 uses the tCKAVGmin MTB field in byte 18.

  @param[in]  Spd  Pointer to the raw SPD data.

  @return  Data rate in MT/s, or 0 for an unknown encoding.
**/
STATIC
UINT32
GetDramSpeed (
  IN CONST UINT8  *Spd
  )
{
  UINT32  Speed;

  if (Spd[2] == CN10K_SPD_MEMORY_TYPE_DDR5) {
    // DDR5

    UINT16  TckavgMin = ((UINT16)Spd[21] << 8) | Spd[20]; // In picoseconds

    // See DDR5 SPD spec -> 1.0 Example SPD Codes tCKmin (Grade vs tCK table)
    switch (TckavgMin) {
      case 625:
        Speed = 3200;
        break;
      case 555:
        Speed = 3600;
        break;
      case 500:
        Speed = 4000;
        break;
      case 454:
        Speed = 4400;
        break;
      case 416:
        Speed = 4800;
        break;
      case 384:
        Speed = 5200;
        break;
      case 357:
        Speed = 5600;
        break;
      case 333:
        Speed = 6000;
        break;
      case 312:
        Speed = 6400;
        break;
      case 294:
        Speed = 6800;
        break;
      case 277:
        Speed = 7200;
        break;
      case 263:
        Speed = 7600;
        break;
      case 250:
        Speed = 8000;
        break;
      case 238:
        Speed = 8400;
        break;
      default:
        Speed = 0;
    }
  } else if (Spd[2] == CN10K_SPD_MEMORY_TYPE_DDR4) {
    // DDR4

    UINT8  MtbUnits = Spd[18];
    // Refer to DDR4 SPD spec -> Byte 18 (0x012): SDRAM Minimum Cycle Time (tCKAVGmin)
    switch (MtbUnits) {
      case 10:
        Speed = 1600;
        break;
      case 9:
        Speed = 1866;
        break;
      case 8:
        Speed = 2133;
        break;
      case 7:
        Speed = 2400;
        break;
      case 6:
        Speed = 2666;
        break;
      case 5:
        Speed = 3200;
        break;
      default:
        Speed = 0;
    }
  } else {
    Speed = 0;
  }

  return Speed;
}

/**
  Collect the DRAM configuration for every DSS in the node.

  For each DSS the matching DDR-SPD-DATA node is read from the configuration
  device tree. If the node is absent, the shared DDR-SPD-DATA node is used for
  soldered memory and the DDR-DMC-MASK is checked to decide whether the DSS is
  populated. DDR4 and DDR5 SPD layouts are decoded into rank, size, speed,
  width, manufacturer, serial number, and part number.

  @param[in,out]  Context  Driver context whose DimmCfg array is filled.
**/
STATIC
VOID
CN10KGetRamConfiguration (
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  UINT8        Dss;
  CHAR8        String[50];
  CONST UINT8  *Spd;
  INT32        Length;
  DIMM_CFG     *DimmCfg;
  UINT8        PackageRanks;
  UINT8        DieCount;
  UINT8        SignalLoad;
  UINT8        SpdVal;
  UINT8        SdramIoWidth;
  UINT64       DieDensity;
  UINT64       Mask = (UINT64)(~((UINT64)0)); // Assume all memory controllers are enabled.

  DEBUG ((DEBUG_INFO, "%a: start\n", __FUNCTION__));
  // Single DSS includes 2 DMCs. Single DMC handles single memory channel.
  // Single DSS can be equipped with 1 DIMM, so max number of DIMMs is equal to number of DSSs.
  // Considering above one DIMM is handled by 2 DMCs (1 DSS).

  for (Dss = 0; Dss < FixedPcdGet8 (PcdDssPerNode); Dss++, Spd = NULL) {
    DimmCfg = &(Context->NodeConfig->DimmCfg[Dss]);

    // Get SPD data of DIMM attached to given DSS
    // (below node is only available when DDR-DMC-MASK allows it)
    AsciiSPrint (String, sizeof (String), "DDR-SPD-DATA.DSS%d", Dss);

    Spd = CN10KGetConfigProperty (String, &Length);

    if (Spd == NULL) {
      // Couldn't locate DDR-SPD-DATA.DSSx node,
      // Either due to DIMM absence in slot or due to soldered RAM existence.
      // In the latter case, there might be virtual SPD data available. Let's check it.
      Spd = CN10KGetConfigProperty ("DDR-SPD-DATA", &Length);

      // Are controllers are disabled by mask? Code shall report no memory for such case.
      CN10KGetConfigPropertyInt ("DDR-DMC-MASK", &Mask, 0);

      // EBF code: enough to disable only even numbered controllers, to have whole DSS "disabled".
      if (((Mask & (1 << (2 * Dss))) == 0) || (Spd == NULL)) {
        DimmCfg->Status = DimmStatusEmpty;
        continue;
      }
    }

    DimmCfg->Status  = DimmStatusAvailable;
    DimmCfg->Type    = Spd[2];
    DimmCfg->SubType = Spd[3];

    if ((Spd[2] == CN10K_SPD_MEMORY_TYPE_DDR4) && (Length >= 320)) {
      // DDR4: bytes 384~511 are reserved for end users.

      DEBUG ((DEBUG_INFO, "%a: Analysing DDR4[%d] SPD %d bytes\n", __FUNCTION__, Dss, Length));
      PackageRanks = ((Spd[12] >> 3) & 0x7) + 1; // Module organization byte: bits 5-3
      DieCount     = ((Spd[6] >> 4) & 0x7) + 1;
      SignalLoad   = (Spd[6] & 0x3);

      if (SignalLoad == 0x2) {
        DimmCfg->Rank = DieCount * PackageRanks;
      } else {
        DimmCfg->Rank = PackageRanks;
      }

      DimmCfg->SizeMb  = 1 << (6 + (Spd[4] & 0xf) + (Spd[13] & 0x3) - (Spd[12] & 0x3));
      DimmCfg->SizeMb *= DimmCfg->Rank;

      DimmCfg->Speed      = GetDramSpeed (Spd);
      DimmCfg->TotalWidth = GetBusTotalWidth (Spd);
      DimmCfg->DataWidth  = GetBusDataWidth (Spd);

      if (Length >= 384) {
        // Gather information only when enough SPD data are provided

        DimmCfg->Mfg = ((UINT16)Spd[321] << 8) | Spd[320];

        AsciiSPrint (
          DimmCfg->Serial,
          sizeof (DimmCfg->Serial),
          "%02x%02x%02x%02x",
          Spd[328],
          Spd[327],
          Spd[326],
          Spd[325]
          );

        AsciiStrnCpyS (
          DimmCfg->PartNo,
          sizeof (DimmCfg->PartNo),
          (CHAR8 *)&Spd[329],
          348 - 328
          );

        DEBUG ((DEBUG_VERBOSE, "%a: PartNo %a\n", __FUNCTION__, DimmCfg->PartNo));
        DEBUG ((DEBUG_VERBOSE, "%a: Serial %a\n", __FUNCTION__, DimmCfg->Serial));
        DEBUG ((DEBUG_VERBOSE, "%a: Mfg 0x%x\n", __FUNCTION__, DimmCfg->Mfg));
      }
    } else if ((Spd[2] == CN10K_SPD_MEMORY_TYPE_DDR5) && (Length >= 512)) {
      // DDR5: bytes 640~1023 are reserved for end users.

      DEBUG ((DEBUG_INFO, "%a: Analysing DDR5[%d] SPD %d bytes\n", __FUNCTION__, Dss, Length));
      SdramIoWidth = 4 << ((Spd[6] >> 5) & 0x7); // 0->x4, 1->x8, 2->x16, 3->x32

      DimmCfg->Rank       = ((Spd[234] >> 3) & 0x7) + 1; // Module organization byte: bits 5-3
      DimmCfg->Speed      = GetDramSpeed (Spd);
      DimmCfg->TotalWidth = GetBusTotalWidth (Spd);
      DimmCfg->DataWidth  = GetBusDataWidth (Spd);

      // Calculate Dies per Package
      SpdVal = ((Spd[4] >> 4) & 0x7);
      switch (SpdVal) {
        case 0x00:
          DieCount = 1;
          break;
        case 0x02:
          DieCount = 2;
          break;
        case 0x03:
          DieCount = 4;
          break;
        case 0x04:
          DieCount = 8;
          break;
        case 0x05:
          DieCount = 16;
          break;
        default:
          DieCount = 0; // Shall not happened
      }

      // Calculate SDRAM Density Per Die
      SpdVal = (Spd[4] & 0x1F);

      switch (SpdVal) {
        case 0x01:
          DieDensity = SIZE_4GB;
          break;
        case 0x02:
          DieDensity = SIZE_8GB;
          break;
        case 0x03:
          DieDensity = SIZE_8GB + SIZE_4GB;   // SIZE_12GB;
          break;
        case 0x04:
          DieDensity = SIZE_16GB;
          break;
        case 0x05:
          DieDensity = SIZE_16GB + SIZE_8GB;  // SIZE_24GB;
          break;
        case 0x06:
          DieDensity = SIZE_32GB;
          break;
        case 0x07:
          DieDensity = SIZE_32GB + SIZE_16GB; // SIZE_48GB;
          break;
        case 0x08:
          DieDensity = SIZE_64GB;
          break;
        default:
          DieDensity = 0;
      }

      /* To calculate the total capacity in bytes for a symmetric module,
       * the following math applies:
       * Capacity in bytes =
       * Number of channels per DIMM *     (SPD byte 235 Bits 6~5)
       * Primary bus width per channel /   (SPD byte 235 Bits 2~0)
       * SDRAM I/O Width *                 (SPD byte 6 Bits 7~5)
       * Die per package *                 (SPD byte 4 Bits 7~5)
       * SDRAM density per die /           (SPD byte 4 Bits 4~0)
       * 8 *
       * Package ranks per channel
       */
      DimmCfg->SizeMb = (DimmCfg->DataWidth / SdramIoWidth) *
                        DieCount *
                        (DieDensity / 8) *
                        DimmCfg->Rank /
                        0x100000;       // Recalculate to MB

      DEBUG ((DEBUG_VERBOSE, "%a: DataWidth    = %d\n", __FUNCTION__, DimmCfg->DataWidth));
      DEBUG ((DEBUG_VERBOSE, "%a: SdramIoWidth = %d\n", __FUNCTION__, SdramIoWidth));
      DEBUG ((DEBUG_VERBOSE, "%a: DieCount     = %d\n", __FUNCTION__, DieCount));
      DEBUG ((DEBUG_VERBOSE, "%a: DieDensity   = 0x%lx\n", __FUNCTION__, DieDensity));
      DEBUG ((DEBUG_VERBOSE, "%a: Ranks        = %d\n", __FUNCTION__, DimmCfg->Rank));
      DEBUG ((DEBUG_VERBOSE, "%a: Size         = %d (MB)\n", __FUNCTION__, DimmCfg->SizeMb));

      if (Length >= 640) {
        // Gather information only when enough SPD data are provided
        // Byte 512 (0x200): Module Manufacturer ID Code, First Byte
        // Byte 513 (0x201): Module Manufacturer ID Code, Second Byte

        DimmCfg->Mfg = ((UINT16)Spd[513] << 8) | Spd[512];

        AsciiSPrint (
          DimmCfg->Serial,
          sizeof (DimmCfg->Serial),
          "%02x%02x%02x%02x",
          Spd[520],
          Spd[519],
          Spd[518],
          Spd[517]
          );

        AsciiStrnCpyS (
          DimmCfg->PartNo,
          sizeof (DimmCfg->PartNo),
          (CHAR8 *)&Spd[521],
          550 - 520
          );

        DEBUG ((DEBUG_VERBOSE, "%a: PartNo %a\n", __FUNCTION__, DimmCfg->PartNo));
        DEBUG ((DEBUG_VERBOSE, "%a: Serial %a\n", __FUNCTION__, DimmCfg->Serial));
        DEBUG ((DEBUG_VERBOSE, "%a: Mfg 0x%x\n", __FUNCTION__, DimmCfg->Mfg));
      }
    }
  }
}

/**
  Collect the SoC and board information into the driver context.

  Derives the CPU type and version from the SMMU product ID register and reads
  the core clock, core part number, serial number, DDR speed, board model,
  serial number, and revision from the configuration device tree.

  @param[in,out]  Context  Driver context whose NodeConfig and board strings
                           are filled.
**/
STATIC
VOID
CN10KGetBoardConfiguration (
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  UINT32  Smmu0Iidr;

  DEBUG ((DEBUG_INFO, "%a: start\n", __FUNCTION__));

  Smmu0Iidr = MmioRead32 (FixedPcdGet64 (PcdSmmuIidrAddress));

  DEBUG ((DEBUG_INFO, "%a: Smmu0Iidr = 0x%x\n", __FUNCTION__, Smmu0Iidr));

  Context->NodeConfig->CPUType = (Smmu0Iidr >> 20) & 0xFF; // productid, e.g. 0xB9 for CN10KA

  AsciiSPrint (
    Context->NodeConfig->CPUVersion,
    sizeof (Context->NodeConfig->CPUVersion),
    "%c%d",
    BitFieldRead32 (Smmu0Iidr, 16, 19) + 0x41,
    BitFieldRead32 (Smmu0Iidr, 12, 15)
    );

  DEBUG ((
    DEBUG_INFO,
    "%a: CpuType = 0x%x (%a)\n",
    __FUNCTION__,
    Context->NodeConfig->CPUType,
    Context->NodeConfig->CPUVersion
    ));

  CN10KGetConfigPropertyInt (
    "CORECLOCK-FREQ-MAX",
    &(Context->NodeConfig->CPUSpeed),
    0
    );

  Context->NodeConfig->RamStart = PcdGet64 (PcdSystemMemoryBase);

  DEBUG ((
    DEBUG_INFO,
    "%a: CPUSpeed = %d, RamStart: 0x%lx\n",
    __FUNCTION__,
    Context->NodeConfig->CPUSpeed,
    Context->NodeConfig->RamStart
    ));

  // None of the current chips support multi-node, thus always use Node 0 data.
  CN10KGetConfigPropertyAscii (
    "CHIP-SKU",
    Context->NodeConfig->CPUPartNumber, // MV-CN10670A0-CF450AA-AAP-ES
    sizeof (Context->NodeConfig->CPUPartNumber)
    );

  CN10KGetConfigPropertyAscii (
    "CHIP-SERIAL",
    Context->NodeConfig->CPUSerial,
    sizeof (Context->NodeConfig->CPUSerial)
    );

  CN10KGetConfigPropertyInt (
    "DDR-SPEED",
    &Context->DdrSpeed,
    0
    );

  CN10KGetConfigPropertyAscii (
    "BOARD-MODEL",
    Context->ProductName,
    sizeof (Context->ProductName)
    );

  CN10KGetConfigPropertyAscii (
    "BOARD-SERIAL",
    Context->SystemSerialNumber,
    sizeof (Context->SystemSerialNumber)
    );

  CN10KGetConfigPropertyAscii (
    "BOARD-REVISION",
    Context->SystemRevision,
    sizeof (Context->SystemRevision)
    );

  DEBUG ((
    DEBUG_INFO,
    "%a: CPUPartNumber = %a, CPUSerial: %a, DdrSpeed: %d\n",
    __FUNCTION__,
    Context->NodeConfig->CPUPartNumber,
    Context->NodeConfig->CPUSerial,
    Context->DdrSpeed
    ));

  DEBUG ((
    DEBUG_INFO,
    "%a: ProductName = %a, SystemSerialNumber: %a, SystemRevision: %a\n",
    __FUNCTION__,
    Context->ProductName,
    Context->SystemSerialNumber,
    Context->SystemRevision
    ));
}

/**
  Read the last-level cache size from the device tree.

  Looks up the /l3-cache node and its cache-size property.

  @param[out]  CacheSize  Receives the cache size in bytes.

  @retval  EFI_SUCCESS     The cache size was found.
  @retval  EFI_ABORTED     The device tree could not be accessed.
  @retval  EFI_NOT_FOUND   The /l3-cache node is missing.
  @retval  EFI_NO_MAPPING  The cache-size property is missing.
**/
EFI_STATUS
GetLlcSizeOutOfDeviceTree (
  OUT UINT64  *CacheSize
  )
{
  INT32         L3CacheHandle;
  CONST UINT32  *L3CacheSize;
  VOID          *Fdt;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  Fdt = CN10KGetFdt ();
  if (Fdt == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: cannot get FDT\n", __FUNCTION__));
    return EFI_ABORTED;
  }

  L3CacheHandle = FdtPathOffset (Fdt, "/l3-cache");
  if (L3CacheHandle < 0) {
    DEBUG ((DEBUG_ERROR, "%a: %s node not found!\n", __FUNCTION__, L"/l3-cache"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "%a: Found: %s in DT.\n", __FUNCTION__, L"/l3-cache"));

  L3CacheSize = FdtGetProp (Fdt, L3CacheHandle, "cache-size", NULL);
  if (NULL == L3CacheSize) {
    DEBUG ((DEBUG_ERROR, "%a: %s property not found!\n", __FUNCTION__, L"\"cache-size\""));
    return EFI_NO_MAPPING;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: L3CacheSize in DT = 0x%x (%d MB)\n",
    __FUNCTION__,
    Fdt32ToCpu (*L3CacheSize),
    Fdt32ToCpu (*L3CacheSize) / SIZE_1MB
    ));

  *CacheSize = Fdt32ToCpu (*L3CacheSize);

  return EFI_SUCCESS;
}

/**
  Set all cache size fields of a Type 7 record from a size in bytes.

  The 16-bit MaximumCacheSize/InstalledSize fields use 1 KiB units unless the
  size exceeds their range, in which case 64 KiB units are used. The extended
  fields use the same units so the record stays consistent.

  @param[in,out]  Cache        Type 7 record to update.
  @param[in]  SizeInBytes  Cache size in bytes.
**/
VOID
SmbiosSetCacheSize (
  IN OUT SMBIOS_TABLE_TYPE7  *Cache,
  IN     UINT64              SizeInBytes
  )
{
  UINT64  SizeInKb;
  UINT32  Size;
  UINT8   Granularity64K;

  SizeInKb = SizeInBytes / SIZE_1KB;

  if (SizeInKb <= SMBIOS_CACHE_SIZE_MAX_SIZE_1K_GRANULARITY) {
    Size           = (UINT32)SizeInKb;
    Granularity64K = 0;
  } else {
    Size           = (UINT32)(SizeInKb / 64);
    Granularity64K = 1;
  }

  Cache->MaximumCacheSize.Size            = (UINT16)Size;
  Cache->MaximumCacheSize.Granularity64K  = Granularity64K;
  Cache->InstalledSize.Size               = (UINT16)Size;
  Cache->InstalledSize.Granularity64K     = Granularity64K;
  Cache->MaximumCacheSize2.Size           = Size;
  Cache->MaximumCacheSize2.Granularity64K = Granularity64K;
  Cache->InstalledSize2.Size              = Size;
  Cache->InstalledSize2.Granularity64K    = Granularity64K;
}

/**
  Add an SMBIOS record and optionally return its handle.

  Builds a temporary copy of the fixed structure followed by the string pack
  and passes it to Smbios->Add(). The structure is copied, so the caller may
  reuse or modify the template after the call.

  @param[in]  Smbios       SMBIOS protocol instance.
  @param[in]  Entry        Fixed SMBIOS structure to add.
  @param[in]  StringArray  NULL terminated string array, may be NULL.
  @param[out]  Handle      If not NULL, receives the handle of the added
                           record.

  @retval EFI_SUCCESS      The record was added.
  @retval Others           Allocation failed or the record was rejected.
**/
EFI_STATUS
SmbiosPlatformAddEntry (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios,
  IN  SMBIOS_STRUCTURE     *Entry,
  IN  CHAR8                **StringArray  OPTIONAL,
  OUT EFI_SMBIOS_HANDLE    *Handle        OPTIONAL
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  UINTN                    Index;
  UINTN                    StringSize;
  UINTN                    Size;
  CHAR8                    *Str;

  // Calculate the size of the fixed record and optional string pack
  Size = Entry->Length;
  if ((StringArray == NULL) || (StringArray[0] == NULL)) {
    Size += 2; // Min string section is double null
  } else {
    for (Index = 0; StringArray[Index] != NULL; Index++) {
      StringSize = AsciiStrSize (StringArray[Index]);
      Size      += StringSize;
    }

    // Don't forget the terminating double null
    Size += 1;
  }

  // Copy over Template
  Record = (EFI_SMBIOS_TABLE_HEADER *)AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Record, Entry, Entry->Length);

  if ((StringArray != NULL) && (StringArray[0] != NULL)) {
    // Append string pack
    Str = ((CHAR8 *)Record) + Record->Length;
    for (Index = 0; StringArray[Index] != NULL; Index++) {
      StringSize = AsciiStrSize (StringArray[Index]);
      CopyMem (Str, StringArray[Index], StringSize);
      Str += StringSize;
    }

    *Str = 0;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status       = Smbios->Add (
                           Smbios,
                           NULL,
                           &SmbiosHandle,
                           Record
                           );

  FreePool (Record);

  if (!EFI_ERROR (Status) && (Handle != NULL)) {
    *Handle = SmbiosHandle;
  }

  return Status;
}

/**
  Compute the total installed RAM size.

  Sums the size of every DIMM whose status is DimmStatusAvailable and stores
  the result in the node configuration, in bytes.

  @param[in,out]  Context  Driver context holding the DIMM configuration.
**/
STATIC
VOID
CN10KCalculateRamSize (
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  UINT8  Dss;

  Context->NodeConfig->RamSize = 0;

  for (Dss = 0; Dss < FixedPcdGet8 (PcdDssPerNode); Dss++) {
    if (Context->NodeConfig->DimmCfg[Dss].Status == DimmStatusAvailable) {
      Context->NodeConfig->RamSize +=
        (UINT64)Context->NodeConfig->DimmCfg[Dss].SizeMb * SIZE_1MB;
    }
  }
}

typedef
EFI_STATUS
(*CN10K_SMBIOS_INSTALL)(
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

//
// Records are installed in dependency order: a record that references another
// record (chassis, cache, memory array) is installed after its target so the
// target handle is known.
//
STATIC CONST CN10K_SMBIOS_INSTALL  mSmbiosInstallList[] = {
  InstallType3SystemEnclosure,
  InstallType2BaseboardInformation,
  InstallType0BiosInformation,
  InstallType1SystemInformation,
  InstallType7CacheInformation,
  InstallType4ProcessorInformation,
  InstallType16PhysicalMemoryArray,
  InstallType17MemoryDevice,
  InstallType19MemoryArrayMappedAddress,
  InstallType20MemoryDeviceMappedAddress,
  InstallType11OemStrings,
  InstallType12SystemConfigurationOptions,
  InstallType23SystemReset,
  InstallType32SystemBootInformation,
};

/**
  Main entry point for the SMBIOS platform driver.

  Locates the SMBIOS protocol, gathers the board and RAM configuration, and
  installs the SMBIOS records. Records that reference another record are
  installed after their target so the target handle is available. The port
  connector and system slot records are installed only on the ebb106s board.

  @param[in]  ImageHandle  Image handle for this driver.
  @param[in]  SystemTable  Pointer to the system table.

  @retval EFI_SUCCESS            All SMBIOS records were installed.
  @retval EFI_OUT_OF_RESOURCES   A context allocation failed.
  @retval Others                 The SMBIOS protocol was not found or a record
                                 could not be installed.

**/
EFI_STATUS
EFIAPI
SmbiosPlatformDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_SMBIOS_PROTOCOL   *Smbios;
  CN10K_SMBIOS_CONTEXT  *Context;
  UINTN                 Index;

  DEBUG ((DEBUG_INFO, "%a: start\n", __FUNCTION__));

  Smbios = NULL;
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: SMBIOS protocol not found (Status = %r)!\n", __FUNCTION__, Status));
    return Status;
  }

  Context = AllocateZeroPool (sizeof (*Context));
  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Couldn't allocate driver context! Terminate.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Context->NodeConfig = AllocateZeroPool (sizeof (NODE_CFG));
  if (Context->NodeConfig == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Couldn't allocate node configuration! Terminate.\n", __FUNCTION__));
    FreePool (Context);
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (Context->ProductName, sizeof (Context->ProductName), "Octeon 10 Unknown");

  CN10KGetBoardConfiguration (Context);
  CN10KGetRamConfiguration (Context);
  CN10KCalculateRamSize (Context);

  // Gather BIOS information out of PCDs:
  // NOTE: PcdFirmwareVersionString and PcdFirmwareReleaseDateString are set within Makefile
  UnicodeStrToAsciiStrS (
    (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString),
    Context->BiosVer,
    sizeof (Context->BiosVer)
    );

  UnicodeStrToAsciiStrS (
    (CHAR16 *)PcdGetPtr (PcdFirmwareReleaseDateString),
    Context->ReleaseDate,
    sizeof (Context->ReleaseDate)
    );

  DEBUG ((
    DEBUG_INFO,
    "%a: Bios Version: %a, Bios Release Date: %a\n",
    __FUNCTION__,
    Context->BiosVer,
    Context->ReleaseDate
    ));

  for (Index = 0; Index < ARRAY_SIZE (mSmbiosInstallList); Index++) {
    Status = mSmbiosInstallList[Index] (Smbios, Context);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to install SMBIOS record (Status = %r)\n",
        __FUNCTION__,
        Status
        ));
      return Status;
    }
  }

  //
  // Port connectors and system slots are board specific. Add them only for the
  // ebb106s board; AsciiStriCmp() is case insensitive.
  //
  if (AsciiStriCmp (Context->ProductName, "ebb106s") == 0) {
    Status = InstallType8PortConnectorInformation (Smbios, Context);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = InstallType9SystemSlots (Smbios, Context);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  DEBUG ((DEBUG_INFO, "%a: successfully dispatched\n", __FUNCTION__));

  return EFI_SUCCESS;
}
