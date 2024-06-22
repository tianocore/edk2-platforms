## @file
#  Platform description.
#
# Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

  #
  # Generic EDKII Lib
  #

  #
  # PEI phase common
  #

[LibraryClasses.common.SEC, LibraryClasses.common.PEI_CORE]
!if $(TARGET) != RELEASE
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif

[LibraryClasses.common.SEC, LibraryClasses.common.PEI_CORE, LibraryClasses.common.PEIM]
  S3BootScriptLib|MdePkg/Library/BaseS3BootScriptLibNull/BaseS3BootScriptLibNull.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLibBase.inf
  !if gMinPlatformPkgTokenSpaceGuid.PcdStandaloneMmEnable == TRUE
    LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  !else
    LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  !endif
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf

!if gMinPlatformPkgTokenSpaceGuid.PcdPerformanceEnable == TRUE
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
!endif

  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf
  CcExitLib|UefiCpuPkg/Library/CcExitLibNull/CcExitLibNull.inf

[LibraryClasses.common.SEC]
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  VariableReadLib|MinPlatformPkg/Library/BaseVariableReadLibNull/BaseVariableReadLibNull.inf

[LibraryClasses.common.PEI_CORE, LibraryClasses.common.PEIM]
  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/PeiAcpiTimerLib.inf

[LibraryClasses.common.PEIM]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf

  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterPei.inf
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/PeiTcg2PhysicalPresenceLib/PeiTcg2PhysicalPresenceLib.inf
  TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLib/PeiDxeTpmPlatformHierarchyLib.inf

  FspMeasurementLib|IntelFsp2WrapperPkg/Library/BaseFspMeasurementLib/BaseFspMeasurementLib.inf
  FspWrapperPlatformMultiPhaseLib|IntelFsp2WrapperPkg/Library/BaseFspWrapperPlatformMultiPhaseLibNull/BaseFspWrapperPlatformMultiPhaseLibNull.inf
  FspWrapperMultiPhaseProcessLib|IntelFsp2WrapperPkg/Library/FspWrapperMultiPhaseProcessLib/FspWrapperMultiPhaseProcessLib.inf
  TcgEventLogRecordLib|SecurityPkg/Library/TcgEventLogRecordLib/TcgEventLogRecordLib.inf
  TpmMeasurementLib|SecurityPkg/Library/PeiTpmMeasurementLib/PeiTpmMeasurementLib.inf

  VariableReadLib|MinPlatformPkg/Library/PeiVariableReadLib/PeiVariableReadLib.inf

  SmmRelocationLib|UefiCpuPkg/Library/SmmRelocationLib/SmmRelocationLib.inf
  SmmControlLib|IntelSiliconPkg/Feature/SmmControl/Library/PeiSmmControlLib/PeiSmmControlLib.inf
  MmUnblockMemoryLib|UefiCpuPkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLib.inf
