## @file
# Flashmap and variable definitions for GlymurOpenBoardPkg FVs and FD
#
# Copyright (C) 2022 Theo Jehl<BR>
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

#
# These three items are tightly coupled.
# The spare area size must be >= the first two areas.
# The total size must match the size in the EFI_FIRMWARE_VOLUME_HEADER.
# The NvStorageVariableSize must also match the VARIABLE_STORE_HEADER size.
# The EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER doesn't have size info.
#
# There isn't really a benefit to a larger spare area unless the FLASH device
# block size is larger than the size specified.
#
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize    = 0x0003C000
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize  = 0x00004000
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize    = gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize

#
# Early FV
#
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemorySize           = 0x00060000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemorySize        = 0x00010000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTSize                = 0x00010000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMSize                = 0x00020000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSSize                = 0x00020000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemorySize          = 0x00080000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspSize                 = 0x00020000

#
# Later FV
#
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootSize            = 0x000C0000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootSize              = 0x00040000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecuritySize            = 0x00080000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvAdvancedSize            = gMinPlatformPkgTokenSpaceGuid.PcdFlashAreaSize - gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize - gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize - gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemorySize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemorySize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemorySize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootSize - gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecuritySize

#
# Calculate Offsets - REORDERED for ARM (Sec First)
#

# 1. Start with PreMemory (Contains SEC / Reset Vector) at Offset 0
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemoryOffset         = 0x00000000

# 2. Stack subsequent stages upwards
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemoryOffset      = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemoryOffset         + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPreMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTOffset              = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemoryOffset      + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspPreMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMOffset              = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTOffset              + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspTSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSOffset              = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMOffset              + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspMSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemoryOffset        = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSOffset              + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvFspSSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspOffset               = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemoryOffset        + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvPostMemorySize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootOffset          = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspOffset               + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvBspSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootOffset            = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootOffset          + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvUefiBootSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecurityOffset          = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootOffset            + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvOsBootSize

# 3. Place Advanced (Filler) at the very end
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashFvAdvancedOffset          = gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecurityOffset          + gMinPlatformPkgTokenSpaceGuid.PcdFlashFvSecuritySize

# (Variable Storage offsets remain unchanged as they are in a separate FD/Base)
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageVariableOffset   = 0x00000000
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingOffset = gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageVariableOffset   + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize
SET gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareOffset   = gMinPlatformPkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingOffset + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize
#
# Calculate base addresses
# NordMinPlatformPkgVars FD
#


SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase      = gMinPlatformPkgTokenSpaceGuid.PcdFlashAreaBaseAddress
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase    = gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize  + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase      = gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase  +  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize

#
# NordMinPlatformPkg FD
#



#
# FV Base Addresses (Bottom-Up for ARM)
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
