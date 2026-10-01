## @file
#  FmpDxe driver for system firmware update.
#
#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

  INF MdeModulePkg/Universal/EsrtFmpDxe/EsrtFmpDxe.inf
  INF MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf
  INF FILE_GUID = $(FMP_SYSTEM_DEVICE) FmpDevicePkg/FmpDxe/FmpDxe.inf