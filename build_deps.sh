#!/bin/sh

TIMESTAMP="$(date +'%Y-%m-%dT%H:%M:%S')"

info() {
  echo "${TIMESTAMP} INFO $@" | tee -a ${LOG_FILE}
}

err() {
  echo "${TIMESTAMP} ERROR $@" >&2
}

make_clean() {
  local makefile_dir="$1"
  make -C ${makefile_dir} clean
}

make_libfreenect() {
  local build_opts="$@"

  if [[ "${CLEAN}" == "true" ]] ; then
    info "Cleaning up libfreenect ..."
    make_clean ${LIBFREENECT_BUILD_DIR} >> ${LOG_FILE} 2>&1 || return
  fi
  info "Building libfreenect ..."
  mkdir -p ${LIBFREENECT_BUILD_DIR} >> ${LOG_FILE} 2>&1 || return
  cmake -S ${LIBFREENECT_DIR} -B ${LIBFREENECT_BUILD_DIR} \
           ${build_opts} >> ${LOG_FILE} 2>&1 || return
  make -C ${LIBFREENECT_BUILD_DIR} >> ${LOG_FILE} 2>&1 || return
}

build_libfreenect() {
  local libs_dir="$1"
  local libs_fakenect_dir=${libs_dir}/fakenect

  make_libfreenect || return
  info "Copying libfreenect to ${libs_dir} ..."
  cp ${LIBFREENECT_BUILD_DIR}/lib/libfreenect.dylib ${libs_dir} >> ${LOG_FILE} 2>&1 || return
  info "Done."

  info "Copying libfakenect as libfreenect to ${libs_fakenect_dir} ..."
  mkdir -p ${libs_fakenect_dir} >> ${LOG_FILE} 2>&1 || return
  cp ${LIBFREENECT_BUILD_DIR}/lib/fakenect/libfakenect.dylib \
     ${libs_fakenect_dir}/libfreenect.dylib >> ${LOG_FILE} 2>&1 || return
  info "Done."
}

build_libfreenect_driver() {
  local libs_dir="$1"
  local libfreenect_driver_build_dir=${LIBFREENECT_BUILD_DIR}/lib/OpenNI2-FreenectDriver

  info "Building libFreenectDriver ..."
  make_libfreenect "-DBUILD_OPENNI2_DRIVER=ON" || return

  info "Copying libFreenect driver to ${BUILD_LIBS_OPENNI2_DRIVERS_DIR} ..."
  cp ${libfreenect_driver_build_dir}/libFreenectDriver.dylib \
     ${BUILD_LIBS_OPENNI2_DRIVERS_DIR} >> ${LOG_FILE} 2>&1 || return
  info "Done."
}

build_libopenni2() {
  local libs_dir="$1"
  local libopenni2_build_dir=${OPENNI2_DIR}/Bin/x64-Release
  local libopenni2_drivers_build_dir=${libopenni2_build_dir}/OpenNI2/Drivers

  if [[ "${CLEAN}" == "true" ]] ; then
    info "Cleaning up libOpenNI2 ..."
    make_clean ${OPENNI2_DIR} >> ${LOG_FILE} 2>&1 || return
  fi
  info "Building libOpenNI2 ...."
  make -C ${OPENNI2_DIR} main >> ${LOG_FILE} 2>&1 || return
  info "Copying libOpenNI2 to ${libs_dir} ..."
  cp ${libopenni2_build_dir}/libOpenNI2.dylib ${libs_dir} >> ${LOG_FILE} 2>&1 || return
  info "Done."

  mkdir -p ${BUILD_LIBS_OPENNI2_DRIVERS_DIR} >> ${LOG_FILE} 2>&1 || return
  info "Copying libOniFile driver to ${BUILD_LIBS_OPENNI2_DRIVERS_DIR} ..."
  cp ${libopenni2_drivers_build_dir}/libOniFile.dylib \
     ${BUILD_LIBS_OPENNI2_DRIVERS_DIR} >> ${LOG_FILE} 2>&1 || return
  info "Done."
}

