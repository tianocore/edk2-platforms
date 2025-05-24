#
#  Copyright (c) 2024 - 2025, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

[PcdsFixedAtBuild.common]
  # System Peripherals
  gArmSgiTokenSpaceGuid.PcdSmcCs0Base|0x08000000
  gArmSgiTokenSpaceGuid.PcdSmcCs1Base|0x0600000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphBase|0x0C000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphSysRegBase|0x0C010000

  # SP804 dual timer
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerBaseAddress|0x0C110000
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerSize|0x00010000
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerInterrupt|216

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
  gArmPlatformTokenSpaceGuid.PL011UartInterrupt|112

  # System Memory (2GB - 128MB of Trusted DRAM at the top of the 32bit address space)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x78000000

  #Second DRAM Block Addresses
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip0|0x008080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip1|0x108080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip2|0x208080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip3|0x308080000000

  # SMMU
  gArmSgiTokenSpaceGuid.PcdSmmuBase|0x280000000
  gArmSgiTokenSpaceGuid.PcdSmmuSize|0x4000000

  # Non-Volatile variable storage
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0x0600000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0x0601400000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0x0602800000

  # Address bus width - 64GB address space
  gArmSgiTokenSpaceGuid.PcdMaxAddressBitsPerChip|36

  # Timer & Watchdog interrupts
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Gsiv|109
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Gsiv|108
  gArmSgiTokenSpaceGuid.PcdWdogWS0Gsiv|110
  gArmSgiTokenSpaceGuid.PcdWdogWS1Gsiv|111

  # Generic Timer
  gArmSgiTokenSpaceGuid.PcdTimerControlBase|0x2A810000
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Base|0x2A830000
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Base|0x2A820000
