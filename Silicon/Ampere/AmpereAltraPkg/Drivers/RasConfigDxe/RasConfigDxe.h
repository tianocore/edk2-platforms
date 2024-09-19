/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RAS_CONFIG_DXE_H_
#define RAS_CONFIG_DXE_H_

#include "RasConfigNVDataStruct.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
//
extern UINT8  RasConfigVfrBin[];

//
// This is the generated String package data for all .UNI files.
//
extern UINT8  RasConfigDxeStrings[];

#define RAS_DDR_CE_THRESHOLD_OFST  OFFSET_OF (RAS_CONFIG_VARSTORE_DATA, RasDdrCeThreshold)
#define RAS_2P_CE_THRESHOLD_OFST   OFFSET_OF (RAS_CONFIG_VARSTORE_DATA, Ras2pCeThreshold)

#define RAS_CONFIG_PRIVATE_SIGNATURE  SIGNATURE_32 ('R', 'A', 'S', 'C')

typedef struct {
  UINTN                              Signature;

  EFI_HANDLE                         DriverHandle;
  EFI_HII_HANDLE                     HiiHandle;
  RAS_CONFIG_VARSTORE_DATA           Configuration;

  //
  // Consumed protocol
  //
  EFI_HII_CONFIG_ROUTING_PROTOCOL    *HiiConfigRouting;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL     ConfigAccess;
} RAS_CONFIG_PRIVATE_DATA;

#define RAS_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, RAS_CONFIG_PRIVATE_DATA, ConfigAccess, RAS_CONFIG_PRIVATE_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

#endif /* RAS_CONFIG_DXE_H_ */
