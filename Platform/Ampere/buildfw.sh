#!/usr/bin/env bash
set -x
##
# @file
#  Build script for platforms with an Altra(R) CPU from Ampere(R).
#
# Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
set -o errexit

tfa_usage () {
  echo -n "cert_create or fiptool could not be found. If running on a "
  echo -n "Debian-based distro (e.g. Ubuntu) you can get them by "
  echo    "installing the arm-trusted-firmware-tools package."
  echo -n "Otherwise, you can build them and add them to the \$PATH by "
  echo    "running the following commands:"
  echo
  echo "  git clone --depth 1 https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git"
  echo "  pushd trusted-firmware-a"
  echo "  ${MAKE_COMMAND} fiptool"
  echo "  ${MAKE_COMMAND} certtool"
  echo "  export PATH=\$PWD/tools/cert_create:\$PWD/tools/fiptool:\$PATH"
  echo "  popd"
  exit 1
}

efitools_usage () {
  echo -n "cert-to-efi-sig-list or sign-efi-sig-list could not be found. If running on a "
  echo -n "Debian-based distro (e.g. Ubuntu) you can get them by "
  echo    "installing the efitools package."
  echo -n "Otherwise, you can build them and add them to the \$PATH by "
  echo    "running the following commands:"
  echo
  echo "  git clone --depth 1 https://git.kernel.org/pub/scm/linux/kernel/git/jejb/efitools.git"
  echo "  pushd efitools"
  echo "  ${MAKE_COMMAND}"
  echo "  export PATH=\$PWD/cert-to-efi-sig-list:\$PWD/sign-efi-sig-list:\$PATH"
  echo "  popd"
  exit 1
}

usage () {
  echo "Usage:"
  echo "  $0 [options]"
  echo
  echo "Options:"
  echo "  -b <bldtype>, --build <bldtype>  Specify the build type: DEBUG or RELEASE"
  echo "  -t <tc>, --toolchain <tc>        Specify the toolchain to use: GCC or CLANG"
  echo "  -m <mfg>, --manufacturer <mfg>   Specify platform manufacturer (e.g. Ampere)"
  echo "  -p <plat>, --platform <plat>     Specify platform to build (e.g. Jade)"
  echo "  -l <kern>, --linuxboot <kern>    Build LinuxBoot firmware instead of full EDK2 with UEFI Shell, specifying path to flashkernel"
  echo "  -f, --flash                      Copy firmware to BMC and flash firmware (keeping EFI variables and NVPARAMs) after building"
  echo "  -F, --full-flash                 Copy firmware to BMC and flash full EEPROM (resetting EFI variables and NVPARAMs) after building"
  echo "  -s, --silent                     Print less messages."
  echo "  -v, --verbose                    Print more messages."
  echo ""
  echo "  Note: flash options require bmc.sh file with env vars BMC_HOST, BMC_USER and BMC_PASS defined"
  echo ""
  echo "  Available manufacturers:"
  echo "    ADLINK"
  echo "    Ampere"
  echo "    ASRockRack"
  echo ""
  echo "  Available platforms:"
  echo "    ADLINK     -> ComHpcAlt"
  echo "    Ampere     -> Jade"
  echo "    ASRockRack -> Altra1L2Q"
  echo "    ASRockRack -> Altra1L2T"
  echo ""
  echo "Environment Variables:"
  echo "  SECUREBOOT_DIR       - directory to store SecureBoot keys, certs etc."
  echo "  USE_EXISTING_SB_KEYS - use existing Secure Boot Platform and Update keys"
  echo "  DOWNLOAD_MS_SB_KEYS  - force re-download of Microsoft Secure Boot KEK and DB certificates"
  echo "  CERT_PASSWORD        - password to use when generating Platform and Update Keys and certificates"
  echo "                         defaults to \"password\" if not specified."
  echo ""
  echo "  EDK2_SECURE_BOOT_ENABLE             (TRUE)"
  echo "  EDK2_NETWORK_ENABLE                 (TRUE)"
  echo "  EDK2_INCLUDE_TFTP_COMMAND           (TRUE)"
  echo "  EDK2_NETWORK_IP6_ENABLE             (TRUE)"
  echo "  EDK2_NETWORK_ALLOW_HTTP_CONNECTIONS (FALSE)"
  echo "  EDK2_NETWORK_TLS_ENABLE             (TRUE)"
  echo "  EDK2_REDFISH_ENABLE                 (TRUE)"
  echo "  EDK2_PERFORMANCE_MEASUREMENT_ENABLE (FALSE)"
  echo "  EDK2_TPM2_ENABLE                    (TRUE)"
  echo "  EDK2_HEAP_GUARD_ENABLE              (FALSE)"
  echo "  EDK2_X86_EMULATOR_ENABLE            (TRUE)"
  echo "  EDK2_SHELL_ENABLE                   (TRUE)"
  echo "  LINUXBOOT_FILE_IN_UEFI_EXTRA        (FALSE)"

  exit 1
}

