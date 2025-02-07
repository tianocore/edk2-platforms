## @file
#  dsc include file for Platform Dxe
#
#  Copyright (c) 2020, 2023 ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]

[BuildOptions.AARCH64]
# Required for pre-processing Platform Dxe which includes platform-type specific build flags.
!ifdef $(ENABLE_MORELLO_CAP)
  *_CLANGDWARF_AARCH64_ASM_FLAGS    = -march=morello
!endif
