##
## libPrevLib static library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(PROJ_PATH)/..
NDK_APP_OUT				:= $(PROJ_PATH)/libs_android

LOCAL_MODULE    		:= libPrevLib
LOCAL_MODULE_FILENAME	:= libPrevLib
LOCAL_CFLAGS    		:= -DCROSSLINK_LIB -DANDROID -std=c++11 -pthread -fexceptions
LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public/ \
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public/utils\
	$(LOCAL_PATH)/public/platform \
	$(LOCAL_PATH)/public/network\
	$(LOCAL_PATH)/public/math\
	$(LOCAL_PATH)/public/imaging\
	$(LOCAL_PATH)/public						\
	$(LOCAL_PATH)/src_dependency/luajit/src		\
	$(LOCAL_PATH)/src_dependency/oolua/include

LOCAL_SRC_FILES := \
	public/utils/CRC32.cpp\
	public/platform/Platform.cpp\
	public/network/net_defs.cpp\
	public/network/c_udp.cpp\
	public/network/NetMessageBuffer.cpp\
	public/network/NetInterfaces.cpp\
	public/math/math_util.cpp\
	public/utils/strtools.cpp\
	public/utils/eqwstring.cpp\
	public/utils/eqtimer.cpp\
	public/utils/eqthread.cpp\
	public/utils/eqstring.cpp\
	public/utils/Tokenizer.cpp\
	public/utils/KeyValues.cpp\
	public/math/FVector.cpp\
	public/math/FixedMath.cpp\
	public/math/Volume.cpp\
	public/math/Vector.cpp\
	public/math/Random.cpp\
	public/VirtualStream.cpp\
	public/math/Quaternion.cpp\
	public/math/QuadTree.cpp\
	public/math/Matrix.cpp\
	public/imaging/ImageLoader.cpp

include $(BUILD_STATIC_LIBRARY)
