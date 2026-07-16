## @file
# Shared MinPlatform flash map: derives the FV/NV offsets and base addresses
# from the per-board sizes (PcdFlashArea*, PcdFlashFv*Size, PcdFlashNvStorage*Size)
# that the including board FDF sets before this include.
#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

#
# Derived sizes: FTW spare must cover variable + working; advanced fills the rest.
#
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize    = gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvAdvancedSize            = gMinPlatformPkgTokenSpaceGuid.PcdFlashAreaSize - gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize - gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize - gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemorySize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemorySize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemorySize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecuritySize

#
# FV offsets - ARM ordering (SEC / PreMemory first at offset 0, advanced last).
#
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemoryOffset         = 0x00000000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemoryOffset      = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemoryOffset         + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTOffset              = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemoryOffset      + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMOffset              = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTOffset              + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSOffset              = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMOffset              + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemoryOffset        = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSOffset              + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspOffset               = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemoryOffset        + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootOffset          = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspOffset               + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootOffset            = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootOffset          + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecurityOffset          = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootOffset            + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvAdvancedOffset          = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecurityOffset          + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecuritySize

#
# NV storage offsets (in the separate Vars FD base).
#
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageVariableOffset   = 0x00000000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingOffset = gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageVariableOffset   + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareOffset   = gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingOffset + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize

#
# FV base addresses (bottom-up from the flash area base).
#
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemoryBase    = gMinPlatformPkgTokenSpaceGuid.PcdFlashAreaBaseAddress
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemoryBase = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemoryBase    + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTBase         = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemoryBase + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMBase         = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTBase         + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSBase         = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMBase         + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemoryBase   = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSBase         + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspBase          = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemoryBase   + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootBase     = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspBase          + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootBase       = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootBase     + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecurityBase     = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootBase       + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvAdvancedBase     = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecurityBase     + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecuritySize
