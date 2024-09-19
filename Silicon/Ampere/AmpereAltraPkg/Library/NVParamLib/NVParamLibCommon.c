/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/NVParamLib.h>

#include "NVParamLibCommon.h"

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
  )
{
  EFI_MM_COMMUNICATE_NVPARAM_RESPONSE  MmNVParamRes;
  EFI_STATUS                           Status;
  UINT64                               MmData[5];

  if (Val == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MmData[0] = MM_NVPARAM_FUNC_READ;
  MmData[1] = Param;
  MmData[2] = (UINT64)ACLRd;

  Status = NVParamMmCommunicate (
             MmData,
             sizeof (MmData),
             &MmNVParamRes,
             sizeof (MmNVParamRes)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (MmNVParamRes.Status) {
    case MM_NVPARAM_RES_SUCCESS:
      *Val = (UINT32)MmNVParamRes.Value;
      return EFI_SUCCESS;

    case MM_NVPARAM_RES_NOT_SET:
      return EFI_NOT_FOUND;

    case MM_NVPARAM_RES_NO_PERM:
      return EFI_ACCESS_DENIED;

    case MM_NVPARAM_RES_FAIL:
      return EFI_DEVICE_ERROR;

    default:
      return EFI_INVALID_PARAMETER;
  }
}

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
  )
{
  EFI_MM_COMMUNICATE_NVPARAM_RESPONSE  MmNVParamRes;
  EFI_STATUS                           Status;
  UINT64                               MmData[5];

  MmData[0] = MM_NVPARAM_FUNC_WRITE;
  MmData[1] = Param;
  MmData[2] = (UINT64)ACLRd;
  MmData[3] = (UINT64)ACLWr;
  MmData[4] = (UINT64)Val;

  Status = NVParamMmCommunicate (
             MmData,
             sizeof (MmData),
             &MmNVParamRes,
             sizeof (MmNVParamRes)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (MmNVParamRes.Status) {
    case MM_NVPARAM_RES_SUCCESS:
      return EFI_SUCCESS;

    case MM_NVPARAM_RES_NO_PERM:
      return EFI_ACCESS_DENIED;

    case MM_NVPARAM_RES_FAIL:
      return EFI_DEVICE_ERROR;

    default:
      return EFI_INVALID_PARAMETER;
  }
}

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
  )
{
  EFI_MM_COMMUNICATE_NVPARAM_RESPONSE  MmNVParamRes;
  EFI_STATUS                           Status;
  UINT64                               MmData[5];

  MmData[0] = MM_NVPARAM_FUNC_CLEAR;
  MmData[1] = Param;
  MmData[2] = 0;
  MmData[3] = (UINT64)ACLWr;

  Status = NVParamMmCommunicate (
             MmData,
             sizeof (MmData),
             &MmNVParamRes,
             sizeof (MmNVParamRes)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (MmNVParamRes.Status) {
    case MM_NVPARAM_RES_SUCCESS:
      return EFI_SUCCESS;

    case MM_NVPARAM_RES_NO_PERM:
      return EFI_ACCESS_DENIED;

    case MM_NVPARAM_RES_FAIL:
      return EFI_DEVICE_ERROR;

    default:
      return EFI_INVALID_PARAMETER;
  }
}

/**
  Clear all non-volatile parameters

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_DEVICE_ERROR        Service is unavailable.
  @retval EFI_INVALID_PARAMETER   Return status is invalid.
**/
EFI_STATUS
NVParamClrAll (
  VOID
  )
{
  EFI_MM_COMMUNICATE_NVPARAM_RESPONSE  MmNVParamRes;
  EFI_STATUS                           Status;
  UINT64                               MmData[5];

  MmData[0] = MM_NVPARAM_FUNC_CLEAR_ALL;

  Status = NVParamMmCommunicate (
             MmData,
             sizeof (MmData),
             &MmNVParamRes,
             sizeof (MmNVParamRes)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (MmNVParamRes.Status) {
    case MM_NVPARAM_RES_SUCCESS:
      return EFI_SUCCESS;

    case MM_NVPARAM_RES_FAIL:
      return EFI_DEVICE_ERROR;

    default:
      return EFI_INVALID_PARAMETER;
  }
}
