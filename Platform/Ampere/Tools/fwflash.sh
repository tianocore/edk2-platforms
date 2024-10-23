#!/usr/bin/env bash

# Flashes the SPI-NOR of an Ampere-based machine via the BMC.
# The BMC must be running OpenBMC.

set -e

BMC_ENV_FILE=bmc.sh

usage () {
	echo "Copies firmware to the BMC (running OpenBMC) and runs ampere_flash_bios.sh to flash the host."
	echo "Usage:"
	echo "  $(basename "$0") [options] <Host firmware file>"
	echo
	echo "Options:"
	echo "  -f, --full                       Flash the entire SPI-NOR chip (default)"
	echo "  -c, --code                       Flash the code (TF-A + UEFI) area"
	echo "  -t, --tfa                        Flash the TF-A (ATF) area"
	echo "  -u, --uefi                       Flash the UEFI area"
	echo "  -y, --yes                        Don't show warning about BMC compatibility requirements"
	echo "  -h, --help                       This help message"
	echo ""
	echo "Note: TF-A (Trusted Firmware for ARMv8-A) is the same as ATF (ARM Trusted Firmware)."
	exit 1
}

if ! [ -f "${BMC_ENV_FILE}" ]; then
  echo "bmc.sh does not exist!"
  echo
  echo "Please create it, with contents such as:"
  echo
  echo "  export BMC_HOST=192.168.0.10"
  echo "  export BMC_USER=root"
  echo "  export BMC_PASS=0penBmc"
  exit 1
fi

. "./${BMC_ENV_FILE}"

if [ -z "${BMC_USER}" ] || [ -z "${BMC_PASS}" ] || [ -z "${BMC_HOST}" ]; then
  echo "${BMC_ENV_FILE} is not correct. BMC_HOST, BMC_USER and/or BMC_PASS is not configured."
  exit 1
fi

export BMC_HOST
export BMC_USER
export BMC_PASS

ACCEPT_PROMPTS=no

OPTIONS=$(getopt -o fctuyh --long full,code,tfa,uefi,yes,help -- "$@")
eval set -- "${OPTIONS}"

while true; do
	case "$1" in
		-f|--full)
			FLASH_ARG="-f"; shift;;
		-c|--code)
			FLASH_ARG="-c"; shift;;
		-t|--tfa)
			FLASH_ARG="-t"; shift;;
		-u|--uefi)
			FLASH_ARG="-u"; shift;;
		-y|--yes)
			ACCEPT_PROMPTS=yes; shift;;
		-h|--help)
			usage;;
		--) shift; break;;
		*) echo "Internal error ($1)!"; exit 1;;
	esac
done

if [ "$#" -eq 1 ] && [ -n "$1" ]; then
	IMAGE="$1"
else
	usage
fi

if [ "${ACCEPT_PROMPTS}" = "no" ]; then
	echo "Warning: this script only works on Ampere systems running OpenBMC with tag 'bexcran'."
	echo -n "Do you want to continue (y/n): "
	read -r cont
	while [ "$cont" != "y" ] && [ "$cont" != "n" ]; do
		echo -n "Please enter 'y' or 'n': "
		read -r cont
	done
	if [ "$cont" != "y" ]; then
		exit 0
	fi
fi
	

if [ -z "${FLASH_ARG}" ]; then
  usage
fi

eval set -- ""

if ! [ -f "${IMAGE}" ]; then
  echo "ERROR: file \"$1\" does not exist."
  exit 1
fi

DIR=$(dirname "${IMAGE}")
FILE=$(basename "${IMAGE}")

expect "$(dirname ${BASH_SOURCE[0]})/fwflash.exp" "${FLASH_ARG}" "${DIR}" "${FILE}"
