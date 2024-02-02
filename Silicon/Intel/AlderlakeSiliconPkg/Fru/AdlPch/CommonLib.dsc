## @file
#  Component description file for the AlderLake PCH Common FRU libraries.
#
#   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
#   SPDX-License-Identifier: BSD-2-Clause-Patent
##

  PchPcrLib|$(PLATFORM_SI_PACKAGE)/IpBlock/P2sb/Library/PeiDxeSmmPchPcrLib/PeiDxeSmmPchPcrLib.inf
  PchSbiAccessLib|$(PLATFORM_SI_PACKAGE)/IpBlock/P2sb/LibraryPrivate/PeiDxeSmmPchSbiAccessLib/PeiDxeSmmPchSbiAccessLib.inf
  P2SbSidebandAccessLib|$(PLATFORM_SI_PACKAGE)/IpBlock/P2sb/LibraryPrivate/PeiDxeSmmP2SbSidebandAccessLib/PeiDxeSmmP2SbSidebandAccessLib.inf

  EspiLib|$(PLATFORM_SI_PACKAGE)/IpBlock/Espi/Library/PeiDxeSmmEspiLib/PeiDxeSmmEspiLib.inf

  SpiAccessLib|$(PLATFORM_SI_PACKAGE)/IpBlock/Spi/Library/PeiDxeSmmSpiAccessLib/PeiDxeSmmSpiAccessLib.inf
  PmcLib|$(PLATFORM_SI_PACKAGE)/IpBlock/Pmc/Library/PeiDxeSmmPmcLib/PeiDxeSmmPmcLib.inf
  PmcPrivateLib|$(PLATFORM_SI_PACKAGE)/IpBlock/Pmc/LibraryPrivate/PeiDxeSmmPmcPrivateLib/PeiDxeSmmPmcPrivateLib.inf
  SpiCommonLib|$(PLATFORM_SI_PACKAGE)/IpBlock/Spi/LibraryPrivate/BaseSpiCommonLib/BaseSpiCommonLib.inf
  GpioLib|$(PLATFORM_SI_PACKAGE)/IpBlock/Gpio/Library/PeiDxeSmmGpioLib/PeiDxeSmmGpioLib.inf
  PchDmiLib|$(PLATFORM_SI_PACKAGE)/IpBlock/PchDmi/LibraryPrivate/PeiDxeSmmPchDmiLib/PeiDxeSmmPchDmiLib.inf

  GpioPrivateLib|$(PLATFORM_SI_PACKAGE)/IpBlock/Gpio/LibraryPrivate/PeiDxeSmmGpioPrivateLib/PeiDxeSmmGpioPrivateLibVer2.inf
  PchPcieRpLib|$(PLATFORM_SI_PACKAGE)/IpBlock/PcieRp/Library/PeiDxeSmmPchPcieRpLib/PeiDxeSmmPchPcieRpLibVer2.inf


  #
  # Common FRU Libraries
  #
  PchInfoLib|$(PLATFORM_SI_PACKAGE)/Fru/AdlPch/Library/PeiDxeSmmPchInfoLib/PeiDxeSmmPchInfoLibAdl.inf

