LOCAL_PATH := $(SRC_PATH)/src_dependency/Shiny

include $(CLEAR_VARS)
LOCAL_MODULE 		:= Shiny
LOCAL_MODULE_FILENAME	:= libShiny
LOCAL_CFLAGS    	:= -std=c++98

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_C_INCLUDES := $(LOCAL_EXPORT_C_INCLUDES)

LOCAL_SRC_FILES := \
	src/ShinyManager.c \
	src/ShinyNode.c \
	src/ShinyNodePool.c \
	src/ShinyNodeState.c \
	src/ShinyOutput.c \
	src/ShinyTools.c \
	src/ShinyZone.c

include $(BUILD_STATIC_LIBRARY)
