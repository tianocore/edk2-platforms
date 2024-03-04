#
# Copyright (c) 2024, ARM Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Generating GPT paritioned image supporting firmware update.
#

#!/bin/bash

PROGNAME=$(basename $0)
FLASH_IMAGE_FILE="$(realpath ./flash_gpt.img)"
FWU_METADATA_FILE="$(realpath ./fwu_metadata.dat)"
FIP_IMAGE_FILE="$(realpath ./fip_fvp.bin)"
OVERWRITE_MODE=0

IMAGE_TYPE_UUID="49757D90-6C22-11EE-A556-1757EBA0420C"
FWU_METADATA_TYPE_UUID="8a7a84a0-8387-40f6-ab41-a8b9a5a60d23"
DISK_UUID=`uuidgen`

SECTOR_SIZE=512
MAX_FIP_IMAGE_SIZE=$((20 * 1024 * 1024))
MAX_FWU_METADATA_SIZE=$((1 * 1024 * 1024))

OPTIONS_STRING="hm:f:oO:"
OPTIONS_LIST=(
  "help"
  "metadata:"
  "fip_image:"
  "overwrite"
  "output:"
)

opts=$(getopt \
  --longoptions "$(printf "%s," "${OPTIONS_LIST[@]}")" \
  --name "$PROGNAME" \
  --options "$OPTIONS_STRING" \
  -- "$@"
)

function print_usage()
{
  echo -e "Usage: $PROGNAME [OPTIONS]"
  echo -e "Generate GPT partition image for system fip image."
  echo -e "Default output name of flash disk image is \"$FLASH_IMAGE_FILE\""
  echo -e ""
  echo -e "OPTIONS:"
  echo -e "\t-h, --help\t\tShow this help message."
  echo -e "\t-m, --metadata [FILE]\t\tFirmware metadata file name (default: ./fwu_metadata.dat)."
  echo -e "\t-f, --fip_image [FILE]\t\tFip Image filename (default: ./fip_fvp.bin)."
  echo -e "\t-o, --overwrite\t\tOverwrite flash disk image if it's already generated."
  echo -e "\t-O, --output [FILE]\t\tOutput name of flash disk image. (default: $FLASH_IMAGE_FILE)."
  echo -e ""
  echo -e "EXAMPLES:"
  echo -e "\t./$PROGNAME flash0.img"
  echo -e "\t./$PROGNAME -m ~/work/fwu_metadata.dat -f ~/work/fip_image.bin flash0.img"
}

function print_error()
{
  echo -e "ERROR: $1"
}

function create_gpt_partition()
{
  local ret=0
  local partition_number=$1
  local partition_name=$2
  local start_sector=$3
  local num_sectors=$4
  local partition_type=$5
  local unique_partition_uuid=`uuidgen`

  echo -e "Creating $partition_name partition($partition_number) on $FLASH_IMAGE_FILE..."

  sgdisk -a 1 -n $partition_number:$start_sector:+$num_sectors \
           -c $partition_number:$partition_name \
           -t $partition_number:$partition_type \
           -u $unique_partition_uuid \
           "$FLASH_IMAGE_FILE"
  if [[ $? -ne 0 ]]; then
    ret=$?
    print_error "Fail to generate $partition_name partition($partition_number)!"

    return $ret
  fi

  return 0
}

function write_gpt_partition()
{
  local ret=0
  local binary_file=$1
  local start_sector=$2
  local num_sectors=$3

  echo "Write binary $bin at sector $start_sector..."
  dd if=$binary_file of="$FLASH_IMAGE_FILE" bs=$SECTOR_SIZE seek=$start_sector \
      count=$num_sectors conv=notrunc
  if [[ $? -ne 0 ]]; then
    ret=$?
    print_error "Fail to write $binary_file at sector $start_sector..."
  fi

  return $?
}


