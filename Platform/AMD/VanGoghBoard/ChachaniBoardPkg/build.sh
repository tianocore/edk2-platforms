#!/bin/bash
## @file
# Linux build script file to launch Chachani Board BIOS build
#
# Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

# Show section header
function echo_section {
    offset=$( echo "$1" | awk -vcols=${COLUMNS:-$(tput cols)} \
        '{ start = int((cols - length($0))/2); print start < 0 ? 0 : start }')
    padding=$(printf "%*s" $offset | tr ' ' '=')
    printf "%s%s%s\n" "$padding" "$1" "$padding"
}


# Build env var
echo_section "Preparing build settings"
export UDK_PATH=edk2
export OemBoard=Chachani
export PLATFORM_PATH=edk2-platforms/Platform/AMD/VanGoghBoard
export BUILD_TYPE=RELEASE
export TOOLCHAIN_TAG=CLANGPDB
export OTA_CAPSULE_NAME=OTACAPSULE # You need to keep this name sync with PlatformCapsule.fdf
#TRUE / FALSE
export COMPRESS_FSP_REGION=TRUE
export KEY_MODE=TK
export BIOSNAME=${OemBoard}Ext
# Compiler
#IASL_PREFIX shall end with a slash.
export IASL_PREFIX=
#NASM_PREFIX shall end with a slash.
export NASM_PREFIX=
#GCC5_BIN shall end with a slash.
export GCC5_BIN=
#CLANG_BIN shall end with a slash.
export CLANG_BIN=
#OPENSSL_PATH shall end with a slash.
export OPENSSL_PATH=

echo "Building for ${OemBoard} board, ${BUILD_TYPE} mode with ${TOOLCHAIN_TAG}."
echo "IASL: ${IASL_PREFIX}iasl, NASM: ${NASM_PREFIX}nasm, GCC: ${GCC5_BIN}gcc, CLANG:${CLANG_BIN}clang, OPENSSL:${OPENSSL_PATH}openssl."
[[ ${COMPRESS_FSP_REGION} == "TRUE" ]] && echo "FSP will be built with compress support."
# Env check
echo_section "Checking compilation environment"
[[ "${IASL_PREFIX}" == "" ]] && export IASL_PREFIX=$(dirname $(which iasl))/
[[ "${NASM_PREFIX}" == "" ]] && export NASM_PREFIX=$(dirname $(which nasm))/
[[ "${OPENSSL_PATH}" == "" ]] && export OPENSSL_PATH=$(dirname $(which openssl))/
[[ -f ${IASL_PREFIX}iasl ]] || (echo "IASL not found! Please specify IASL_PREFIX!";exit -1)
[[ -f ${NASM_PREFIX}nasm ]] || (echo "NASM not found! Please specify NASM_PREFIX!";exit -1)
[[ -f ${OPENSSL_PATH}openssl ]] || (echo "OpenSSL not found! Please specify OPENSSL_PATH!";exit -1)

echo "IASL version $(LC_ALL=C ${IASL_PREFIX}iasl -v | sed -n '3,3p' | cut -d' ' -f5) detected."
echo "NASM version $(LC_ALL=C ${NASM_PREFIX}nasm --version | head -n1 | cut -d' ' -f3) detected."
echo "OpenSSL version $(LC_ALL=C ${OPENSSL_PATH}openssl version | head -n1 | cut -d' ' -f2) detected."

if [ ${TOOLCHAIN_TAG} != "CLANGPDB" ]
then
    [[ "${GCC5_BIN}" == "" ]]   && export GCC5_BIN=$(dirname $(which gcc))/
    [[ -f ${GCC5_BIN}gcc ]]     || (echo "gcc not found! Please specify GCC5_BIN!"   ;exit -1)
    echo "Your GCC version is $(LC_ALL=C gcc --version | head -n1 | cut -d' ' -f4),"\
    "please ensure it is at least 5.X to fulfill EDK2's ${TOOLCHAIN_TAG} requirement."