ctrl_c() {
  popd
}

TFA_VERSION=${TFA_VERSION:-2.10.20230517}
SCP_VERSION=${SCP_VERSION:-2.10.20230517}

TFA_SLIM=${TFA_SLIM:-$PWD/altra_atf_signed_${TFA_VERSION}.slim}
SCP_SLIM=${SCP_SLIM:-$PWD/altra_scp_signed_${SCP_VERSION}.slim}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
ROOT_DIR=$(realpath ${SCRIPT_DIR}/../../..)
trap ctrl_c INT
pushd "${ROOT_DIR}"

SPI_SIZE_MB=32

MANUFACTURER=Ampere
BOARD_NAME=Jade

FLASHFW=0
RESET_NV_STORAGE=0
TOOLCHAIN=GCC
BLDTYPE=RELEASE
BUILD_THREADS=$(getconf _NPROCESSORS_ONLN)
SILENT=0
VERBOSE=0

export PYTHON_COMMAND=python3
export WORKSPACE=$PWD

if [ "$(uname -o)" = "FreeBSD" ]; then
  MAKE_COMMAND=gmake
  GETOPT_COMMAND=/usr/local/bin/getopt
  if ! command -v ${GETOPT_COMMAND} >/dev/null 2>&1; then
    echo "GNU getopt is required. Please install the getopt package."
    exit 1
  fi
  if ! command -v ${MAKE_COMMAND} >/dev/null 2>&1; then
    echo "GNU make is required. Please install the gmake package."
    exit 1
  fi
  mkdir bin || true
  ln -sfv /usr/local/bin/gmake bin/make
  export PATH=$PWD/bin:$PATH
elif [ "$(uname -o)" = "Darwin" ]; then
  MAKE_COMMAND=gmake
  GETOPT_COMMAND=/opt/homebrew/opt/gnu-getopt/bin/getopt
  if ! command -v ${GETOPT_COMMAND} >/dev/null 2>&1; then
    echo "GNU getopt is required. Please install the gnu-getopt homebrew package."
    exit 1
  fi
else
  MAKE_COMMAND=make
  GETOPT_COMMAND=getopt
fi

if ! command -v cert_create >/dev/null 2>&1; then
  tfa_usage
fi

if ! command -v fiptool >/dev/null 2>&1; then
  tfa_usage
fi

if ! command -v cert-to-efi-sig-list >/dev/null 2>&1; then
  efitools_usage
fi

if ! command -v sign-efi-sig-list >/dev/null 2>&1; then
  efitools_usage
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "Could not find python3. Please install the python3 package."
  exit 1
fi

OPTIONS=$(${GETOPT_COMMAND} -o t:b:fFsvhl:m:p: --long toolchain:,build:,linuxboot:,manufacturer:,platform:,flash,full-flash,silent,verbose,help -- "$@")
eval set -- "$OPTIONS"

