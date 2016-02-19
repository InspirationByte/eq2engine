##
## libcoreLib static library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(PROJ_PATH)/..
NDK_APP_OUT				:= $(PROJ_PATH)/libs_android

LOCAL_MODULE    		:= libcoreLib
LOCAL_MODULE_FILENAME	:= libcoreLib
LOCAL_CFLAGS    		:= -DCROSSLINK_LIB -DANDROID -std=c++11 -pthread
LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public/ \
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform

LOCAL_SRC_FILES := \
	public/core/cmd_pacifier.cpp \
	public/core/ConCommandBase.cpp \
	public/core/ConCommand.cpp \
	public/core/ConVar.cpp

include $(BUILD_STATIC_LIBRARY)
