#!/bin/bash

PLATFORM=x64
CONFIGURATION=Release
EXE_SUFFIX=_$CONFIGURATION
LIB_PATHS=
LIB_NAMES_PREFIX=
DESTINATION_PATH=lib

LIB_PATHS=$LIB_PATHS$(pwd)/src_dependency/ffmpeg/lib
LIB_NAMES_PREFIX+=(libopenal libav libsw)

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIB_PATHS

mkdir $DESTINATION_PATH
for lib in ${LIB_NAMES_PREFIX[@]}
do
    cp -Lf $(ldd bin/${PLATFORM}/${CONFIGURATION}/DrvSyn${EXE_SUFFIX} | awk '/ => / { print $3 }' | grep ${lib}) $DESTINATION_PATH
done
