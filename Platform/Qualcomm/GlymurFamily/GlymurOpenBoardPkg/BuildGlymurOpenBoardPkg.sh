#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildGlymurOpenBoardPkg.sh - Convenience wrapper to build the Glymur OpenBoard.
##

export EXTRA_BUILD_FLAGS="-D QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE=TRUE"

../../BuildOpenBoardPkg.sh --silicon Glymur --pkg-name GlymurOpenBoardPkg "$@" --signing-tool qtestsign