function create_gpt_flash_image()
{
  local ret=0
  local start_sector=0
  local num_sectors=0

  # Create 64MB flash image.
  echo -e "Creating $FLASH_IMAGE_FILE..."
  dd if=/dev/zero of="$FLASH_IMAGE_FILE" iflag=fullblock bs=1M count=64 && sync
  if [[ $? -ne 0 ]]; then
    ret=$?
    print_error "Couldn't make flash image! result: $ret"

    return $ret
  fi

  echo -e "Generating GPT header on $FLASH_IMAGE_FILE..."
  sgdisk -g -U $DISK_UUID "$FLASH_IMAGE_FILE"
  if [[ $? -ne 0 ]]; then
    ret=$?
    print_error "Fail to generate GPT header: $ret"

    return $ret
  fi

  #
  # To make BL1 boot to BL2 directly in case of using BL1
  # (see tthe arm-tf/plat/arm/common/fconf/arm_fconf_io.c).
  # In case of Base FVP, the PLAT_ARM_FIP_OFFSET_IN_GPT is 34 sector
  # Offset = 34 * 512(sector_size) = 17408 i.e 0x4400
  # (see arm-tf/plat/arm/board/fvp/include/platform_def.h)
  #
  start_sector=34
  num_sectors=$((MAX_FIP_IMAGE_SIZE / SECTOR_SIZE))
  create_gpt_partition 1 "FIP_A" $start_sector $num_sectors $IMAGE_TYPE_UUID
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  write_gpt_partition "$FIP_IMAGE_FILE" $start_sector $num_sectors
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  start_sector=$((start_sector + num_sectors))
  num_sectors=$((MAX_FIP_IMAGE_SIZE / SECTOR_SIZE))
  create_gpt_partition 2 "FIP_B" $start_sector $num_sectors $IMAGE_TYPE_UUID
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  write_gpt_partition "$FIP_IMAGE_FILE" $start_sector $num_sectors
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  start_sector=$((start_sector + num_sectors))
  num_sectors=$((MAX_FWU_METADATA_SIZE / SECTOR_SIZE))
  create_gpt_partition 3 "FWU-Metadata" $start_sector $num_sectors $FWU_METADATA_TYPE_UUID
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  write_gpt_partition "$FWU_METADATA_FILE" $start_sector $num_sectors
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  start_sector=$((start_sector + num_sectors))
  num_sectors=$((MAX_FWU_METADATA_SIZE / SECTOR_SIZE))
  create_gpt_partition 4 "Bkup-FWU-Metadata" $start_sector $num_sectors $FWU_METADATA_TYPE_UUID
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  write_gpt_partition "$FWU_METADATA_FILE" $start_sector $num_sectors
  if [[ $? -ne 0 ]]; then
    return $?
  fi

  return 0
}

######################
#       Main         #
######################
eval set --$opts

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      print_usage
      exit 0
      ;;
    -o|--overwrite)
      OVERWRITE_MODE=1
      shift 1
      ;;
    -f|--fip_image)
      FIP_IMAGE_FILE="$(realpath $2)"
      shift 2
      ;;
    -m|--metadata)
      FWU_METADATA_FILE="$(realpath $2)"
      shift 2
      ;;
    -O|--output)
      FLASH_IMAGE_FILE="$(realpath $2)"
      shift 2
      ;;
    --)
      shift 1
      ;;
    *)
      print_usage
      exit 22
      ;;
  esac
done

if [[ -z "$FLASH_IMAGE_FILE" || -d "$FLASH_IMAGE_FILE" || ! -f "$FIP_IMAGE_FILE" || ! -f "$FWU_METADATA_FILE" ]]; then
  print_error "Invalid Arguments. Please check the filename."

  exit 22
fi

if [[ -f "$FLASH_IMAGE_FILE" && $OVERWRITE_MODE -ne 1 ]]; then
  print_error "flash image already exists. If you want to overwrite, please use -o option..."

  exit 8
fi

FIP_IMAGE_SIZE=$(stat -c%s "$FIP_IMAGE_FILE")
if [[ $? -ne 0 ]]; then
  print_error "Fail to get size of fip_image..."

  exit 1
fi

if [[ $FIP_IMAGE_SIZE -gt $MAX_FIP_IMAGE_SIZE ]]; then
  print_error "fip_image is too big ($FIP_IMAGE_SIZE bytes). it should be less than $MAX_FIP_IMAGE_SIZE bytes..."

  exit 7
fi

FWU_METADATA_SIZE=$(stat -c%s "$FWU_METADATA_FILE")
if [[ $? -ne 0 ]]; then
  print_error "Fail to get size of fwu_metadata file..."

  exit 1
fi

if [[ $FWU_METADATA_SIZE -gt $MAX_FWU_METADATA_SIZE ]]; then
  print_error "fwu_metadata is too big ($FWU_METADATA_SIZE bytes). it should be less than $MAX_FWU_METADATA_SIZE bytes..."

  exit 7
fi

create_gpt_flash_image
if [[ $? -ne 0 ]]; then
  print_error "Fail to create gpt flash image.. remove $FLASH_IMAGE_FILE"
  rm -f "$FLASH_IMAGE_FILE"

  exit 1
fi
