#!/bin/bash

#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

${GCC5_AARCH64_PREFIX}objcopy -I binary -B aarch64 -O elf64-littleaarch64 ${WORKSPACE}/Build/RB3Gen2/${1}_GCC/FV/RB3GEN2_EFI.fd edk2-elf.o
${GCC5_AARCH64_PREFIX}ld edk2-elf.o -o edk2.elf -EL -T ${WORKSPACE}/edk2-platforms/Silicon/Qualcomm/qcom-edk2-elf.lds --defsym=ELFENTRY=0x9fc00000 -Ttext=0x9fc00000
rm -f edk2-elf.o

if [ ! -d "${WORKSPACE}/qtestsign" ]; then
	git clone https://github.com/msm8916-mainline/qtestsign.git ${WORKSPACE}/qtestsign
fi

${WORKSPACE}/qtestsign/qtestsign.py -v6 aboot -o uefi.elf edk2.elf
rm -f edk2.elf
