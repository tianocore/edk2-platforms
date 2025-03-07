/** @file
  Implements FspSmmDataExchangeBuffer.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/GlobalNvsArea.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/TcgService.h>
#include <Protocol/Variable.h>

// You may define a customized callback function whenever the exchange buffer is updated.

typedef EFI_STATUS (EFIAPI *DETECT_AND_INSTALL_NEW_PROTOCOL)(VOID);

#pragma pack(push,1)
typedef struct _FSP_SMM_DATA_EXCHANGE_BUFFER {
  EFI_GLOBAL_NVS_AREA_PROTOCOL    *NvsAreaProtocol;               // gEfiGlobalNvsAreaProtocolGuid
  EFI_TCG2_PROTOCOL               *EfiTcg2Protocol;               // gEfiTcg2ProtocolGuid
} FSP_SMM_DATA_EXCHANGE_BUFFER;
#pragma pack(pop)
