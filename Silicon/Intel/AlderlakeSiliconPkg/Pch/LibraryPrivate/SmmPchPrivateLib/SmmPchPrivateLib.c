/** @file
  PCH SMM private lib.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuPlatformLib.h>
#include <CpuRegs.h>
#include <Register/CommonMsr.h>
#include <Register/PttPtpRegs.h>


/**
  Set InSmm.Sts bit
**/
VOID
PchSetInSmmSts (
  VOID
  )
{
  UINT32      Data32;

  ///
  /// If platform disables TXT_PTLEN strap, NL socket(s) will target abort
  /// when trying to access LT register space below, and writes to
  /// NL's MSR 0x1FE will GP fault. Check straps enabled first.
  ///

  Data32 = MmioRead32 (R_LT_EXISTS);

  if (Data32 == 0xFFFFFFFF) {
    return;
  }
  ///
  /// Read memory location FED30880h OR with 00000001h, place the result in EAX,
  /// and write data to lower 32 bits of MSR 1FEh (sample code available)
  ///
  Data32 = MmioRead32 (R_LT_UCS);
  AsmWriteMsr32 (MSR_SPCL_CHIPSET_USAGE, Data32 | BIT0);
  ///
  /// Read FED30880h back to ensure the setting went through.
  ///
  Data32 = MmioRead32 (R_LT_UCS);
}

/**
  Clear InSmm.Sts bit
**/
VOID
PchClearInSmmSts (
  VOID
  )
{
  UINT32      Data32;

  ///
  /// If platform disables TXT_PTLEN strap, NL socket(s) will target abort
  /// when trying to access LT register space below, and writes to
  /// NL's MSR 0x1FE will GP fault. Check straps enabled first.
  ///

  Data32 = MmioRead32 (R_LT_EXISTS);
  if (Data32 == 0xFFFFFFFF) {
    return;
  }

  ///
  /// Read memory location FED30880h AND with FFFFFFFEh, place the result in EAX,
  /// and write data to lower 32 bits of MSR 1FEh (sample code available)
  ///
  Data32 = MmioRead32 (R_LT_UCS);
  AsmWriteMsr32 (MSR_SPCL_CHIPSET_USAGE, Data32 & (UINT32) (~BIT0));
  ///
  /// Read FED30880h back to ensure the setting went through.
  ///
  Data32 = MmioRead32 (R_LT_UCS);
}
