APP_PROJECT_PATH := $(call my-dir)
APP_MODULES      := libcoreLib libPrevLib
APP_ABI          := armeabi armeabi-v7a x86 x86_64
APP_STL			 := gnustl_static
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/Android.mk
