## @file
#  DSC file for advanced features.
#
#  This file is intended to be included into another package so advanced features
#  can be conditionally built by enabling the respective feature via its FeaturePCD.
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

#
# Debugging features
#
!if gAcpiDebugFeaturePkgTokenSpaceGuid.PcdAcpiDebugFeatureEnable == TRUE
  !include AcpiDebugFeaturePkg/Include/AcpiDebugFeature.dsc
!endif

!if gBeepDebugFeaturePkgTokenSpaceGuid.PcdBeepDebugFeatureEnable == TRUE
  !include BeepDebugFeaturePkg/Include/BeepDebugFeature.dsc
!endif

!if gPostCodeDebugFeaturePkgTokenSpaceGuid.PcdPostCodeDebugFeatureEnable == TRUE
  !include PostCodeDebugFeaturePkg/Include/PostCodeDebugFeature.dsc
!endif

!if gUsb3DebugFeaturePkgTokenSpaceGuid.PcdUsb3DebugFeatureEnable == TRUE
  !include Usb3DebugFeaturePkg/Include/Usb3DebugFeature.dsc
!endif

#
# Network features
#
!if gNetworkFeaturePkgTokenSpaceGuid.PcdNetworkFeatureEnable == TRUE
  !include NetworkFeaturePkg/Include/NetworkFeature.dsc
!endif

#
# OutOfBandManagement features
#
!if gIpmiFeaturePkgTokenSpaceGuid.PcdIpmiFeatureEnable == TRUE
  !include IpmiFeaturePkg/Include/IpmiFeature.dsc
!endif

!if gSpcrFeaturePkgTokenSpaceGuid.PcdSpcrFeatureEnable == TRUE
  !include SpcrFeaturePkg/Include/SpcrFeature.dsc
!endif

#
# PowerManagement features
#
!if gS3FeaturePkgTokenSpaceGuid.PcdS3FeatureEnable == TRUE
  !include S3FeaturePkg/Include/S3Feature.dsc
!endif

#
# SystemInformation features
#
!if gSmbiosFeaturePkgTokenSpaceGuid.PcdSmbiosFeatureEnable == TRUE
  !include SmbiosFeaturePkg/Include/SmbiosFeature.dsc
!endif

#
# UserInterface features
#
!if gLogoFeaturePkgTokenSpaceGuid.PcdLogoFeatureEnable == TRUE
  !include LogoFeaturePkg/Include/LogoFeature.dsc
!endif

!if gUserAuthFeaturePkgTokenSpaceGuid.PcdUserAuthenticationFeatureEnable == TRUE
  !include UserAuthFeaturePkg/Include/UserAuthFeature.dsc
!endif

!if gVirtualKeyboardFeaturePkgTokenSpaceGuid.PcdVirtualKeyboardFeatureEnable == TRUE
  !include VirtualKeyboardFeaturePkg/Include/VirtualKeyboardFeature.dsc
!endif

#
# Individual features
#
!if gPlatformPayloadPkgTokenSpaceGuid.PcdPayloadPackageFeatureEnable == TRUE
  !include PlatformPayloadPkg/Include/PlatformPayloadFeature.dsc
!endif
