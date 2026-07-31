# GlymurOpenBoardPkg

This project brings UEFI support to a Glymur (AARCH64) board, built as a thin
board layer on top of the [GlymurMinPlatformPkg](../GlymurMinPlatformPkg)
MinPlatform silicon layer.

## Capabilities
- Boot UEFI Shell, and UiApp

## How to build
- build -b DEBUG -a AARCH64 -t GCC -p edk2-platforms/Platform/Qualcomm/GlymurFamily/GlymurOpenBoardPkg/GlymurOpenBoardPkg.dsc -j $WORKSPACE/Build/GlymurOpenBoardPkg.log

### Prerequesites
- Ensure a GCC toolchain that can target AARCH64 is on `PATH`. If building on a
  native AARCH64 host the system `gcc` is sufficient; otherwise set
  `GCC_AARCH64_PREFIX` to a cross compiler, e.g. `aarch64-linux-gnu-`.

- EDK2
  - How to setup a local tree: https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II

- EDK2 Platforms
  - https://github.com/tianocore/edk2-platforms

- Environment variables:
  - WORKSPACE set to your current workspace
  - PACKAGES_PATH should contain path to:
    - edk2
    - edk2-platforms
    - edk2-platforms/Platform
    - edk2/ArmPlatformPkg
    - edk2-platforms/Platform/Qualcomm
    - edk2-platforms/Silicon/Qualcomm
    - edk2-platforms/Platform/Qualcomm/GlymurFamily

## Build and Sign (Recommended)

Use the provided build script which handles compilation, ELF wrapping, and signing in one step.

### 1. Configure toolchain paths

```bash
cp Platform/Qualcomm/SetupPathTemplate.sh Platform/Qualcomm/SetupPath.sh
```


Edit `SetupPath.sh` and set the variables for your environment:

| Variable | Description |
|---|---|
| `AARCH64_TOOLCHAIN_PREFIX` | AArch64 GCC prefix (used by `objcopy`/`ld` during signing) |
| `CLANG_BASE` | LLVM/Clang root directory (only needed for `CLANGDWARF` toolchain) |
| `QTESTSIGN_REPO` | Git URL for qtestsign (auto-cloned if not present) |

### 2. Run the build script

```bash
cd Platform/Qualcomm/GlymurFamily/GlymurOpenBoardPkg
./BuildGlymurOpenBoardPkg.sh [OPTIONS]
```

Options:

| Option | Default | Description |
|---|---|---|
| `-b <type>` | `DEBUG` | Build type: `DEBUG`, `RELEASE`, `NOOPT`, or comma-separated (e.g. `DEBUG,RELEASE`) |
| `-t <toolchain>` | `GCC` | Toolchain: `GCC` or `CLANGDWARF` |
| `-n <cores>` | auto-detected | Override the number of parallel build threads |

Examples:

```bash
# Default DEBUG build
./BuildGlymurOpenBoardPkg.sh

# RELEASE build
./BuildGlymurOpenBoardPkg.sh -b RELEASE

# Build both DEBUG and RELEASE
./BuildGlymurOpenBoardPkg.sh -b DEBUG,RELEASE
```

### 3. Signing flow

The script automatically produces a signed flashable ELF after the EDK2 build:

```
GLYMUR.fd
    |  objcopy  (binary -> AArch64 ELF object)
    |  ld       (link at load address 0xA7000000)
    v
unsigned/uefi_unsigned.elf
    |  qtestsign -v7 aboot
    v
signed/uefi.elf  ->  uefi.elf   <- flash this file
```

### 4. Final output

```
Build/GlymurOpenBoardPkg/<BUILD>_<TOOLCHAIN>/FV/uefi.elf
```

Flash `uefi.elf` to the UEFI partition using your platform's download tool.

## Important notes
- Secure boot is not available.
- `qtestsign` uses test keys only and is suitable for development and bring-up.
