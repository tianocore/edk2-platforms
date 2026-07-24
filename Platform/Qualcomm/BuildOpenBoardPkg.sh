#!/bin/bash

#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# BuildOpenBoardPkg.sh - Generic EDK2 Open Board Package build script
#
# Usage: BuildOpenBoardPkg.sh --silicon <name> [OPTIONS]
#

# Source path configuration (toolchain paths, repo URLs, etc.)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=SetupPathTemplate.sh
. "${SCRIPT_DIR}/SetupPathTemplate.sh" || { echo "[ERROR] Failed to source SetupPathTemplate.sh" >&2; exit 1; }

# Helper function for error messages
exit_error() {
    echo "[ERROR] $*" >&2
    exit 1
}

usage() {
    echo ""
    echo "Usage: $0 --silicon <name> [OPTIONS]"
    echo ""
    echo "Required:"
    echo "  --silicon <name>        Silicon/platform name (e.g., glymur). Case-insensitive."
    echo ""
    echo "Options:"
    echo "  -b <build_type>         Build type: DEBUG, RELEASE, NOOPT, or comma-separated"
    echo "                          combination (e.g., DEBUG,RELEASE). Default: DEBUG"
    echo "  -t <toolchain>          Toolchain: GCC, CLANGDWARF (default: GCC)"
    echo "  -n <cores>              Number of parallel build threads (default: 1)"
    echo "  --signing-tool <tool>   Signing tool: sectools, qtestsign (default: sectools)"
    echo "  -h                      Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --silicon glymur"
    echo "  $0 --silicon Glymur -b RELEASE -t CLANGDWARF -n 8"
    echo "  $0 --silicon glymur -b DEBUG,RELEASE -n 4"
    echo "  $0 --silicon glymur --signing-tool qtestsign -n 4"
    echo ""
    exit 0
}

# Defaults
BUILD_TYPE="DEBUG"
TOOLCHAIN="GCC"
CORES=$((`getconf _NPROCESSORS_ONLN`))
SIGNING_TOOL="sectools"
SILICON=""

# Parse options using getopt (supports both short and long options)
PARSED=$(getopt -o hb:t:n: --long silicon:,signing-tool: -n "$0" -- "$@") \
    || { usage; exit 1; }
eval set -- "$PARSED"

while true; do
    case "$1" in
        -h)             usage ;;
        --silicon)      SILICON="$2";       shift 2 ;;
        -b)             BUILD_TYPE="$2";    shift 2 ;;
        -t)             TOOLCHAIN="$2";     shift 2 ;;
        -n)             CORES="$2";         shift 2 ;;
        --signing-tool) SIGNING_TOOL="$2";  shift 2 ;;
        --)             shift; break ;;
        *)              exit_error "Unknown option: $1" ;;
    esac
done

# Validate required options
[ -z "$SILICON" ] && exit_error "--silicon is required. Use -h for help."

# Normalize silicon name to lowercase
SILICON=$(echo "$SILICON" | tr '[:upper:]' '[:lower:]')

# Parse and validate each build type in the (possibly comma-separated) list
BUILD_TYPES=()
IFS=',' read -ra _BT_LIST <<< "$BUILD_TYPE"
for _bt in "${_BT_LIST[@]}"; do
    case "$_bt" in
        DEBUG|RELEASE|NOOPT)
            BUILD_TYPES+=("$_bt")
            ;;
        *)
            exit_error "Invalid build type '$_bt'. Must be DEBUG, RELEASE, or NOOPT."
            ;;
    esac
done

# Validate toolchain
case "$TOOLCHAIN" in
    GCC|CLANGDWARF) ;;
    *) exit_error "Invalid toolchain '$TOOLCHAIN'. Must be GCC or CLANGDWARF." ;;
esac

# Validate signing tool
case "$SIGNING_TOOL" in
    sectools|qtestsign) ;;
    *) exit_error "Invalid signing tool '$SIGNING_TOOL'. Must be sectools or qtestsign." ;;
esac

# -----------------------------------------------------------------------
# Silicon-specific lookup table
# Add new silicon entries here as needed.
# -----------------------------------------------------------------------
case "$SILICON" in
    glymur)
        LOAD_ADDR="0xA7000000"
        QTESTSIGN_VER="v7"
        ;;

    rb3)
        LOAD_ADDR="0x9FC00000"
        QTESTSIGN_VER="v6"
        ;;
    nord)
        LOAD_ADDR="0xD8D00000"
        QTESTSIGN_VER="v6"
        ;;
    *)
        exit_error "Unknown silicon '$SILICON'. Add it to the lookup table in this script."
        ;;
