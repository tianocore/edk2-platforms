/** @file
  Implements FspmUpd.h

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FSPMUPD_H_
#define FSPMUPD_H_

#include <FspUpd.h>

#pragma pack(1)

/** Fsp M Configuration
**/
typedef struct {
	/** Offset 0x0040**/	UINT32                      bert_size;
	/** Offset 0x0044**/	UINT32                      tseg_size;
	/** Offset 0x0048**/	UINT32                      dxio_descriptor_table_pointer;
	/** Offset 0x004C**/	UINT32                      pcie_reset_function_pointer;
	/** Offset 0x0050**/	UINT32                      ddi_descriptor_table_pointer;
	/** Offset 0x0054**/	UINT32                      temp_memory_base_addr;
	/** Offset 0x0058**/	UINT32                      temp_memory_size;
	/** Offset 0x005C**/	UINT32                      fsp_o_pei_volume_address;
	/** Offset 0x0060**/	UINT32                      fsp_o_pei_upd_address;
	/** Offset 0x0064**/	UINT32                      pei_reset_ppi_addr;
	/** Offset 0x0068**/	UINT32                      resource_size_for_each_rb_ptr;
	/** Offset 0x006C**/	UINT32                      resource_size_for_each_rb_size;
	/** Offset 0x0070**/	UINT32                      total_number_of_root_bridges_ptr;
	/** Offset 0x0074**/	UINT32                      total_number_of_root_bridges_size;
	/** Offset 0x0078**/	UINT32                      amd_pbs_setup_ptr;
	/** Offset 0x007C**/	UINT32                      amd_pbs_setup_size;
	/** Offset 0x0080**/	UINT32                      ap_sync_flag_nv_ptr;
	/** Offset 0x0084**/	UINT32                      ap_sync_flag_nv_size;
	/** Offset 0x0088**/	UINT8                       FchUsbUsb0DrdMode;
	/** Offset 0x0089**/	UINT8                       FchUsbUsb2DrdMode;
	/** Offset 0x008A**/	UINT8                       CmnGnbGfxUmaMode;
	/** Offset 0x008B**/	UINT32                      CmnGnbGfxUmaFrameBufferSize;
	/** Offset 0x008F**/	UINT8                       CmnGnbNbIOMMU;
	/** Offset 0x0090**/	UINT8                       PPTCtl;
	/** Offset 0x0091**/	UINT32                      FastPPTLimit;
	/** Offset 0x0095**/	UINT32                      SlowPPTLimit;
	/** Offset 0x0099**/	UINT8                       CmnCpuVolOffsetCtl;
	/** Offset 0x009A**/	UINT32                      CmnCpuVoltageOffset;
	/** Offset 0x009E**/	UINT8                       CmnGpuVolOffsetCtl;
	/** Offset 0x009F**/	UINT32                      CmnGpuVoltageOffset;
	/** Offset 0x00A3**/	UINT8                       CmnSocVolOffsetCtl;
	/** Offset 0x00A4**/	UINT32                      CmnSocVoltageOffset;
	/** Offset 0x00A8**/	UINT16                      CclkFmaxOverride;
	/** Offset 0x00AA**/	UINT16                      GfxclkFmaxOverride;
	/** Offset 0x00AC**/	UINT8                       padding1[8];
	/** Offset 0x00B4**/	UINT16                      UpdTerminator;
} FSP_M_CONFIG;

/** Fsp M UPD Configuration
**/
typedef struct {
  /** Offset 0x0000**/ FSP_UPD_HEADER    FspUpdHeader;
  /** Offset 0x0020**/ FSPM_ARCH_UPD     FspmArchUpd;
  /** Offset 0x0040**/ FSP_M_CONFIG      FspmConfig;
} FSPM_UPD;

#pragma pack()

#endif
