# Introduction to SiFive U5 Series Platforms

**U5SeriesPkg** provides a set of common EDK2 libraries and drivers for SiFive U5 series platforms.

---

## U540 Platform

This package includes a reference platform for the **SiFive Freedom U540 HiFive Unleashed** development board.

For detailed hardware documentation, see the [SiFive Freedom U540-C000 Manual](https://www.sifive.com).

The firmware binary built from
`Platform/SiFive/U5SeriesPkg/FreedomU540HiFiveUnleashedBoard/`
can run on:

- Physical HiFive Unleashed hardware
- [QEMU `sifive_u` machine](https://git.qemu.org/?p=qemu.git;a=summary)

**Note:** EDK2 currently lacks a storage driver for U540 platforms. Therefore, only the QEMU `sifive_u` machine is supported for booting Linux with RAM disk support.

---

## Getting the EDK2 Sources

```
git clone https://github.com/tianocore/edk2.git
git submodule update --init
cd edk2
git clone https://github.com/tianocore/edk2-platforms.git
```

## Building the U540 Platform

Prerequisite: Install a RISC-V 64-bit GNU toolchain (e.g., riscv64-linux-gnu-gcc).


```
export WORKSPACE=$(pwd)
export GCC5_RISCV64_PREFIX=riscv64-linux-gnu-
export PACKAGES_PATH=$WORKSPACE:$WORKSPACE/edk2-platforms
export EDK_TOOLS_PATH=$WORKSPACE/BaseTools

# Set up the build environment
source edksetup.sh --reconfig
make -C BaseTools clean
make -C BaseTools
source edksetup.sh BaseTools

# Clean and build
build -a RISCV64 --buildtarget RELEASE \
  -p Platform/SiFive/U5SeriesPkg/FreedomU540HiFiveUnleashedBoard/U540.dsc \
  -t GCC5 cleanall

build -a RISCV64 --buildtarget RELEASE \
  -p Platform/SiFive/U5SeriesPkg/FreedomU540HiFiveUnleashedBoard/U540.dsc \
  -t GCC5
```

After the build completes, the firmware image U540.fd will be created.

## Testing the Firmware
### Step 1: Create a RAM Disk
```
dd if=/dev/zero of=fat32.img bs=1M count=32
mkfs.vfat fat32.img
mkdir rootfs
sudo mount -o loop fat32.img rootfs
```

Copy your Linux kernel (Image) and root filesystem (e.g., rootfs.cpio) into the mounted directory:
```
sudo cp Image rootfs/
sudo cp rootfs.cpio rootfs/
sudo umount rootfs
```

### Step 2: Run QEMU
```
qemu-system-riscv64 -M sifive_u -smp 5 -m 2G \
  -display none -serial stdio \
  -device loader,file=/work/acpi_next/tmp/fat32.img,addr=0x90000000 \
  -kernel U540.fd
```

Note: The address 0x90000000 must match the value of PcdFixedRamdiskBase.

### Step 3: Boot Linux from EFI Shell

At the EFI shell prompt:
```
FS0:\Image root=/dev/ram initrd=\rootfs.cpio
```

## Testing with a Custom OpenSBI Binary
```
qemu-system-riscv64 -M sifive_u -smp 5 -m 2G \
  -display none -serial stdio \
  -device loader,file=/work/acpi_next/tmp/fat32.img,addr=0x90000000 \
  -bios fw_dynamic.bin \
  -kernel U540.fd
```

## Platform Status

FreedomU540HiFiveUnleashedBoard

    - Boots to EFI shell in QEMU (sifive_u)
    - Console input/output is functional
    - Linux can boot using a RAM disk

## Supported Operating Systems

    Linux (via QEMU with RAM disk)

## Known Issues and Limitations

    1) Only QEMU sifive_u is supported
    2) Storage drivers are not implemented in EDK2 for the U540 platform
    3) No physical disk or filesystem boot support

## Related Documentation

[SiFive Freedom U540-C000 Manual](https://sifive.cdn.prismic.io/sifive%2F834354f0-08e6-423c-bf1f-0cb58ef14061_fu540-c000-v1.0.pdf)

[SiFive RISC-V Core Document](https://www.sifive.com/documentation)


## U5SeriesPkg Platform PCD Settings
|PCD Name            |	Description                   |
|--------------------|--------------------------------|
|PcdFixedRamdiskBase |	RAM disk base address for boot|
|PcdFixedRamdiskSize |	RAM disk size in bytes        |
