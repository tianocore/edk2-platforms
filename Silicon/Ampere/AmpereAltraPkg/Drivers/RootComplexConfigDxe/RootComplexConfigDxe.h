/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BOARD_PCIE_SCREEN_H_
#define BOARD_PCIE_SCREEN_H_

#include "RootComplexConfigNVDataStruct.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  RootComplexConfigVfrBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  RootComplexConfigDxeStrings[];

#define MAX_EDITABLE_ELEMENTS  3
#define RC0_STATUS_OFFSET  \
  OFFSET_OF (ROOT_COMPLEX_CONFIG_VARSTORE_DATA, RCStatus[0])
#define RC0_BIFUR_LO_OFFSET  \
  OFFSET_OF (ROOT_COMPLEX_CONFIG_VARSTORE_DATA, RCBifurcationLow[0])
#define RC0_BIFUR_HI_OFFSET  \
  OFFSET_OF (ROOT_COMPLEX_CONFIG_VARSTORE_DATA, RCBifurcationHigh[0])
#define SMMU_PMU_OFFSET  \
  OFFSET_OF (ROOT_COMPLEX_CONFIG_VARSTORE_DATA, SmmuPmu)

#define STRONG_ORDERING_OFFSET  \
  OFFSET_OF (NVPARAM_ROOT_COMPLEX_CONFIG_VARSTORE_DATA, PcieStrongOrdering)

//
// Signature: Ampere Computing PCIe Screen
//
#define SCREEN_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('A', 'C', 'P', 'S')

#define MAX_STRING_SIZE  32

#define STRONG_ORDERING_DEFAULT_OPTION_VALUE   1
#define STRONG_ORDERING_DEFAULT_NVPARAM_VALUE  0xFFFFFFFF

typedef struct {
  UINTN                                        Signature;

  EFI_HANDLE                                   DriverHandle;
  EFI_HII_HANDLE                               HiiHandle;
  ROOT_COMPLEX_CONFIG_VARSTORE_DATA            VarStoreConfig;
  NVPARAM_ROOT_COMPLEX_CONFIG_VARSTORE_DATA    NVParamVarStoreConfig;

  //
  // Consumed protocol
  //
  EFI_HII_DATABASE_PROTOCOL                    *HiiDatabase;
  EFI_HII_STRING_PROTOCOL                      *HiiString;
  EFI_HII_CONFIG_ROUTING_PROTOCOL              *HiiConfigRouting;
  EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL          *HiiKeywordHandler;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL               ConfigAccess;
} SCREEN_PRIVATE_DATA;

typedef struct {
  UINTN            PciDevIdx;
  EFI_STRING_ID    GotoStringId;
  EFI_STRING_ID    GotoHelpStringId;
  UINT16           GotoKey;
  BOOLEAN          ShowItem;
} SETUP_GOTO_DATA;

#define SCREEN_PRIVATE_FROM_THIS(a)  \
  CR (a, SCREEN_PRIVATE_DATA, ConfigAccess, SCREEN_PRIVATE_DATA_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

UINT8
PcieRCDevMapLowDefaultSetting (
  IN UINTN                RCIndex,
  IN SCREEN_PRIVATE_DATA  *PrivateData
  );

UINT8
PcieRCDevMapHighDefaultSetting (
  IN UINTN                RCIndex,
  IN SCREEN_PRIVATE_DATA  *PrivateData
  );

BOOLEAN
PcieRCActiveDefaultSetting (
  IN UINTN                RCIndex,
  IN SCREEN_PRIVATE_DATA  *PrivateData
  );

#endif /* BOARD_PCIE_SCREEN_H_ */
