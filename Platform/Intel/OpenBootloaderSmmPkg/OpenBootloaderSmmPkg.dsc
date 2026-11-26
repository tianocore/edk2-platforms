# @file
#  Copyright (c) 2025 Intel Corporation. All rights reserved. <BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

[Defines]
  PLATFORM_NAME                       = OpenBootloaderSmmPkg
  PLATFORM_GUID                       = D7EAF54D-C9B9-4075-89F0-71943DBCFA62
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  OUTPUT_DIRECTORY                    = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES             = X64
  BUILD_TARGETS                       = DEBUG|RELEASE
  SKUID_IDENTIFIER                    = ALL
  VPD_TOOL_GUID                       = 7C3D856A-8BE6-468E-850A-24F7A8D38E08
  FLASH_DEFINITION                    = Platform/Intel/$(PLATFORM_NAME)/OpenBootloaderSmmPkg.fdf
  SHOW_INCLUDES                       = FALSE
  PLAT_NAME                           = OpenBootloaderSmmPkg
  OB_PKG                              = MinPlatformPkg
  DXE_ARCH                            = X64

[Packages]
  MdePkg/MdePkg.dec
  OpenBootloaderSmmPkg/OpenBootloaderSmmPkg.dec

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel |0x80000043

#
# Fsp Extentsion Package
#
[Components.X64]
  !include OpenBootloaderSmmPkg/Include/OpenBootloaderSmmPkg.dsc.inc


[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  TimerLib|UefiCpuPkg/Library/CpuTimerLib/BaseCpuTimerLib.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  HobLib|StandaloneMmPkg/Library/StandaloneMmHobLib/StandaloneMmHobLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  StackCheckLib|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/StandaloneMmReportStatusCodeLib.inf
  MemoryAllocationLib|StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemLib|StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
  MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf

[BuildOptions]
  GCC:*_*_*_CC_FLAGS = -D MAX_SOCKET=4  -D MAX_IMC=4 -D MAX_MC_CH=2 -D MAX_SAD_RULES=16 -D MAX_DRAM_CLUSTERS=4 -D MAX_KTI_PORTS=8 -D MAX_IIO_STACK=16 -D MAX_LOGIC_IIO_STACK=18 -D MAX_SUB_CH=4  -D DMR_HOST -D MAX_RANK_DIMM=5 -D MAX_RANK_CH=4 -D MAX_PPR_ENTRIES_DDR=128 "-DEFIAPI=__attribute__((ms_abi))"
  MSFT:*_*_*_CC_FLAGS = -D MAX_SOCKET=4  -D MAX_IMC=4 -D MAX_MC_CH=2 -D MAX_SAD_RULES=16 -D MAX_DRAM_CLUSTERS=4 -D MAX_KTI_PORTS=8 -D MAX_IIO_STACK=16 -D MAX_LOGIC_IIO_STACK=18 -D MAX_SUB_CH=4  -D DMR_HOST -D MAX_RANK_DIMM=5 -D MAX_RANK_CH=4 -D MAX_PPR_ENTRIES_DDR=128

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER, BuildOptions.common.EDKII.DXE_SMM_DRIVER, BuildOptions.common.EDKII.SMM_CORE, BuildOptions.common.EDKII.MM_STANDALONE, BuildOptions.common.EDKII.MM_CORE_STANDALONE]
 MSFT:*_*_*_DLINK_FLAGS        = /ALIGN:4096 /FILEALIGN:4096
 GCC:*_GCC*_*_DLINK_FLAGS      = -z common-page-size=0x1000
 CLANGPDB:*_*_*_DLINK_FLAGS    = /ALIGN:4096 /FILEALIGN:4096