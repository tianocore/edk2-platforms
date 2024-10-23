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
VER="$(date +%Y.%m.%d)"
