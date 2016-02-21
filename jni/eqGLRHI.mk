##
## libeqGLRHI shared library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= eqGLRHI
LOCAL_MODULE_FILENAME	:= libeqGLRHI
LOCAL_CFLAGS    		:= -DANDROID -DUSE_GLES2 -DCORE_EXPORT -DDLL_EXPORT -DRENDER_EXPORT -DIS_OPENGL -std=c++11 -pthread -fexceptions

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public\
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public/materialsystem\
	$(LOCAL_PATH)/public/materialsystem/renderers\

LOCAL_SRC_FILES := \
	MaterialSystem/Renderers/GL/VertexBufferGL.cpp	\
	MaterialSystem/Renderers/GL/CGLTexture.cpp		\
	MaterialSystem/Renderers/GL/GLMeshBuilder.cpp	\
	MaterialSystem/Renderers/GL/GLOcclusionQuery.cpp\
	MaterialSystem/Renderers/GL/GLShaderProgram.cpp	\
	MaterialSystem/Renderers/GL/IndexBufferGL.cpp	\
	MaterialSystem/Renderers/GL/ShaderAPIGL.cpp		\
	MaterialSystem/Renderers/GL/CGLRenderLib.cpp	\
	MaterialSystem/Renderers/GL/VertexFormatGL.cpp	\
	MaterialSystem/Renderers/GL/gl_caps.cpp			\
	MaterialSystem/Renderers/ShaderAPI_Base.cpp		\
	MaterialSystem/Renderers/CTexture.cpp

LOCAL_LDLIBS := -lGLESv2 -lEGL

LOCAL_STATIC_LIBRARIES := coreLib prevLib jpeg eqCore

include $(BUILD_SHARED_LIBRARY)
