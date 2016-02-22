#!/bin/bash
###########################################################
# Builds LuaJIT for android target
###########################################################

export NDK=${HOME}"/Android/Sdk/ndk-bundle" #/android-ndk-r10e"
export NDK_ROOT=$NDK

EQENGINE_PATH=${PWD}/..

LUAJIT_PATH=$EQENGINE_PATH/src_dependency/luajit
LUAJIT_INSTALL=$EQENGINE_PATH/obj/local/

 build luajit using android-18 api toolchain
./mk-luajit.sh clean 18 $LUAJIT_PATH $LUAJIT_INSTALL

./mk-luajit.sh armeabi 18 $LUAJIT_PATH $LUAJIT_INSTALL
./mk-luajit.sh clean 18 $LUAJIT_PATH $LUAJIT_INSTALL

./mk-luajit.sh armeabi-v7a 18 $LUAJIT_PATH $LUAJIT_INSTALL
./mk-luajit.sh clean 18 $LUAJIT_PATH $LUAJIT_INSTALL
