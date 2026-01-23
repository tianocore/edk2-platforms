/** @file
  Provide vendor-specific version and identifiers to core TPM library
  for return in capabilities. These may not be compile time constants and
  therefore are provided by platform callbacks.
  These platform functions are expected to always be available,
  even in failure mode.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformTpmLib.h>

#define FIRMWARE_V1  (0x20251101)
#define FIRMWARE_V2  (0x00200000)
#define MAX_SVN      255

/*
 * Below value from the TCG Reference library's
 * VendorInfo.c's implemenation.
 */
STATIC UINT32  CurrentHash = 0x00120000;
STATIC UINT16  CurrentSvn  = 0x10;

/**
  _plat__GetManufacturerCapabilityCode()

  return the 4 character Manufacturer Capability code.
  This should come from the platform library since
  that is provided by the manufacturer.


  @return               Manufacturer capability Code.

**/
UINT32
EFIAPI
PlatformTpmLibGetManufacturerCapabilityCode (
  VOID
  )
{
  return 0x41524D20; // "ARM "
}

/**
  _plat__GetVendorCapabilityCode()

  return the 4 character VendorStrings for Capabilities.
  Index is ONE-BASED, and may be in the range [1,4] inclusive.
  Any other index returns all zeros. The return value will be interpreted
  as an array of 4 ASCII characters (with no null terminator).

  @param[in] Index      index

  @return               Vendor specific capability code.

**/
UINT32
EFIAPI
PlatformTpmLibGetVendorCapabilityCode (
  IN INT32  Index
  )
{
  switch (Index) {
    case 1:
      return 0x54434720; // "TCG "
    case 2:
      return 0x6654504D; // "fTPM"
    case 3:
      return 0;
    case 4:
      return 0;
  }

  return 0;
}

/**
  _plat__GetTpmFirmwareVersionHigh()

  return the most-significant 32-bits of the TPM Firmware Version

  @return               High 32-bits of TPM Firmware Version.

**/
UINT32
EFIAPI
PlatformTpmLibGetTpmFirmwareVersionHigh (
  VOID
  )
{
  return FIRMWARE_V1;
}

/**
  _plat__GetTpmFirmwareVersionLow()

  return the least-significant 32-bits of the TPM Firmware Version

  @return               Low 32-bits of TPM Firmware Version.

**/
UINT32
EFIAPI
PlatformTpmLibGetTpmFirmwareVersionLow (
  VOID
  )
{
  return FIRMWARE_V2;
}

/**
  _plat__GetTpmFirmwareSvn()

  return the TPM Firmware current SVN.

  @return               current SVN.

**/
UINT16
EFIAPI
PlatformTpmLibGetTpmFirmwareSvn (
  VOID
  )
{
  return CurrentSvn;
}

/**
  _plat__GetTpmFirmwareMaxSvn()

  return the TPM Firmware maximum SVN.

  @return               Maximum SVN.

**/
UINT16
EFIAPI
PlatformTpmLibGetTpmFirmwareMaxSvn (
  VOID
  )
{
  return MAX_SVN;
}

/**
  _plat__GetTpmFirmwareSvnSecret()

  return the TPM Firmware SVN Secret value associated with SVN

  NOTE:
    This implementation follows TCG TPM reference library's
    VendorInfo.c -- dummy implementation.

  @param[in]            Svn                     Svn
  @param[in]            SecretBufferSize        Secret Buffer Size
  @param[out]           SecretBuffer            Secret Buffer
  @param[out]           SecretSize              Secret Svn Size

  @return               0                       Success
  @return               < 0                     Error

**/
INT32
EFIAPI
PlatformTpmLibGetTpmFirmwareSvnSecret (
  IN  UINT16  Svn,
  IN  UINT16  SecretBufferSize,
  OUT UINT8   *SecretBuffer,
  OUT UINT16  *SecretSize
  )
{
  UINTN  Idx;

  if (Svn > CurrentSvn) {
    return -1;
  }

  for (Idx = 0; Idx < SecretBufferSize; Idx++) {
    SecretBuffer[Idx] = ((UINT8 *)&Svn)[Idx & sizeof (Svn)];
  }

  *SecretSize = SecretBufferSize;

  return 0;
}

/**
  _plat__GetTpmFirmwareSecret()

  return the TPM Firmware Secret value associated with SVN

  NOTE:
    This implementation follows TCG TPM reference library's
    VendorInfo.c -- dummy implementation.

  @param[in]            SecretBufferSize        Secret Buffer Size
  @param[out]           SecretBuffer            Secret Buffer
  @param[out]           SecretSize              Secret Svn Size

  @return               0                       Success
  @return               < 0                     Error

**/
INT32
EFIAPI
PlatformTpmLibGetTpmFirmwareSecret (
  IN  UINT16  SecretBufferSize,
  OUT UINT8   *SecretBuffer,
  OUT UINT16  *SecretSize
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < SecretBufferSize; Idx++) {
    SecretBuffer[Idx] = ((UINT8 *)&CurrentHash)[Idx & sizeof (CurrentHash)];
  }

  *SecretSize = SecretBufferSize;

  return 0;
}

/**
  _plat__GetVendorTpmType()

  return the Platform TPM type.

  @return               TPM type.

**/
UINT32
EFIAPI
PlatformTpmLibGetVendorTpmType (
  VOID
  )
{
  return PLATFORM_TPM_TYPE_FTPM;
}
