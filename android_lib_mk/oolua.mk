LOCAL_PATH := $(SRC_PATH)/src_dependency/oolua

include $(CLEAR_VARS)
LOCAL_MODULE := oolua

LOCAL_CFLAGS := -O2 -Wall -fsigned-char

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

SRC_FILES := $(wildcard $(LOCAL_PATH)/src/*.cpp)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES := $(SRC_FILES)

LOCAL_C_INCLUDES := $(LOCAL_EXPORT_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)
