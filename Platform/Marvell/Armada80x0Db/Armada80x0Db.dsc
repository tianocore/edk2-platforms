#Copyright (C) 2017 Marvell International Ltd.
#
#SPDX-License-Identifier: BSD-2-Clause-Patent
#
################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = Armada80x0Db
  PLATFORM_GUID                  = 5cc803a0-9c42-498e-9086-e176d4a1f598
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001A
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)-$(ARCH)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Silicon/Marvell/Armada7k8k/Armada7k8k.fdf
  BOARD_DXE_FV_COMPONENTS        = Platform/Marvell/Armada80x0Db/Armada80x0Db.fdf.inc

  #
  # Network definition
  #
  DEFINE NETWORK_IP6_ENABLE             = FALSE
  DEFINE NETWORK_TLS_ENABLE             = FALSE
  DEFINE NETWORK_HTTP_BOOT_ENABLE       = FALSE
  DEFINE NETWORK_ISCSI_ENABLE           = FALSE

!include Silicon/Marvell/Armada7k8k/Armada7k8k.dsc.inc

!include MdePkg/MdeLibs.dsc.inc

[Components.common]
  Silicon/Marvell/Armada7k8k/DeviceTree/Armada80x0Db.inf

[Components.AARCH64]
  Silicon/Marvell/Armada7k8k/AcpiTables/Armada80x0Db.inf

