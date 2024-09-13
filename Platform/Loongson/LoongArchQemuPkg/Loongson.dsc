## @file
#
#  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
###############################################################################
[Defines]
  PLATFORM_NAME                  = LoongArchQemu
  PLATFORMPKG_NAME               = LoongArchQemu
  PLATFORM_GUID                  = 7926ea52-b0dc-4ee8-ac63-341eebd84ed4
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = LOONGARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Loongson/LoongArchQemuPkg/Loongson.fdf
  TTY_TERMINAL                   = FALSE

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  DEFINE TTY_TERMINAL            = FALSE
  DEFINE SECURE_BOOT_ENABLE      = FALSE
  DEFINE TPM2_ENABLE             = FALSE
  DEFINE TPM2_CONFIG_ENABLE      = FALSE

  #
  # Network definition
  #
  DEFINE NETWORK_IP6_ENABLE              = FALSE
  DEFINE NETWORK_HTTP_BOOT_ENABLE        = FALSE
  DEFINE NETWORK_SNP_ENABLE              = FALSE
  DEFINE NETWORK_TLS_ENABLE              = FALSE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS  = TRUE
  DEFINE NETWORK_ISCSI_ENABLE            = FALSE

!include NetworkPkg/NetworkDefines.dsc.inc
############################################################################
#
# Defines for default states.  These can be changed on the command line.
# -D FLAG=VALUE
############################################################################
[BuildOptions]
  GCC:RELEASE_*_*_CC_FLAGS       = -DSPEEDUP

  #
  # Disable deprecated APIs.
  #
  GCC:*_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES

!include NetworkPkg/NetworkBuildOptions.dsc.inc

[BuildOptions.LOONGARCH64.EDKII.SEC]
  *_*_*_CC_FLAGS                 =

#
# default page size is 16K for loongarch qemu tcg
# code section separated with data section with 16K page alignment, else data
# write operation in the same page with code section will cause qemu TB flush
#
[BuildOptions.common.EDKII.DXE_CORE,BuildOptions.common.EDKII.DXE_DRIVER,BuildOptions.common.EDKII.UEFI_DRIVER,BuildOptions.common.EDKII.UEFI_APPLICATION]
  GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x4000

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_LOONGARCH64_DLINK_FLAGS = -z common-page-size=0x10000

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  PcdLib                           | MdePkg/Library/DxePcdLib/DxePcdLib.inf
  TimerLib                         | Platform/Loongson/LoongArchQemuPkg/Library/StableTimerLib/TimerLib.inf
  PrintLib                         | MdePkg/Library/BasePrintLib/BasePrintLib.inf
  BaseMemoryLib                    | MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  # Networking Requirements
!include NetworkPkg/NetworkLibs.dsc.inc
!if $(NETWORK_TLS_ENABLE) == TRUE
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
!endif

  BaseLib                          | MdePkg/Library/BaseLib/BaseLib.inf
  SafeIntLib                       | MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  TimeBaseLib                      | EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf
  BmpSupportLib                    | MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  SynchronizationLib               | MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  CpuLib                           | MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  PerformanceLib                   | MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PeCoffLib                        | MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  CacheMaintenanceLib              | MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  UefiDecompressLib                | MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  UefiHiiServicesLib               | MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib                           | MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  CapsuleLib                       | MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  DxeServicesLib                   | MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib              | MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  PeCoffGetEntryPointLib           | MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PciLib                           | MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib                    | OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  PciCapLib                        | OvmfPkg/Library/BasePciCapLib/BasePciCapLib.inf
  PciCapPciSegmentLib              | OvmfPkg/Library/BasePciCapPciSegmentLib/BasePciCapPciSegmentLib.inf
  PciCapPciIoLib                   | OvmfPkg/Library/UefiPciCapPciIoLib/UefiPciCapPciIoLib.inf
  DxeHardwareInfoLib               | OvmfPkg/Library/HardwareInfoLib/DxeHardwareInfoLib.inf
  IoLib                            | MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PlatformHookLib                  | Platform/Loongson/LoongArchQemuPkg/Library/Fdt16550SerialPortHookLib/Fdt16550SerialPortHookLib.inf
  SerialPortLib                    | MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  ResetSystemLib                   | Platform/Loongson/LoongArchQemuPkg/Library/ResetSystemAcpiLib/BaseResetSystemAcpiGedLib.inf

  UefiLib                          | MdePkg/Library/UefiLib/UefiLib.inf
  UefiBootServicesTableLib         | MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib      | MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiDriverEntryPoint             | MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint        | MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  DevicePathLib                    | MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
  FileHandleLib                    | MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SecurityManagementLib            | MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  UefiUsbLib                       | MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  SerializeVariablesLib            | OvmfPkg/Library/SerializeVariablesLib/SerializeVariablesLib.inf
  CustomizedDisplayLib             | MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  DebugPrintErrorLevelLib          | MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  TpmMeasurementLib                | MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  AuthVariableLib                  | MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
  VarCheckLib                      | MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  VariablePolicyLib                | MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  VariablePolicyHelperLib          | MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  SortLib                          | MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  FdtLib                           | EmbeddedPkg/Library/FdtLib/FdtLib.inf
  PciSegmentLib                    | MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PciHostBridgeLib                 | OvmfPkg/Fdt/FdtPciHostBridgeLib/FdtPciHostBridgeLib.inf
  PciHostBridgeUtilityLib          | OvmfPkg/Library/PciHostBridgeUtilityLib/PciHostBridgeUtilityLib.inf
  MmuLib                           | Platform/Loongson/LoongArchQemuPkg/Library/MmuLib/MmuBaseLib.inf
  FileExplorerLib                  | MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  ImagePropertiesRecordLib         | MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf

