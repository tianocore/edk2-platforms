# GlymurMinPlatformPkg

This project brings UEFI support to Glymur (AARCH64) following the MinPlatform specification.

## Capabilities
- Boot UEFI Shell, and UiApp

## How to build
- GCC_AARCH64_PREFIX=aarch64-linux-gnu- build -b DEBUG -a AARCH64 -t GCC -p edk2-qualcomm/GlymurMinPlatformPkg/GlymurMinPlatformPkg.dsc -j $WORKSPACE/Build/GlymurMinPlatformPkg.log

### Prerequesites
- Ensure cross compiler is setup: aarch64-linux-gnu-gcc

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
cd Platform/Qualcomm/GlymurMinPlatformPkg
./BuildGlymurMinPlatformPkg.sh [OPTIONS]
```

Options:

| Option | Default | Description |
|---|---|---|
| `-b <type>` | `DEBUG` | Build type: `DEBUG`, `RELEASE`, `NOOPT`, or comma-separated (e.g. `DEBUG,RELEASE`) |
| `-t <toolchain>` | `GCC` | Toolchain: `GCC` or `CLANGDWARF` |
| `-n <cores>` | auto | Number of parallel build threads |

Examples:

```bash
# Default DEBUG build
./BuildGlymurMinPlatformPkg.sh

# RELEASE build with 8 threads
./BuildGlymurMinPlatformPkg.sh -b RELEASE -n 8

# Build both DEBUG and RELEASE
./BuildGlymurMinPlatformPkg.sh -b DEBUG,RELEASE -n 4
```

### 3. Signing flow

The script automatically produces a signed flashable ELF after the EDK2 build:

```
GLYMURMINPLATFORMPKG.fd
    │  objcopy  (binary → AArch64 ELF object)
    │  ld       (link at load address 0xA7000000)
    ▼
unsigned/uefi_unsigned.elf
    │  qtestsign -v7 aboot
    ▼
signed/uefi.elf  →  uefi.elf   ← flash this file
```

### 4. Final output

```
Build/GlymurMinPlatformPkg/<BUILD>_<TOOLCHAIN>/FV/uefi.elf
```

Flash `uefi.elf` to the UEFI partition using your platform's download tool.

## Important notes
- Secure boot is not available.
- `qtestsign` uses test keys only and is suitable for development and bring-up.