else
    [[ "${CLANG_BIN}" == "" ]]     && export CLANG_BIN=$(dirname $(which clang))/
    [[ -f ${CLANG_BIN}clang ]]     || (echo "clang not found! Please specify CLANG_BIN!"   ;exit -1)
    [[ -f ${CLANG_BIN}lld-link ]]  || (echo "lld-link not found! Please install LLD!"   ;exit -1)
    [[ -f ${CLANG_BIN}llvm-lib ]]  || (echo "llvm-lib not found! Please install LLVM!"   ;exit -1)
    echo "Your clang version is $(LC_ALL=C clang --version | head -n1 | cut -d' ' -f4),"\
    "please ensure it is at least 9.X to fulfill EDK2's ${TOOLCHAIN_TAG} requirement."
    echo "Your lld version is $(LC_ALL=C lld-link --version | head -n1 | cut -d' ' -f3)."
fi

# Prepare EDK2 build env.
echo_section "Preparing EDK2 build environment"
export PROJECT_PKG=${PLATFORM_PATH}/ChachaniBoardPkg
export BDK_PATH=${PLATFORM_PATH}/AgesaPublic
export NonOsi_PKG=edk2-non-osi/Silicon/AMD/VanGogh
export PlatformSi_PKG=edk2-platforms/Silicon/AMD/VanGoghBoard
export PlatformAMD_PKG=edk2-platforms/Platform/AMD
export PACKAGES_PATH=$(pwd)/${UDK_PATH}:$(pwd)/${BDK_PATH}:$(pwd)/${PLATFORM_PATH}:$(pwd)/${NonOsi_PKG}:$(pwd)/${PlatformSi_PKG}:$(pwd)/${PlatformAMD_PKG}
export WORKSPACE=$(pwd)
export FD_NAME=${OemBoard}
export CONF_PATH=$(pwd)/${PROJECT_PKG}/Conf


source edk2/edksetup.sh
[[ ${COMPRESS_FSP_REGION} == "TRUE" ]] && export COMPRESS_FSP_EXTRA_ARG="-D COMPRESS_FSP_REGION=TRUE"
echo "DEFINE INTERNAL_IDS = NO" > ${PLATFORM_PATH}/ChachaniBoardPkg/Set.env
echo "DEFINE BUILD_BOARD = Chachani" >> ${PLATFORM_PATH}/ChachaniBoardPkg/Set.env

# Source override before build.
echo_section "Overriding source"
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdePkg/Include/Register/Intel/SmramSaveStateMap.h \
    ${WORKSPACE}/edk2/MdePkg/Include/Register/Intel/SmramSaveStateMap.h
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/UefiCpuPkg/PiSmmCpuDxeSmm/SmramSaveState.c \
    ${WORKSPACE}/edk2/UefiCpuPkg/PiSmmCpuDxeSmm/SmramSaveState.c
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLibCommon.c \
    ${WORKSPACE}/edk2/UefiCpuPkg/Library/SmmCpuFeaturesLib
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdeModulePkg/Universal/PCD/Dxe/Pcd.inf \
    ${WORKSPACE}/edk2/MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdeModulePkg/Universal/PCD/Dxe/Pcd.c \
    ${WORKSPACE}/edk2/MdeModulePkg/Universal/PCD/Dxe/Pcd.c
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdeModulePkg/Universal/PCD/Pei/Pcd.c \
    ${WORKSPACE}/edk2/MdeModulePkg/Universal/PCD/Pei/Pcd.c
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdeModulePkg/Universal/PCD/Pei/Pcd.inf \
    ${WORKSPACE}/edk2/MdeModulePkg/Universal/PCD/Pei/Pcd.inf
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/BaseTools/Source/Python/GenFds/Capsule.py \
    ${WORKSPACE}/edk2/BaseTools/Source/Python/GenFds/Capsule.py
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf \
    ${WORKSPACE}/edk2/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleService.c \
    ${WORKSPACE}/edk2/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleService.c
