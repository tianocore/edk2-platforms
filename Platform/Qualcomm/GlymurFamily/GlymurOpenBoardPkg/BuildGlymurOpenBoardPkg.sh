#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildGlymurOpenBoardPkg.sh - Convenience wrapper to build the Glymur OpenBoard.
##

../../BuildOpenBoardPkg.sh --silicon Glymur --pkg-name GlymurOpenBoardPkg "$@" --signing-tool qtestsign
