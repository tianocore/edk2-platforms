## @file
#
#  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses.common.SEC]

[LibraryClasses.Common.PEIM]
  ## APCB
  ApcbLibV3Pei|AgesaModulePkg/Library/ApcbLibV3Pei/ApcbLibV3Pei.inf

[LibraryClasses.Common.DXE_DRIVER]
  AmdPspRomArmorLib|AgesaModulePkg/Library/AmdPspRomArmorLibNull/AmdPspRomArmorLibNull.inf
  ApcbLibV3|AgesaModulePkg/Library/ApcbLibV3/ApcbLibV3.inf

[LibraryClasses.Common.DXE_SMM_DRIVER]
  AmdPspRomArmorLib|AgesaModulePkg/Library/AmdPspRomArmorLibNull/AmdPspRomArmorLibNull.inf
  ApcbLibV3|AgesaModulePkg/Library/ApcbLibV3/ApcbLibV3.inf

[LibraryClasses]
  #
  # Agesa specific common libraries
  #

  ## PSP Libs
  AmdPspBaseLibV2|AgesaModulePkg/Library/AmdPspBaseLibV2/AmdPspBaseLibV2.inf
  AmdPspMboxLibV2|AgesaModulePkg/Library/AmdPspMboxLibV2/AmdPspMboxLibV2.inf

  ## DF Lib
  BaseFabricTopologyLib|AgesaModulePkg/Library/BaseFabricTopologyBrhLib/BaseFabricTopologyBrhLib.inf

  ## Fch Lib
  FchBaseLib|AgesaModulePkg/Library/FchBaseLib/FchBaseLib.inf

  NbioHandleLib|AgesaModulePkg/Library/NbioHandleLib/NbioHandleLib.inf
  PcieConfigLib|AgesaModulePkg/Library/PcieConfigLib/PcieConfigLib.inf
  SmnAccessLib|AgesaModulePkg/Library/SmnAccessLib/SmnAccessLib.inf
  NbioCommonDxeLib|AgesaModulePkg/Nbio/Library/CommonDxe/NbioCommonDxeLib.inf

[Components.IA32]
  AgesaModulePkg/Psp/ApcbDrv/ApcbV3Pei/ApcbV3Pei.inf

[Components.X64]
  AgesaModulePkg/Psp/ApcbDrv/ApcbV3Dxe/ApcbV3Dxe.inf

