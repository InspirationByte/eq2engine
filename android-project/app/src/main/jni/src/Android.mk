LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL2
GLM_PATH := ../glm

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
    $(LOCAL_PATH)/$(GLM_PATH)/ \
    $(LOCAL_PATH)/$(GLM_PATH)/gtc \
    $(LOCAL_PATH)/$(GLM_PATH)/gtx \
    $(LOCAL_PATH)/$(GLM_PATH)/detail \
    $(LOCAL_PATH)/$(GLM_PATH)/glm.hpp

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	Main.cpp

LOCAL_SHARED_LIBRARIES := SDL2

#LOCAL_LDLIBS := -lGLESv1_CM -lEGL -lGLESv3 -llog
LOCAL_LDLIBS := -lGLESv1_CM -lEGL -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)