[LibraryClasses.common]
  ArmadaBoardDescLib|Platform/Marvell/Armada80x0Db/Armada80x0DbBoardDescLib/Armada80x0DbBoardDescLib.inf
  NonDiscoverableInitLib|Platform/Marvell/Armada80x0Db/NonDiscoverableInitLib/NonDiscoverableInitLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild.common]
  #Platform description
  gMarvellSiliconTokenSpaceGuid.PcdProductPlatformName|"Armada 8040 DB"
  gMarvellSiliconTokenSpaceGuid.PcdProductVersion|"Rev. 1.4"

  #MPP
  gMarvellSiliconTokenSpaceGuid.PcdMppChipCount|3

  # APN806-A0 MPP SET
  gMarvellSiliconTokenSpaceGuid.PcdChip0MppReverseFlag|FALSE
  gMarvellSiliconTokenSpaceGuid.PcdChip0MppBaseAddress|0xF06F4000
  gMarvellSiliconTokenSpaceGuid.PcdChip0MppPinCount|20
  gMarvellSiliconTokenSpaceGuid.PcdChip0MppSel0|{ 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdChip0MppSel1|{ 0x1, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3 }

  # CP110 MPP SET - master
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppReverseFlag|FALSE
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppBaseAddress|0xF2440000
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppPinCount|64
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppSel0|{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppSel1|{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppSel2|{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppSel3|{ 0xFF, 0xFF, 0x7,  0x0,  0x7,  0xA,  0xA,  0x2,  0x2,  0x5  }
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppSel4|{ 0x9,  0x9,  0x8,  0x8,  0x1,  0x1,  0x1,  0x1,  0x1,  0x1  }
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppSel5|{ 0x1,  0x1,  0x1,  0x1,  0x1,  0x1,  0xE,  0xE,  0xE,  0xE  }
  gMarvellSiliconTokenSpaceGuid.PcdChip1MppSel6|{ 0xE,  0xE,  0xE,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0  }

  # CP110 MPP SET - slave
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppReverseFlag|FALSE
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppBaseAddress|0xF4440000
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppPinCount|64
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppSel0|{ 0x3,  0x3,  0x3,  0x3,  0x3,  0x3,  0x3,  0x3,  0x3,  0x3  }
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppSel1|{ 0x3,  0x3,  0x3,  0x3,  0x3,  0x3,  0x3,  0xFF, 0xFF, 0xFF }
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppSel2|{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x8,  0x9,  0xA  }
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppSel3|{ 0xA,  0x8,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppSel4|{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppSel5|{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  gMarvellSiliconTokenSpaceGuid.PcdChip2MppSel6|{ 0xFF, 0xFF, 0xFF, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0  }

  # I2C
  gMarvellSiliconTokenSpaceGuid.PcdI2cSlaveAddresses|{ 0x50, 0x57, 0x50, 0x57, 0x21, 0x25 }
  gMarvellSiliconTokenSpaceGuid.PcdI2cSlaveBuses|{ 0x0, 0x0, 0x1, 0x1, 0x0, 0x0 }
  gMarvellSiliconTokenSpaceGuid.PcdI2cControllersEnabled|{ 0x0, 0x1, 0x0, 0x0, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdEepromI2cAddresses|{ 0x50, 0x57, 0x50, 0x57 }
  gMarvellSiliconTokenSpaceGuid.PcdEepromI2cBuses|{ 0x0, 0x0, 0x1, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdI2cClockFrequency|250000000
  gMarvellSiliconTokenSpaceGuid.PcdI2cBaudRate|100000
  gMarvellSiliconTokenSpaceGuid.PcdI2cBusCount|2

  #SPI
  gMarvellSiliconTokenSpaceGuid.PcdSpiRegBase|0xF4700680
  gMarvellSiliconTokenSpaceGuid.PcdSpiMaxFrequency|10000000
  gMarvellSiliconTokenSpaceGuid.PcdSpiClockFrequency|200000000

  gMarvellSiliconTokenSpaceGuid.PcdSpiFlashMode|3
  gMarvellSiliconTokenSpaceGuid.PcdSpiFlashCs|0

  #ComPhy
  gMarvellSiliconTokenSpaceGuid.PcdComPhyDevices|{ 0x1, 0x1 }
  # ComPhy0
  # 0: PCIE0         5 Gbps
  # 1: SATA0         5 Gbps
  # 2: SFI           10.31 Gbps
  # 3: SATA1         5 Gbps
  # 4: USB_HOST1     5 Gbps
  # 5: PCIE2         5 Gbps
  gMarvellSiliconTokenSpaceGuid.PcdChip0ComPhyTypes|{ $(CP_PCIE0), $(CP_SATA0), $(CP_SFI), $(CP_SATA1), $(CP_USB3_HOST1), $(CP_PCIE2) }
  gMarvellSiliconTokenSpaceGuid.PcdChip0ComPhySpeeds|{ $(CP_5G), $(CP_5G), $(CP_10_3125G), $(CP_5G), $(CP_5G), $(CP_5G) }
  # ComPhy1
  # 0: PCIE0         5 Gbps
  # 1: SATA0         5 Gbps
  # 2: SFI           10.31 Gbps
  # 3: SATA1         5 Gbps
  # 4: PCIE1         5 Gbps
  # 5: PCIE2         5 Gbps
  gMarvellSiliconTokenSpaceGuid.PcdChip1ComPhyTypes|{ $(CP_PCIE0), $(CP_SATA2), $(CP_SFI), $(CP_SATA3), $(CP_PCIE1), $(CP_PCIE2) }
  gMarvellSiliconTokenSpaceGuid.PcdChip1ComPhySpeeds|{ $(CP_5G), $(CP_5G), $(CP_10_3125G), $(CP_5G), $(CP_5G), $(CP_5G) }

  #UtmiPhy
  gMarvellSiliconTokenSpaceGuid.PcdUtmiControllersEnabled|{ 0x1, 0x1, 0x1, 0x0 }
  gMarvellSiliconTokenSpaceGuid.PcdUtmiPortType|{ $(UTMI_USB_HOST0), $(UTMI_USB_HOST1), $(UTMI_USB_HOST0), $(UTMI_USB_HOST1) }

  #MDIO
  gMarvellSiliconTokenSpaceGuid.PcdMdioControllersEnabled|{ 0x1, 0x1 }

  #PHY
  gMarvellSiliconTokenSpaceGuid.PcdPhy2MdioController|{ 0x0, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdPhyDeviceIds|{ 0x0, 0x0 }
  gMarvellSiliconTokenSpaceGuid.PcdPhySmiAddresses|{ 0x1, 0x0 }
  gMarvellSiliconTokenSpaceGuid.PcdPhyStartupAutoneg|FALSE

  #NET
  gMarvellSiliconTokenSpaceGuid.PcdPp2GopIndexes|{ 0x0, 0x3, 0x0, 0x2 }
  gMarvellSiliconTokenSpaceGuid.PcdPp2InterfaceAlwaysUp|{ 0x0, 0x0, 0x0, 0x0 }
  gMarvellSiliconTokenSpaceGuid.PcdPp2InterfaceSpeed|{ $(PHY_SPEED_10000), $(PHY_SPEED_1000), $(PHY_SPEED_10000), $(PHY_SPEED_1000) }
  gMarvellSiliconTokenSpaceGuid.PcdPp2PhyConnectionTypes|{ $(PHY_SFI), $(PHY_RGMII), $(PHY_SFI), $(PHY_RGMII) }
  gMarvellSiliconTokenSpaceGuid.PcdPp2PhyIndexes|{ 0xFF, 0x0, 0xFF, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdPp2Port2Controller|{ 0x0, 0x0, 0x1, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdPp2PortIds|{ 0x0, 0x2, 0x0, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdPp2Controllers|{ 0x1, 0x1 }

  #PciEmulation
  gMarvellSiliconTokenSpaceGuid.PcdPciEXhci|{ 0x1, 0x1, 0x1, 0x0 }
  gMarvellSiliconTokenSpaceGuid.PcdPciEAhci|{ 0x1, 0x1 }
  gMarvellSiliconTokenSpaceGuid.PcdPciESdhci|{ 0x1, 0x1 }

  #RTC
  gMarvellSiliconTokenSpaceGuid.PcdRtcBaseAddress|0xF4284000
