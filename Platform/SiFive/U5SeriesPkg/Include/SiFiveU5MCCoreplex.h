/** @file
  SiFive U54 Coreplex library definitions.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef SIFIVE_U5MC_COREPLEX_H_
#define SIFIVE_U5MC_COREPLEX_H_

#include <PiPei.h>

#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>

#define SIFIVE_U5MC_COREPLEX_MC_HART_ID     0

/**
  Build processor and platform information for the U5 platform

  @return EFI_SUCCESS     Status.

**/
EFI_STATUS
BuildRiscVSmbiosHobs (
  VOID
);

#endif
