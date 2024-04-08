## @file
#  Definitions of Flash definition file on Bosc NanHuDev RISC-V platform
#
#  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
#  Copyright (c) 2024, Bosc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
DEFINE BLOCK_SIZE        = 0x1000

DEFINE FW_BASE_ADDRESS   = 0x80200000
DEFINE FW_SIZE           = 0x00800000
DEFINE FW_BLOCKS         = 0x800

#
# 0x000000-0x7DFFFF code
# 0x7E0000-0x800000 variables
#
DEFINE CODE_BASE_ADDRESS = $(FW_BASE_ADDRESS)
DEFINE CODE_SIZE         = 0x00780000
DEFINE CODE_BLOCKS       = 0x780
DEFINE VARS_BLOCKS       = 0x20

#
# Other FV regions are in the second FW domain.
# The size of memory region must be power of 2.
# The base address must be aligned with the size.
#
# FW memory region
#
DEFINE FVMAIN_OFFSET                 = 0x00000000
DEFINE FVMAIN_SIZE                   = 0x00780000

#
# EFI Variable memory region.
# The total size of EFI Variable FD must include
# all of sub regions of EFI Variable
#
DEFINE VARS_OFFSET                   = 0x00780000
DEFINE VARS_SIZE                     = 0x00007000
DEFINE VARS_FTW_WORKING_OFFSET       = $(VARS_OFFSET) + $(VARS_SIZE)
DEFINE VARS_FTW_WORKING_SIZE         = 0x00001000
DEFINE VARS_FTW_SPARE_OFFSET         = $(VARS_FTW_WORKING_OFFSET) + $(VARS_FTW_WORKING_SIZE)
DEFINE VARS_FTW_SPARE_SIZE           = 0x00018000

DEFINE VARIABLE_FW_SIZE  = $(VARS_FTW_SPARE_OFFSET) + $(VARS_FTW_SPARE_SIZE) - $(VARS_OFFSET)

SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFdBaseAddress             = $(FW_BASE_ADDRESS) + $(VARS_OFFSET)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFdSize                    = $(VARS_SIZE) + $(VARS_FTW_WORKING_SIZE) + $(VARS_FTW_SPARE_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFdBlockSize               = $(BLOCK_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFirmwareRegionBaseAddress = $(CODE_BASE_ADDRESS) + $(VARS_OFFSET)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFirmwareRegionSize        = $(VARIABLE_FW_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamBase                  = $(CODE_BASE_ADDRESS) + $(FW_SIZE) + 0x1FF0000
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamSize                  = 0x10000

SET gUefiCpuPkgTokenSpaceGuid.PcdCpuCoreCrystalClockFrequency         = 500000 # Adapted CPU clock is 50MHz
SET gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase              = 0x310B0000
SET gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate                   = 115200
SET gHisiTokenSpaceGuid.PcdSerialPortSendDelay                        = 50
SET gHisiTokenSpaceGuid.PcdUartClkInHz                                = 50000000