copy_libnite2() {
  local libs_dir="$1"
  local nite2_data_dir="$(realpath ${NITE2_DIR}/Data)"

  info "Copying libNiTE2 to ${libs_dir} ..."
  cp ${NITE2_DIR}/libNiTE2.dylib ${libs_dir} >> ${LOG_FILE} 2>&1 || return
  install_name_tool -id "@rpath/libNiTE2.dylib" \
    ${libs_dir}/libNiTE2.dylib >> ${LOG_FILE} 2>&1 || return
  install_name_tool -change "@executable_path/libOpenNI2.dylib" "@rpath/libOpenNI2.dylib" \
    ${libs_dir}/libNiTE2.dylib >> ${LOG_FILE} 2>&1 || return
  info "Done."

  info "Setting up libNiTE2 Data directory..."
  local new_data_dir="$(realpath -s ${libs_dir}/NiTE2)"
  ln -s ${nite2_data_dir} ${libs_dir}/NiTE2 >> ${LOG_FILE} 2>&1 || return
  cat ${NITE2_DIR}/NiTE.ini | sed -e "s~\(DataDir=\).*~\1${new_data_dir}~g" \
    > ${BUILD_BIN_DIR}/NiTE.ini
  info "Done."
}

build_openni2_device_deps() {
  info "Building ${DEVICE} deps..."
  if ! build_libopenni2 ${BUILD_LIBS_DIR} ; then
    err "Couldn't build libOpenNI2. See \`${LOG_FILE}\` for more details."
    exit 1
  fi
  if ! copy_libnite2 ${BUILD_LIBS_DIR} ; then
    err "Couldn't copy libNiTE2. See \`${LOG_FILE}\` for more details."
    exit 1
  fi
  if ! build_libfreenect_driver ${BUILD_LIBS_DIR} ; then
    err "Couldn't build libFreenectDriver. See \`${LOG_FILE}\` for more details."
    exit 1
  fi
  info "Done building ${DEVICE} deps."
}

build_openkinect_device_deps() {
  info "Building ${DEVICE} deps ..."
  if ! build_libfreenect ${BUILD_LIBS_DIR} ; then
    err "Couldn't build libfreenect. See \`${LOG_FILE}\` for more details."
    exit 1
  fi
  info "Done building ${DEVICE} deps."
}

usage() {
  cat << USAGE
Usage: $0 [OPTIONS] <device>

Builds the necessary dependencies for the provided <device>.
Value for <device> can be any of: OpenKinect | OpenNI2

Options:
  -c                Force a 'make clean' before building each dependencies.
  -o  <directory>   Destination <directory> for the built dependencies (defaults to build/libs).
  -h                Show this message.
USAGE
}

main() {
  DEVICE="$1"
  readonly DEVICE

  if [[ -e "${LOG_FILE}" ]]; then
    rm ${LOG_FILE}
    touch ${LOG_FILE}
  fi
  
  if [[ -z "${DEVICE}" ]]; then
    echo "$0: Need to provide the <device> to build dependencies for.\n" >&2
    usage
    exit 1
  fi

  case "${DEVICE}" in
    OpenNI2) build_openni2_device_deps ;;
    OpenKinect) build_openkinect_device_deps ;;
    *)
      echo "$0: Unrecognized device \`${DEVICE}\`. <device> must be any of: OpenNI2 | OpenKinect.\n" >&2
      usage
      exit 1
      ;;
  esac
}

readonly LOG_FILE=build_deps.log
readonly OPENNI2_DIR=libs/OpenNI2
readonly NITE2_DIR=libs/NiTE2
readonly LIBFREENECT_DIR=libs/libfreenect
readonly LIBFREENECT_BUILD_DIR=${LIBFREENECT_DIR}/build
readonly BUILD_BIN_DIR=build/bin
BUILD_LIBS_DIR=build/libs
DEVICE=''
CLEAN='false'

while getopts "ch:o:" flag; do
  case "${flag}" in
    c) CLEAN='true' ;;
    h)
      usage
      exit 0
      ;;
    o) BUILD_LIBS_DIR="${OPTARG}" ;;
    \?)
      usage
      exit 1
  esac
done
shift $((OPTIND - 1))

readonly CLEAN
readonly BUILD_LIBS_DIR
readonly BUILD_LIBS_OPENNI2_DRIVERS_DIR=${BUILD_LIBS_DIR}/OpenNI2/Drivers

main $@