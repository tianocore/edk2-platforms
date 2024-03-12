# @file
#  Component description file for the AlderLake SiPkg DXE libraries.
#
#   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
#   SPDX-License-Identifier: BSD-2-Clause-Patent
##

#
# FRUs
#
!include $(PLATFORM_SI_PACKAGE)/Fru/AdlCpu/DxeLib.dsc
!include $(PLATFORM_SI_PACKAGE)/Fru/AdlPch/DxeLib.dsc

#
# Pch
#
 SmmPchPrivateLib|$(PLATFORM_SI_PACKAGE)/Pch/LibraryPrivate/SmmPchPrivateLib/SmmPchPrivateLib.inf

#
# Common
#
 AslUpdateLib|IntelSiliconPkg/Library/DxeAslUpdateLib/DxeAslUpdateLib.inf
 SiConfigBlockLib|$(PLATFORM_SI_PACKAGE)/Library/BaseSiConfigBlockLib/BaseSiConfigBlockLib.inf

#
# SystemAgent
#
 DxeSaPolicyLib|$(PLATFORM_SI_PACKAGE)/SystemAgent/Library/DxeSaPolicyLib/DxeSaPolicyLib.inf
