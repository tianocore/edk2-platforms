#;*****************************************************************************
#; Copyright (C) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
#;
#;******************************************************************************
#
## @file
# CRB specific - Internal AGESA build.
#
##
  #
  # AMD AGESA Includes - Internal
  #
  !include AgesaModulePkg/AgesaSp5$(SOC_SKU_TITLE)ModulePkg.inc.dsc
  !include AgesaModulePkg/AgesaIdsIntBrh.inc.dsc
  !if $(CBS_INCLUDE) == TRUE
    !if $(SOC_FAMILY) != $(SOC_FAMILY_2)
      !include AmdCbsPkg/Library/Family/$(SOC_FAMILY_2)/$(SOC_SKU_2)/Internal/Cbs$(SOC2_2).inc.dsc
    !endif
    !include AmdCbsPkg/Library/Family/$(SOC_FAMILY)/$(SOC_SKU)/Internal/Cbs$(SOC2).inc.dsc
  !else
    !include AmdCbsPkg/Library/CbsInstanceNull/CbsInstanceNull.inc.dsc
  !endif
