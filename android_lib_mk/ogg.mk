LOCAL_PATH := $(SRC_PATH)/src_dependency/libogg
include $(CLEAR_VARS)
LOCAL_MODULE := ogg

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include $(SRC_PATH)/src_dependency/libogg/include
LOCAL_C_INCLUDES := $(LOCAL_EXPORT_C_INCLUDES)

LOCAL_SRC_FILES := \
  src/bitwise.c \
  src/framing.c

include $(BUILD_STATIC_LIBRARY)