while true; do
  case "$1" in
    -t|--toolchain)
      TOOLCHAIN=$2; shift 2;;
    -b|--build)
      BLDTYPE=$2; shift 2;;
    -f|--flash)
      FLASHFW=1; shift;;
    -F|--full-flash)
      FLASHFW=1; RESET_NV_STORAGE=1; shift;;
    -l|--linuxboot)
      LINUXBOOT=LinuxBoot; FLASHKERNEL=$2; shift 2;;
    -m|--manufacturer)
      MANUFACTURER=$2; shift 2;;
    -p|--platform)
      BOARD_NAME=$2;  shift 2;;
    -s|--silent)
      SILENT=1; shift;;
    -v|--verbose)
      VERBOSE=1; shift;;
    -h|--help)
      usage; shift;;
    --) shift; break;;
    *) echo "Internal error ($1)!"; exit 1;;
  esac
done

eval set -- ""

BOARD_SETTINGS_CFG=edk2-platforms/Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/${BOARD_NAME}BoardSetting.cfg
OUTPUT_BIN_DIR=$PWD/Build/${BOARD_NAME}
EDK2_BUILD_DIR=$PWD/Build/${BOARD_NAME}
OUTPUT_BOARD_SETTINGS_BIN=${OUTPUT_BIN_DIR}/$(basename ${BOARD_SETTINGS_CFG}).bin

export PACKAGES_PATH=$PWD:$PWD/edk2:$PWD/edk2-non-osi:$PWD/edk2-platforms:$PWD/edk2-platforms/Features/Intel/Debugging:$PWD/edk2-platforms/Features:$PWD/edk2-platforms/Features/Intel

case $(uname -m) in
  "x86_64")
    if [ "$TOOLCHAIN" = "GCC" -a -z "${GCC_AARCH64_PREFIX}" ]; then
      echo "Error: need to define \$GCC_AARCH64_PREFIX since the native compiler won't work"
      exit 1
    fi
    ;;
esac

if [ "${TOOLCHAIN}" = "CLANG" ]; then
  TOOLCHAIN=CLANGDWARF
fi

if [ -n "${LINUXBOOT}" ]; then
  if [ ! -f "${FLASHKERNEL}" ]; then
    echo "LinuxBoot flashkernel file \"${FLASHKERNEL}\" does not exist."
    exit 1
  else
    EXTRA_BUILD_FLAGS="-D LINUXBOOT_FILE=${FLASHKERNEL}"
  fi
fi

mkdir -p "${OUTPUT_BIN_DIR}"
cp -v "${BOARD_SETTINGS_CFG}" "${OUTPUT_BIN_DIR}/$(basename ${BOARD_SETTINGS_CFG}).txt"
python3 "${WORKSPACE}/edk2-ampere-tools/nvparam.py" -f "${BOARD_SETTINGS_CFG}" -o "${OUTPUT_BOARD_SETTINGS_BIN}"
rm -fv "${OUTPUT_BOARD_SETTINGS_BIN}.padded"

${MAKE_COMMAND} -C edk2/BaseTools -j ${BUILD_THREADS}

. "${WORKSPACE}/edk2-platforms/Platform/Ampere/Tools/fw_ver.sh" UPDATE
. edk2/edksetup.sh

if [ -e "${WORKSPACE}/build.conf" ]; then
  . "${WORKSPACE}/build.conf"
fi

pushd edk2
cp -vf BaseTools/Conf/tools_def.template Conf/tools_def.txt
patch -p0 < "${WORKSPACE}/edk2-platforms/Platform/Ampere/Tools/tools_def.txt.patch"
popd

if [ -z "${SECUREBOOT_DIR}" ]; then
  SECUREBOOT_DIR="${WORKSPACE}/secureboot_objects/"
  export SECUREBOOT_DIR
fi

if [ -n "${USE_EXISTING_SB_KEYS}" ]; then
  export USE_EXISTING_SB_KEYS
fi
if [ -n "${CERT_PASSWORD}" ]; then
  export CERT_PASSWORD
fi

