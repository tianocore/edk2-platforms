#/** @file
# FmpDxe driver for AArch64 firmware update.
#
#  Copyright (c) 2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#
#**/

  INF MdeModulePkg/Universal/EsrtDxe/EsrtDxe.inf
  INF MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf
  INF FILE_GUID = $(FMP_SYSTEM_FIP_IMAGE_GUID) FmpDevicePkg/FmpDxe/FmpDxeRuntime.inf
