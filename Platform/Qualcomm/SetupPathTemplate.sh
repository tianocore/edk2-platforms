#!/bin/bash

#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# SetupPathTemplate.sh - Toolchain and tool path configuration template for BuildOpenBoardPkg.sh
#
# Copy this file to SetupPath.sh in the same directory and edit it to match
# your local environment. BuildOpenBoardPkg.sh will source SetupPath.sh automatically.
#

# ---------------------------------------------------------------------------
# AArch64 GCC toolchain prefix (fallback when aarch64-linux-gnu-gcc is not in PATH).
# Set to the full path prefix of your local AArch64 GCC installation, e.g.:
#   AARCH64_TOOLCHAIN_PREFIX=/opt/my-toolchain/bin/aarch64-none-elf-
# Leave empty to skip the custom-toolchain fallback.
# ---------------------------------------------------------------------------
AARCH64_TOOLCHAIN_PREFIX="${AARCH64_TOOLCHAIN_PREFIX:-}"

# ---------------------------------------------------------------------------
# LLVM/Clang base directory
# Used when building with the CLANGDWARF toolchain.
# Set to the root of your local LLVM/Clang installation, e.g.:
#   CLANG_BASE=/opt/llvm-20
# ---------------------------------------------------------------------------
CLANG_BASE="${CLANG_BASE:-}"

# ---------------------------------------------------------------------------
# qtestsign repository URL
# Used to clone qtestsign if it is not already present in the workspace.
# ---------------------------------------------------------------------------
QTESTSIGN_REPO="${QTESTSIGN_REPO:-https://github.com/msm8916-mainline/qtestsign.git}"
