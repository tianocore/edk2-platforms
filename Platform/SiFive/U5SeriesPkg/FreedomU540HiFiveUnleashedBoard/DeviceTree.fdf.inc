## @file
#  FDF include file with Layout Regions that define an empty variable store.
#
#  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#  Copyright (C) 2014, Red Hat, Inc.
#  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

$(DTB_OFFSET)|$(DTB_SIZE)
gUefiRiscVPlatformPkgTokenSpaceGuid.PcdRiscVDtbFvBase|gUefiRiscVPlatformPkgTokenSpaceGuid.PcdRiscVDtbFvSize
FV = DTBFV

[FV.DTBFV]
BlockSize          = 0x1000
FvAlignment        = 16
ERASE_POLARITY     = 1
MEMORY_MAPPED      = TRUE
STICKY_WRITE       = TRUE
LOCK_CAP           = TRUE
LOCK_STATUS        = TRUE
WRITE_DISABLED_CAP = TRUE
WRITE_ENABLED_CAP  = TRUE
WRITE_STATUS       = TRUE
WRITE_LOCK_CAP     = TRUE
WRITE_LOCK_STATUS  = TRUE
READ_DISABLED_CAP  = TRUE
READ_ENABLED_CAP   = TRUE
READ_STATUS        = TRUE
READ_LOCK_CAP      = TRUE
READ_LOCK_STATUS   = TRUE

INF RuleOverride = DTB Platform/SiFive/U5SeriesPkg/FreedomU540HiFiveUnleashedBoard/DeviceTree/U540DeviceTree.inf
