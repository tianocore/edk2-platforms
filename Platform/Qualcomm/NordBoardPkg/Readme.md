# Qualcomm Nord Generic UEFI platform (NordBoardPkg)

This builds an EDK2/TianoCore UEFI firmware for the Qualcomm
(Oryon) Nord Generic, intended to run as the TF-A non-trusted firmware
(BL33). It follows the MinPlatformPkg staged boot model (Stage 1-4) and
reuses the shared Qualcomm support: the GENI UART console
(GeniSerialPortLib), QualcommPlatformPkg's ArmPlatformLib and
RamPartitionTableLib for the Oryon memory map and dynamic DRAM sizing,
and QualcommSiliconPkg's SmemLib to read the SMEM usable RAM partition
table.

## Structure

- `NordBoardPkg.dsc` - MinPlatform staged DSC: sets `PcdBootStage`,
  includes `MinPlatformPkg` `CoreCommonLib`/`CorePeiLib`/`CoreDxeLib` and
  the board `Include/Dsc/Stage1-4.dsc.inc`, then the Qualcomm silicon and
  platform `.dsc.inc` files, then the board PCD overrides.
- `NordBoardPkg.fdf` - multi-FV flash layout: sets the board flash sizes and
  includes the shared `QualcommMinPlatformPkg/Include/Fdf/FlashMap.fdf.inc`
  for the derived offsets/bases. The whole FD is loaded by TF-A as BL33 and
  entered at offset 0, so `FvPreMemory` (containing SEC) is first.
- `Library/BoardInitLib` - MinPlatform board hooks.

## Build

```
export WORKSPACE=<workspace>
export PACKAGES_PATH=$WORKSPACE/edk2:$WORKSPACE/edk2-platforms:\
$WORKSPACE/edk2-platforms/Platform:$WORKSPACE/edk2-platforms/Platform/Qualcomm:\
$WORKSPACE/edk2-platforms/Silicon/Qualcomm:$WORKSPACE/edk2/ArmPlatformPkg
export GCC_AARCH64_PREFIX=aarch64-linux-gnu-
. edk2/edksetup.sh BaseTools
build -a AARCH64 -t GCC -p Platform/Qualcomm/NordBoardPkg/NordBoardPkg.dsc
```

The firmware device
`Build/NordBoardPkg/<TARGET>_GCC/FV/NORDBOARDPKG.fd` is loaded by
TF-A as BL33.

## Device tree

By default the platform builds a placeholder DummyDeviceTree and lets
`DtPlatformDxe` install it as the EFI DT configuration table; a boot
loader (e.g. a UKI) is expected to supply the real board DTB.

To embed a board device tree in the firmware instead, drop the DTB at
`Platform/Qualcomm/NordBoardPkg/DeviceTree/nord-board.dtb` and build
with `-D USER_PROVIDED_DTB=TRUE`; `DtPlatformDxe` then publishes the
embedded DTB (via `UserDTB.inf`) as the EFI DT configuration table.
