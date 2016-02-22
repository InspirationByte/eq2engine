# add LuaJIT prebuilt
include $(CLEAR_VARS)

LOCAL_MODULE	:= luajit
LOCAL_SRC_FILES	:= $(SRC_PATH)/obj/local/$(TARGET_ARCH_ABI)/lib/libluajit-5.1.a
LOCAL_EXPORT_C_INCLUDES := $(SRC_PATH)/obj/local/$(TARGET_ARCH_ABI)/include

include $(PREBUILT_STATIC_LIBRARY)
