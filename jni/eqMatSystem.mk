##
## libeqMatSystem shared library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= eqMatSystem
LOCAL_MODULE_FILENAME	:= libeqMatSystem
LOCAL_CFLAGS    		:= -DANDROID -std=c++11 -pthread -fexceptions

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public\
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public/materialsystem\
	$(LOCAL_PATH)/public/materialsystem/renderers


LOCAL_SRC_FILES := \
	MaterialSystem/materialvar.cpp		\
	MaterialSystem/material.cpp		\
	MaterialSystem/MaterialSystem.cpp	\
	MaterialSystem/DynamicMesh.cpp		\
	MaterialSystem/MaterialProxy.cpp	\
	MaterialSystem/DefaultShader.cpp	\
	public/materialsystem/BaseShader.cpp	\

LOCAL_STATIC_LIBRARIES := coreLib prevLib jpeg eqCore

include $(BUILD_SHARED_LIBRARY)
