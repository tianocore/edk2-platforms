## @file
#
#  CRB specific - External AGESA build.
#  
#  Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
  #
  # AMD AGESA Includes - External
  #
  !include AgesaModulePkg/AgesaSp5$(SOC_SKU_TITLE)ModulePkg.inc.dsc
  !if $(CBS_INCLUDE) == TRUE
    !if $(SOC_FAMILY) != $(SOC_FAMILY_2)
      !include AmdCbsPkg/Library/Family/$(SOC_FAMILY_2)/$(SOC_SKU_2)/External/Cbs$(SOC2_2).inc.dsc
    !endif
    !include AmdCbsPkg/Library/Family/$(SOC_FAMILY)/$(SOC_SKU)/External/Cbs$(SOC2).inc.dsc
  !endif