!if $(HTTP_BOOT_ENABLE) == TRUE
  HttpLib                          | MdeModulePkg/Library/DxeHttpLib/DxeHttpLib.inf
!endif
  UefiBootManagerLib               | MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  OrderedCollectionLib             | MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
  ReportStatusCodeLib              | MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf

  PeCoffGetEntryPointLib           | MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib             | MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DebugAgentLib                    | MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  CpuExceptionHandlerLib           | MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf

  PlatformBootManagerLib           | Platform/Loongson/LoongArchQemuPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  BootLogoLib                      | MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  QemuBootOrderLib                 | OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf
  QemuFwCfgSimpleParserLib         | OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf
  QemuLoadImageLib                 | OvmfPkg/Library/GenericQemuLoadImageLib/GenericQemuLoadImageLib.inf

  #
  # Virtio Support
  #
  VirtioLib                        | OvmfPkg/Library/VirtioLib/VirtioLib.inf
  FrameBufferBltLib                | MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  QemuFwCfgLib                     | OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgMmioPeiLib.inf
  DebugLib                         | MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  PeiServicesLib                   | MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  VariableFlashInfoLib             | MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  VirtNorFlashPlatformLib          | Platform/Loongson/LoongArchQemuPkg/Library/NorFlashQemuLib/NorFlashQemuLib.inf

[LibraryClasses.common.SEC]
  PcdLib                           | MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  ReportStatusCodeLib              | MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  HobLib                           | MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib              | MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  SerialPortLib                    | Platform/Loongson/LoongArchQemuPkg/Library/SerialPortLib/EarlySerialPortLib16550.inf

  # Loongson platforms have SEC modules with standard entry points, so we can generically link StackCheckLib
  NULL                             | MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf

[LibraryClasses.common.PEI_CORE]
  PcdLib                           | MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib                           | MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib       | Platform/Loongson/LoongArchQemuPkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  MemoryAllocationLib              | MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiCoreEntryPoint                | MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  ReportStatusCodeLib              | MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  OemHookStatusCodeLib             | MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  PeCoffGetEntryPointLib           | MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  QemuFwCfgLib                     | Platform/Loongson/LoongArchQemuPkg/Library/QemuFwCfgLib/QemuFwCfgPeiLib.inf
  MmuLib                           | Platform/Loongson/LoongArchQemuPkg/Library/MmuLib/MmuBaseLibPei.inf
  SerialPortLib                    | Platform/Loongson/LoongArchQemuPkg/Library/SerialPortLib/EarlySerialPortLib16550.inf

