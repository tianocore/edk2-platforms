touch .fw_bld
BUILD="$(cat .fw_bld)"

if [ "${BUILD}" = "" ]; then
  BUILD="1"
  echo ${BUILD} > .fw_bld
elif [ "$1" == "UPDATE" ]; then
  BUILD=$((BUILD + 1))
  echo ${BUILD} > .fw_bld
fi

MAJOR_VER="$(date +%y)"
MINOR_VER="$(date +%m)"
MICRO_VER="$(date +%d)"
VER="${MAJOR_VER}.${MINOR_VER}.${MICRO_VER}-$(printf '%02d' ${BUILD})"
YHEX=$(printf '%03x' $(date +%y))
MHEX=$(printf '%01x' $(date +%m))
DHEX=$(printf '%02x' $(date +%e))
BHEX=$(printf '%02x' ${BUILD})
VER_HEX=0x${YHEX}${MHEX}${DHEX}${BHEX}
