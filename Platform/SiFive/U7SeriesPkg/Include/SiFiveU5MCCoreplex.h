/** @file
  SiFive U74 Coreplex library definitions.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2022, Boyang Han <yqszxx@gmail.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SIFIVE_U7MC_COREPLEX_H_
#define SIFIVE_U7MC_COREPLEX_H_

#include <PiPei.h>

#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>

#define SIFIVE_U7MC_COREPLEX_MC_HART_ID  0

/**
  Build processor and platform information for the U7 platform

  @return EFI_SUCCESS     Status.

**/
EFI_STATUS
BuildRiscVSmbiosHobs (
  VOID
  );

#endif
