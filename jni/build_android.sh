#!/bin/bash
###########################################################
# Builds parts of Equilibrium Engine 
# and Driver Syndicate itself.
#
# For Android.
#
###########################################################

export NDK=${HOME}"/Android/Sdk/ndk-bundle/android-ndk-r10e"
export NDK_ROOT=$NDK

EQENGINE_PATH=${PWD}/..
MAKEFILE_PATH=${PWD}/Application.mk

LUAJIT_PATH=$EQENGINE_PATH/src_dependency/luajit
LUAJIT_INSTALL=$EQENGINE_PATH/obj/local/

# build luajit using android-18 api toolchain
./mk-luajit.sh armeabi-v7a 18 $LUAJIT_PATH $LUAJIT_INSTALL
./mk-luajit.sh armeabi 18 $LUAJIT_PATH $LUAJIT_INSTALL

cd $NDK_ROOT
./ndk-build NDK_PROJECT_PATH=$EQENGINE_PATH NDK_APPLICATION_MK=$MAKEFILE_PATH
