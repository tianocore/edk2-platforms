## @file
# Common DSC content to begin Stage 1 enabling
#
# @copyright
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

[LibraryClasses]
  PciSegmentInfoLib       | MinPlatformPkg/Pci/Library/PciSegmentInfoLibSimple/PciSegmentInfoLibSimple.inf
  BoardInitLib            | QemuOpenBoardPkg/Library/BoardInitLib/BoardInitLib.inf
  SetCacheMtrrLib         | MinPlatformPkg/Library/SetCacheMtrrLib/SetCacheMtrrLib.inf
  ReportCpuHobLib         | MinPlatformPkg/PlatformInit/Library/ReportCpuHobLib/ReportCpuHobLib.inf
  SiliconPolicyInitLib    | MinPlatformPkg/PlatformInit/Library/SiliconPolicyInitLibNull/SiliconPolicyInitLibNull.inf
  SiliconPolicyUpdateLib  | MinPlatformPkg/PlatformInit/Library/SiliconPolicyUpdateLibNull/SiliconPolicyUpdateLibNull.inf
  ReportFvLib             | QemuOpenBoardPkg/Library/PeiReportFvLib/PeiReportFvLib.inf
  PciLib                  | MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf

[LibraryClasses.Common.SEC]
  TestPointCheckLib       | MinPlatformPkg/Test/Library/TestPointCheckLib/SecTestPointCheckLib.inf
  TimerLib                | MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.Common.PEI_CORE, LibraryClasses.Common.PEIM]
  TestPointCheckLib       | MinPlatformPkg/Test/Library/TestPointCheckLib/PeiTestPointCheckLib.inf
  TestPointCheckDmaProtectionLib|MinPlatformPkg/Test/Library/TestPointCheckDmaProtectionLib/PeiTestPointCheckDmaProtectionLib.inf
  TestPointLib            | MinPlatformPkg/Test/Library/TestPointLib/PeiTestPointLib.inf
  TimerLib                | MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[Components.$(PEI_ARCH)]
  UefiCpuPkg/SecCore/SecCore.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
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
  MinPlatformPkg/PlatformInit/PlatformInitPei/PlatformInitPreMem.inf
  MinPlatformPkg/PlatformInit/ReportFv/ReportFvPei.inf
  MinPlatformPkg/PlatformInit/SiliconPolicyPei/SiliconPolicyPeiPreMem.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  QemuOpenBoardPkg/PlatformInitPei/PlatformInitPei.inf
  !if $(SMM_REQUIRED) == TRUE
    OvmfPkg/SmmAccess/SmmAccessPei.inf
  !endif
