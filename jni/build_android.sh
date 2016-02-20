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
MAKEFILE_PATH=${PWD}/Application.mk

cd $NDK_ROOT
./ndk-build NDK_PROJECT_PATH=$EQENGINE_PATH NDK_APPLICATION_MK=$MAKEFILE_PATH
