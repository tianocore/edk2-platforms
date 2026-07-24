#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildKodiakMinPlatformPkg.sh - Convenience wrapper to build the Kodiak Min Platform Package
##

export EXTRA_BUILD_FLAGS="-D QUALCOMM_DEVICETREE_FRAMEWORK_ENABLE=TRUE"

../../BuildOpenBoardPkg.sh --silicon Kodiak --signing-tool qtestsign -n 8 "$@"
