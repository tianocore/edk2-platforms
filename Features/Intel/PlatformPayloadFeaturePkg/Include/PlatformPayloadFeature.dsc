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

  !ifndef SMM_VARIABLE
    DEFINE SMM_VARIABLE                 = TRUE
  !endif


################################################################################
#
# PCD definitions section - list of all PCD definitions needed by this Platform.
#
################################################################################

[PcdsPatchableInModule]
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

[LibraryClasses.common.DXE_SMM_DRIVER,LibraryClasses.common.DXE_RUNTIME_DRIVER]
  !if $(SMM_VARIABLE) == TRUE
    SpiFlashLib|PlatformPayloadFeaturePkg/Library/SpiFlashLib/SpiFlashLib.inf
    FlashDeviceLib|PlatformPayloadFeaturePkg/Library/FlashDeviceLib/FlashDeviceLib.inf
    DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLib/DxeHobListLib.inf
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
[Components.$(DXE_ARCH)]
  #
  # SMM Variable Support
  #
  !if $(SMM_VARIABLE) == TRUE
    PlatformPayloadFeaturePkg/PchSmiDispatchSmm/PchSmiDispatchSmm.inf {
      <LibraryClasses>
        PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
    }

    PlatformPayloadFeaturePkg/Fvb/FvbSmm.inf {
      <LibraryClasses>
        PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
        NULL|PlatformPayloadFeaturePkg/Library/PcdInitLib/PcdInitLib.inf
    }

    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
      <LibraryClasses>
        PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
        NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
        NULL|MdeModulePkg/Library/VarCheckHiiLib/VarCheckHiiLib.inf
        NULL|MdeModulePkg/Library/VarCheckPcdLib/VarCheckPcdLib.inf
        NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
        NULL|PlatformPayloadFeaturePkg/Library/PcdInitLib/PcdInitLib.inf
    }

    MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf {
      <LibraryClasses>
        PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
        NULL|PlatformPayloadFeaturePkg/Library/PcdInitLib/PcdInitLib.inf
    }
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf {
      <LibraryClasses>
        PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
    }
  !endif
