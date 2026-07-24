#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildNordMinPlatformPkg.sh - Convenience wrapper to build the Nord MinPlatform Package
##

export EXTRA_BUILD_FLAGS="-D QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE=TRUE"

../../BuildOpenBoardPkg.sh --silicon Nord --signing-tool qtestsign -n 8 "$@"
