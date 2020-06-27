##
## libeqCore shared library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= eqCore
LOCAL_MODULE_FILENAME	:= libeqCore
LOCAL_CFLAGS    		:= -DCROSSLINK_LIB -DPPMEM_DISABLE -DCORE_EXPORT -DDLL_EXPORT -DSUPRESS_DEVMESSAGES
#LOCAL_LDFLAGS			:= -Wl,-soname=libeqCore.so

LOCAL_LDFLAGS			:= -pthread -llog

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public

LOCAL_SRC_FILES := \
	Core/platform/Platform.cpp	\
	Core/platform/MessageBox.cpp	\
	Core/CmdLineParser.cpp		\
	Core/ConCommandFactory.cpp	\
	Core/CoreVersion.cpp		\
	Core/DPKFileReader.cpp		\
	Core/DkCore.cpp				\
	Core/ExceptionHandler.cpp	\
	Core/FileSystem.cpp			\
	Core/Localize.cpp			\
	Core/debuginterface.cpp		\
	Core/eqCPUServices.cpp		\
	Core/ppmem.cpp

LOCAL_STATIC_LIBRARIES := coreLib prevLib

include $(BUILD_SHARED_LIBRARY)
