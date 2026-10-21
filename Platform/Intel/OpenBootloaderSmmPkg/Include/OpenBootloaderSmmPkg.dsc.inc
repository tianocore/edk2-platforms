# @file
#  Copyright (c) 2025 Intel Corporation. All rights reserved. <BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

  OpenBootloaderSmmPkg/FspNvsStandaloneMm/FspNvsStandaloneMm.inf {
    <LibraryClasses>
    DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/StandaloneMmReportStatusCodeLib.inf
    RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
    MemoryAllocationLib|StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib.inf
    PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
    BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
    BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
    MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf
    NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
    StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
    MemLib|StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf
    IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
    TimerLib|UefiCpuPkg/Library/CpuTimerLib/BaseCpuTimerLib.inf
    HobLib|OpenBootloaderSmmPkg/Library/BootloaderMmHobLib/BootloaderMmHobLib.inf

    # UEFI debug only, will remove in the edk2platform upstream PR
    PdaReadLib|ServerPlatformPkg/Library/PdaAccessLib/PdaReadLib.inf
    PdaVerificationLib|ServerPlatformPkg/Library/PdaAccessLib/PdaVerificationLib.inf
    DataCacheLib|ServerPlatformPkg/Library/DataCacheLibHob/DataCacheLibHob.inf
    DebugPrintErrorLevelLib|ServerPlatformPkg/Library/DebugPrintErrorLevelLibPda/StandaloneMmDebugPrintErrorLevelLibPda.inf
    UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
    FspNvsReadLib|ServerPlatformPkg/Library/FspNvsAccessLib/FspNvsReadLib.inf
 }