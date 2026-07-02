#!/bin/bash

## @file
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildGlymurOpenBoardPkg.sh - Convenience wrapper to build the Glymur Open Board Package
##

../BuildOpenBoardPkg.sh --silicon Glymur "$@" --signing-tool qtestsign
