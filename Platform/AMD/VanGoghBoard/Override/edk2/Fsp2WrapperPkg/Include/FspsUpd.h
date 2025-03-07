/** @file
  Implements FspsUpd.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FSPSUPD_H_
#define FSPSUPD_H_

#include <FspUpd.h>

#pragma pack(1)

typedef struct {
  /** Offset 0x0030**/ UINT32    page_address_below_1mb;
  /** Offset 0x0034**/ UINT32    smram_hob_base_addr;
  /** Offset 0x0038**/ UINT32    smram_hob_size;
  /** Offset 0x003C**/ UINT32    nv_storage_variable_base;
  /** Offset 0x0040**/ UINT32    nv_storage_variable_size;
  /** Offset 0x0044**/ UINT32    nv_storage_ftw_working_base;
  /** Offset 0x0048**/ UINT32    nv_storage_ftw_working_size;
  /** Offset 0x004C**/ UINT32    nv_storage_ftw_spare_base;
  /** Offset 0x0050**/ UINT32    nv_storage_ftw_spare_size;
  /** Offset 0x0054**/ UINT32    dgpu_ssid;
  /** Offset 0x0058**/ UINT32    dgpu_audio_ssid;
  /** Offset 0x005C**/ UINT32    smram_hob_descriptor_base_addr;
  /** Offset 0x0060**/ UINT32    smram_hob_descriptor_size;
  /** Offset 0x0064**/ UINT64    smm_data_buffer_address;
  /** Offset 0x006C**/ UINT32    fsp_o_dxe_volume_address;
  /** Offset 0x0070**/ UINT32    fsp_o_dxe_upd_address;
  /** Offset 0x0074**/ UINT16    UpdTerminator;
} FSP_S_CONFIG;

/** Fsp S UPD Configuration
**/
typedef struct {
  /** Offset 0x0000**/ FSP_UPD_HEADER    FspUpdHeader;
  /** Offset 0x0030**/ FSP_S_CONFIG      FspsConfig;
} FSPS_UPD;

#pragma pack()

#endif
