LOCAL_PATH := $(SRC_PATH)/src_dependency/oolua

include $(CLEAR_VARS)
LOCAL_MODULE := oolua

LOCAL_CFLAGS := -O2 -Wall -fsigned-char

SRC_FILES := $(wildcard $(LOCAL_PATH)/src/*.cpp)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES := $(SRC_FILES)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/

include $(BUILD_STATIC_LIBRARY)
