/** @file

  The non-volatile parameter layout in SPI-NOR is shown below. There is
  two copies. The master copy is changeable by the user. The Last Known
  copy is handled by the fail safe future. It is a last know bootable copy.

   ---------------------------
   | Master Copy             | 16KB
   | Pre-boot parameters     |
   ---------------------------
   | Master Copy             | 16KB
   | Pre-boot parameters     |
   | w/o failsafe support    |
   ---------------------------
   | Master Copy             |
   | Manufactory &           | 32KB
   | Users parameters        |
   ---------------------------
   | Last Known Copy         | 16KB
   | Pre-boot parameters     |
   ---------------------------
   |                         | 16KB
   ---------------------------
   | Last Known Copy         |
   | Manufactory &           | 32KB
   | Users parameters        |
   ---------------------------

  As each non-volatile parameter requires 8 bytes, there is a total of 8K
  parameters.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NV_PARAM_LIB_H_
#define NV_PARAM_LIB_H_

#define NV_PARAM_MAX_SIZE   (64 * 1024)
#define NV_PARAM_ENTRYSIZE  8

#define NV_PERM_ALL    0xFFFF  /* Allowed for all */
#define NV_PERM_ATF    0x0001  /* Allowed for EL3 code */
#define NV_PERM_OPTEE  0x0004  /* Allowed for secure El1 */
#define NV_PERM_BIOS   0x0008  /* Allowed for EL2 non-secure */
#define NV_PERM_MANU   0x0010  /* Allowed for manufactory interface */
#define NV_PERM_BMC    0x0020  /* Allowed for BMC interface */

#define NVPARAM_SIZE  0x8

/**
  Retrieve a non-volatile parameter.

  NOTE: If you need a signed value, cast it. It is expected that the
  caller will carry the correct permission over various call sequences.

  @param[in]  Param               Parameter ID to retrieve
  @param[in]  ACLRd               Permission for read operation.
  @param[out] Val                 Pointer to an UINT32 to the return value.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_ACCESS_DENIED       Permission not allowed.
  @retval EFI_DEVICE_ERROR        Service is unavailable.
  @retval EFI_INVALID_PARAMETER   Val is NULL or return status is invalid.
  @retval EFI_NOT_FOUND           NVParam entry is not set.
**/
EFI_STATUS
NVParamGet (
  IN  UINT32  Param,
  IN  UINT16  ACLRd,
  OUT UINT32  *Val
  );

/**
  Set a non-volatile parameter.

  NOTE: If you have a signed value, cast to unsigned. If the parameter has
  not being created before, the provied permission is used to create the
  parameter. Otherwise, it is checked for access. It is expected that the
  caller will carry the correct permission over various call sequences.

  @param[in] Param                Parameter ID to set
  @param[in] ACLRd                Permission for read operation.
  @param[in] ACLWr                Permission for write operation.
  @param[in] Val                  Unsigned int value to set.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_ACCESS_DENIED       Permission not allowed.
  @retval EFI_DEVICE_ERROR        Service is unavailable.
  @retval EFI_INVALID_PARAMETER   Return status is invalid.
**/
EFI_STATUS
NVParamSet (
  IN UINT32  Param,
  IN UINT16  ACLRd,
  IN UINT16  ACLWr,
  IN UINT32  Val
  );

/**
  Clear a non-volatile parameter.

  NOTE: It is expected that the caller will carry the correct permission
  over various call sequences.

  @param[in] Param                Parameter ID to set
  @param[in] ACLWr                Permission for write operation.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_ACCESS_DENIED       Permission not allowed.
  @retval EFI_DEVICE_ERROR        Service is unavailable.
  @retval EFI_INVALID_PARAMETER   Return status is invalid.
**/
EFI_STATUS
NVParamClr (
  IN UINT32  Param,
  IN UINT16  ACLWr
  );

/**
  Clear all non-volatile parameters

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_DEVICE_ERROR        Service is unavailable.
  @retval EFI_INVALID_PARAMETER   Return status is invalid.
**/
EFI_STATUS
NVParamClrAll (
  VOID
  );

#endif /* NV_PARAM_LIB_H_ */
