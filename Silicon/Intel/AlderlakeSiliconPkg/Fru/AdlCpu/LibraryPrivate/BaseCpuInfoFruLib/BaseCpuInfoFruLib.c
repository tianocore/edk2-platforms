/** @file
  This file contains the Cpu Information for specific generation.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <CpuGenInfoFruLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Register/Cpuid.h>
#include <Library/CpuPlatformLib.h>
#include <Library/CpuInfoFruLib.h>
#include <Register/ArchitecturalMsr.h>
#include <Library/MemoryAllocationLib.h>
#include <Register/CommonMsr.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcdLib.h>
#include <Library/PchInfoLib.h>
#include <IndustryStandard/Pci22.h>
#include <Register/IgdRegs.h>

typedef struct {
  UINT32  CPUID;
  UINT8   CpuSku;
  CHAR8   *String;
} CPU_REV;

typedef struct {
  CPU_IDENTIFIER                 CpuIdentifier;
  UINT8                          SupportedCores;
  UINT8                          SupportedAtomCores;
} CPU_CORE_COUNT;

GLOBAL_REMOVE_IF_UNREFERENCED CONST CPU_REV  mProcessorRevisionTable[] = {
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlJ0, EnumCpuUlt,     "J0"},
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlK0, EnumCpuUlt,     "K0"},
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlL0, EnumCpuUlt,     "L0"},
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlQ0, EnumCpuUlt,     "Q0"},
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlR0, EnumCpuUlt,     "R0"},
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlQ0, EnumCpuUlx,     "Q0"},
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlR0, EnumCpuUlx,     "R0"},
  {CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE      + EnumAdlS0, EnumCpuUlx,     "S0"}
};

///
/// PowerLimits Override table for all SKUs. Non-cTDP parts would have '0' data for TDP level information.
///
GLOBAL_REMOVE_IF_UNREFERENCED PPM_OVERRIDE_TABLE mPowerLimitsOverrideTable[] = {
///
/// CpuIdentifier                      TDP      MSR PL1   MSR PL2    TdpUp    TdpUp    TdpNominal   TdpNominal   TdpDown    TdpDown      MSR     MSR Disablefvm
///                                              PL1       PL2        PL1      PL2       PL1          PL2          PL1        PL2        PL4         PL4
  {EnumAdlP15Watt282fCpuId,           1500,      1500,     5500,       0,      5500,      0,          5500,         0,        5500,     12300,       0}, ///  15W  282 ADL-P
  {EnumAdlP15Watt142fCpuId,           1500,      1500,     5500,       0,      5500,      0,          5500,         0,        5500,     12300,       0}, ///  15W  142 ADL-P
  {EnumAdlP15Watt242fCpuId,           1500,      1500,     5500,       0,      5500,      0,          5500,         0,        5500,     12300,       0}, ///  15W  242 ADL-P
  {EnumAdlP28Watt282fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,      9000,       0}, ///  28W  282 ADL-P
  {EnumAdlP28Watt482fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,      9000,       0}, ///  28W  482 ADL-P
  {EnumAdlP28Watt682fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,     14000,       0}, ///  26W  682 ADL-P
  {EnumAdlP28Watt142fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,      9000,       0}, ///  28W  142 ADL-P
  {EnumAdlP28Watt242fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,      9000,       0}, ///  28W  242 ADL-P
  {EnumAdlP28Watt442fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,      9000,       0}, ///  28W  442 ADL-P
  {EnumAdlP28Watt182fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,      9000,       0}, ///  28W  182 ADL-P
  {EnumAdlP28Watt642fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,     14000,       0}, ///  26W  682 ADL-P
  {EnumAdlP28Watt662fCpuId,           2800,      2800,     6400,       0,      6400,      0,          6400,         0,        6400,     14000,       0}, ///  26W  682 ADL-P
  {EnumAdlP45Watt682fCpuId,           4500,      4500,    11500,       0,     11500,      0,         11500,         0,       11500,     21500,       0}, ///  45W  682 ADL-P
  {EnumAdlP45Watt242fCpuId,           4500,      4500,     9500,       0,      9500,      0,          9500,         0,        9500,     12500,       0}, ///  45W  242 ADL-P
  {EnumAdlP45Watt482fCpuId,           4500,      4500,     9500,       0,      9500,      0,          9500,         0,        9500,     12500,       0}, ///  45W  482 ADL-P
  {EnumAdlP45Watt442fCpuId,           4500,      4500,     9500,       0,      9500,      0,          9500,         0,        9500,     12500,       0}, ///  45W  442 ADL-P
  {EnumAdlP45Watt642fCpuId,           4500,      4500,    11500,       0,     11500,      0,         11500,         0,       11500,     21500,       0}, ///  45W  642 ADL-P
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST  CPU_CORE_COUNT  mCpuCoreCountMappingTable[] = {
  { EnumAdlP15Watt282fCpuId,            2,  8},
  { EnumAdlP28Watt282fCpuId,            2,  8},
  { EnumAdlP28Watt482fCpuId,            4,  8},
  { EnumAdlP28Watt682fCpuId,            6,  8},
  { EnumAdlP45Watt682fCpuId,            6,  8},
  { EnumAdlP45Watt482fCpuId,            4,  8},
  { EnumAdlP45Watt442fCpuId,            4,  4},
  { EnumAdlP28Watt442fCpuId,            4,  4},
  { EnumAdlP15Watt142fCpuId,            1,  4},
  { EnumAdlP28Watt142fCpuId,            1,  4},
  { EnumAdlP15Watt242fCpuId,            2,  4},
  { EnumAdlP28Watt242fCpuId,            2,  4},
  { EnumAdlP45Watt242fCpuId,            2,  4},
  { EnumAdlP45Watt642fCpuId,            6,  4},
  { EnumAdlP28Watt182fCpuId,            1,  8},
  { EnumAdlP28Watt642fCpuId,            6,  4},
  { EnumAdlP28Watt662fCpuId,            6,  6},
};

/**
  Return CPU Sku

  @param[in]  UINT32             CpuFamilyModel
  @param[in]  UINT16             CpuDid

  @retval     UINT8              CPU Sku
**/
UINT8
GetCpuSkuInfo (
  IN UINT32 CpuFamilyModel,
  IN UINT16 CpuDid
  )
{
  UINT8              CpuType;
  BOOLEAN            SkuFound;

  SkuFound  = TRUE;
  CpuType   = EnumCpuUnknown;

  switch (CpuFamilyModel) {
    case CPUID_FULL_FAMILY_MODEL_ALDERLAKE_MOBILE:
      switch (CpuDid) {
        case V_SA_DEVICE_ID_MB_ULT_1:    // AlderLake P (6+8+GT)
        case V_SA_DEVICE_ID_MB_ULT_2:    // AlderLake P (6+4(f)+GT)
        case V_SA_DEVICE_ID_MB_ULT_3:    // AlderLake P (4(f)+8+GT)
        case V_SA_DEVICE_ID_MB_ULT_4:    // AlderLake P (2(f)+4(f)+GT)
        case V_SA_DEVICE_ID_MB_ULT_5:    // AlderLake P (2+8+GT)
        case V_SA_DEVICE_ID_MB_ULT_6:    // AlderLake P (2+4(f)+GT)
        case V_SA_DEVICE_ID_MB_ULT_7:    // AlderLake P (4+4(f)+GT)
        case V_SA_DEVICE_ID_MB_ULT_8:    // AlderLake P (1+4+GT) SA DID
        case V_SA_DEVICE_ID_MB_ULT_9:    // AlderLake P (1+8+GT) SA DID
        case V_SA_DEVICE_ID_MB_ULT_10:   // AlderLake P (6+6+GT) SA DID
          CpuType = EnumCpuUlt;
          break;
        default:
          SkuFound = FALSE;
          break;
      }
    break;
        default:
          SkuFound = FALSE;
          break;
      }

  if (!SkuFound) {
    DEBUG ((DEBUG_ERROR, "Unsupported CPU SKU, Device ID: 0x%02X, CPUID: 0x%08X!\n", CpuDid, CpuFamilyModel));
    ASSERT (FALSE);
  }

  return CpuType;
}


/**
  This function returns the supported Physical Address Size

  @retval returns the supported Physical Address Size.
**/
UINT8
GetMaxPhysicalAddressSizeFru (
  VOID
  )
{
  //
  // Even though CPUID Leaf CPUID_VIR_PHY_ADDRESS_SIZE (0x80000008) MAX_PA will report 46.
  // For ADL BIOS will return Memory expansion 39 bit (0 - 38) + MKTME (Bits 39-41 must be zero - 3 bit hole in the middle) 42-45 bit is MKTME Keys.
  //
  return 39;
}


