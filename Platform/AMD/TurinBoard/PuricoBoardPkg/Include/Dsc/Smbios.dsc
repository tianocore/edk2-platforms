## @file
#
#  Smbios Platform description.
#  
#  Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[PcdsFixedAtBuild]
  #****************************************************************************
  # COMMON SMBIOS
  #****************************************************************************
  #
  # IPMI Interface Type
  #
  # 0 - Unknown
  # 1 - KCS
  # 2 - SMIC
  # 3 - BT
  # 4 - SSIF
  gAmdPlatformPkgTokenSpaceGuid.PcdIpmiInterfaceType|1

  # SMBIOS Type 4 Processor Information
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdSmbiosSerialNumberSocket0|"To be filled by O.E.M."
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdSmbiosAssetTagSocket0|"To be filled by O.E.M."
  gEfiAmdAgesaPkgTokenSpaceGuid.PcdAmdSmbiosPartNumberSocket0|"To be filled by O.E.M."

# AMD SMBIOS Type 8 record
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8Number|7

  # AMD SMBIOS Type 9 record
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType9SlotCharacteristics1.Provides33Volts|1
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType9SlotCharacteristics2.BifurcationSupported|1

  # Port #0
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[0].Type8Data.InternalReferenceDesignator|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[0].Type8Data.InternalConnectorType|PortConnectorTypeNone
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[0].Type8Data.ExternalReferenceDesignator|0x02
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[0].Type8Data.ExternalConnectorType|PortConnectorTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[0].Type8Data.PortType|PortTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[0].DesinatorStr.IntDesignatorStr|"USB1"
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[0].DesinatorStr.ExtDesignatorStr|"USB-Rear 1"

  # Port #1
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[1].Type8Data.InternalReferenceDesignator|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[1].Type8Data.InternalConnectorType|PortConnectorTypeNone
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[1].Type8Data.ExternalReferenceDesignator|0x02
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[1].Type8Data.ExternalConnectorType|PortConnectorTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[1].Type8Data.PortType|PortTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[1].DesinatorStr.IntDesignatorStr|"USB1"
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[1].DesinatorStr.ExtDesignatorStr|"USB-Rear 2"

  # Port #2
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[2].Type8Data.InternalReferenceDesignator|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[2].Type8Data.InternalConnectorType|PortConnectorTypeNone
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[2].Type8Data.ExternalReferenceDesignator|0x02
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[2].Type8Data.ExternalConnectorType|PortConnectorTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[2].Type8Data.PortType|PortTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[2].DesinatorStr.IntDesignatorStr|"F_USB1"
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[2].DesinatorStr.ExtDesignatorStr|"USB-Front 1"

  # Port #3
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[3].Type8Data.InternalReferenceDesignator|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[3].Type8Data.InternalConnectorType|PortConnectorTypeNone
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[3].Type8Data.ExternalReferenceDesignator|0x02
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[3].Type8Data.ExternalConnectorType|PortConnectorTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[3].Type8Data.PortType|PortTypeUsb
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[3].DesinatorStr.IntDesignatorStr|"F_USB1"
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[3].DesinatorStr.ExtDesignatorStr|"USB-Front 2"

  # Port #4
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[4].Type8Data.InternalReferenceDesignator|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[4].Type8Data.InternalConnectorType|PortConnectorTypeNone
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[4].Type8Data.ExternalReferenceDesignator|0x02
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[4].Type8Data.ExternalConnectorType|PortConnectorTypeRJ45
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[4].Type8Data.PortType|PortTypeNetworkPort
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[4].DesinatorStr.IntDesignatorStr|"LAN0"
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[4].DesinatorStr.ExtDesignatorStr|"LAN0"

  # Port #5
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[5].Type8Data.InternalReferenceDesignator|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[5].Type8Data.InternalConnectorType|PortConnectorTypeNone
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[5].Type8Data.ExternalReferenceDesignator|0x02
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[5].Type8Data.ExternalConnectorType|PortConnectorTypeRJ45
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[5].Type8Data.PortType|PortTypeNetworkPort
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[5].DesinatorStr.IntDesignatorStr|"LAN1"
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[5].DesinatorStr.ExtDesignatorStr|"LAN1"

  # Port #6
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[6].Type8Data.InternalReferenceDesignator|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[6].Type8Data.InternalConnectorType|PortConnectorTypeNone
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[6].Type8Data.ExternalReferenceDesignator|0x02
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[6].Type8Data.ExternalConnectorType|PortConnectorTypeDB15Female
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[6].Type8Data.PortType|PortTypeVideoPort
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[6].DesinatorStr.IntDesignatorStr|"J129"
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType8.SmbiosPortConnectorRecords[6].DesinatorStr.ExtDesignatorStr|"VGA"

  # AMD SMBIOS Type 41 record
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41Number|1
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41.SmbiosOnboardDevExtInfos[0].ReferenceDesignation|0x01
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41.SmbiosOnboardDevExtInfos[0].DeviceType|OnBoardDeviceExtendedTypeEthernet
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41.SmbiosOnboardDevExtInfos[0].DeviceEnabled|1
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41.SmbiosOnboardDevExtInfos[0].DeviceTypeInstance|1
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41.SmbiosOnboardDevExtInfos[0].VendorId|0x14E4
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41.SmbiosOnboardDevExtInfos[0].DeviceId|0x165F
  gAmdPlatformPkgTokenSpaceGuid.PcdAmdSmbiosType41.SmbiosOnboardDevExtInfos[0].RefDesignationStr|"Onboard Ethernet"

