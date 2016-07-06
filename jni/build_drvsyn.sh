#!/bin/bash
###########################################################
# Builds parts of Equilibrium Engine 
# and Driver Syndicate itself.
#
# For Android.
#
###########################################################

export NDK=${HOME}"/Android/Sdk/ndk-bundle"
export NDK_ROOT=$NDK

EQENGINE_PATH=${PWD}/..
MAKEFILE_PATH=${PWD}/Application.mk

LUAJIT_PATH=$EQENGINE_PATH/src_dependency/luajit
LUAJIT_INSTALL=$EQENGINE_PATH/obj/local/

cd $NDK_ROOT
./ndk-build NDK_PROJECT_PATH=$EQENGINE_PATH NDK_APPLICATION_MK=$MAKEFILE_PATH