EDK2_SECURE_BOOT_ENABLE=${EDK2_SECURE_BOOT_ENABLE:-TRUE}
EDK2_NETWORK_ENABLE=${EDK2_NETWORK_ENABLE:-TRUE}
EDK2_INCLUDE_TFTP_COMMAND=${EDK2_INCLUDE_TFTP_COMMAND:-TRUE}
EDK2_NETWORK_IP6_ENABLE=${EDK2_NETWORK_IP6_ENABLE:-TRUE}
EDK2_NETWORK_ALLOW_HTTP_CONNECTIONS=${EDK2_NETWORK_ALLOW_HTTP_CONNECTIONS:-FALSE}
EDK2_NETWORK_TLS_ENABLE=${EDK2_NETWORK_TLS_ENABLE:-TRUE}
EDK2_REDFISH_ENABLE=${EDK2_REDFISH_ENABLE:-TRUE}
EDK2_PERFORMANCE_MEASUREMENT_ENABLE=${EDK2_PERFORMANCE_MEASUREMENT_ENABLE:-FALSE}
EDK2_TPM2_ENABLE=${EDK2_TPM2_ENABLE:-TRUE}
EDK2_HEAP_GUARD_ENABLE=${EDK2_HEAP_GUARD_ENABLE:-FALSE}
EDK2_X86_EMULATOR_ENABLE=${EDK2_X86_EMULATOR_ENABLE:-TRUE}
EDK2_SHELL_ENABLE=${EDK2_SHELL_ENABLE:-TRUE}
LINUXBOOT_FILE_IN_UEFI_EXTRA=${LINUXBOOT_FILE_IN_UEFI_EXTRA:-FALSE}

if [ -z "${AMPERE_PLATFORM_DBB_KEY}" ]; then
  AMPERE_PLATFORM_DBB_KEY="${SECUREBOOT_DIR}/keys/platform_key.priv"
  export AMPERE_PLATFORM_DBB_KEY
fi
if [ -z "${AMPERE_PLATFORM_DBu_KEY}" ]; then
  AMPERE_PLATFORM_DBU_KEY="${SECUREBOOT_DIR}/keys/update_key.priv"
  export AMPERE_PLATFORM_DBU_KEY
fi

if [ "${BLDTYPE}" = "RELEASE" ]; then
  EDK2_SHELL_ENABLE=${EDK2_SHELL_ENABLE:-FALSE}
else
  EDK2_SHELL_ENABLE=${EDK2_SHELL_ENABLE:-TRUE}
fi

if [ "${EDK2_HEAP_GUARD_ENABLE}" = "TRUE" ] && [ "${EDK2_X86_EMULATOR_ENABLE}" = "TRUE" ]; then
  echo "Error: HeapGuard and X86 emulator are incompatible. Only one may be enabled at a time."
  exit 1
fi

if [ "${EDK2_SECURE_BOOT_ENABLE}" = "TRUE" ]; then
  export MANUFACTURER
  export BOARD_NAME
  "${WORKSPACE}/edk2-platforms/Platform/Ampere/Tools/GenerateSecureBootKeys.sh"

  EXTRA_BUILD_FLAGS+=" -D DEFAULT_KEYS=TRUE"
  EXTRA_BUILD_FLAGS+=" -D PK_DEFAULT_FILE=${SECUREBOOT_DIR}/certs/platform_key.der"
  EXTRA_BUILD_FLAGS+=" -D KEK_DEFAULT_FILE1=${SECUREBOOT_DIR}/certs/ms_kek1.der"
  EXTRA_BUILD_FLAGS+=" -D KEK_DEFAULT_FILE2=${SECUREBOOT_DIR}/certs/ms_kek2.der"
  EXTRA_BUILD_FLAGS+=" -D DB_DEFAULT_FILE1=${SECUREBOOT_DIR}/certs/ms_db1.der"
  EXTRA_BUILD_FLAGS+=" -D DB_DEFAULT_FILE2=${SECUREBOOT_DIR}/certs/ms_db2.der"
  EXTRA_BUILD_FLAGS+=" -D DB_DEFAULT_FILE3=${SECUREBOOT_DIR}/certs/ms_db3.der"
  EXTRA_BUILD_FLAGS+=" -D DB_DEFAULT_FILE4=${SECUREBOOT_DIR}/certs/ms_db4.der"
  EXTRA_BUILD_FLAGS+=" -D DB_DEFAULT_FILE5=${SECUREBOOT_DIR}/certs/ms_db5.der"
  EXTRA_BUILD_FLAGS+=" -D DBX_DEFAULT_FILE1=${SECUREBOOT_DIR}/certs/dummy_dbx.der"