cp -fv ${WORKSPACE}/edk2-platforms/Platform/AMD/VanGoghBoard/Override/edk2/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleService.h \
    ${WORKSPACE}/edk2/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleService.h


# Call build process.
echo_section "Calling build process"
[[ -f "$EDK_TOOLS_PATH/Source/C/bin/GenFw" ]] || (pushd ${EDK_TOOLS_PATH}; make -f GNUmakefile; popd)
build -p ${PROJECT_PKG}/Project.dsc -t ${TOOLCHAIN_TAG} -b ${BUILD_TYPE} ${COMPRESS_FSP_EXTRA_ARG} \
    -DINTERNAL_IDS=NO -DBUILD_BOARD=Chachani
[[ $? -ne 0 ]] && exit -1


# Post build process.
echo_section "Processing post-build process"
export BIOS_FV_PATH=${WORKSPACE}/Build/ChachaniBoardPkg/${BUILD_TYPE}_${TOOLCHAIN_TAG}/FV
export COMPRESS_TOOL_PATH=${WORKSPACE}/edk2-non-osi/Silicon/AMD/VanGogh/FspBlobs/AmdTools/CompressBios
export PEI_COMPRESS_SIZE=120000
[[ "${COMPRESS_FSP_REGION}" == "FALSE" ]] && export PEI_COMPRESS_SIZE=100000
pushd ${PROJECT_PKG}; python3 py-GenerateBiosVersion.py && (exit -1); popd
${COMPRESS_TOOL_PATH}/CompressBios.lnx64 ${BIOS_FV_PATH}/CHACHANISPHPEI.fd \
    ${BIOS_FV_PATH}/CHACHANISPHPEICOMPRESS.fd ${PEI_COMPRESS_SIZE}
[[ $? -ne 0 ]] && exit -1
cat ${BIOS_FV_PATH}/CHACHANI.fd ${BIOS_FV_PATH}/CHACHANISPHPEICOMPRESS.fd > \
    ${BIOS_FV_PATH}/CHACHANIMERGED.fd
