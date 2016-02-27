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
	MaterialSystem/EngineShaders/SkinnedModel.cpp				\
	MaterialSystem/EngineShaders/ShadersOverride.cpp			\
	MaterialSystem/EngineShaders/ShadersMain.cpp				\
	MaterialSystem/EngineShaders/Shaders/SkyBox.cpp				\
	MaterialSystem/EngineShaders/Shaders/InternalShaders.cpp	\
	MaterialSystem/EngineShaders/Shaders/DeferredAmbient.cpp	\
	MaterialSystem/EngineShaders/Shaders/BaseUnlit.cpp			\
	MaterialSystem/EngineShaders/FlashlightReflector.cpp		\
	MaterialSystem/EngineShaders/ErrorShader.cpp				\
	MaterialSystem/EngineShaders/DeferredSunLight.cpp			\
	MaterialSystem/EngineShaders/DeferredSpotLight.cpp			\
	MaterialSystem/EngineShaders/DeferredPointlight.cpp			\
	MaterialSystem/EngineShaders/BleachBypass.cpp				\
	MaterialSystem/EngineShaders/BaseVertexTransition4.cpp		\
	MaterialSystem/EngineShaders/BaseVertexTransDecal4.cpp		\
	MaterialSystem/EngineShaders/BaseSingle.cpp					\
	MaterialSystem/EngineShaders/BaseParticle.cpp				\
	MaterialSystem/EngineShaders/BaseDecal.cpp					\
	MaterialSystem/EngineShaders/DepthWriteLighting.cpp			\
	MaterialSystem/EngineShaders/Blur.cpp						\
	MaterialSystem/EngineShaders/Water.cpp						\
	MaterialSystem/EngineShaders/HDRBlur.cpp					\
	MaterialSystem/EngineShaders/HDRBloom.cpp					\
	public/materialsystem/BaseShader.cpp						\
	public/ViewParams.cpp

LOCAL_STATIC_LIBRARIES := coreLib prevLib jpeg eqCore

include $(BUILD_SHARED_LIBRARY)