fi

if [ -n "${LINUXBOOT}" ] && [ "${LINUXBOOT_FILE_IN_UEFI_EXTRA}" = "TRUE" ]; then
  EXTRA_BUILD_FLAGS+=" -D LINUXBOOT_FILE_IN_UEFI_EXTRA=TRUE"
fi

if [ "${SILENT}" = 1 ]; then
  EXTRA_BUILD_FLAGS+=" --silent --quiet"
  MAKE_FLAGS="--quiet"
fi

if [ "${VERBOSE}" = 1 ]; then
  EXTRA_BUILD_FLAGS+=" --verbose"
fi

UPD720202_ROM_FILE="K2026090.mem"
if [ "${BOARD_NAME}" = "ComHpcAlt" ] && [ -e "${WORKSPACE}/${UPD720202_ROM_FILE}" ]; then
  EXTRA_BUILD_FLAGS+=" -D USB_UPD720202_ROM_FILE=${WORKSPACE}/${UPD720202_ROM_FILE}"
fi

if [ -e "${WORKSPACE}/IntelUndiBin/Release/AARCH64/GigUndiDxe.efi" ]; then
  EXTRA_BUILD_FLAGS+=" -D INTEL_UNDI_BIN=TRUE"
fi

# YearMonthDayBuild (0xYYMMDDBB)
echo "#define CURRENT_FIRMWARE_VERSION ${VER_HEX}" > "${WORKSPACE}/edk2-platforms/Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/Capsule/SystemFirmwareDescriptor/HostFwInfo.h"
echo "#define CURRENT_FIRMWARE_VERSION_STRING L\"${FW_STR}\"" >> "${WORKSPACE}/edk2-platforms/Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/Capsule/SystemFirmwareDescriptor/HostFwInfo.h"
echo "#define LOWEST_SUPPORTED_FIRMWARE_VERSION 0x00000000" >> "${WORKSPACE}/edk2-platforms/Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/Capsule/SystemFirmwareDescriptor/HostFwInfo.h"

if [ -f "${SCP_SLIM}" ]; then
  cp -vf "${SCP_SLIM}" "${OUTPUT_BIN_DIR}/altra_scp.slim"
fi
if [ -f "${TFA_SLIM}" ]; then
  cp -vf "${TFA_SLIM}" "${OUTPUT_BIN_DIR}/altra_atf.slim"
fi

# shellcheck disable=SC2086
build -a AARCH64 -t ${TOOLCHAIN} -b ${BLDTYPE} -n ${BUILD_THREADS}                 \
        -D FIRMWARE_VER_FULL="${VER} TF-A ${TFA_VERSION}"                          \
        -D FIRMWARE_VER="${VER}"                                                   \
        -D FIRMWARE_VER_HEX="${VER_HEX}"                                           \
        -D MAJOR_VER=${MAJOR_VER} -D MINOR_VER=${MINOR_VER}                        \
        -D UEFI_SECURE_BOOT_ENABLE=${EDK2_SECURE_BOOT_ENABLE}                      \
        -D NETWORK_ENABLE=${EDK2_NETWORK_ENABLE}                                   \
        -D INCLUDE_TFTP_COMMAND=${EDK2_INCLUDE_TFTP_COMMAND}                       \
        -D NETWORK_IP6_ENABLE=${EDK2_NETWORK_IP6_ENABLE}                           \
        -D NETWORK_ALLOW_HTTP_CONNECTIONS=${EDK2_NETWORK_ALLOW_HTTP_CONNECTIONS}   \
        -D NETWORK_TLS_ENABLE=${EDK2_NETWORK_TLS_ENABLE}                           \
        -D REDFISH_ENABLE=${EDK2_REDFISH_ENABLE}                                   \
        -D PERFORMANCE_MEASUREMENT_ENABLE=${EDK2_PERFORMANCE_MEASUREMENT_ENABLE}   \
        -D TPM2_ENABLE=${EDK2_TPM2_ENABLE}                                         \
        -D HEAP_GUARD_ENABLE=${EDK2_HEAP_GUARD_ENABLE}                             \
        -D X86_EMULATOR_ENABLE=${EDK2_X86_EMULATOR_ENABLE}                         \
        -D SHELL_ENABLE=${EDK2_SHELL_ENABLE}                                       \
        -y BuildReport.log                                         \
        -p Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/${BOARD_NAME}${LINUXBOOT}.dsc \
        ${EXTRA_BUILD_FLAGS}

