## @file
#  DSC file for defining default PCD for MinPlatform features.
#
# This makes it easy for boards to initially configure their build.
#
# This initializes features to be disable by default.
# A board can enable a feature by configuring the PCD in the board DSC file
# after this file has been included.
#
# Copyright (C) 2022 - 2024 Intel Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[PcdsFeatureFlag]
 gMinPlatformPkgTokenSpaceGuid.PcdStopAfterDebugInit            |FALSE
 gMinPlatformPkgTokenSpaceGuid.PcdStopAfterMemInit              |FALSE
 gMinPlatformPkgTokenSpaceGuid.PcdBootToShellOnly               |FALSE
 gMinPlatformPkgTokenSpaceGuid.PcdSerialTerminalEnable          |FALSE

 gMinPlatformPkgTokenSpaceGuid.PcdUefiSecureBootEnable          |FALSE
 gMinPlatformPkgTokenSpaceGuid.PcdTpm2Enable                    |FALSE

 gMinPlatformPkgTokenSpaceGuid.PcdPerformanceEnable             |FALSE

 gMinPlatformPkgTokenSpaceGuid.PcdStandaloneMmEnable            |FALSE