esac

# -----------------------------------------------------------------------
# Derive all package names and paths from parameters
# -----------------------------------------------------------------------
SILICON_CAP="${SILICON^}"                                           # glymur  -> Glymur
PKG_NAME="${SILICON_CAP}MinPlatformPkg"                               # GlymurOpenBoardPkg
PKG_UPPER=$(echo "$PKG_NAME" | tr '[:lower:]' '[:upper:]')          # GLYMUROPENBOARDPKG
DSC_PATH="edk2-platforms/Platform/Qualcomm/${PKG_NAME}/${PKG_NAME}.dsc"

echo ""
echo "=========================================="
echo " BuildOpenBoardPkg.sh"
echo "=========================================="
echo " Silicon:      ${SILICON_CAP}"
echo " Package:      ${PKG_NAME}"
echo " Build type(s): $(IFS=','; echo "${BUILD_TYPES[*]}")"
echo " Toolchain:    ${TOOLCHAIN}"
echo " Cores:        ${CORES}"
echo " Signing tool: ${SIGNING_TOOL}"
echo "=========================================="
echo ""

# -----------------------------------------------------------------------
# Set up workspace (done once, outside the per-build-type loop)
# -----------------------------------------------------------------------
pwd
cd ../../../.. || exit_error "Failed to navigate to workspace root"
pwd
export WORKSPACE=$PWD

export PACKAGES_PATH=\
$WORKSPACE/edk2:\
$WORKSPACE/edk2-platforms:\
$WORKSPACE/edk2-platforms/Platform:\
$WORKSPACE/edk2/ArmPlatformPkg:\
$WORKSPACE/edk2-platforms/Platform/Qualcomm:\
$WORKSPACE/edk2-platforms/Silicon/Qualcomm

# Initialize EDK2 build environment
cd "$WORKSPACE/edk2" || exit_error "Failed to enter edk2 directory"
. edksetup.sh || exit_error "edksetup.sh failed"
make -C BaseTools/ -j 16 || exit_error "BaseTools build failed"
cd "$WORKSPACE" || exit_error "Failed to return to workspace root"

# Verify all package paths exist
echo "Verifying package paths..."
for p in $(echo "$PACKAGES_PATH" | tr ':' ' '); do
    [ -d "$p" ] || echo "WARNING: Missing PATH: $p"
done

mkdir -p Build

# -----------------------------------------------------------------------
# Build loop (one iteration per build type)
# -----------------------------------------------------------------------
for BUILD_TYPE in "${BUILD_TYPES[@]}"; do

echo ""
echo "Building ${PKG_NAME} [${BUILD_TYPE}/${TOOLCHAIN}] with ${CORES} thread(s)..."

if [ "$TOOLCHAIN" = "GCC" ]; then
    echo "Auto-detecting GCC toolchain..."
    if command -v aarch64-linux-gnu-gcc &> /dev/null; then
        echo "Found aarch64-linux-gnu-gcc, using GNU toolchain"
        export GCC_AARCH64_PREFIX=aarch64-linux-gnu-
    elif [ -n "${AARCH64_TOOLCHAIN_PREFIX}" ] && [ -f "${AARCH64_TOOLCHAIN_PREFIX}gcc" ]; then
        echo "GNU toolchain not found, using custom toolchain: ${AARCH64_TOOLCHAIN_PREFIX}"
        export GCC_AARCH64_PREFIX="${AARCH64_TOOLCHAIN_PREFIX}"
    else
        exit_error "No suitable GCC toolchain found. Install aarch64-linux-gnu-gcc or set AARCH64_TOOLCHAIN_PREFIX in SetupPathTemplate.sh."
    fi

    build -b "$BUILD_TYPE" -a AARCH64 \
        -t GCC \
        -p "$DSC_PATH" \
        -n "$CORES" \
        -j "$WORKSPACE/Build/${PKG_NAME}.log" \
        -s \
        || exit_error "EDK2 build failed"

