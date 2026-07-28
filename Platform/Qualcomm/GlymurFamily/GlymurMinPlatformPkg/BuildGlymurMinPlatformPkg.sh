#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildGlymurMinPlatformPkg.sh - Convenience wrapper to build the Glymur MinPlatform.
##

export EXTRA_BUILD_FLAGS="-D QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE=TRUE"

../../BuildOpenBoardPkg.sh --silicon Glymur "$@" --signing-tool qtestsign
