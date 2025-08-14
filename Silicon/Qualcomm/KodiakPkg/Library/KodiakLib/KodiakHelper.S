#
#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

#include <AsmMacroLib.h>
#include <Library/ArmLib.h>
#include "KodiakHelper.h"

ASM_FUNC(ArmPlatformPeiBootAction)
// Many Qualcomm targets use Gunyah as a firmware-launched hypervisor, making
// EL2 unavailable for Non-secure firmware and operating systems, preventing
// use of alternative hypervisors.

// Quoting Arm Base Boot Requirements here:
//
// 6.3.1.1. UEFI boot at EL2
//   Systems must boot UEFI at EL2, to enable the installation of a hypervisor
//   or a virtualization-aware operating system.

// So lets use a TZ SMC call supported on these Qcom targets to clean up
// EL2/Gunyah configuration and transition to booting edk2 (UEFI) at EL2
// exiting Gunyah. All the SMMU configuration gets cleaned up and handed
// over in EL2 in bypass mode. The edk2 is able to configure EL2 MMU/SMMU as
// you would expect on an Arm silicon executing edk2 in EL2.

  EL1_OR_EL2(x0)
1:ldr   w0, =TZ_EL2_SWITCH_SMC_ID
  ldr   w1, =TZ_EL2_SWITCH_PARAM_ID
  mov   w2, wzr
  mov   w3, wzr
  ldr   w4, =TZ_EL2_SWITCH_PARAM2_EXIT_GUNYAH
  smc   #0
// From this point onwards executing in EL2

2:ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
