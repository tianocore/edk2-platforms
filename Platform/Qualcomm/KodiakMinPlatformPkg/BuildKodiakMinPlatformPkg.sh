#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildKodiakMinPlatformPkg.sh - Convenience wrapper to build the Kodiak Min Platform Package
##
../BuildOpenBoardPkg.sh --silicon Kodiak --signing-tool qtestsign -n 8 "$@"
