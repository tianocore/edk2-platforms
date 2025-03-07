## @file
#
#  Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

# CPM path
!ifndef CPM_DIR_PATH
  CPM_DIR_PATH = $(AMD_PROCESSOR)/AmdCpm$(AMD_PROCESSOR)
!endif
!include AmdCpmPkg/Addendum/Oem/$(PLATFORM_CRB)/Processor/$(CPM_DIR_PATH)$(PLATFORM_CRB)Pkg.inc.dsc

# AMD AGESA Include Path
!ifdef $(INTERNAL_IDS)
  !include $(PROCESSOR_PATH)/Include/AgesaInc/AgesaInt.inc.dsc
!else
  !include $(PROCESSOR_PATH)/Include/AgesaInc/AgesaExt.inc.dsc
!endif
