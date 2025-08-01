#
#  Copyright (c) 2020 - 2025, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

[PcdsFixedAtBuild.common]
  # System Peripherals
  gArmSgiTokenSpaceGuid.PcdSmcCs0Base|0x08000000
  gArmSgiTokenSpaceGuid.PcdSmcCs1Base|0x1050000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphBase|0x0C000000
  gArmSgiTokenSpaceGuid.PcdSysPeriphSysRegBase|0x0C010000

  # Non-Volatile variable storage
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0x1050000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0x1051400000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0x1052800000

  # PL011 - Serial Terminal
  gArmPlatformTokenSpaceGuid.PL011UartInterrupt|112

  # PL370 - HDLCD1
  gArmPlatformTokenSpaceGuid.PcdArmHdLcdBase|0x0EF60000

  # PL011 - Serial Debug UART
  gArmPlatformTokenSpaceGuid.PcdSerialDbgRegisterBase|0x0EF70000
  gArmPlatformTokenSpaceGuid.PcdSerialDbgInterrupt|403

  # PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x0C170000

  # Virtio Disk
  gArmSgiTokenSpaceGuid.PcdVirtioBlkBaseAddress|0x0C130000
  gArmSgiTokenSpaceGuid.PcdVirtioBlkSize|0x10000
  gArmSgiTokenSpaceGuid.PcdVirtioBlkInterrupt|458

  # Ethernet
  gArmSgiTokenSpaceGuid.PcdVirtioNetBaseAddress|0x0C150000
  gArmSgiTokenSpaceGuid.PcdVirtioNetInterrupt|460

  # Virtio P9
  gArmSgiTokenSpaceGuid.PcdVirtioP9BaseAddress|0x0C190000
  gArmSgiTokenSpaceGuid.PcdVirtioP9Size|0x10000
  gArmSgiTokenSpaceGuid.PcdVirtioP9Interrupt|459

  # PCIe
  gArmTokenSpaceGuid.PcdPciMmio32Base|0x60000000
  gArmTokenSpaceGuid.PcdPciMmio32Size|0x10000000
  gArmTokenSpaceGuid.PcdPciMmio64Base|0x4000000000
  gArmTokenSpaceGuid.PcdPciMmio64Size|0x4000000000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x1010000000

  # Address bus width - 64TB address space
  gArmSgiTokenSpaceGuid.PcdMaxAddressBitsPerChip|46

  # Timer & Watchdog interrupts
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Gsiv|109
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Gsiv|108
  gArmSgiTokenSpaceGuid.PcdWdogWS0Gsiv|110
  gArmSgiTokenSpaceGuid.PcdWdogWS1Gsiv|111

  # Generic Timer
  gArmSgiTokenSpaceGuid.PcdTimerControlBase|0x2A810000
  gArmSgiTokenSpaceGuid.PcdTimerFrame0Base|0x2A830000
  gArmSgiTokenSpaceGuid.PcdTimerFrame1Base|0x2A820000

  # SP804 dual timer
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerBaseAddress|0x0C110000
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerSize|0x00010000
  gArmSgiTokenSpaceGuid.PcdSp804DualTimerInterrupt|486

  # SMMU
  gArmSgiTokenSpaceGuid.PcdSmmuBase|0x40000000
  gArmSgiTokenSpaceGuid.PcdSmmuSize|0x10000000

  # GPIO controller
  gArmSgiTokenSpaceGuid.PcdGpioController0BaseAddress|0x0C1D0000
  gArmSgiTokenSpaceGuid.PcdGpioController0Size|0x00010000
  gArmSgiTokenSpaceGuid.PcdGpioController0Interrupt|392

  # IO virtualization block
  gArmSgiTokenSpaceGuid.PcdIoVirtSocExpBlk0Base|0x1080000000

  # System Memory (2GB - 16MB of Trusted DRAM at the top of the
  # 32bit address space) and Normal shared area with
  # Secure world is reserved 1MB from (2GB - 16MB).
  # Therefore, set the default System Memory size to (2GB - 17MB)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x7EF00000

  #Second DRAM Block Addresses
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip0|0x008080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip1|0x408080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip2|0x808080000000
  gArmSgiTokenSpaceGuid.PcdDramBlock2AddrChip3|0xC08080000000
