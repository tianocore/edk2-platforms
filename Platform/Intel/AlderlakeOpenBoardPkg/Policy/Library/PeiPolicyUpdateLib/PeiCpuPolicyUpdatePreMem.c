/** @file
  This file is SampleCode of the library for Intel CPU PEI Policy initialization.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "PeiCpuPolicyUpdate.h"
#include <Library/ConfigBlockLib.h>
#include <Library/CpuPlatformLib.h>
#include <Library/FirmwareBootMediaLib.h>
#include <Library/HobLib.h>
#include <Library/PchCycleDecodingLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/PeiSiPolicyUpdateLib.h>
#include <Library/PmcLib.h>
#include <Library/SiPolicyLib.h>
#include <Register/CommonMsr.h>
#include <Register/PchRegs.h>
#include <PlatformBoardConfig.h>
#include <PolicyUpdateMacro.h>

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))


/**
  This function performs CPU PEI Policy initialization in Pre-memory.

  @retval EFI_SUCCESS              The PPI is installed and initialized.
  @retval EFI ERRORS               The PPI is not successfully installed.
  @retval EFI_OUT_OF_RESOURCES     Do not have enough resources to initialize the driver
**/
EFI_STATUS
EFIAPI
UpdatePeiCpuPolicyPreMem (
  VOID
  )
{
  EFI_STATUS                      Status;
  CPU_SECURITY_PREMEM_CONFIG      *CpuSecurityPreMemConfig;
  CPU_CONFIG_LIB_PREMEM_CONFIG    *CpuConfigLibPreMemConfig;
  SI_PREMEM_POLICY_PPI            *SiPreMemPolicyPpi;
  UINT32                          MaxLogicProcessors;
  MSR_CORE_THREAD_COUNT_REGISTER  MsrCoreThreadCount;

  DEBUG ((DEBUG_INFO, "Update PeiCpuPolicyUpdate Pre-Mem Start\n"));

  SiPreMemPolicyPpi           = NULL;
  CpuSecurityPreMemConfig     = NULL;
  CpuConfigLibPreMemConfig    = NULL;

  Status = PeiServicesLocatePpi (&gSiPreMemPolicyPpiGuid, 0, NULL, (VOID **) &SiPreMemPolicyPpi);
  ASSERT_EFI_ERROR (Status);
  Status = GetConfigBlock ((VOID *) SiPreMemPolicyPpi, &gCpuSecurityPreMemConfigGuid, (VOID *) &CpuSecurityPreMemConfig);
  ASSERT_EFI_ERROR (Status);
  Status = GetConfigBlock ((VOID *) SiPreMemPolicyPpi, &gCpuConfigLibPreMemConfigGuid, (VOID *) &CpuConfigLibPreMemConfig);
  ASSERT_EFI_ERROR (Status);

  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.SkipStopPbet, CpuSecurityPreMemConfig->SkipStopPbet, FALSE);

  DEBUG ((DEBUG_INFO, "BIOS Guard PCD and Policy are disabled\n"));
  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.BiosGuard, CpuSecurityPreMemConfig->BiosGuard, CPU_FEATURE_DISABLE);

  UPDATE_POLICY (((FSPM_UPD *) FspmUpd)->FspmConfig.CpuRatio, CpuConfigLibPreMemConfig->CpuRatio, 0);

  ///
  /// Set PcdCpuMaxLogicalProcessorNumber to max number of logical processors enabled
  /// Read MSR_CORE_THREAD_COUNT (0x35) to check the total active Threads
  ///
  MsrCoreThreadCount.Uint64 = AsmReadMsr64 (MSR_CORE_THREAD_COUNT);
  MaxLogicProcessors = MsrCoreThreadCount.Bits.Threadcount;
  DEBUG ((DEBUG_INFO, "MaxLogicProcessors = %d\n", MaxLogicProcessors));

  PcdSetEx32S (&gUefiCpuPkgTokenSpaceGuid, PcdCpuMaxLogicalProcessorNumber, MaxLogicProcessors);

  return EFI_SUCCESS;
}
