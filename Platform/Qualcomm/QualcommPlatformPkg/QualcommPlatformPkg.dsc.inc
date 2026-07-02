## @file
# Qualcomm Platform dsc.inc file.
#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses.Common]
  SerialPortLib|Silicon/Qualcomm/Library/GeniSerialPortLib/GeniSerialPortLib.inf
  ReportFvLib|QualcommPlatformPkg/Library/PeiReportFvLib/PeiReportFvLib.inf

[LibraryClasses.AARCH64]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  ArmLib|MdePkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|UefiCpuPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
  ArmTransferListLib|ArmPkg/Library/ArmTransferListLib/ArmTransferListLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  PeiServicesTablePointerLib|ArmPkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf

  #
  # Qualcomm Libraries
  #
  PeCoffExtraActionLib|QualcommPlatformPkg/Library/DebugPecoffExtraActionTrace32Lib/DebugPecoffExtraActionTrace32Lib.inf

[LibraryClasses.AARCH64.SEC]
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  ArmPlatformLib|ArmPlatformPkg/Library/ArmPlatformLibNull/ArmPlatformLibNull.inf

[LibraryClasses.AARCH64.PEIM]
  FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  SortLib|MdeModulePkg/Library/BaseSortLib/BaseSortLib.inf

  PlatformPeiLib|ArmPlatformPkg/PlatformPei/PlatformPeiLib.inf
  MemoryInitPeiLib|ArmPlatformPkg/MemoryInitPei/MemoryInitPeiLib.inf

  #
  # Qualcomm Libraries
  #
  ReportCpuHobLib|QualcommPlatformPkg/Library/ReportCpuHobLib/ReportCpuHobLib.inf
  ArmPlatformLib|QualcommPlatformPkg/Library/ArmPlatformLib/ArmPlatformLib.inf
  RamPartitionTableLib|QualcommPlatformPkg/Library/RamPartitionTableLib/RamPartitionTableLib.inf

[Components.AARCH64]
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf

  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf

  MdeModulePkg/Bus/Ufs/UfsPassThruDxe/UfsPassThruDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf

[BuildOptions]
  *_*_AARCH64_CC_FLAGS = -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-inline

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_AARCH64_DLINK_FLAGS  = -z common-page-size=0x10000

