##
## libeqBaseShaders shared library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= eqBaseShaders
LOCAL_MODULE_FILENAME	:= libeqBaseShaders
LOCAL_CFLAGS    		:= -DANDROID -DEQSHADER_LIBRARY -std=c++11 -pthread -fexceptions

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public\
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public/materialsystem\

LOCAL_SRC_FILES := \
	MaterialSystem/EngineShaders/Shaders/BaseParticle.cpp	\
	MaterialSystem/EngineShaders/Shaders/BaseSkinned.cpp	\
	MaterialSystem/EngineShaders/Shaders/BaseUnlit.cpp	\
	MaterialSystem/EngineShaders/Shaders/BlurFilter.cpp	\
	MaterialSystem/EngineShaders/Shaders/Error.cpp	\
	MaterialSystem/EngineShaders/Shaders/SDFFont.cpp	\
	MaterialSystem/EngineShaders/Shaders/Sky.cpp	\
	MaterialSystem/EngineShaders/ShadersOverride.cpp	\
	MaterialSystem/EngineShaders/ShadersMain.cpp	\
	public/materialsystem/BaseShader.cpp		\
	public/ViewParams.cpp

LOCAL_STATIC_LIBRARIES := coreLib prevLib jpeg eqCore

include $(BUILD_SHARED_LIBRARY)
