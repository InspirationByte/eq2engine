# Android Native Library makefile
# compile equilibrium engine modules

PROJ_PATH:= $(call my-dir)
DEP_LIBS_MK:= $(PROJ_PATH)/../android_lib_mk

include $(DEP_LIBS_MK)/jpeg.mk

include $(PROJ_PATH)/coreLib.mk
include $(PROJ_PATH)/prevLib.mk

#include $(CLEAR_VARS)
#LOCAL_MODULE := libpng_static
#LOCAL_MODULE_FILENAME := png
#LOCAL_SRC_FILES := $(EQ_LIBS)/libpng.a
#include $(PREBUILT_STATIC_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := libpng_static
#LOCAL_MODULE_FILENAME := png
#LOCAL_SRC_FILES := $(EQ_LIBS)/libpng.a
#include $(PREBUILT_STATIC_LIBRARY)


# $(call import-module, SDL)

