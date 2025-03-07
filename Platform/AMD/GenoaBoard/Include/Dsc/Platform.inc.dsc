#;*****************************************************************************
#;
#; Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
#; SPDX-License-Identifier: BSD-2-Clause-Patent
#;
#;*****************************************************************************

# CPM patch
!include AmdCpmPkg/Addendum/Oem/$(PLATFORM_CRB)/Processor/$(AMD_PROCESSOR)/AmdCpm$(AMD_PROCESSOR)$(PLATFORM_CRB)Pkg.inc.dsc

# AMD AGESA Include Path
!ifdef $(INTERNAL_IDS)
  !include $(PROCESSOR_PATH)/Include/AgesaInc/AgesaInt.inc.dsc
!else
  !include $(PROCESSOR_PATH)/Include/AgesaInc/AgesaExt.inc.dsc
!endif
