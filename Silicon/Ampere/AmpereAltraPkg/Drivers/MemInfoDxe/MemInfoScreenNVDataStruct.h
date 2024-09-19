/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEM_INFO_SCREEN_NV_DATA_STRUCT_H_
#define MEM_INFO_SCREEN_NV_DATA_STRUCT_H_

#define MEM_INFO_VARSTORE_NAME              L"MemInfoIfrNVData"
#define MEM_INFO_VARSTORE_ID                0x1234
#define MEM_INFO_FORM_ID                    0x1235
#define MEM_INFO_FORM_PERFORMANCE_ID        0x1236
#define MEM_INFO_FORM_NVDIMM_ID             0x1237
#define MEM_INFO_FORM_SET_GUID              { 0xd58338ee, 0xe9f7, 0x4d8d, { 0xa7, 0x08, 0xdf, 0xb2, 0xc6, 0x66, 0x1d, 0x61 } }
#define MEM_INFO_FORM_SET_PERFORMANCE_GUID  { 0x4a072c78, 0x42f9, 0x11ea, { 0xb7, 0x7f, 0x2e, 0x28, 0xce, 0x88, 0x12, 0x62 } }

#pragma pack(1)

//
// NV data structure definition
//
typedef struct {
  UINT32    DDRSpeedSel;
  UINT32    EccMode;
  UINT32    ErrCtrl_DE;
  UINT32    ErrCtrl_FI;
  UINT32    Slave32bit;
  UINT32    ScrubPatrol;
  UINT32    DemandScrub;
  UINT32    WriteCrc;
  UINT32    FGRMode;
  UINT32    Refresh2x;
  UINT32    NvdimmModeSel;
} MEM_INFO_VARSTORE_DATA;

//
// Labels definition
//
#define LABEL_UPDATE  0x2223
#define LABEL_END     0x2224

#pragma pack()

#endif
