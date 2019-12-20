# add LuaJIT prebuilt
include $(CLEAR_VARS)

LOCAL_MODULE	:= luajit
LOCAL_SRC_FILES	:= $(SRC_PATH)/src_dependency/lua/$(TARGET_ARCH_ABI)/lib/libluajit-5.1.a
LOCAL_EXPORT_C_INCLUDES := $(SRC_PATH)/src_dependency/lua/src

include $(PREBUILT_STATIC_LIBRARY)