elif [ "$TOOLCHAIN" = "CLANGDWARF" ]; then
    export CLANGDWARF_BIN="${CLANG_BASE}/bin/"
    export CLANGDWARF_ARM_PREFIX="${CLANG_BASE}/tools/bin/"

    build -b "$BUILD_TYPE" -a AARCH64 \
        -t CLANGDWARF \
        -p "$DSC_PATH" \
        -n "$CORES" \
        -j "$WORKSPACE/Build/${PKG_NAME}.log" \
        -s \
        || exit_error "EDK2 build failed"
fi

# -----------------------------------------------------------------------
# Derive output paths
# -----------------------------------------------------------------------
OUTPUT_DIR="${BUILD_TYPE}_${TOOLCHAIN}"
FV_DIR="$WORKSPACE/Build/${PKG_NAME}/${OUTPUT_DIR}/FV"
FD_PATH="${FV_DIR}/${PKG_UPPER}.fd"
UNSIGNED_DIR="${FV_DIR}/unsigned"
SIGNED_DIR="${FV_DIR}/signed"
UNSIGNED_ELF="${UNSIGNED_DIR}/uefi_unsigned.elf"
SIGNED_ELF="${SIGNED_DIR}/uefi.elf"
FINAL_ELF="${FV_DIR}/uefi.elf"

mkdir -p "$UNSIGNED_DIR"
mkdir -p "$SIGNED_DIR"

# -----------------------------------------------------------------------
# Sign the image
# -----------------------------------------------------------------------
echo ""
echo "Signing image with ${SIGNING_TOOL}..."

if [ "$SIGNING_TOOL" = "qtestsign" ]; then

    # Use GCC_AARCH64_PREFIX if already set (GCC toolchain), otherwise auto-detect.
    if [ -z "$GCC_AARCH64_PREFIX" ]; then
        if command -v aarch64-linux-gnu-gcc &> /dev/null; then
            export GCC_AARCH64_PREFIX=aarch64-linux-gnu-
        elif [ -n "${AARCH64_TOOLCHAIN_PREFIX}" ] && [ -f "${AARCH64_TOOLCHAIN_PREFIX}gcc" ]; then
            export GCC_AARCH64_PREFIX="${AARCH64_TOOLCHAIN_PREFIX}"
        else
            exit_error "qtestsign requires an AArch64 GCC toolchain for objcopy/ld. Install aarch64-linux-gnu-gcc or set AARCH64_TOOLCHAIN_PREFIX in SetupPathTemplate.sh."
        fi
    fi

    # Clone qtestsign if not present
    if [ ! -d "${WORKSPACE}/qtestsign" ]; then
        echo "Cloning qtestsign..."
        git clone "${QTESTSIGN_REPO}" "${WORKSPACE}/qtestsign" \
            || exit_error "Failed to clone qtestsign"
    fi

    # Convert FD binary to AArch64 ELF object
    ${GCC_AARCH64_PREFIX}objcopy \
        -I binary -B aarch64 -O elf64-littleaarch64 \
        "$FD_PATH" "${FV_DIR}/edk2-elf.o" \
        || exit_error "objcopy failed"

    # Link into a proper ELF at the platform load address
    ${GCC_AARCH64_PREFIX}ld \
        "${FV_DIR}/edk2-elf.o" \
        -o "$UNSIGNED_ELF" \
        -EL \
        -T "${WORKSPACE}/edk2-platforms/Silicon/Qualcomm/ElfPayload.lds" \
        --defsym=ELFENTRY=${LOAD_ADDR} \
        -Ttext=${LOAD_ADDR} \
        || exit_error "ld failed"
    rm -f "${FV_DIR}/edk2-elf.o"

    # Sign with qtestsign
    "${WORKSPACE}/qtestsign/qtestsign.py" \
        -${QTESTSIGN_VER} aboot \
        -o "$SIGNED_ELF" \
        "$UNSIGNED_ELF" \
        || exit_error "qtestsign failed"

fi

# Copy signed ELF to FV root for easy access
cp "$SIGNED_ELF" "$FINAL_ELF" || exit_error "Failed to copy final uefi.elf to FV root"

echo ""
echo "=========================================="
echo " Build complete! [${BUILD_TYPE}/${TOOLCHAIN}]"
echo "------------------------------------------"
echo " Unsigned: ${UNSIGNED_ELF}"
echo " Signed:   ${SIGNED_ELF}"
echo " Final:    ${FINAL_ELF}"
echo "=========================================="
echo ""

done
