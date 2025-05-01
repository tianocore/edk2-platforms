/** @file
  Header file of SMM core platform hook for AMD SPI Host Controller state library

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_CORE_HOOK_AMD_SPI_HC_H_
#define SMM_CORE_HOOK_AMD_SPI_HC_H_

///
/// Structure of AMD SPI HC State record
///
typedef struct {
  VOID          *SmmSpiHcStateInterface; ///< AMD SMM SPI HC State Protocol Interface
  EFI_HANDLE    SmmSpiHcStateHandle;     ///< Handle of MD SMM SPI HC State Protocol handle
} SMM_CORE_HOOK_AMD_SPI_HC_STATE_CONTEXT;

#endif // SMM_CORE_HOOK_AMD_SPI_HC_H_
