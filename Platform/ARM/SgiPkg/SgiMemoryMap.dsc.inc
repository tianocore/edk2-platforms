#
#  Copyright (c) 2020 - 2025, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

[PcdsFixedAtBuild.common]
  # System Peripherals
  gArmSgiTokenSpaceGuid.PcdSmcCs0Base|0x08000000
  gArmSgiTokenSpaceGuid.PcdSmcCs1Base|0x0C000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphBase|0x1C000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphSysRegBase|0x1C010000

  # Non-Volatile variable storage
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0x0C000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0x0D400000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0x0E800000

  # PL011 - Serial Terminal
  gArmPlatformTokenSpaceGuid.PL011UartInterrupt|95

  # PL370 - HDLCD1
  gArmPlatformTokenSpaceGuid.PcdArmHdLcdBase|0x7FF60000

  # PL011 - Serial Debug UART
  gArmPlatformTokenSpaceGuid.PcdSerialDbgRegisterBase|0x7FF70000
  gArmPlatformTokenSpaceGuid.PcdSerialDbgInterrupt|147

  # PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x1C170000

  # Virtio Disk
  gArmSgiTokenSpaceGuid.PcdVirtioBlkBaseAddress|0x1C130000
  gArmSgiTokenSpaceGuid.PcdVirtioBlkSize|0x10000
  gArmSgiTokenSpaceGuid.PcdVirtioBlkInterrupt|202

  # Ethernet
  gArmSgiTokenSpaceGuid.PcdVirtioNetBaseAddress|0x1C150000
  gArmSgiTokenSpaceGuid.PcdVirtioNetInterrupt|204

  # PCIe
  gArmTokenSpaceGuid.PcdPciMmio32Base|0x70000000
  gArmTokenSpaceGuid.PcdPciMmio32Size|0x07800000
  gArmTokenSpaceGuid.PcdPciMmio64Base|0x5000000000
  gArmTokenSpaceGuid.PcdPciMmio64Size|0x3000000000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x60000000

  # Address bus width - 4TB address space
  gArmSgiTokenSpaceGuid.PcdMaxAddressBitsPerChip|42

  # Timer & Watchdog interrupts
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Gsiv|92
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Gsiv|91
  gArmSgiTokenSpaceGuid.PcdWdogWS0Gsiv|93
  gArmSgiTokenSpaceGuid.PcdWdogWS1Gsiv|94

  # Generic Timer
  gArmSgiTokenSpaceGuid.PcdTimerControlBase|0x2A810000
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Base|0x2A830000
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Base|0x2A820000

  # SP804 dual timer
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerBaseAddress|0x1C110000
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerSize|0x00010000
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerInterrupt|230

  # SMMU
  gArmSgiTokenSpaceGuid.PcdSmmuBase|0x4F000000
  gArmSgiTokenSpaceGuid.PcdSmmuSize|0x01000000

  # GPIO Controller
  gArmSgiTokenSpaceGuid.PcdGpioController0BaseAddress|0x1C1D0000
  gArmSgiTokenSpaceGuid.PcdGpioController0Size|0x00010000
  gArmSgiTokenSpaceGuid.PcdGpioController0Interrupt|136

  # System Memory (2GB - 16MB of Trusted DRAM at the top of the
  # 32bit address space) and Normal shared area with
  # Secure world is reserved 1MB from (2GB - 16MB).
  # Therefore, set the default System Memory size to (2GB - 17MB)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x7EF00000

  #Second DRAM Block Addresses
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip0|0x08080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip1|0x48080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip2|0x88080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip3|0xC8080000000
