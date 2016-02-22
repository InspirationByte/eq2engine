# Android Native Library makefile
# compile equilibrium engine modules

PROJ_PATH:= $(call my-dir)
SRC_PATH:= $(call my-dir)/..
DEP_LIBS_MK:= $(SRC_PATH)/android_lib_mk

# glue is needed
include $(NDK_ROOT)/sources/android/native_app_glue/Android.mk

# include modules
include $(PROJ_PATH)/coreLib.mk
include $(PROJ_PATH)/prevLib.mk
include $(PROJ_PATH)/eqCore.mk

include $(DEP_LIBS_MK)/jpeg.mk
include $(DEP_LIBS_MK)/png.mk
include $(DEP_LIBS_MK)/ogg.mk
include $(DEP_LIBS_MK)/vorbis.mk

include $(DEP_LIBS_MK)/OpenAL-MOB.mk
include $(DEP_LIBS_MK)/Shiny.mk

include $(PROJ_PATH)/eqMatSystem.mk
include $(PROJ_PATH)/eqBaseShaders.mk
include $(PROJ_PATH)/eqNullRHI.mk
include $(PROJ_PATH)/eqGLRHI.mk
include $(SRC_PATH)/src_dependency/SDL2/Android.mk

# add LuaJIT prebuilt
include $(CLEAR_VARS)

LOCAL_MODULE	:= libluajit
LOCAL_SRC_FILES	:= $(SRC_PATH)/obj/local/$(TARGET_ARCH_ABI)/lib/libluajit-5.1.a
LOCAL_EXPORT_C_INCLUDES := $(SRC_PATH)/obj/local/$(TARGET_ARCH_ABI)/include

include $(PREBUILT_STATIC_LIBRARY)


# include $(PROJ_PATH)/Game_DrvSyn.mk
