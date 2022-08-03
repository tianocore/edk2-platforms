## @file
# This is a build description file for the Payload Platform advanced feature.
# This file should be included into another package DSC file to build this feature.
#
# Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################

[Defines]
  !ifndef $(DXE_ARCH)
    !error "DXE_ARCH must be specified to build this feature!"
  !endif

  DEFINE SMM_VARIABLE                 = TRUE


################################################################################
#
# PCD definitions section - list of all PCD definitions needed by this Platform.
#
################################################################################

[PcdsPatchableInModule.X64]
!if $(SMM_VARIABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase  |0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize  |0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize  |0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase  |0
!endif

################################################################################
#
# Library Class section - list of all Library Classes needed by this feature.
#
################################################################################

[LibraryClasses]
  !if $(SMM_VARIABLE) == TRUE
    PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
    PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
    # (Optional for variable modules debug output
    PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
    DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !endif

[LibraryClasses.common.DXE_SMM_DRIVER,LibraryClasses.common.DXE_RUNTIME_DRIVER]
  !if $(SMM_VARIABLE) == TRUE
    SpiFlashLib|PlatformPayloadPkg/Library/SpiFlashLib/SpiFlashLib.inf
    FlashDeviceLib|PlatformPayloadPkg/Library/FlashDeviceLib/FlashDeviceLib.inf
    DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLib/DxeHobListLib.inf
    HobLib|UefiPayloadPkg/Library/DxeHobLib/DxeHobLib.inf
    TimerLib|UefiCpuPkg/Library/CpuTimerLib/BaseCpuTimerLib.inf
  !endif

################################################################################
#
# Component section - list of all components that need built for this feature.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
################################################################################
#
# Feature DXE Components
#

# @todo: Change below line to [Components.$(DXE_ARCH)] after https://bugzilla.tianocore.org/show_bug.cgi?id=2308
#        is completed.
[Components.X64]
  #
  # SMM Variable Support
  #
  !if $(SMM_VARIABLE) == TRUE
    PlatformPayloadPkg/PchSmiDispatchSmm/PchSmiDispatchSmm.inf
      PlatformPayloadPkg/Fvb/FvbSmm.inf {
        <LibraryClasses>
          NULL|PlatformPayloadPkg/Library/PcdInitLib/PcdInitLib.inf
      }
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
      <LibraryClasses>
        NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
        NULL|MdeModulePkg/Library/VarCheckHiiLib/VarCheckHiiLib.inf
        NULL|MdeModulePkg/Library/VarCheckPcdLib/VarCheckPcdLib.inf
        NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
        NULL|PlatformPayloadPkg/Library/PcdInitLib/PcdInitLib.inf
    }

    MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf {
      <LibraryClasses>
        NULL|PlatformPayloadPkg/Library/PcdInitLib/PcdInitLib.inf
    }
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
  !endif
