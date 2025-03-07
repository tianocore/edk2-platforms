## @file
#
#  The DSC include file for edk2 package to pull in the necessary AGESA modules
#  into build process.
#  
#  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses]
  NbioCommonDxeLib|AgesaModulePkg/Nbio/Library/CommonDxe/NbioCommonDxeLib.inf

  ## Gnb Lib
  NbioHandleLib|AgesaModulePkg/Library/NbioHandleLib/NbioHandleLib.inf
  PcieConfigLib|AgesaModulePkg/Library/PcieConfigLib/PcieConfigLib.inf

