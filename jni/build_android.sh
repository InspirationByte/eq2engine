#!/bin/bash
###########################################################
# Builds parts of Equilibrium Engine 
# and Driver Syndicate itself.
#
# For Android.
#
###########################################################

# temporary
export NDK_ROOT=${HOME}"/Android/Sdk/ndk-bundle"
EQENGINE_PATH=${PWD}/..
EQENGINE_ANDROID_OUT=$EQENGINE_PATH/libs_android
MAKEFILE_PATH=${PWD}/Application.mk

cd $NDK_ROOT
./ndk-build NDK_PROJECT_PATH=$EQENGINE_PATH NDK_APPLICATION_MK=$MAKEFILE_PATH APP_PLATFORM=android-15 NDK_OUT=$EQENGINE_ANDROID_OUT
