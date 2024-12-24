#
#  Copyright (c) 2025, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

[PcdsFixedAtBuild.common]
  # System Peripherals
  gArmSgiTokenSpaceGuid.PcdSmcCs0Base|0x08000000
  gArmSgiTokenSpaceGuid.PcdSmcCs1Base|0x0650000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphBase|0x0C000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphSysRegBase|0x0C010000

  # Virtio Disk
  gArmSgiTokenSpaceGuid.PcdVirtioBlkBaseAddress|0x0C130000
  gArmSgiTokenSpaceGuid.PcdVirtioBlkSize|0x10000
  gArmSgiTokenSpaceGuid.PcdVirtioBlkInterrupt|184

  # GPIO controller
  gArmSgiTokenSpaceGuid.PcdGpioController0BaseAddress|0x0C1D0000
  gArmSgiTokenSpaceGuid.PcdGpioController0Size|0x00010000
  gArmSgiTokenSpaceGuid.PcdGpioController0Interrupt|168

   # Ethernet
  gArmSgiTokenSpaceGuid.PcdVirtioNetBaseAddress|0x0C150000
  gArmSgiTokenSpaceGuid.PcdVirtioNetInterrupt|186

  # PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x0C170000

  # Virtio P9
  gArmSgiTokenSpaceGuid.PcdVirtioP9BaseAddress|0x0C190000
  gArmSgiTokenSpaceGuid.PcdVirtioP9Size|0x10000
  gArmSgiTokenSpaceGuid.PcdVirtioP9Interrupt|185

  # PL370 - HDLCD1
  gArmPlatformTokenSpaceGuid.PcdArmHdLcdBase|0x0EF60000

  # PL011 - Serial Debug UART
  gArmPlatformTokenSpaceGuid.PcdSerialDbgRegisterBase|0x0EF70000
  gArmPlatformTokenSpaceGuid.PcdSerialDbgInterrupt|179

  # PL011 - Serial Terminal
  gArmPlatformTokenSpaceGuid.PL011UartInterrupt|103

  # System Memory (first DRAM block). The upper 128MB of this memory region is
  # carved out as trusted DRAM. So the size of this DRAM region is (2GB - 128MB)
  # which is 0x73000000.
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x73000000
  # DRAM Block2 Base and Size
  gArmSgiTokenSpaceGuid.PcdDramBlock2Base|0x20080000000
!ifndef $(PLAT_DRAM2_SIZE)
  gArmSgiTokenSpaceGuid.PcdDramBlock2Size|0x180000000
!else
  gArmSgiTokenSpaceGuid.PcdDramBlock2Size|$(PLAT_DRAM2_SIZE)
!endif

  # SMMU
  gArmSgiTokenSpaceGuid.PcdSmmuBase|0x1C0000000
  gArmSgiTokenSpaceGuid.PcdSmmuSize|0x4000000

  # Non-Volatile variable storage
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0x0650000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0x0651400000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0x0652800000

  # PCIe
  gArmTokenSpaceGuid.PcdPciMmio32Base|0x60000000
  gArmTokenSpaceGuid.PcdPciMmio32Size|0x10000000
  gArmTokenSpaceGuid.PcdPciMmio64Base|0x10100000000
  gArmTokenSpaceGuid.PcdPciMmio64Size|0x1000000000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x10000000000

  ## SBSA Watchdog Count
  gArmPlatformTokenSpaceGuid.PcdWatchdogCount|1
  gArmTokenSpaceGuid.PcdGenericWatchdogControlBase|0x1a440000
  gArmTokenSpaceGuid.PcdGenericWatchdogRefreshBase|0x1a450000

  ## PL011 - Serial Terminal
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x1A400000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultReceiveFifoDepth|0
  gArmPlatformTokenSpaceGuid.PL011UartInteger|4
  gArmPlatformTokenSpaceGuid.PL011UartFractional|0

  # Counter, Timer and Watchdog
  gArmSgiTokenSpaceGuid.PcdTimerControlBase|0x1A810000
  gArmSgiTokenSpaceGuid.PcdTimerControlSize|0x00010000
  gArmSgiTokenSpaceGuid.PcdTimerCounterReadBase|0x1A800000
  gArmSgiTokenSpaceGuid.PcdTimerCounterReadSize|0x00010000
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Base|0x1A830000
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Base|0x1A820000
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Size|0x00010000
  gArmSgiTokenSpaceGuid.PcdWdogBase|0x1A440000
  gArmSgiTokenSpaceGuid.PcdWdogSize|0x00020000

  # Address bus width - 64GB address space
  gArmSgiTokenSpaceGuid.PcdMaxAddressBitsPerChip|36

  # Timer & Watchdog interrupts
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Gsiv|99
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Gsiv|100
  gArmSgiTokenSpaceGuid.PcdWdogWS0Gsiv|101
  gArmSgiTokenSpaceGuid.PcdWdogWS1Gsiv|102

 #Second DRAM Block Addresses
 gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip0|0x20080000000
 gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip1|0x30080000000
 gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip2|0x40080000000
 gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip3|0x50080000000