[LibraryClasses.common.PEIM]
  HobLib                           | MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib       | Platform/Loongson/LoongArchQemuPkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  MemoryAllocationLib              | MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeimEntryPoint                   | MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  ReportStatusCodeLib              | MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  OemHookStatusCodeLib             | MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  PeCoffGetEntryPointLib           | MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeiResourcePublicationLib        | MdePkg/Library/PeiResourcePublicationLib/PeiResourcePublicationLib.inf
  ExtractGuidedSectionLib          | MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  PcdLib                           | MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  QemuFwCfgS3Lib                   | OvmfPkg/Library/QemuFwCfgS3Lib/PeiQemuFwCfgS3LibFwCfg.inf
  QemuFwCfgLib                     | Platform/Loongson/LoongArchQemuPkg/Library/QemuFwCfgLib/QemuFwCfgPeiLib.inf
  MmuLib                           | Platform/Loongson/LoongArchQemuPkg/Library/MmuLib/MmuBaseLibPei.inf
  SerialPortLib                    | Platform/Loongson/LoongArchQemuPkg/Library/SerialPortLib/EarlySerialPortLib16550.inf

[LibraryClasses.common.DXE_CORE]
  HobLib                           | MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DxeCoreEntryPoint                | MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib              | MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ReportStatusCodeLib              | MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PciExpressLib                    | MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciPcdProducerLib                | OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  PcdLib                           | MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib                           | Platform/Loongson/LoongArchQemuPkg/Library/VirtDxeHobLib/VirtDxeHobLib.inf
  DxeCoreEntryPoint                | MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib              | MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib              | MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
  UefiRuntimeLib                   | MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  ExtractGuidedSectionLib          | MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  QemuFwCfgS3Lib                   | OvmfPkg/Library/QemuFwCfgS3Lib/DxeQemuFwCfgS3LibFwCfg.inf
  RealTimeClockLib                 | Platform/Loongson/LoongArchQemuPkg/Library/LsRealTimeClockLib/LsRealTimeClockLib.inf
  VariablePolicyLib                | MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
  QemuFwCfgLib                     | Platform/Loongson/LoongArchQemuPkg/Library/QemuFwCfgLib/QemuFwCfgPeiLib.inf
  ResetSystemLib                   | Platform/Loongson/LoongArchQemuPkg/Library/ResetSystemAcpiLib/DxeResetSystemAcpiGedLib.inf
  PciExpressLib                    | MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
!if $(TARGET) != RELEASE
  DebugLib                         | MdePkg/Library/DxeRuntimeDebugLibSerialPort/DxeRuntimeDebugLibSerialPort.inf
!endif

[LibraryClasses.common.UEFI_DRIVER]
  PcdLib                           | MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib                           | Platform/Loongson/LoongArchQemuPkg/Library/VirtDxeHobLib/VirtDxeHobLib.inf
  DxeCoreEntryPoint                | MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib              | MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib              | MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  UefiScsiLib                      | MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  ExtractGuidedSectionLib          | MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  QemuFwCfgLib                     | OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgMmioDxeLib.inf
  PciPcdProducerLib                | OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf

[LibraryClasses.common.DXE_DRIVER]
  PcdLib                           | MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib                           | Platform/Loongson/LoongArchQemuPkg/Library/VirtDxeHobLib/VirtDxeHobLib.inf
  MemoryAllocationLib              | MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib              | MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  UefiScsiLib                      | MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  CpuExceptionHandlerLib           | UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  ExtractGuidedSectionLib          | MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  QemuFwCfgS3Lib                   | OvmfPkg/Library/QemuFwCfgS3Lib/DxeQemuFwCfgS3LibFwCfg.inf
  QemuFwCfgLib                     | OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgMmioDxeLib.inf
  PciPcdProducerLib                | OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  PciExpressLib                    | MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  AcpiPlatformLib                  | OvmfPkg/Library/AcpiPlatformLib/DxeAcpiPlatformLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  PcdLib                           | MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib                           | Platform/Loongson/LoongArchQemuPkg/Library/VirtDxeHobLib/VirtDxeHobLib.inf
  MemoryAllocationLib              | MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ExtractGuidedSectionLib          | MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  PciPcdProducerLib                | OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  PciExpressLib                    | MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform.
#
################################################################################
[PcdsFeatureFlag]
   gEfiMdeModulePkgTokenSpaceGuid.PcdHiiOsRuntimeSupport               | FALSE
