#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildNordMinPlatformPkg.sh - Convenience wrapper to build the Nord MinPlatform Package
##

../../BuildOpenBoardPkg.sh --silicon Nord --signing-tool qtestsign -n 8 "$@"
