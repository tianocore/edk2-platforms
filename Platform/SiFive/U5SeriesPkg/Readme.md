# Introduction
U5SeriesPkg provides the common EFI library and driver modules for SiFive
U5 series core platforms. Currently the supported platforms are Freedom
U500 VC707 platform and Freedom U540 HiFive Unleashed platform.

Both platforms are built with below common packages,
- **U5SeriesPkg**, edk2 platform branch
  (Currently is in edk2-platforms/devel-riscv-v2-PATCHv5 branch)
- **RiscVPlatformPkg**, edk2 master branch
  (Currently is in edk2-staging/RISC-V-V2 branch)
- **RiscVPkg**, edk2 master branch
  (Currently is in edk2-staging/RISC-V-V2 branch)

## U500 Platform
This is a sample RISC-V EDK2 platform package used agaist SiFive Freedom U500
VC707 FPGA Dev Kit, please refer to "SiFive Freedom U500 VC707 FPGA Getting
Started Guide" on https://www.sifive.com/documentation.

## U540 Platform
This is a sample RISC-V EDK2 platform package used agaist SiFive Freedom U540
HiFive Unleashed development board, please refer to "SiFive Freedom U540-C000
Manual" on https://www.sifive.com.
The binary built from Platform/SiFive/FreedomU540HiFiveUnleashedBoard can run
on main stream [QEMU](https://git.qemu.org/?p=qemu.git;a=summary)
using qemu-system-riscv64 under riscv64-softmmu. Launch the binary with

```
-cpu sifive-u54 -machine sifive_u
```

## Download the sources
```
git clone https://github.com/tianocore/edk2-staging.git
# Checkout RISC-V-V2 branch
git clone https://github.com/tianocore/edk2-platforms.git
# Checkout devel-riscv-v2-PATCHv5 branch
git clone https://github.com/tianocore/edk2-non-osi.git
```

## Platform Owners
Chang, Abner <abner.chang@hpe.com>
Chen, Gilbert <gilbert.chen@hpe.com>

## Platform Status
Currently the binary built from U500Pkg can boot SiFive Freedom U500 VC707
FPGA to EFI shell with console in/out enabled.

## Linux Build Instructions
You can build the RISC-V platform using below script,
`build -a RISCV64  -t GCC5 -p Platform/SiFive/FreedomU500VC707Board/U500.dsc`

## Supported Operating Systems
Only support to boot to EFI Shell so far.

## Known Issues and Limitations
Only RISC-V RV64 is verified on this platform.

## Related Materials
- [RISC-V OpenSbi](https://github.com/riscv/opensbi)
- [SiFive U500 VC707 FPGA Getting Started Guide](https://sifive.cdn.prismic.io/sifive%2Fc248fabc-5e44-4412-b1c3-6bb6aac73a2c_sifive-u500-vc707-gettingstarted-v0.2.pdf)
- [SiFive Freedom U540-C000 Manual](https://sifive.cdn.prismic.io/sifive%2F834354f0-08e6-423c-bf1f-0cb58ef14061_fu540-c000-v1.0.pdf)
- [SiFive RISC-V Core Document](https://www.sifive.com/documentation)

## U5SeriesPkg Libraries and Drivers
### PeiCoreInfoHobLib
This is the library to create RISC-V core characteristics for building up
RISC-V related SMBIOS records to support the unified boot loader and OS image.
This library leverage the silicon libraries provided in Silicon/SiFive.

### RiscVPlatformTimerLib
This is common U5 series  platform timer library which has the
platform-specific timer implementation.

### TimerDxe
This is U5 series  platform timer DXE driver whcih has the platform-specific
timer implementation.

## U500 Platform Libraries and Drivers
### RiscVOpnesbiPlatformLib
In order to reduce the dependencies with RISC-V OpenSBI project
(https://github.com/riscv/opensbi) and less burdens to EDK2 build process, the
implementation of RISC-V EDK2 platform is leverage platform source code from
OpenSBI code tree. The "platform.c" under OpenSbiPlatformLib is cloned from
RISC-V OpenSBI code tree (in EDK2 RiscVPkg) and built based on EDK2 build
environment.

### PlatformPei
This is the platform-implementation specific library which is executed in early
PEI phase for U500 platform initialization.

## U540 Platform Libraries and Drivers
### RiscVOpnesbiPlatformLib
In order to reduce the dependencies with RISC-V OpenSBI project
(https://github.com/riscv/opensbi) and less burdens to EDK2 build process, the
implementation of RISC-V EDK2 platform is leverage platform source code from
OpenSBI code tree. The "platform.c" under OpenSbiPlatformLib is cloned from
RISC-V OpenSBI code tree (in EDK2 RiscVPkg) and built based on EDK2 build
environment.

### PlatformPei
This is the platform-implementation specific library which is executed in early
PEI phase for U540 platform initialization.

## U5SeriesPkg Platform PCD settings

| **PCD name** |**Usage**|
|----------------|----------|
|PcdNumberofU5Cores| Number of U5 core enabled on U5 series platform|
|PcdE5MCSupported| Indicates whether the Monitor Core (E5) is supported on U5 series platform|
|PcdU5UartBase|Platform serial port base address|