#  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial               | TRUE
#  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseMemory               | TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportUefiDecompress        | TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport                   | TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport                   | FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciBusHotplugDeviceSupport         | FALSE
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuBootOrderPciTranslation            | TRUE
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuBootOrderMmioTranslation           | TRUE
[PcdsFixedAtBuild]
## BaseLib ##
  gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength               | 1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength                 | 1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength                  | 1000000
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout                          | 10000000

  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeMemorySize               | 1
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange | FALSE
  gEfiMdePkgTokenSpaceGuid.PcdMaximumGuidedExtractHandler              | 0x10
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize                    | 0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize       | 0x8000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress                     | 0x0
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuMemorySize                        | 48
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize                            | 32
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask             | 0x07
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel                     | 0x8000004F

  # Use MMIO for accessing Serial port registers.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio                      | TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialPciDeviceInfo                | {0xFF}
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate                     | 115200

  # DEBUG_INIT      0x00000001  // Initialization
  # DEBUG_WARN      0x00000002  // Warnings
  # DEBUG_LOAD      0x00000004  // Load events
  # DEBUG_FS        0x00000008  // EFI File system
  # DEBUG_POOL      0x00000010  // Alloc & Free (pool)
  # DEBUG_PAGE      0x00000020  // Alloc & Free (page)
  # DEBUG_INFO      0x00000040  // Informational debug messages
  # DEBUG_DISPATCH  0x00000080  // PEI/DXE/SMM Dispatchers
  # DEBUG_VARIABLE  0x00000100  // Variable
  # DEBUG_BM        0x00000400  // Boot Manager
  # DEBUG_BLKIO     0x00001000  // BlkIo Driver
  # DEBUG_NET       0x00004000  // Network Io Driver
  # DEBUG_UNDI      0x00010000  // UNDI Driver
  # DEBUG_LOADFILE  0x00020000  // LoadFile
  # DEBUG_EVENT     0x00080000  // Event messages
  # DEBUG_GCD       0x00100000  // Global Coherency Database changes
  # DEBUG_CACHE     0x00200000  // Memory range cachability changes
  # DEBUG_VERBOSE   0x00400000  // Detailed debug messages that may
  # DEBUG_ERROR     0x80000000  // Error

!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask                        | 0x21
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask                        | 0x2f
!endif
  # DEBUG_ASSERT_ENABLED       0x01
  # DEBUG_PRINT_ENABLED        0x02
  # DEBUG_CODE_ENABLED         0x04
  # CLEAR_MEMORY_ENABLED       0x08
  # ASSERT_BREAKPOINT_ENABLED  0x10
  # ASSERT_DEADLOOP_ENABLED    0x20

