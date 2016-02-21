##
## libcoreLib static library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= coreLib
LOCAL_MODULE_FILENAME	:= coreLib
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
