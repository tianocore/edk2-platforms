## @file
#  dsc include file for Configuration Manager
#
#  Copyright (c) 2017 - 2020, Arm Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]

[BuildOptions]

[LibraryClasses.common]

[Components.common]
  # Configuration Manager
  Platform/ARM/JunoPkg/ConfigurationManager/ConfigurationManagerDxe/ConfigurationManagerDxe.inf {
!if $(ENABLE_TPM) == TRUE
    <LibraryClasses>
      NULL|SecurityPkg/Library/Tpm2DeviceLibFfa/Tpm2InstanceLibFfa.inf
      NULL|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
!endif
  }
