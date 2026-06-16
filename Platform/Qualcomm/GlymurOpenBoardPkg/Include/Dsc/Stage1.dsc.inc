## @file
# Common DSC content to begin MinPlatformPkg Stage 1.
#
# Copyright (C) 2022 Intel Corporation<BR>
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

[LibraryClasses]
  ReportCpuHobLib         | MinPlatformPkg/PlatformInit/Library/ReportCpuHobLib/ReportCpuHobLib.inf
  SiliconPolicyInitLib    | MinPlatformPkg/PlatformInit/Library/SiliconPolicyInitLibNull/SiliconPolicyInitLibNull.inf
  SiliconPolicyUpdateLib  | MinPlatformPkg/PlatformInit/Library/SiliconPolicyUpdateLibNull/SiliconPolicyUpdateLibNull.inf

[LibraryClasses.Common.SEC]
  TestPointCheckLib       | MinPlatformPkg/Test/Library/TestPointCheckLib/SecTestPointCheckLib.inf
  TimerLib                | MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.Common.PEI_CORE, LibraryClasses.Common.PEIM]
  TestPointCheckLib       | MinPlatformPkg/Test/Library/TestPointCheckLib/PeiTestPointCheckLib.inf
  TestPointLib            | MinPlatformPkg/Test/Library/TestPointLib/PeiTestPointLib.inf
  TimerLib                | MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.AARCH64.SEC]
  TestPointCheckLib       | MinPlatformPkg/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf

[LibraryClasses.AARCH64.PEI_CORE, LibraryClasses.AARCH64.PEIM]
  TestPointCheckLib       | MinPlatformPkg/Test/Library/TestPointCheckLibNull/TestPointCheckLibNull.inf

[LibraryClasses.AARCH64]
  SetCacheMtrrLib|MinPlatformPkg/Library/SetCacheMtrrLib/SetCacheMtrrLibNull.inf

[Components.AARCH64]
  ArmPlatformPkg/Sec/Sec.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  ArmPlatformPkg/PlatformPei/PlatformPeim.inf
  ArmPlatformPkg/MemoryInitPei/MemoryInitPeim.inf
  ArmPkg/Drivers/CpuPei/CpuPei.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }


  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  UefiCpuPkg/CpuIoPei/CpuIoPei.inf
  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf
  MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      PcdLib | MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf
  MinPlatformPkg/PlatformInit/PlatformInitPei/PlatformInitPreMemNonFsp.inf
  MinPlatformPkg/PlatformInit/ReportFv/ReportFvPei.inf
  MinPlatformPkg/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPreMem.inf
