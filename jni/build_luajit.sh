#!/bin/bash
###########################################################
# Builds LuaJIT for android target
###########################################################

export NDK=${HOME}"/Android/Sdk/ndk-bundle/android-ndk-r10e"
export NDK_ROOT=$NDK

EQENGINE_PATH=${PWD}/..

LUAJIT_PATH=$EQENGINE_PATH/src_dependency/luajit
LUAJIT_INSTALL=$EQENGINE_PATH/obj/local/

 build luajit using android-18 api toolchain
./mk-luajit.sh clean 16 $LUAJIT_PATH $LUAJIT_INSTALL

./mk-luajit.sh armeabi 16 $LUAJIT_PATH $LUAJIT_INSTALL
./mk-luajit.sh clean 16 $LUAJIT_PATH $LUAJIT_INSTALL

./mk-luajit.sh armeabi-v7a 16 $LUAJIT_PATH $LUAJIT_INSTALL
./mk-luajit.sh clean 16 $LUAJIT_PATH $LUAJIT_INSTALL

if [ "x$NDK" == "x" ]; then
	NDK=/opt/android-ndk
fi
if [ ! -d "$NDK" ]; then
	echo 'NDK not found. Please set NDK environment variable and have it point to the NDK dir.'
	exit 1
fi
