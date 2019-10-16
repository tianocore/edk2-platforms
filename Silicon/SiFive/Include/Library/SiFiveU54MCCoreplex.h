/** @file
  SiFive U54 Coreplex library definitions.

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef SIFIVE_U54MC_COREPLEX_CORE_H_
#define SIFIVE_U54MC_COREPLEX_CORE_H_

#include <PiPei.h>

#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>

#define SIFIVE_U54MC_COREPLEX_E51_HART_ID     0
#define SIFIVE_U54MC_COREPLEX_U54_0_HART_ID   1
#define SIFIVE_U54MC_COREPLEX_U54_1_HART_ID   2
#define SIFIVE_U54MC_COREPLEX_U54_2_HART_ID   3
#define SIFIVE_U54MC_COREPLEX_U54_3_HART_ID   4

/**
  Build up U54MC coreplex processor core-specific information.

  @param  UniqueId      U54MC unique ID.

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
CreateU54MCCoreplexProcessorSpecificDataHob (
  IN UINTN UniqueId
  );

/**
  Function to build processor related SMBIOS information. RISC-V SMBIOS DXE driver collect
  this information and build SMBIOS Type4 and Type7 record.

  @param  ProcessorUid    Unique ID of pysical processor which owns this core.
  @param  SmbiosHobPtr    Pointer to receive RISC_V_PROCESSOR_SMBIOS_HOB_DATA. The pointers
                          maintained in this structure is only valid before memory is discovered.
                          Access to those pointers after memory is installed will cause unexpected issues.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
CreateU54MCProcessorSmbiosDataHob (
  IN UINTN     ProcessorUid,
  IN RISC_V_PROCESSOR_SMBIOS_HOB_DATA **SmbiosHobPtr
  );
#endif
