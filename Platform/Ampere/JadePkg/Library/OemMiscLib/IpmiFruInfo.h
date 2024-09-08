/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IPMI_FRU_INFO_H_
#define IPMI_FRU_INFO_H_

#include <Uefi.h>

//
// IPMI FRU Field IDs
//
typedef enum {
  FruChassisPartNumber = 0,
  FruChassisSerialNumber,
  FruChassisExtra,
  FruBoardManufacturerName,
  FruBoardProductName,
  FruBoardSerialNumber,
  FruBoardPartNumber,
  FruProductManufacturerName,
  FruProductName,
  FruProductPartNumber,
  FruProductVersion,
  FruProductSerialNumber,
  FruProductAssetTag,
  FruProductFruFileId,
  FruProductExtra,
  FruFieldIdMax
} IPMI_FRU_FIELD_ID;

CHAR8 *
EFIAPI
IpmiFruInfoGet (
  IPMI_FRU_FIELD_ID  FieldId
  );

#endif /* IPMI_FRU_INFO_H_ */
