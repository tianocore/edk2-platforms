# Introduction of SiFive U7 Series Platforms
U7SeriesPkg provides the common EDK2 libraries and drivers for SiFive U7 series
platforms. Currently the supported platforms are Freedom U740 HiFive Unmatched
platform.

## U740 Platform
This is a platform package used for the SiFive Freedom U740 HiFive Unmatched
development board.

To test this package on hardware, first build EDK2 with:
```
build -a RISCV64 -t GCC5 -p Platform/SiFive/U7SeriesPkg/FreedomU740HiFiveUnmatchBoard/U740.dsc
```
The produced image is located at
```
./Build/FreedomU740HiFiveUnmatched/DEBUG_GCC5/FV/U740.fd
```

Then clone and build the fsbl (first stage bootloader, I think this will
eventually reside in the UEFI PEI stage?):
```
git clone https://github.com/yqszxx/freedom-u740-c000-bootloader
cd freedom-u740-c000-bootloader
CROSSCOMPILE=riscv64-unknown-elf- make
```
And you will get the fsbl image as `fsbl.bin`.

Next prepare a micro-SD card, properly partition it, and write the images.
```
sudo sgdisk -g --clear -a 1 \
  --new=1:34:2081         --change-name=1:fsbl --typecode=1:5B193300-FC78-40CD-8002-E86C45580B47 \
  --new=2:2082:34849      --change-name=2:edk2  --typecode=2:2E54B353-1271-4842-806F-E436D6AF6985
  /dev/sdX
sudo dd if=fsbl.bin of=/dev/sdX1
sudo dd if=U740.fd of=/dev/sdX2
```

Insert the card into the slot on the unmatched board, press power button, expect
to see the UEFI shell on serial console.