mv -f ${BIOS_FV_PATH}/CHACHANIMERGED.fd ${BIOS_FV_PATH}/CHACHANI.fd
#PSP Build
export PSP_PLATFORM_PATH=${WORKSPACE}/edk2-non-osi/Silicon/AMD/VanGogh/Firmwares
export PSP_CONFIG_FILE_PATH=${WORKSPACE}/${PROJECT_PKG}
export PSP_FW_PATH=${WORKSPACE}/edk2-non-osi/Silicon/AMD/VanGogh/Firmwares
export PSPKIT_PATH=${WORKSPACE}/edk2-non-osi/Silicon/AMD/VanGogh/FspBlobs/AmdTools
export PSP_TEMP_PATH=${WORKSPACE}/Build/ChachaniBoardPkg/NewPspKit
export CUSTOM_APCB_PATH=${WORKSPACE}/edk2-non-osi/Silicon/AMD/VanGogh/FspBlobs/Apcb
export RTM_FILE=FV_COMBINE.fv
cat ${BIOS_FV_PATH}/FVMAIN_COMPACT.Fv ${BIOS_FV_PATH}/RECOVERYFV.Fv >  ${BIOS_FV_PATH}/${RTM_FILE}
rm -rf ${PSP_TEMP_PATH};mkdir -p ${PSP_TEMP_PATH}
cp -R ${PSPKIT_PATH}/* ${PSP_TEMP_PATH}
cp ${BIOS_FV_PATH}/CHACHANI.fd                     ${PSP_TEMP_PATH}
cp ${BIOS_FV_PATH}/${RTM_FILE}                     ${PSP_TEMP_PATH}
cp -R ${PSP_PLATFORM_PATH}/*                       ${PSP_TEMP_PATH}
cp -R ${CUSTOM_APCB_PATH}/*                        ${PSP_TEMP_PATH}
export PSPKIT_PATH=${PSP_TEMP_PATH}
export BIOS_IMAGE_CONFIG_FILE=${PSP_CONFIG_FILE_PATH}/BIOSImageDirectory32M.xml
[[ "${COMPRESS_FSP_REGION}" == "FALSE" ]] && \
    export BIOS_IMAGE_CONFIG_FILE=${PSP_CONFIG_FILE_PATH}/BIOSImageDirectory32M_no_compress_fsp.xml
export OUTPUT_BIOS=${BIOSNAME}UDK-temp.FD
pushd ${PSP_TEMP_PATH}
${PSPKIT_PATH}/PspDirectoryTool/BuildPspDirectory.lnx64 bb \
    CHACHANI.fd ${BIOS_IMAGE_CONFIG_FILE} ${OUTPUT_BIOS}
[[ $? -ne 0 ]] && exit -1
popd


# Generating A/B image.
echo_section "Generating A/B image"
export BINARY_BUILD_PATH=${WORKSPACE}/Build/ChachaniBoardPkg/${BUILD_TYPE}_${TOOLCHAIN_TAG}/IA32/VanGoghCommonPkg/Flash_AB

export F1_ECSIG=${WORKSPACE}/${PROJECT_PKG}/Binary/EC/EcSig.bin
export F2_EC=${WORKSPACE}/${PROJECT_PKG}/Binary/EC/ChachaniEC.bin
export F3_EFS=${BINARY_BUILD_PATH}/NewEFS/NewEFS/OUTPUT/NewEFS.bin
export F4_PSP_L1_DIRECTORY=${BINARY_BUILD_PATH}/PspL1Directory/PspL1Directory/OUTPUT/PspL1Directory.bin
export F5_PD=${WORKSPACE}/${PROJECT_PKG}/Binary/PD/TIPD.bin
export F6_SLOT_HEADER_1=${BINARY_BUILD_PATH}/ImageSlotHeader/ImageSlotHeader_1/OUTPUT/ImageSlotHeader_1.bin
export F7_SLOT_HEADER_2=${BINARY_BUILD_PATH}/ImageSlotHeader/ImageSlotHeader_2/OUTPUT/ImageSlotHeader_2.bin
export F8_SLOT_A=${PSP_TEMP_PATH}/Output/${OUTPUT_BIOS}
export F9_SLOT_B=${PSP_TEMP_PATH}/Output/${OUTPUT_BIOS}
export F10_OUT_IMAGE=${WORKSPACE}/${BIOSNAME}UDK.FD

pushd ${WORKSPACE}/${PROJECT_PKG}
python3 -X dev py-UpdatePspL1DirCksm.py ${F4_PSP_L1_DIRECTORY}
python3 FlashABImage32M.py ${F1_ECSIG} ${F2_EC} ${F3_EFS} ${F4_PSP_L1_DIRECTORY} ${F5_PD} \
    ${F6_SLOT_HEADER_1} ${F7_SLOT_HEADER_2} ${F8_SLOT_A} ${F9_SLOT_B} ${F10_OUT_IMAGE}
popd

echo_section "Generating Capsule image"
rm -r ${WORKSPACE}/Build/ChachaniBoardPkg/${BUILD_TYPE}_${TOOLCHAIN_TAG}/FV/SYSTEMFIRMWAREUPDATECARGO*
touch ${WORKSPACE}/Build/ChachaniBoardPkg/${BUILD_TYPE}_${TOOLCHAIN_TAG}/FV/SYSTEMFIRMWAREUPDATECARGO.Fv
build -p ${PROJECT_PKG}/PlatformCapsule.dsc -t ${TOOLCHAIN_TAG} -b ${BUILD_TYPE} -D BIOS_FILE=${BIOSNAME}UDK.FD
[[ $? -ne 0 ]] && exit -1
cp ${WORKSPACE}/Build/ChachaniBoardPkg/${BUILD_TYPE}_${TOOLCHAIN_TAG}/FV/${OTA_CAPSULE_NAME}.Cap .

echo_section "Build success @ $(date)"
