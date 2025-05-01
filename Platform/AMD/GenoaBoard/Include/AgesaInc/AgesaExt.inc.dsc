#;*****************************************************************************
#;
#; Copyright (C) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
#; SPDX-License-Identifier: BSD-2-Clause-Patent
#;
#;*****************************************************************************
#
## @file
# CRB specific - External AGESA build.
#
##
  #
  # AMD AGESA Includes - External
  #
  !include AgesaModulePkg/AgesaSp5RsModulePkg.inc.dsc
  !if $(CBS_INCLUDE) == TRUE
    !include AmdCbsPkg/Library/Family/0x19/RS/External/CbsStones.inc.dsc
  !endif
