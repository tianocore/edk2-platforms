#;*****************************************************************************
#; Copyright (C) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
#;
#;******************************************************************************
#
## @file
# CRB specific - External AGESA build.
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
