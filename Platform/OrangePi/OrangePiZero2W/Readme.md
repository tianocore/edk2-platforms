Orange Pi Zero 2W Platform
==========================

# Summary

This is a port of 64-bit TianoCore UEFI firmware for the Orange Pi Zero 2W
single-board computer, powered by the Allwinner H618 Quad-Core
ARM Cortex-A53 SoC.

This is meant as a generally useful 64-bit TF-A + UEFI implementation for
Allwinner H616/H618 based platforms, which should be good enough for most kinds
of UEFI development, as well as for running consumer Operating Systems such as
Linux (Ubuntu, Debian, Armbian).

# Hardware Supported

The OrangePiZero2W target supports boards based on the Allwinner H618 SoC:
- Orange Pi Zero 2W (1 GB / 1.5 GB / 2 GB / 4 GB LPDDR4 variants; configured
  for 1 GB DRAM baseline by default)
- Orange Pi Zero 3 (Allwinner H618, compatible memory and UART0 layout)
- Related Allwinner H616 / H618 boards sharing the `0x40000000` DRAM base and
  `0x05000000` UART0 console layout

# Status

This firmware, that has been validated to compile against the current
[edk2](https://github.com/tianocore/edk2)/[edk2-platforms](https://github.com/tianocore/edk2-platforms),
boots into the UEFI Interactive Shell and Boot Manager UI (`UiApp`), providing
full runtime and SMBIOS services.

It also provides support for ATF ([Arm Trusted Firmware](https://github.com/ARM-software/arm-trusted-firmware))
running at EL3 (`0x40000000 - 0x4007FFFF`).

The 16550-compatible UART0 serial port (`0x05000000`) is used as both the input
and output console device (`115200 8N1`).

Currently supported hardware and firmware features:
- **CPU & SMP**: 4x ARM Cortex-A53 cores (Cluster 0, MPIDR `0x0..0x3`) with PSCI
  SMC secondary core bring-up and system reset
- **Interrupt Controller**: ARM Generic Interrupt Controller v2 (GIC-400,
  Distributor at `0x03021000`, CPU Interface at `0x03022000`)
- **System Timer**: ARMv8 Generic Timer (24 MHz `osc24M`, PPI IRQs 29, 30, 27, 26)
- **Memory & MMU**: Full AArch64 MMU page tables covering 1 GiB DRAM
  (`0x40080000 - 0x7FFFFFFF`, `WRITE_BACK`), SRAM A1/C (`0x00020000 - 0x00057FFF`),
  SRAM A2 (`0x00100000 - 0x00117FFF`), Peripherals Low (`0x03000000 - 0x03FFFFFF`),
  and Peripherals High (`0x04000000 - 0x09FFFFFF`)
- **Serial Console**: Standard 16550 UART console (`BaseSerialPortLib16550`)
  mapped to UART0 (`0x05000000`, 32-bit MMIO register stride `4`, 24 MHz clock)
- **Security ID (SID / eFuse)**: Read access to the 512-byte eFuse array at
  `0x03006000` (`SID_PRCTL` / `SID_RDKEY`) via the `siddump` UEFI Shell command
- **Real-Time Clock (RTC)**: Allwinner H618 RTC controller support at
  `0x07000000` (`AllwinnerRtcLib`, 16-bit linear day + HH:MM:SS registers with
  ARM Generic Timer fallback)
- **SMBIOS 3.7 Tables**: Full Type 0, 1, 2, 3, 4, 7, and 32 SMBIOS tables via
  `OemMiscLib`, dynamically populating the system `SerialNumber` and `UUID` from
  the 128-bit H618 SID eFuse (`0x03006000`)
- **Arm SystemReady IR Alignment**: Designed for standard Device Tree based OS boot
  model (FDT handoff targeted for Phase 2)
- **UEFI Services**: FAT filesystem (`FatPkg`), Partition driver (`PartitionDxe`),
  BDS (`BdsDxe`), and UEFI Shell v2

# Hardware Description

The Orange Pi Zero 2W firmware adopts the **Arm SystemReady IR (IoT Ready)** model:

- **Device Tree (FDT / DTB) (Primary & Default)**: Operating systems (e.g. Linux)
  must be booted using a Flattened Device Tree (`sun50i-h618-orangepi-zero2w.dtb`).
  The upstream Linux Allwinner driver subsystem (`sunxi-ng` CCU clocks, reset,
  pinctrl, DMA, and MMC) is implemented entirely with Device Tree bindings.
- **ACPI**: Not provided. Mainline Linux kernel drivers for Allwinner H616/H618 SoCs
  do not support ACPI hardware description.

# Building

Build instructions from the top level edk2-platforms Readme.md apply.

# Booting the firmware

1. Format a uSD card with an SPL + TF-A (`bl31.bin`) + U-Boot bootloader for
   the Orange Pi Zero 2W (`sun50i_h616`), or use an existing Ubuntu/Armbian uSD
   image.
2. Copy the generated `ORANGEPI_ZERO2W.fd` firmware onto the boot partition
   (e.g. `/boot/ORANGEPI_ZERO2W.fd`).
3. Connect a 3.3V USB-to-TTL serial cable (`115200 8N1`) to the 40-pin header:
   - Pin 6: `GND`
   - Pin 8: `UART0_TX` (`PH0`)
   - Pin 10: `UART0_RX` (`PH1`)
4. Power up the board, interrupt U-Boot at the `=>` prompt, and load the UEFI
   firmware at `0x40080000`:
   ```
   => ext4load mmc 0:1 0x40080000 /boot/ORANGEPI_ZERO2W.fd
   => go 0x40080000
   ```
   If using a FAT32 boot partition, replace `ext4load` with `fatload`:
   ```
   => fatload mmc 0:1 0x40080000 ORANGEPI_ZERO2W.fd
   => go 0x40080000
   ```
5. To automate booting into UEFI without typing commands manually, compile a
   `boot.scr` U-Boot script containing the commands above and place it in
   `/boot/boot.scr`.

# Notes

## ARM Trusted Firmware (TF-A)

On Allwinner H616/H618 platforms (`PLAT=sun50i_h616`), TF-A `BL31` is loaded by
U-Boot SPL into the first 512 KiB of DRAM (`0x40000000 - 0x4007FFFF`) with
`SUNXI_BL31_IN_DRAM=1`.

The UEFI firmware image (`ORANGEPI_ZERO2W.fd`) is linked at `0x40080000` so that
TF-A `BL31` remains intact in EL3 throughout the lifetime of UEFI and the
operating system.

## Architecture & Upstream Roadmap

This port follows a phased bring-up strategy:
- **Phase 1 (Current Series)**: Core silicon bring-up (UART, GIC-400, Generic Timer,
  RTC, SMBIOS 3.7 tables with hardware UUID/Serial from SID eFuses, PSCI secondary
  core bring-up) chainloaded via U-Boot SPL / TF-A BL31 into UEFI Shell and UiApp.
- **Phase 2 (Upcoming)**: Native Allwinner SMHC0 SD/MMC host controller driver
  and FDT handoff driver to enable direct booting of Linux via the EFI Stub / GRUB.

# Limitations

## HDMI

HDMI video output via the Allwinner Display Engine 3.3 (DE33) is not yet
supported in UEFI. All UEFI interaction occurs over the `UART0` serial console.

## NVRAM

The Orange Pi Zero 2W has no dedicated UEFI NVRAM flash storage configured.

NVRAM is emulated in RAM (`PcdEmuVariableNvModeEnable = TRUE`). Changes made to
UEFI variables during runtime are maintained in memory until the system is
reset or powered off.

## RTC

`AllwinnerRtcLib` reads and writes the Allwinner H618 RTC calendar registers at
`0x07000000` (`LOSC_CTRL_REG`, `RTC_DAY_REG`, and `RTC_HH_MM_SS_REG`). Because
the Orange Pi Zero 2W does not populate a dedicated backup battery on `VCC_RTC`
by default, RTC values are not retained across full power removal unless an
external backup supply is connected.

## uSD and USB

Native UEFI Block I/O drivers for the Allwinner SMHC (`0x04020000`) SD/MMC
controllers and USB 2.0 EHCI/OHCI controllers (`0x05101000`..`0x05311400`) are
not yet included in this core bring-up series (targeted for Phase 2).

## ACPI

ACPI tables are not provided for the Allwinner H618 SoC. The Allwinner hardware
ecosystem in mainline Linux relies exclusively on Device Tree bindings for clocks,
power management, pin multiplexing, and peripheral IP controllers. Operating
systems must be booted using a Flattened Device Tree (`.dtb`).

## Missing Functionality

- SD/MMC (`SMHC0..SMHC2`) Block I/O driver in UEFI
- USB 2.0 Host / OTG controller drivers
- HDMI GOP display driver
- On-chip Audio Codec / I2S / OWA drivers
- GMAC Ethernet / SDIO Wi-Fi network booting in UEFI

# Configuration Settings

Platform hardware parameters are configured via FixedAtBuild PCDs in
`OrangePiZero2W.dsc` and `AllwinnerH618Pkg.dec`, and runtime inspection/boot
control is provided through custom UEFI Shell commands in
`H618ShellCommandsLib`.

UEFI Setting / Parameter     |PCD / Command               |VALUE / Default
-----------------------------|----------------------------|-----------------------------
**CPU & SMP Configuration**  |                            |
CPU Core Count               | `PcdCoreCount`             | `4` (4x Cortex-A53, Cluster 0)
Cluster Count                | `PcdClusterCount`          | `1`
PSCI Conduit                 | `PcdMonitorConduitHvc`     | `FALSE` (`SMC` to TF-A EL3)
**Memory & Flash Layout**    |                            |
Firmware FD Base Address     | `PcdFdBaseAddress`         | `0x40080000` (after 512 KiB TF-A)
Firmware FD Size             | `PcdFdSize`                | `0x00400000` (4 MiB, 1024 blocks x 4 KiB)
System DRAM Base             | `PcdSystemMemoryBase`      | `0x40080000`
System DRAM Size             | `PcdSystemMemorySize`      | `0x3FF80000` (~1023.5 MiB)
**Interrupt & Timer Config** |                            |
GICv2 Distributor Base       | `PcdGicDistributorBase`    | `0x03021000`
GICv2 CPU Interface Base     | `PcdGicInterruptInterfaceBase` | `0x03022000`
Generic Timer IRQs (PPI)     | `PcdArmArchTimer*IntrNum`  | Sec = `29`, Non-Sec = `30`<br>Virt = `27`, Hyp = `26`
**Serial / UART0 Config**    |                            |
UART Base Address            | `PcdSerialRegisterBase`    | `0x05000000` (`UART0`, MMIO)
UART Input Clock Rate        | `PcdSerialClockRate`       | `24000000` (24 MHz `APB2_CLK`)
UART Register Stride         | `PcdSerialRegisterStride`  | `4` (32-bit MMIO shift 2)
UART Baud Rate               | `PcdSerialBaudRate`        | `115200` (`8N1`, no flow control)
**RTC & SMBIOS Config**      |                            |
RTC Base Address             | `PcdAllwinnerRtcBaseAddress` | `0x07000000` (`AllwinnerRtcLib`)
SMBIOS Version               | `PcdSmbiosVersion`         | `0x0307` (SMBIOS 3.7, SID eFuse UUID/Serial)
**UEFI Shell Commands**      |                            |
SID eFuse Dump               | `siddump [Offset] [Bytes]` | Default `Offset = 0x0`, `Bytes = 0x10`<br>Max range: `0x000..0x1FF` (512 bytes)
SMBIOS Table Viewer          | `smbiosview [-t Type]`     | Inspect Type 0, 1, 2, 3, 4, 7, 32 tables

**Examples (in the UEFI Shell):**

- To dump the first 16 bytes (`0x10`) of the Allwinner H618 Security ID (SID)
  eFuse controller at `0x03006000`:
```
Shell> siddump
```

- To read 16 bytes (`0x10`) of SID eFuse starting at offset `0x10`:
```
Shell> siddump 0x10 0x10
```

- To inspect the SMBIOS System Information (Type 1) and Processor Information
  (Type 4) tables:
```
Shell> smbiosview -t 1
Shell> smbiosview -t 4
```
