Marvell CN913x Development Board
================================

# Summary

This is a port of 64-bit TianoCore EDK II firmware for the Marvell CN913x Development Board.

# Supported features

Features supported in EDK2:

* 1x PCIE root complex
* Networking:
  * 3x 10 GbE via SFP+
  * 2x 1 GbE RGMII via RJ45
* 5x USB 2.0/3.0
* 3x SATA
* 2x uSD
* 1x eMMC
* RTC
* SPI flash & memory-mapped variable storage access
* I2C
* GPIO

Hardware description:

* ACPI (default)
* Device Tree

Others:

* Signed capsule update
* X64 option ROM emulator

# Building the firmware

## Prepare EDKII environment:

Please follow instructions from [Obtaining source code](https://github.com/tianocore/edk2-platforms#obtaining-source-code)
and [Manual building](https://github.com/tianocore/edk2-platforms#manual-building) from the
top level edk2-platforms [Readme.md](https://github.com/tianocore/edk2-platforms#readme).

## Build EDKII:

Use below build command:

  ```
  $ build -a AARCH64 -t GCC5 -b RELEASE -D CN9132 -D X64EMU_ENABLE -p Platform/Marvell/Cn913xDb/Cn913xDbA.dsc
  ```

---
**NOTE**

'-D INCLUDE_TFTP_COMMAND' is optional and can be added in order to enable `tftp` command in UEFI Shell.

---

## Build the final firmware image:

In addition to EDKII binary, the complete firmware image comprises the TF-A and other components.
A complete build instruction can be found at [wiki page](https://github.com/Semihalf/edk2-platforms/wiki/Build_firmware).

## Burning the firmware

Please follow instruction at [wiki page](https://github.com/Semihalf/edk2-platforms/wiki/Burning_firmware)
to burn image to desired boot device.

# ARM System Ready certification.

CN913x Development Board is [System Ready ES](https://developer.arm.com/architectures/system-architectures/arm-systemready/es) certified. Release binary and the firmware components' baselines list are available in a dedicated [wiki page](https://github.com/semihalf/edk2-platforms/wiki).
