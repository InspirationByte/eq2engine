#!/bin/bash
APPIMAGE_SRC_FOLDER=${1}
GAME_EXE=${APP_NAME}${EXE_SUFFIX}

APPDIR=Game.AppDir
SRC_BIN_DIR=build/bin/${BUILD_PLATFORM}/${BUILD_CONFIGURATION}
TARGET_BIN_DIR=${APPDIR}/bin${BUILD_PLATFORM:1}linux

# prep folders
cp -R ${APPIMAGE_SRC_FOLDER} ${APPDIR}
mkdir -p ${TARGET_BIN_DIR}

# add binaries
cp ${SRC_BIN_DIR}/*.so ${TARGET_BIN_DIR}
cp ${SRC_BIN_DIR}/${GAME_EXE} ${TARGET_BIN_DIR}

LIB_PATHS=$(pwd)/src_dependency/ffmpeg/lib
LIB_NAMES_PREFIX=(libopenal libav libsw)

# ldd needs this
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIB_PATHS

for lib in ${LIB_NAMES_PREFIX[@]}
do
    cp -Lf $(ldd ${SRC_BIN_DIR}/${GAME_EXE} | awk '/ => / { print $3 }' | grep ${lib}) ${TARGET_BIN_DIR}
done

appimagetool ${APPDIR}