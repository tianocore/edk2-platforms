/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RAS_CONFIG_NV_DATA_STRUCT_H_
#define RAS_CONFIG_NV_DATA_STRUCT_H_

#define RAS_CONFIG_VARSTORE_ID  0x1234
#define RAS_CONFIG_FORM_ID      0x1235

#define RAS_VARSTORE_NAME  L"RasConfigNVData"

#define RAS_CONFIG_FORMSET_GUID \
  { \
    0x96934cc6, 0xcb15, 0x4d8a, { 0xbe, 0x5f, 0x8e, 0x7d, 0x55, 0x0e, 0xc9, 0xc6 } \
  }

//
// Labels definition
//
#define LABEL_UPDATE  0x3234
#define LABEL_END     0xffff

#pragma pack(1)

//
// Ras Configuration NV data structure definition
//
typedef struct {
  UINT32    RasHardwareEinj;
  UINT32    RasPcieAerFwFirstEnabled;
  UINT32    RasBertEnabled;
  UINT32    RasSdeiEnabled;
  UINT32    RasDdrCeThreshold;
  UINT32    Ras2pCeThreshold;
  UINT32    RasCpmCeThreshold;
  UINT32    RasLinkErrThreshold;
} RAS_CONFIG_VARSTORE_DATA;

#pragma pack()

#endif /* RAS_CONFIG_NV_DATA_STRUCT_H_ */