OUTPUT_BASENAME=${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_tfa_uefi_${BLDTYPE,,}_${VER}

OUTPUT_UEFI_IMAGE=${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_uefi_${BLDTYPE,,}_${VER}.bin
OUTPUT_TFA_UEFI_IMAGE=${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_tfa_uefi_${BLDTYPE,,}_${VER}.bin
OUTPUT_UEFI_IMAGE_SIGNED="${OUTPUT_UEFI_IMAGE/'bin'/'dbu.sig.bin'}"
OUTPUT_TFA_UEFI_IMAGE_SIGNED="${OUTPUT_TFA_UEFI_IMAGE/'bin'/'dbu.sig.bin'}"
echo "OUTPUT_TFA_UEFI_IMAGE_SIGNED=${OUTPUT_TFA_UEFI_IMAGE_SIGNED}"
OUTPUT_SPINOR_IMAGE=${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_rom_${BLDTYPE,,}_${VER}.bin

cp -v "${EDK2_BUILD_DIR}/${BLDTYPE}_${TOOLCHAIN}/FV/BL33_${BOARD_NAME^^}_UEFI.fd" "${OUTPUT_UEFI_IMAGE}.unsigned"

# Create BL33 content certificate
cert_create -n --ntfw-nvctr 0 --key-alg rsa     \
  --hash-alg sha384                             \
  --nt-fw-key "${AMPERE_PLATFORM_DBB_KEY}"      \
  --nt-fw-cert "${OUTPUT_UEFI_IMAGE}.crt"       \
  --nt-fw "${OUTPUT_UEFI_IMAGE}.unsigned"
# Create BL33 fip image
fiptool --verbose create --nt-fw-cert "${OUTPUT_UEFI_IMAGE}.crt" --nt-fw  "${OUTPUT_UEFI_IMAGE}.unsigned" "${OUTPUT_UEFI_IMAGE}"
rm -fr "${OUTPUT_UEFI_IMAGE}.crt" "${OUTPUT_UEFI_IMAGE}.unsigned"

cp -vf "${OUTPUT_UEFI_IMAGE}" "${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_uefi.bin"

if [ -n "${LINUXBOOT}" ] && [ "${LINUXBOOT_FILE_IN_UEFI_EXTRA}" = "TRUE" ]; then
  OUTPUT_LINUXBOOT_UEFI_IMAGE=${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_linuxboot_uefiextra_${BLDTYPE,,}_${VER}.bin
  if [ -f "${EDK2_BUILD_DIR}/${BLDTYPE}_${TOOLCHAIN}/FV/LINUXBOOT_UEFI_EXTRA.fd" ]; then
    cp -fv "${OUTPUT_BIN_DIR}/${BLDTYPE}_${TOOLCHAIN}/FV/LINUXBOOT_UEFI_EXTRA.fd" "${OUTPUT_LINUXBOOT_UEFI_IMAGE}"
  fi
fi

if [ -f "${TFA_SLIM}" ]; then
  # Create a 2MB file with 0xff
  dd bs=1024 count=2048 if=/dev/zero | tr "\000" "\377" > "${OUTPUT_TFA_UEFI_IMAGE}"
  dd bs=1024 conv=notrunc if="${TFA_SLIM}" of="${OUTPUT_TFA_UEFI_IMAGE}"

  # Create TFA certificate
  cert_create -n --ntfw-nvctr 0 --key-alg rsa   \
    --hash-alg sha384                           \
    --nt-fw-key "${AMPERE_PLATFORM_DBB_KEY}"    \
    --nt-fw-cert "${TFA_SLIM}.crt"              \
    --nt-fw "${TFA_SLIM}"
  dd bs=1 seek=1572864 conv=notrunc if="${TFA_SLIM}.crt" of="${OUTPUT_TFA_UEFI_IMAGE}"
  rm -fr "${TFA_SLIM}.crt"

  dd bs=1 seek=2031616 conv=notrunc if="${OUTPUT_BOARD_SETTINGS_BIN}" of="${OUTPUT_TFA_UEFI_IMAGE}"
fi

rm -f "${OUTPUT_BOARD_SETTINGS_BIN}"
rm -f "${OUTPUT_BIN_DIR}/$(basename ${BOARD_SETTINGS_CFG}).txt"

if [ -f "${TFA_SLIM}" ]; then
  # Write the UEFI image
  dd bs=1024 seek=2048 if="${OUTPUT_UEFI_IMAGE}" of="${OUTPUT_TFA_UEFI_IMAGE}"

  # Create the full SPI-NOR image
  dd bs=1M count=${SPI_SIZE_MB} if=/dev/zero | tr "\000" "\377" > "${OUTPUT_SPINOR_IMAGE}"
  # Write the code (TF-A+UEFI) area
  dd bs=1M seek=4 conv=notrunc if="${OUTPUT_TFA_UEFI_IMAGE}" of="${OUTPUT_SPINOR_IMAGE}"

  dd bs=512 count=26623 if=/dev/zero | tr "\000" "\377" > "${OUTPUT_TFA_UEFI_IMAGE}.append"
  dd bs=1024 seek=0 conv=notrunc if="${OUTPUT_TFA_UEFI_IMAGE}" of="${OUTPUT_TFA_UEFI_IMAGE}.append"
  openssl dgst -sha384 -sign "${AMPERE_PLATFORM_DBU_KEY}" -out "${OUTPUT_TFA_UEFI_IMAGE}.sig" "${OUTPUT_TFA_UEFI_IMAGE}.append"
  cat "${OUTPUT_TFA_UEFI_IMAGE}.sig" "${OUTPUT_TFA_UEFI_IMAGE}.append" > "${OUTPUT_TFA_UEFI_IMAGE}.signed"
  cp -vf "${OUTPUT_TFA_UEFI_IMAGE}.signed" "${OUTPUT_TFA_UEFI_IMAGE_SIGNED}"
  rm -f "${OUTPUT_TFA_UEFI_IMAGE}.signed" "${OUTPUT_TFA_UEFI_IMAGE}.append" "${OUTPUT_TFA_UEFI_IMAGE}.sig"
  cp -vf "${OUTPUT_TFA_UEFI_IMAGE_SIGNED}" "${EDK2_BUILD_DIR}/${BOARD_NAME,,}_tfa_uefi.bin"
else
  echo "Create UEFI.fip signature with DBU key"
  dd bs=1024 count=11264 if=/dev/zero | tr "\000" "\377" > "${OUTPUT_UEFI_IMAGE}.append"
  dd bs=1024 seek=0 conv=notrunc if="${OUTPUT_UEFI_IMAGE}" of="${OUTPUT_UEFI_IMAGE}.append"
  openssl dgst -sha384 -sign "${AMPERE_PLATFORM_DBU_KEY}" -out "${OUTPUT_UEFI_IMAGE}.sig" "${OUTPUT_UEFI_IMAGE}.append"
  cat "${OUTPUT_UEFI_IMAGE}.sig" "${OUTPUT_UEFI_IMAGE}.append" > "${OUTPUT_UEFI_IMAGE}.signed"
  cp -vf "${OUTPUT_UEFI_IMAGE}.signed" "${OUTPUT_UEFI_IMAGE_SIGNED}"
  rm -f "${OUTPUT_UEFI_IMAGE}.signed" "${OUTPUT_UEFI_IMAGE}.append" "${OUTPUT_UEFI_IMAGE}.sig"
  cp -vf "${OUTPUT_UEFI_IMAGE_SIGNED}" "${EDK2_BUILD_DIR}/${BOARD_NAME,,}_uefi.bin"
fi

if [ -f "${TFA_SLIM}" ]; then
  INCLUDE_TFA_FW=TRUE
else
  INCLUDE_TFA_FW=FALSE
fi

# LinuxBoot doesn't support capsule updates
if [ -z "${LINUXBOOT}" ] && [ -f "${TFA_SLIM}" ] && [ -f "${SCP_SLIM}" ]; then

  # Build the capsule (for upgrading from the UEFI Shell or Linux)
  build -a AARCH64 -t ${TOOLCHAIN} -b ${BLDTYPE} -n ${BUILD_THREADS} \
      -D FIRMWARE_VER_FULL="${VER} TF-A ${TFA_VERSION}"        \
      -D FIRMWARE_VER="${VER}" \
      -D FIRMWARE_VER_HEX="${VER_HEX}" \
      -D MAJOR_VER=${MAJOR_VER}  \
      -D MINOR_VER=${MINOR_VER}  \
      -D INCLUDE_TFA_FW=${INCLUDE_TFA_FW} \
      -y BuildReportCapsule.log                                         \
      -p Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/${BOARD_NAME}Capsule.dsc

  cp -vf "${EDK2_BUILD_DIR}/${BLDTYPE}_${TOOLCHAIN}/FV/${BOARD_NAME^^}HOSTFIRMWARE.Cap" "${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_host_${BLDTYPE,,}_${VER}.cap"
  cp -vf "${EDK2_BUILD_DIR}/${BLDTYPE}_${TOOLCHAIN}/AARCH64/CapsuleApp.efi" "${OUTPUT_BIN_DIR}/"
  mkdir ${OUTPUT_BIN_DIR}/Cab || true
  rm -f ${OUTPUT_BIN_DIR}/Cab/*
  METAINFO_FILE="${OUTPUT_BIN_DIR}/Cab/firmware.metainfo.xml"
  cp -vf "${WORKSPACE}/edk2-platforms/Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/firmware.metainfo.xml" "${METAINFO_FILE}"
  cp -vf "${OUTPUT_BIN_DIR}/${BOARD_NAME,,}_host_${BLDTYPE,,}_${VER}.cap" "${OUTPUT_BIN_DIR}/Cab/firmware.bin"
  sed -i "s/{URGENCY}/high/g" "${METAINFO_FILE}"
  sed -i "s/{FW_VERSION}/$(printf '%d' ${VER_HEX})/g" "${METAINFO_FILE}"
  sed -i "s/{FW_DATE}/$(date +%Y-%m-%d)/g" "${METAINFO_FILE}"
  sed -i "s/{RELEASE_NOTES}//g" "${METAINFO_FILE}"
  pushd "${OUTPUT_BIN_DIR}/Cab"
  gcab -c -z -v "../${BOARD_NAME,,}_host_${BLDTYPE,,}_${VER}.cab" ./*
  popd
fi

if [ "${BOARD_NAME}" = "ComHpcAlt" ] && [ ! -e "${WORKSPACE}/${UPD720202_ROM_FILE}" ]; then
  echo "Warning: the Renesas UPD720202 USB3 Controller firmware file ${WORKSPACE}/${UPD720202_ROM_FILE} was not found."
  echo "The firmware was built without the firmware. The USB3 controller will not work unless the firmware is loaded in the OS."
  echo "See edk2-platforms/Drivers/OptionRomPkg/RenesasFirmwarePD720202/README.md for details on how to obtain it."
  exit 1
fi

if [ ! -e "${TFA_SLIM}" ]; then
  echo "Warning: the TF-A (Trusted Firmware) binary wasn't found. Only the UEFI firmware was built."
fi

echo "Done. Firmware is in ${OUTPUT_BIN_DIR}/"

if [ "${FLASHFW}" = "1" ]; then
    echo "Copying firmware to BMC and flashing host."
    if [ "$RESET_NV_STORAGE" = "1" ]; then
        "${WORKSPACE}/edk2-platforms/Platform/Ampere/Tools/fwflash.sh" -f "${OUTPUT_SPINOR_IMAGE}"
    else
        "${WORKSPACE}/edk2-platforms/Platform/Ampere/Tools/fwflash.sh" -u "${OUTPUT_UEFI_IMAGE}"
    fi
fi
