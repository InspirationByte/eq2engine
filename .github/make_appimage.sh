#!/bin/bash
APPIMAGE_SRC_FOLDER=${1}
GAME_EXE=${2}

APPDIR=Game.AppDir
BINDIR=${APPDIR}/bin${BUILD_PLATFORM:1}linux

# prep folders
cp -R ${APPIMAGE_SRC_FOLDER} ${APPDIR}
mkdir -p ${BINDIR}

# add binaries
cp bin/${BUILD_PLATFORM}/${BUILD_CONFIGURATION_U}/*.so ${BINDIR}
cp bin/${BUILD_PLATFORM}/${BUILD_CONFIGURATION_U}/${GAME_EXE} ${BINDIR}

appimagetool ${APPDIR}