[PcdsDynamicDefault]
  #****************************************************************************
  # BASIC SMBIOS
  #****************************************************************************
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0305
  # SMBIOS Type 0 BIOS Information Strings
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0StringBiosReleaseDate|"$(RELEASE_DATE)"
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0StringBiosVersion|"$(FIRMWARE_VERSION_STR)"
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.SystemBiosMajorRelease|0xFF
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.SystemBiosMinorRelease|0xFF
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.EmbeddedControllerFirmwareMajorRelease|0xFF
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.EmbeddedControllerFirmwareMinorRelease|0xFF
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.ExtendedBiosSize.Size|32
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.ExtendedBiosSize.Unit|0x00
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.PlugAndPlayIsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.EDDSpecificationIsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.Floppy525_12IsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.Floppy35_720IsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.Floppy35_288IsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.PrintScreenIsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.Keyboard8042IsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.SerialIsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.PrinterIsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BiosCharacteristics.CgaMonoIsSupported|0
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BIOSCharacteristicsExtensionBytes[0]|0x01
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0BiosInformation.BIOSCharacteristicsExtensionBytes[1]|0x0C
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType0StringVendor|"AMD Corporation"

  # SMBIOS Type 1 System Information Strings
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType1StringFamily|$(AMD_PROCESSOR)
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType1StringProductName|$(PLATFORM_CRB)
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType1SystemInformation.Uuid|{GUID("4462C5BB-B061-4771-85D3-674849AB82E0")}
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType1StringManufacturer|"AMD Corporation"
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType1StringSerialNumber|"To be filled by O.E.M."
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType1StringVersion|"To be filled by O.E.M."

  # SMBIOS Type 2 Baseboard Information Strings
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType2StringManufacturer|"AMD Corporation"
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType2StringProductName|$(PLATFORM_CRB)
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType2StringVersion|"To be filled by O.E.M."
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType2StringAssetTag|"To be filled by O.E.M."
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType2StringLocationInChassis|"To be filled by O.E.M."
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType2StringSerialNumber|"To be filled by O.E.M."

  # SMBIOS Type 3 System Enclosure Information Strings
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType3StringManufacturer|"AMD Corporation"
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType3StringVersion|"To be filled by O.E.M."
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType3StringAssetTag|"To be filled by O.E.M."
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType3StringSKUNumber|"To be filled by O.E.M."
  gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosType3StringSerialNumber|"To be filled by O.E.M."

  # SMBIOS Type 11 OEM Strings
  gAmdPlatformPkgTokenSpaceGuid.PcdType11OemStringsCount|1
  gAmdPlatformPkgTokenSpaceGuid.PcdType11OemStrings|{"To be filled by O.E.M."}

  # SMBIOS Type 12 System Configuration Options
  gAmdPlatformPkgTokenSpaceGuid.PcdType12SystemCfgOptionsCount|1
  gAmdPlatformPkgTokenSpaceGuid.PcdType12SystemCfgOptions|{"To be filled by O.E.M."}

[Components.X64]
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  SystemInformation/SmbiosFeaturePkg/SmbiosBasicDxe/SmbiosBasicDxe.inf
  AmdPlatformPkg/Universal/SmbiosCommonDxe/SmbiosCommonDxe.inf {
    <LibraryClasses>
      PciSegmentLib|MdePkg/Library/PciSegmentLibSegmentInfo/BasePciSegmentLibSegmentInfo.inf
      PciSegmentInfoLib|AgesaPkg/Addendum/PciSegments/PciExpressPciSegmentInfoLib/PciExpressPciSegmentInfoLib.inf
  }
