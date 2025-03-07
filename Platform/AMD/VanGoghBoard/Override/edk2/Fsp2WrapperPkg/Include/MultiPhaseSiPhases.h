/** @file
  Implements MultiPhaseSiPhases.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MULTI_PHASE_SI_PHASES_H_
#define MULTI_PHASE_SI_PHASES_H_

typedef enum {
  EnumMultiPhaseAmdCpmDxeTableReadyPhase = 1,  // In FSP Doc, the index starts from 1.
  EnumMultiPhaseAmdSmmCoreBroughtUpPhase,
  EnumMultiPhaseAmdRuntimeServicesReadyPhase,
  // ......
  EnumMultiPhaseAmdMaxPhase
} AMD_MULTI_PHASE_SI_PHASES_H;
#endif