#######################################################################################
  gLoongArchQemuPkgTokenSpaceGuid.PcdSecPeiTempRamBase                 | 0x10000
  gLoongArchQemuPkgTokenSpaceGuid.PcdSecPeiTempRamSize                 | 0x10000
  gLoongArchQemuPkgTokenSpaceGuid.PcdDeviceTreeBase                    | 0x100000
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiExposedTableVersions           | 0x06

  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile                | { 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

  #
  # Network Pcds
  #
!include NetworkPkg/NetworkPcds.dsc.inc

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize         | 0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize         | 0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize       | 0x40000

  gEfiMdeModulePkgTokenSpaceGuid.PcdNullPointerDetectionPropertyMask   | 1

################################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsDynamicDefault]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase         | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64       | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64       | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase         | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase       | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64     | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvStoreReserved         | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration           | FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution          | 800
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution            | 600
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut                      | 2

  # Set video resolution for text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution     | 640
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution       | 480

  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion                      | 0x0300
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev                       | 0x0

  gLoongArchQemuPkgTokenSpaceGuid.PcdRamSize                           | 0x40000000

  ## If TRUE, OvmfPkg/AcpiPlatformDxe will not wait for PCI
  #  enumeration to complete before installing ACPI tables.
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration           |TRUE
  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation                         |0x0
  # set PcdPciExpressBaseAddress to MAX_UINT64, which signifies that this
  # PCD and PcdPciDisableBusEnumeration above have not been assigned yet
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress                    |0xFFFFFFFFFFFFFFFF

  #
  # IPv4 and IPv6 PXE Boot support.
  #
  gEfiNetworkPkgTokenSpaceGuid.PcdIPv4PXESupport                       | 0x01
  gEfiNetworkPkgTokenSpaceGuid.PcdIPv6PXESupport                       | 0x01

  #
  # SMBIOS entry point version
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0300
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev|0x0
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuSmbiosValidated|TRUE

[PcdsPatchableInModule.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x0

[Components]

  #
  # SEC Phase modules
  #
  Platform/Loongson/LoongArchQemuPkg/Sec/SecMain.inf

  #
  # PEI Phase modules
  #
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  Platform/Loongson/LoongArchQemuPkg/PlatformPei/PlatformPei.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  }

  #
  # DXE Phase modules
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL                             | MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
      DevicePathLib                    | MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      ExtractGuidedSectionLib          | MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  }

  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf  {
   <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  Platform/Loongson/LoongArchQemuPkg/Drivers/CpuDxe/CpuDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  Platform/Loongson/LoongArchQemuPkg/Drivers/StableTimerDxe/TimerDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf

  #
  # Variable
  #
  OvmfPkg/VirtNorFlashDxe/VirtNorFlashDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|EmbeddedPkg/Library/NvVarStoreFormattedLib/NvVarStoreFormattedLib.inf
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }

  #
  # Platform Driver
  #
  OvmfPkg/VirtioBlkDxe/VirtioBlk.inf
  OvmfPkg/VirtioScsiDxe/VirtioScsi.inf
  OvmfPkg/VirtioRngDxe/VirtioRng.inf

  #
  # FAT filesystem + GPT/MBR partitioning + UDF filesystem + virtio-fs
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Universal/Disk/UdfDxe/UdfDxe.inf
  OvmfPkg/VirtioFsDxe/VirtioFsDxe.inf

  #
  #BDS
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf {
    <LibraryClasses>
      DevicePathLib                    | MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      PcdLib                           | MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Logo/LogoDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

  OvmfPkg/QemuKernelLoaderFsDxe/QemuKernelLoaderFsDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Library/BlobVerifierLibNull/BlobVerifierLibNull.inf
  }

  #
  # Network Support
  #
#!include NetworkPkg/NetworkComponents.dsc.inc

#  NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf {
#    <LibraryClasses>
#      NULL|OvmfPkg/Library/PxeBcPcdProducerLib/PxeBcPcdProducerLib.inf
#  }

!if $(NETWORK_TLS_ENABLE) == TRUE
  NetworkPkg/TlsAuthConfigDxe/TlsAuthConfigDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Library/TlsAuthConfigLib/TlsAuthConfigLib.inf
  }
!endif
  OvmfPkg/VirtioNetDxe/VirtioNet.inf

  #
  # IDE/SCSI
  #
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  #
  # NVME Driver
  #
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  #
  # SMBIOS Support
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf {
    <LibraryClasses>
      NULL                             | OvmfPkg/Library/SmbiosVersionLib/DetectSmbiosVersionLib.inf
  }
  OvmfPkg/SmbiosPlatformDxe/SmbiosPlatformDxe.inf

  #
  # PCI
  #
  Platform/Loongson/LoongArchQemuPkg/Drivers/PciCpuIo2Dxe/PciCpuIo2Dxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
      NULL|OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  }
  EmbeddedPkg/Drivers/FdtClientDxe/FdtClientDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
      NULL|OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  }
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
      NULL|OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  }
  OvmfPkg/VirtioPciDeviceDxe/VirtioPciDeviceDxe.inf
  OvmfPkg/Virtio10Dxe/Virtio10.inf

  #
  # Console
  #
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }

  #
  # Video
  #
  OvmfPkg/QemuVideoDxe/QemuVideoDxe.inf
  OvmfPkg/QemuRamfbDxe/QemuRamfbDxe.inf
  OvmfPkg/VirtioGpuDxe/VirtioGpu.inf
  OvmfPkg/PlatformDxe/Platform.inf

  #
  # Usb Support
  #
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  #
  # ACPI Support
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf
  OvmfPkg/AcpiPlatformDxe/AcpiPlatformDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }

  #
  #app
  #
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
      FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
      SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
      PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
<PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|8000
  }
