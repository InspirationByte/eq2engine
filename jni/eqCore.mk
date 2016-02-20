##
## libeqCore static library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)
NDK_APP_OUT				:= $(PROJ_PATH)/libs_android

LOCAL_MODULE    		:= eqCore
LOCAL_MODULE_FILENAME	:= libeqCore
LOCAL_CFLAGS    		:= -DCROSSLINK_LIB -DANDROID -DCORE_EXPORT -DDLL_EXPORT -DSUPRESS_DEVMESSAGES -std=c++11 -pthread -fexceptions
#LOCAL_LDFLAGS			:= -Wl,-soname=libeqCore.so
LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public

LOCAL_SRC_FILES := \
	Core/CmdLineParser.cpp		\
	Core/ConCommandFactory.cpp	\
	Core/CoreVersion.cpp		\
	Core/DPKFileReader.cpp		\
	Core/DPKFileWriter.cpp		\
	Core/DkCore.cpp				\
	Core/ExceptionHandler.cpp	\
	Core/FileSystem.cpp			\
	Core/Localize.cpp			\
	Core/debuginterface.cpp		\
	Core/eqCPUServices.cpp		\
	Core/ppmem.cpp

LOCAL_STATIC_LIBRARIES := coreLib prevLib

include $(BUILD_SHARED_LIBRARY)
