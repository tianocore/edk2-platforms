#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildGlymurMinPlatformPkg.sh - Convenience wrapper to build the Glymur MinPlatform.
##

../../BuildOpenBoardPkg.sh --silicon Glymur "$@" --signing-tool qtestsign
