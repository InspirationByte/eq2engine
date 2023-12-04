//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include <lz4.h>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/IFileSystem.h"
#include "core/IConsoleCommands.h"
#include "utils/KeyValues.h"
#include "../RenderWorker.h"

#include "shaderapigl_def.h"
#include "ShaderAPIGL.h"
#include "GLTexture.h"
#include "GLVertexFormat.h"
#include "GLVertexBuffer.h"
#include "GLIndexBuffer.h"
#include "GLShaderProgram.h"
#include "GLRenderState.h"
#include "GLOcclusionQuery.h"

#include "imaging/ImageLoader.h"

#ifdef USE_GLES2
#define SHADERCACHE_FOLDER		"ShaderCache/GLES"
#else
#define SHADERCACHE_FOLDER		"ShaderCache/GL"
#endif

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;
extern CEqMutex	g_sapi_ShaderMutex;
extern CEqMutex	g_sapi_VBMutex;
extern CEqMutex	g_sapi_IBMutex;
extern CEqMutex	g_sapi_Mutex;

extern CEqMutex g_sapi_ProgressiveTextureMutex;

ShaderAPIGL s_renderApi;
IShaderAPI* g_renderAPI = &s_renderApi;

void PrintGLExtensions()
{
	const char* ver = (const char*)glGetString(GL_VERSION);
	Msg("OpenGL version: %s\n \n", ver);
	const char* exts = (const char*)glGetString(GL_EXTENSIONS);

	Array<EqString> splExts(PP_SL);
	xstrsplit(exts, " ", splExts);

	MsgWarning("Supported OpenGL extensions:\n");
	int i;
	for (i = 0; i < splExts.numElem(); i++)
	{
		MsgInfo("%s\n", splExts[i].ToCString());
	}
	MsgWarning("Total extensions supported: %i\n", i);
}

DECLARE_CMD(gl_extensions, "Print supported OpenGL extensions", 0)
{
	PrintGLExtensions();
}

DECLARE_CVAR(gl_report_errors, "0", nullptr, 0);
DECLARE_CVAR(gl_break_on_error, "0", nullptr, 0);
DECLARE_CVAR(gl_bypass_errors, "0", nullptr, 0);
DECLARE_CVAR(r_preloadShaderCache, "1", nullptr, 0);
DECLARE_CVAR(r_skipShaderCache, "0", "Shader debugging purposes", 0);

bool GLCheckError(const char* op, ...)
{
	GLenum lastError = glGetError();
	if(lastError != GL_NO_ERROR)
	{
        EqString errString = EqString::Format("code %x", lastError);

        switch(lastError)
        {
            case GL_NO_ERROR:
                errString = "GL_NO_ERROR";
                break;
            case GL_INVALID_ENUM:
                errString = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errString = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errString = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errString = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errString = "GL_OUT_OF_MEMORY";
                break;
        }

		va_list argptr;
		va_start(argptr, op);
		EqString errorMsg = EqString::FormatVa(op, argptr);
		va_end(argptr);

		if(gl_break_on_error.GetBool())
		{
			_DEBUG_BREAK;
		}
		//ASSERT_MSG(!gl_break_on_error.GetBool(), "OpenGL: %s - %s", errorMsg.ToCString(), errString.ToCString());

		if (gl_report_errors.GetBool())
			MsgError("*OGL* error occured while '%s' (%s)\n", errorMsg.ToCString(), errString.ToCString());

		return gl_bypass_errors.GetBool();
	}

	return true;
}

void InitGLHardwareCapabilities(ShaderAPICaps& caps)
{
	memset(&caps, 0, sizeof(caps));

#if GL_ARB_texture_filter_anisotropic
	if (GLAD_GL_ARB_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.maxTextureAnisotropicLevel);
#elif GL_EXT_texture_filter_anisotropic
	if (GLAD_GL_EXT_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &caps.maxTextureAnisotropicLevel);
#else
	caps.maxTextureAnisotropicLevel = 1;
#endif

	caps.isHardwareOcclusionQuerySupported = true;
	caps.isInstancingSupported = true; // GL ES 3

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);

	caps.maxRenderTargets = MAX_RENDERTARGETS;

	caps.maxVertexAttributes = MAX_GL_GENERIC_ATTRIB;

	caps.maxTextureUnits = 1;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &caps.maxTextureUnits);
	caps.maxTextureUnits = min(MAX_TEXTUREUNIT, caps.maxTextureUnits);

	caps.maxRenderTargets = 1;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &caps.maxRenderTargets);
	caps.maxRenderTargets = min(MAX_RENDERTARGETS, caps.maxRenderTargets);

	caps.maxVertexStreams = MAX_VERTEXSTREAM;
	caps.maxVertexTextureUnits = MAX_VERTEXTEXTURES;

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &caps.maxVertexTextureUnits);
	caps.maxVertexTextureUnits = min(caps.maxVertexTextureUnits, MAX_VERTEXTEXTURES);

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &caps.maxVertexAttributes);
	caps.maxVertexAttributes = min(MAX_GL_GENERIC_ATTRIB, caps.maxVertexAttributes);

	caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED | SHADER_CAPS_PIXEL_SUPPORTED;

	// get texture capabilities
	{
		caps.INTZSupported = true;
		caps.INTZFormat = FORMAT_D16;

		caps.NULLSupported = true;
		caps.NULLFormat = FORMAT_NONE;

		for (int i = FORMAT_R8; i <= FORMAT_RGBA16; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}
		
		for (int i = FORMAT_D16; i <= FORMAT_D24S8; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}

#if GL_ARB_depth_buffer_float
		caps.textureFormatsSupported[FORMAT_D32F] = 
			caps.renderTargetFormatsSupported[FORMAT_D32F] = GLAD_GL_ARB_depth_buffer_float;
#endif // GL_ARB_depth_buffer_float

#if GL_IMG_texture_compression_pvrtc
		if(GLAD_GL_IMG_texture_compression_pvrtc)
		{
			for (int i = FORMAT_PVRTC_2BPP; i <= FORMAT_PVRTC_A_4BPP; i++)
				caps.textureFormatsSupported[i] = true;
		}
#endif

#if GL_OES_compressed_ETC1_RGB8_texture
		if(GLAD_GL_OES_compressed_ETC1_RGB8_texture)
		{
			for (int i = FORMAT_ETC1; i <= FORMAT_ETC2A8; i++)
				caps.textureFormatsSupported[i] = true;
		}
#endif

#if GL_EXT_texture_compression_s3tc || GL_EXT_texture_compression_s3tc_srgb
		bool s3tcSupported = GLAD_GL_EXT_texture_compression_s3tc;
#if GL_EXT_texture_compression_s3tc_srgb
		s3tcSupported = s3tcSupported || GLAD_GL_EXT_texture_compression_s3tc_srgb;
#endif
		if (s3tcSupported)
		{
			for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
				caps.textureFormatsSupported[i] = true;

			caps.textureFormatsSupported[FORMAT_ATI1N] = false;
		}
#endif
	}

	GLCheckError("caps check");
}

typedef GLvoid (APIENTRY *UNIFORM_FUNC)(GLint location, GLsizei count, const void *value);
typedef GLvoid (APIENTRY *UNIFORM_MAT_FUNC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

EConstantType GetConstantType(GLenum type)
{
	switch (type)
	{
		case GL_FLOAT:			return CONSTANT_FLOAT;
		case GL_FLOAT_VEC2:	return CONSTANT_VECTOR2D;
		case GL_FLOAT_VEC3:	return CONSTANT_VECTOR3D;
		case GL_FLOAT_VEC4:	return CONSTANT_VECTOR4D;
		case GL_INT:			return CONSTANT_INT;
		case GL_INT_VEC2:		return CONSTANT_IVECTOR2D;
		case GL_INT_VEC3:		return CONSTANT_IVECTOR3D;
		case GL_INT_VEC4:		return CONSTANT_IVECTOR4D;
		case GL_BOOL:			return CONSTANT_BOOL;
		case GL_BOOL_VEC2:		return CONSTANT_BVECTOR2D;
		case GL_BOOL_VEC3:		return CONSTANT_BVECTOR3D;
		case GL_BOOL_VEC4:		return CONSTANT_BVECTOR4D;
		case GL_FLOAT_MAT2:	return CONSTANT_MATRIX2x2;
		case GL_FLOAT_MAT3:	return CONSTANT_MATRIX3x3;
		case GL_FLOAT_MAT4:	return CONSTANT_MATRIX4x4;
	}

	MsgError("Invalid constant type (%d)\n", type);

	return (EConstantType) -1;
}

void* s_uniformFuncs[CONSTANT_COUNT] = {};

void ShaderAPIGL::PrintAPIInfo() const
{
	Msg("ShaderAPI: ShaderAPIGL\n");

	Msg("  Maximum texture anisotropy: %d\n", m_caps.maxTextureAnisotropicLevel);
	Msg("  Maximum drawable textures: %d\n", m_caps.maxTextureUnits);
	Msg("  Maximum vertex textures: %d\n", m_caps.maxVertexTextureUnits);
	Msg("  Maximum texture size: %d x %d\n", m_caps.maxTextureSize, m_caps.maxTextureSize);

	Msg("  Instancing supported: %d\n", m_caps.isInstancingSupported);

	MsgInfo("------ Loaded textures ------\n");

	CScopedMutex scoped(g_sapi_TextureMutex);
	for (auto it = m_TextureList.begin(); !it.atEnd(); ++it)
	{
		CGLTexture* pTexture = (CGLTexture*)*it;

		MsgInfo("     %s (%d) - %dx%d\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(),pTexture->GetHeight());
	}
}

// Init + Shurdown
void ShaderAPIGL::Init( const ShaderAPIParams &params)
{
	Msg("Initializing OpenGL Shader API...\n");

	const char* vendorStr = (const char *) glGetString(GL_VENDOR);

	if(xstristr(vendorStr, "nvidia"))
		m_vendor = VENDOR_NV;
	else if(xstristr(vendorStr, "ati") || xstristr(vendorStr, "amd") || xstristr(vendorStr, "radeon"))
		m_vendor = VENDOR_ATI;
	else if(xstristr(vendorStr, "intel"))
		m_vendor = VENDOR_INTEL;
	else
		m_vendor = VENDOR_OTHER;

	DevMsg(DEVMSG_RENDER, "[DEBUG] ShaderAPIGL vendor: %d\n", m_vendor);

	// Set some of my preferred defaults
	glDisable(GL_DEPTH_TEST);
	GLCheckError("def param GL_DEPTH_TEST");

	glDepthMask(0);
	GLCheckError("def param glDepthMask");

	glDepthFunc(GL_LEQUAL);
	GLCheckError("def param GL_LEQUAL");

	glFrontFace(GL_CW);
	GLCheckError("def param GL_CW");

	glPixelStorei(GL_PACK_ALIGNMENT,   1);
	GLCheckError("def param GL_PACK_ALIGNMENT");

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLCheckError("def param GL_UNPACK_ALIGNMENT");

	glGenVertexArrays(1, &m_drawVAO);
	GLCheckError("gen VAO");

	for (int i = 0; i < m_caps.maxRenderTargets; i++)
		m_drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;

	HOOK_TO_CVAR(r_anisotropic);
	const int desiredAnisotropicLevel = min(r_anisotropic->GetInt(), m_caps.maxTextureAnisotropicLevel);
	m_caps.maxTextureAnisotropicLevel = desiredAnisotropicLevel;

	g_fileSystem->MakeDir(SHADERCACHE_FOLDER, SP_ROOT);
	PreloadShadersFromCache();

	// Init the base shader API
	ShaderAPI_Base::Init(params);

	// all shaders supported, nothing to report
	s_uniformFuncs[CONSTANT_FLOAT]		= (void *) glUniform1fv;
	s_uniformFuncs[CONSTANT_VECTOR2D]	= (void *) glUniform2fv;
	s_uniformFuncs[CONSTANT_VECTOR3D]	= (void *) glUniform3fv;
	s_uniformFuncs[CONSTANT_VECTOR4D]	= (void *) glUniform4fv;
	s_uniformFuncs[CONSTANT_INT]		= (void *) glUniform1iv;
	s_uniformFuncs[CONSTANT_IVECTOR2D]	= (void *) glUniform2iv;
	s_uniformFuncs[CONSTANT_IVECTOR3D]	= (void *) glUniform3iv;
	s_uniformFuncs[CONSTANT_IVECTOR4D]	= (void *) glUniform4iv;
	s_uniformFuncs[CONSTANT_BOOL]		= (void *) glUniform1iv;
	s_uniformFuncs[CONSTANT_BVECTOR2D]	= (void *) glUniform2iv;
	s_uniformFuncs[CONSTANT_BVECTOR3D]	= (void *) glUniform3iv;
	s_uniformFuncs[CONSTANT_BVECTOR4D]	= (void *) glUniform4iv;
	s_uniformFuncs[CONSTANT_MATRIX2x2]	= (void *) glUniformMatrix2fv;
	s_uniformFuncs[CONSTANT_MATRIX3x3]	= (void *) glUniformMatrix3fv;
	s_uniformFuncs[CONSTANT_MATRIX4x4]	= (void *) glUniformMatrix4fv;

	for(int i = 0; i < CONSTANT_COUNT; i++)
	{
		if(s_uniformFuncs[i] == nullptr)
			ASSERT_FAIL("Uniform function for '%d' is not ok, pls check extensions\n", i);
	}
}

void ShaderAPIGL::Shutdown()
{
	glDeleteVertexArrays(1, &m_drawVAO);
	GLCheckError("delete VAO");

	glDeleteFramebuffers(1, &m_frameBuffer);
	m_frameBuffer = 0;

	ShaderAPI_Base::Shutdown();

	// shutdown worker and detach context
	g_renderWorker.Shutdown();
}

void ShaderAPIGL::Reset(int nResetType/* = STATE_RESET_ALL*/)
{
	ShaderAPI_Base::Reset(nResetType);

	// TODO: reset shaders
}

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------

void ShaderAPIGL::ApplyTextures()
{
	GLint maxImageUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxImageUnits);
	ASSERT(maxImageUnits > 0);

	for (int i = 0; i < m_caps.maxTextureUnits; i++)
	{
		CGLTexture* pSelectedTexture = (CGLTexture*)m_pSelectedTextures[i].Ptr();
		ITexture* pCurrentTexture = m_pCurrentTextures[i];
		GLTextureRef_t& currentGLTexture = m_currentGLTextures[i];
		
		if(pSelectedTexture == m_pCurrentTextures[i])
			continue;

		// Set the active texture unit and bind the selected texture to target
		glActiveTexture(GL_TEXTURE0 + i);
		GLCheckError("active texture %d", i);

		if (pSelectedTexture == nullptr)
		{
			//if(pCurrentTexture != nullptr)
			glBindTexture(g_gl_texTargetType[currentGLTexture.type], 0);
		}
		else
		{
			GLTextureRef_t newGLTexture = pSelectedTexture->GetCurrentTexture();

			// unbind texture at old target
			if(newGLTexture.type != currentGLTexture.type && currentGLTexture.type != IMAGE_TYPE_INVALID)
				glBindTexture(g_gl_texTargetType[currentGLTexture.type], 0);

			glBindTexture(g_gl_texTargetType[newGLTexture.type], newGLTexture.glTexID);
			currentGLTexture = newGLTexture;
		}
		GLCheckError("bind texture");

		m_pCurrentTextures[i] = ITexturePtr(pSelectedTexture);
	}
}

void ShaderAPIGL::ApplySamplerState()
{

}

void ShaderAPIGL::ApplyBlendState()
{
	CGLBlendingState* pSelectedState = (CGLBlendingState*)m_pSelectedBlendstate;

	if (m_pCurrentBlendstate != pSelectedState)
	{
		int mask = COLORMASK_ALL;
	
		if (pSelectedState == nullptr)
		{
			if (m_bCurrentBlendEnable)
			{
				glDisable(GL_BLEND);
 				m_bCurrentBlendEnable = false;
			}
		}
		else
		{
			BlendStateParams& state = pSelectedState->m_params;
		
			if (state.enable)
			{
				if (!m_bCurrentBlendEnable)
				{
					glEnable(GL_BLEND);
					m_bCurrentBlendEnable = true;
				}

				if (state.srcFactor != m_nCurrentSrcFactor || state.dstFactor != m_nCurrentDstFactor)
				{
					glBlendFunc(g_gl_blendingConsts[m_nCurrentSrcFactor = state.srcFactor],
								g_gl_blendingConsts[m_nCurrentDstFactor = state.dstFactor]);
				}

				if (state.blendFunc != m_nCurrentBlendFunc)
					glBlendEquation(g_gl_blendingModes[m_nCurrentBlendFunc = state.blendFunc]);
			}
			else
			{
				if (m_bCurrentBlendEnable)
				{
					glDisable(GL_BLEND);
 					m_bCurrentBlendEnable = false;
				}
			}

#if 0 // don't use FFP alpha test it's freakin slow and deprecated
			if(state.alphaTest)
			{
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER, state.alphaTestRef);
			}
			else
			{
				glDisable(GL_ALPHA_TEST);
			}
#endif // 0

			mask = state.mask;
		}

		if (mask != m_nCurrentMask)
		{
			glColorMask((mask & COLORMASK_RED) ? 1 : 0, ((mask & COLORMASK_GREEN) >> 1) ? 1 : 0, ((mask & COLORMASK_BLUE) >> 2) ? 1 : 0, ((mask & COLORMASK_ALPHA) >> 3)  ? 1 : 0);
			m_nCurrentMask = mask;
		}

		m_pCurrentBlendstate = pSelectedState;
	}
}

void ShaderAPIGL::ApplyDepthState()
{
	// stencilRef currently not used
	static CGLDepthStencilState defaultState;
	CGLDepthStencilState* pSelectedState = m_pSelectedDepthState ? (CGLDepthStencilState*)m_pSelectedDepthState : &defaultState;
	CGLDepthStencilState* pCurrentState = m_pCurrentDepthState ? (CGLDepthStencilState*)m_pCurrentDepthState : &defaultState;

	if (pSelectedState != m_pCurrentDepthState)
	{
		const DepthStencilStateParams& newState = pSelectedState->m_params;
		const DepthStencilStateParams& prevState = pCurrentState->m_params;
		
		if (newState.depthTest != prevState.depthTest)
		{
			if(newState.depthTest)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}
		
		// BUG? for some reason after switching GL_DEPTH_TEST we have update the depth mask again, otherwise it won't work as expected
		if (newState.depthWrite != prevState.depthWrite || newState.depthTest != prevState.depthTest)
		{
			glDepthMask((newState.depthWrite) ? GL_TRUE : GL_FALSE);
		}

		if (newState.depthFunc != prevState.depthFunc)
			glDepthFunc(g_gl_depthConst[newState.depthFunc]);

		if (newState.useDepthBias != false)
		{
			if (m_fCurrentDepthBias != newState.depthBias || m_fCurrentSlopeDepthBias != newState.depthBiasSlopeScale)
			{
				const float POLYGON_OFFSET_SCALE = 320.0f;	// FIXME: is a depth buffer bit depth?

				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset((m_fCurrentDepthBias = newState.depthBias) * POLYGON_OFFSET_SCALE, (m_fCurrentSlopeDepthBias = newState.depthBiasSlopeScale) * POLYGON_OFFSET_SCALE);
			}
		}
		else
		{
			if (m_fCurrentDepthBias != 0.0f || m_fCurrentSlopeDepthBias != 0.0f)
			{
				glDisable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(0.0f, 0.0f);

				m_fCurrentDepthBias = 0.0f;
				m_fCurrentSlopeDepthBias = 0.0f;
			}
		}

		#pragma todo(GL: stencil tests)

		m_pCurrentDepthState = pSelectedState;
	}
}

void ShaderAPIGL::ApplyRasterizerState()
{
	CGLRasterizerState* pSelectedState = (CGLRasterizerState*)m_pSelectedRasterizerState;

	if (m_pCurrentRasterizerState != pSelectedState)
	{
		if (pSelectedState == nullptr)
		{
			if (CULL_BACK != m_nCurrentCullMode)
			{
				if (m_nCurrentCullMode == CULL_NONE)
					glEnable(GL_CULL_FACE);

				glCullFace(g_gl_cullConst[m_nCurrentCullMode = CULL_BACK]);
			}

#ifndef USE_GLES2
			if (FILL_SOLID != m_nCurrentFillMode)
				glPolygonMode(GL_FRONT_AND_BACK, g_gl_fillConst[m_nCurrentFillMode = FILL_SOLID]);

			if (m_bCurrentMultiSampleEnable)
			{
				glDisable(GL_MULTISAMPLE);
				m_bCurrentMultiSampleEnable = false;
			}
#endif // USE_GLES2
			if (m_bCurrentScissorEnable)
			{
				glDisable(GL_SCISSOR_TEST);
				m_bCurrentScissorEnable = false;
			}		
		}
		else
		{
			RasterizerStateParams& state = pSelectedState->m_params;
		
			if (state.cullMode != m_nCurrentCullMode)
			{
				if (state.cullMode > CULL_NONE)
				{
					if (m_nCurrentCullMode == CULL_NONE)
						glEnable(GL_CULL_FACE);

					glCullFace(g_gl_cullConst[state.cullMode]);
				}
				else
					glDisable(GL_CULL_FACE);

				m_nCurrentCullMode = state.cullMode;
			}

#ifndef USE_GLES2
			if (state.fillMode != m_nCurrentFillMode)
				glPolygonMode(GL_FRONT_AND_BACK, g_gl_fillConst[m_nCurrentFillMode = state.fillMode]);

			if (state.multiSample != m_bCurrentMultiSampleEnable)
			{
				(state.multiSample ? glEnable : glDisable)(GL_MULTISAMPLE);
				m_bCurrentMultiSampleEnable = state.multiSample;
			}
#endif // USE_GLES2

			if (state.scissor != m_bCurrentScissorEnable)
			{
				(state.scissor ? glEnable : glDisable)(GL_SCISSOR_TEST);
				m_bCurrentScissorEnable = state.scissor;
			}
		}
	}

	m_pCurrentRasterizerState = pSelectedState;
}

void ShaderAPIGL::ApplyShaderProgram()
{
	CGLShaderProgram* selectedShader = (CGLShaderProgram*)m_pSelectedShader.Ptr();

	if (selectedShader != m_pCurrentShader)
	{
		GLhandleARB program = selectedShader ? selectedShader->m_program : 0;

		glUseProgram(program);
		m_pCurrentShader = m_pSelectedShader;
	}
}

void ShaderAPIGL::ApplyConstants()
{
	CGLShaderProgram* currentShader = (CGLShaderProgram*)m_pCurrentShader.Ptr();

	if (!currentShader)
		return;

	Map<int, GLShaderConstant_t>& constants = currentShader->m_constants;

	for (auto it = constants.begin(); !it.atEnd(); ++it)
	{
		GLShaderConstant_t& uni = *it;
		if (!uni.dirty)
			continue;

		uni.dirty = false;

		if (uni.type >= CONSTANT_MATRIX2x2)
			((UNIFORM_MAT_FUNC)s_uniformFuncs[uni.type])(uni.uniformLoc, uni.nElements, GL_TRUE, (float*)uni.data);
		else
			((UNIFORM_FUNC)s_uniformFuncs[uni.type])(uni.uniformLoc, uni.nElements, (float*)uni.data);
		GLCheckError("apply uniform %s", uni.name);
	}
}


void ShaderAPIGL::Clear(bool bClearColor,
						bool bClearDepth,
						bool bClearStencil,
						const MColor& fillColor,
						float fDepth,
						int nStencil)
{
	GLbitfield clearBits = 0;

	if (bClearColor)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		GLCheckError("clr mask");

		glClearColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
		GLCheckError("clr color");
	}

	int depthMaskOn;
	glGetIntegerv(GL_DEPTH_WRITEMASK, &depthMaskOn);

	if (bClearDepth)
	{
		clearBits |= GL_DEPTH_BUFFER_BIT;

		// make depth buffer writeable
		glDepthMask(GL_TRUE);
		GLCheckError("clr depth msk");

#ifdef USE_GLES2
		glClearDepthf(fDepth);
#else
		glClearDepth(fDepth);
		GLCheckError("clr depth");
#endif // USE_GLES2
	}

	if (bClearStencil)
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;

		glStencilMask(GL_TRUE);
		GLCheckError("clr stencil msk");

		glClearStencil(nStencil);
		GLCheckError("clr stencil");
	}

	if (clearBits)
	{
		glClear(clearBits);
		GLCheckError("clear");

		// restore depth buffer write flag
		if (clearBits & GL_DEPTH_BUFFER_BIT)
		{
			glDepthMask(depthMaskOn);
			GLCheckError("rst depth msk");
		}
	}
}
//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

// Renderer string (ex: OpenGL, D3D9)
const char* ShaderAPIGL::GetRendererName() const
{
	return "OpenGL";
}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

// Synchronization
void ShaderAPIGL::Flush()
{
	glFlush();
}

void ShaderAPIGL::Finish()
{
	glFinish();
}

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

// creates occlusion query object
IOcclusionQuery* ShaderAPIGL::CreateOcclusionQuery()
{
	if(!m_caps.isHardwareOcclusionQuerySupported)
		return nullptr;

	CGLOcclusionQuery* occQuery = PPNew CGLOcclusionQuery();

	{
		CScopedMutex m(g_sapi_Mutex);
		m_OcclusionQueryList.append(occQuery);
	}

	return occQuery;
}

// removal of occlusion query object
void ShaderAPIGL::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
	if(pQuery)
		delete pQuery;

	m_OcclusionQueryList.fastRemove( pQuery );
}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

// It will add new rendertarget
ITexturePtr ShaderAPIGL::CreateRenderTarget(const char* pszName, ETextureFormat format, int width, int height, int arraySize, const SamplerStateParams& sampler, int flags)
{
	CRefPtr<CGLTexture> pTexture = CRefPtr_new(CGLTexture);

	pTexture->SetDimensions(width,height);
	pTexture->SetFormat(format);
	pTexture->SetFlags(flags | TEXFLAG_RENDERTARGET);
	pTexture->SetName(pszName);
	pTexture->SetSamplerState(sampler);

	pTexture->m_glTarget = (flags & TEXFLAG_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	if (flags & TEXFLAG_RENDERDEPTH)
	{
		glGenTextures(1, &pTexture->m_glDepthID);
		GLCheckError("gen depth tex");

		glBindTexture(GL_TEXTURE_2D, pTexture->m_glDepthID);
		SetupGLSamplerState(GL_TEXTURE_2D, sampler);
	}

	// create texture
	GLTextureRef_t texture;

	if (flags & TEXFLAG_CUBEMAP)
		texture.type = IMAGE_TYPE_CUBE;
	else
		texture.type = IMAGE_TYPE_2D;

	if(format != FORMAT_NONE)
	{
		glGenTextures(1, &texture.glTexID);
		GLCheckError("gen tex");

		glBindTexture(pTexture->m_glTarget, texture.glTexID);
		SetupGLSamplerState(pTexture->m_glTarget, sampler);

		pTexture->m_textures.append(texture);

		// this generates the render target
		ResizeRenderTarget(ITexturePtr(pTexture), width, height, arraySize);
	}
	else
	{
		texture.glTexID = 0;
		pTexture->m_textures.append(texture);
	}

	{
		CScopedMutex m(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(pTexture);
		m_TextureList.insert(pTexture->m_nameHash, pTexture);
	}

	return ITexturePtr(pTexture);
}

void ShaderAPIGL::ResizeRenderTarget(const ITexturePtr& renderTarget, int newWide, int newTall, int newArraySize)
{
	CGLTexture* pTex = (CGLTexture*)renderTarget.Ptr();
	ETextureFormat format = pTex->GetFormat();

	pTex->SetDimensions(newWide, newTall, newArraySize);

	if (format == FORMAT_NONE)
		return;

	if (pTex->m_glTarget == GL_RENDERBUFFER)
	{
		// Bind render buffer
		glBindRenderbuffer(GL_RENDERBUFFER, pTex->m_glDepthID);
		glRenderbufferStorage(GL_RENDERBUFFER, g_gl_internalFormats[format], newWide, newTall);
		GLCheckError("gen tex renderbuffer storage");

		// Restore renderbuffer
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	else
	{
		if (pTex->m_glDepthID != GL_NONE)
		{
			// make a depth texture first
			// use glDepthID
			ETextureFormat depthFmt = FORMAT_D16;
			GLint depthInternalFormat = g_gl_internalFormats[depthFmt];
			GLenum depthSrcFormat = IsStencilFormat(depthFmt) ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT;
			GLenum depthSrcType = g_gl_chanTypePerFormat[depthFmt];

			glBindTexture(GL_TEXTURE_2D, pTex->m_glDepthID);
			glTexImage2D(GL_TEXTURE_2D, 0, depthInternalFormat, newWide, newTall, 0, depthSrcFormat, depthSrcType, nullptr);
			GLCheckError("gen tex image depth");
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		GLint internalFormat = g_gl_internalFormats[format];
		GLenum srcFormat = g_gl_chanCountTypes[GetChannelCount(format)];
		GLenum srcType = g_gl_chanTypePerFormat[format];

		if (IsDepthFormat(format))
		{
			if (IsStencilFormat(format))
				srcFormat = GL_DEPTH_STENCIL;
			else
				srcFormat = GL_DEPTH_COMPONENT;
		}

		// Allocate all required surfaces.
		glBindTexture(pTex->m_glTarget, pTex->m_textures[0].glTexID);

		if (pTex->GetFlags() & TEXFLAG_CUBEMAP)
		{
			for (int i = 0; i < 6; i++)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, newWide, newTall, 0, srcFormat, srcType, nullptr);
			GLCheckError("gen tex image cube");
		}
		else
		{
			glTexImage2D(pTex->m_glTarget, 0, internalFormat, newWide, newTall, 0, srcFormat, srcType, nullptr);
			GLCheckError("gen tex image");
		}

		glBindTexture(pTex->m_glTarget, 0);
	}
}

ITexturePtr ShaderAPIGL::CreateTextureResource(const char* pszName)
{
	CRefPtr<CGLTexture> texture = CRefPtr_new(CGLTexture);
	texture->SetName(pszName);

	m_TextureList.insert(texture->m_nameHash, texture);
	return ITexturePtr(texture);
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

// Copy render target to texture
void ShaderAPIGL::CopyFramebufferToTexture(const ITexturePtr& pTargetTexture)
{
	// store the current rendertarget states
	ITexturePtr currentRenderTarget[MAX_RENDERTARGETS];
	int	currentCRTSlice[MAX_RENDERTARGETS];
	ITexturePtr currentDepthTarget = m_pCurrentDepthRenderTarget;
	m_pCurrentDepthRenderTarget = nullptr;

	int currentNumRTs = 0;
	for(; currentNumRTs < MAX_RENDERTARGETS;)
	{
		ITexturePtr& rt = m_pCurrentColorRenderTargets[currentNumRTs];

		if(!rt)
			break;

		currentRenderTarget[currentNumRTs] = rt;
		currentCRTSlice[currentNumRTs] = m_nCurrentCRTSlice[currentNumRTs];

		m_pCurrentColorRenderTargets[currentNumRTs] = nullptr;
		m_nCurrentCRTSlice[currentNumRTs] = 0;

		currentNumRTs++;
	}

	ChangeRenderTarget(pTargetTexture);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_frameBuffer);

	// FIXME: this could lead to regression problem, so states must be revisited
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);

	glBlitFramebuffer(0, 0, m_backbufferSize.x, m_backbufferSize.y, 0, 0, pTargetTexture->GetWidth(), pTargetTexture->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// restore render targets back
	// or to backbuffer if no RTs were set
	if(currentNumRTs)
		ChangeRenderTargets(ArrayCRef(currentRenderTarget, currentNumRTs), ArrayCRef(currentCRTSlice, currentNumRTs), currentDepthTarget);
	else
		ChangeRenderTargetToBackBuffer();
}

// Copy render target to texture
void ShaderAPIGL::CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect, IAARectangle* destRect)
{
	// BUG BUG:
	// this process is very bugged as fuck
	// TODO: double-check main framebuffer attachments from de-attaching

	// store the current rendertarget states
	ITexturePtr currentRenderTarget[MAX_RENDERTARGETS];
	int	currentCRTSlice[MAX_RENDERTARGETS];
	ITexturePtr currentDepthTarget = m_pCurrentDepthRenderTarget;
	m_pCurrentDepthRenderTarget = nullptr;

	int currentNumRTs = 0;
	for(; currentNumRTs < MAX_RENDERTARGETS;)
	{
		ITexturePtr& rt = m_pCurrentColorRenderTargets[currentNumRTs];

		if (!rt)
			break;

		currentRenderTarget[currentNumRTs] = rt;
		currentCRTSlice[currentNumRTs] = m_nCurrentCRTSlice[currentNumRTs];

		m_pCurrentColorRenderTargets[currentNumRTs] = nullptr;
		m_nCurrentCRTSlice[currentNumRTs] = 0;

		currentNumRTs++;
	}

	// 1. preserve old render targets
	// 2. set shader
	// 3. render to texture
	CGLTexture* srcTexture = (CGLTexture*)srcTarget.Ptr();
	CGLTexture* destTexture = (CGLTexture*)destTex.Ptr();

	GLTextureRef_t srcTexRef = srcTexture->m_textures.front();
	GLTextureRef_t destTexRef = destTexture->m_textures.front();

	IAARectangle _srcRect(0,0,srcTexture->GetWidth(), srcTexture->GetHeight());
	IAARectangle _destRect(0,0,destTexture->GetWidth(), destTexture->GetHeight());

	if(srcRect)
		_srcRect = *srcRect;

	if(destRect)
		_destRect = *destRect;

	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

	// FIXME: this could lead to regression problem, so states must be revisited
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);

	// setup read and write texture
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_gl_texTargetType[srcTexRef.type], srcTexRef.glTexID, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, g_gl_texTargetType[destTexRef.type], destTexRef.glTexID, 0);

	// setup GL_COLOR_ATTACHMENT1 as destination
	GLenum drawBufferSetting = GL_COLOR_ATTACHMENT1;
	glDrawBuffers(1, &drawBufferSetting);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	GLCheckError("glDrawBuffers att1");

	// copy
	glBlitFramebuffer(	_srcRect.leftTop.x, _srcRect.leftTop.y, _srcRect.rightBottom.x, _srcRect.rightBottom.y,
						_destRect.leftTop.x, _destRect.leftTop.y, _destRect.rightBottom.x, _destRect.rightBottom.y, 
						GL_COLOR_BUFFER_BIT, GL_NEAREST);
	GLCheckError("blit");

	// reset
	drawBufferSetting = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &drawBufferSetting);
	glReadBuffer(GL_NONE);
	GLCheckError("glDrawBuffer rst");

	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_gl_texTargetType[srcTexRef.type], GL_NONE, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, g_gl_texTargetType[destTexRef.type], GL_NONE, 0);

	// change render targets back
	if(currentNumRTs)
		ChangeRenderTargets(ArrayCRef(currentRenderTarget, currentNumRTs), ArrayCRef(currentCRTSlice, currentNumRTs), currentDepthTarget);
	else
		ChangeRenderTargetToBackBuffer();
}

// Changes render target (MRT)
void ShaderAPIGL::ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets, ArrayCRef<int> rtSlice, const ITexturePtr& depthTarget, int depthSlice)
{
	ASSERT_MSG(!rtSlice.ptr() || renderTargets.numElem() == rtSlice.numElem(), "ChangeRenderTargets - renderTargets and rtSlice must be equal");

	uint glFrameBuffer = m_frameBuffer;

	if (glFrameBuffer == 0)
	{
		glGenFramebuffers(1, &glFrameBuffer);
		m_frameBuffer = glFrameBuffer;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, glFrameBuffer);

	for (int i = 0; i < renderTargets.numElem(); i++)
	{
		CGLTexture* colorRT = (CGLTexture*)renderTargets[i].Ptr();

		const int nCubeFace = rtSlice.ptr() ? rtSlice[i] : 0;

		if (colorRT->GetFlags() & TEXFLAG_CUBEMAP)
		{
			if (colorRT != m_pCurrentColorRenderTargets[i] || m_nCurrentCRTSlice[i] != nCubeFace)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER,
						GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + nCubeFace, colorRT->m_textures[0].glTexID, 0);

				m_nCurrentCRTSlice[i] = nCubeFace;
			}
		}
		else
		{
			if (colorRT != m_pCurrentColorRenderTargets[i])
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorRT->m_textures[0].glTexID, 0);
			}
		}

		m_pCurrentColorRenderTargets[i] = renderTargets[i];
	}

	if (renderTargets.numElem() != m_nCurrentRenderTargets)
	{
		for (int i = renderTargets.numElem(); i < m_nCurrentRenderTargets; i++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
			m_pCurrentColorRenderTargets[i] = nullptr;
			m_nCurrentCRTSlice[i] = -1;
		}

		if (renderTargets.numElem() == 0)
		{
			GLenum drawBufferSetting = GL_COLOR_ATTACHMENT0;
			glDrawBuffers(1, &drawBufferSetting);
			glReadBuffer(GL_NONE);
		}
		else
		{
			glDrawBuffers(renderTargets.numElem(), m_drawBuffers);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
		}

		m_nCurrentRenderTargets = renderTargets.numElem();
	}

	GLuint bestDepth = renderTargets.numElem() ? ((CGLTexture*)renderTargets[0].Ptr())->m_glDepthID : GL_NONE;
	GLuint bestTarget = GL_TEXTURE_2D;
	ETextureFormat bestFormat = renderTargets.numElem() ? FORMAT_D16 : FORMAT_NONE;
	
	CGLTexture* pDepth = (CGLTexture*)depthTarget.Ptr();

	if (pDepth != m_pCurrentDepthRenderTarget)
	{
		if(pDepth)
		{
			bestDepth = (pDepth->m_glTarget == GL_RENDERBUFFER) ? pDepth->m_glDepthID : pDepth->m_textures[0].glTexID;
			bestTarget = pDepth->m_glTarget == GL_RENDERBUFFER ? GL_RENDERBUFFER : GL_TEXTURE_2D;
			bestFormat = pDepth->GetFormat();
		}
		
		m_pCurrentDepthRenderTarget = depthTarget;
	}

	if (m_currentGLDepth != bestDepth)
	{
		bool isStencil = IsStencilFormat(bestFormat);
		
		if(bestTarget == GL_RENDERBUFFER)
		{
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, bestTarget, bestDepth);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, bestTarget, isStencil ? bestTarget : GL_NONE);
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, bestTarget, bestDepth, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, bestTarget, isStencil ? bestTarget : GL_NONE, 0);
		}
		
		m_currentGLDepth = bestDepth;
	}

	// like in D3D, set the viewport as texture size by default
	if (m_nCurrentRenderTargets > 0 && m_pCurrentColorRenderTargets[0] != nullptr)
	{
		SetViewport(IAARectangle(0, 0, m_pCurrentColorRenderTargets[0]->GetWidth(), m_pCurrentColorRenderTargets[0]->GetHeight()));
	}
	else if(m_pCurrentDepthRenderTarget != nullptr)
	{
		SetViewport(IAARectangle(0, 0, m_pCurrentDepthRenderTarget->GetWidth(), m_pCurrentDepthRenderTarget->GetHeight()));
	}
}

void ShaderAPIGL::InternalChangeFrontFace(int nCullFaceMode)
{
	if (nCullFaceMode != m_nCurrentFrontFace)
		glFrontFace(m_nCurrentFrontFace = nCullFaceMode);
}

// Changes back to backbuffer
void ShaderAPIGL::ChangeRenderTargetToBackBuffer()
{
	if (m_frameBuffer == 0)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_backbufferSize.x, m_backbufferSize.y);

	int numRTs = m_nCurrentRenderTargets;

	for (int i = 0; i < numRTs; i++)
		m_pCurrentColorRenderTargets[i] = nullptr;

	m_pCurrentDepthRenderTarget = nullptr;
	m_currentGLDepth = GL_NONE;
}

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

// Set Depth range for next primitives
void ShaderAPIGL::SetDepthRange(float fZNear,float fZFar)
{
#ifdef USE_GLES2
	glDepthRangef(fZNear,fZFar);

#else
	glDepthRange(fZNear,fZFar);
#endif // USE_GLES2

}

void ShaderAPIGL::ApplyBuffers()
{
	// HACK: use VAO to separate drawing
	glBindVertexArray(m_drawVAO);
	GLCheckError("bind VAO");

	// First change the vertex format
	ChangeVertexFormat(m_pSelectedVertexFormat);

	for (int i = 0; i < MAX_VERTEXSTREAM; i++)
		ChangeVertexBuffer(m_pSelectedVertexBuffers[i], i, m_nSelectedOffsets[i]);

	ChangeIndexBuffer(m_pSelectedIndexBuffer);

	glBindVertexArray(0);
}

// Changes the vertex format
void ShaderAPIGL::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	static CVertexFormatGL zero("", nullptr);

	CVertexFormatGL* pSelectedFormat = pVertexFormat ? (CVertexFormatGL*)pVertexFormat : &zero;
	CVertexFormatGL* pCurrentFormat = m_pCurrentVertexFormat ? (CVertexFormatGL*)m_pCurrentVertexFormat : &zero;

	if( pVertexFormat != pCurrentFormat)
	{
		for (int i = 0; i < m_caps.maxVertexAttributes; i++)
		{
			const CVertexFormatGL::eqGLVertAttrDesc_t& selDesc = pSelectedFormat->m_genericAttribs[i];
			const CVertexFormatGL::eqGLVertAttrDesc_t& curDesc = pCurrentFormat->m_genericAttribs[i];

			const bool shouldDisable = !selDesc.count && curDesc.count;
			const bool shouldEnable = selDesc.count && !curDesc.count;

			if (shouldDisable)
			{
				glDisableVertexAttribArray(i);
				GLCheckError("disable vtx attrib");

				glVertexAttribDivisor(i, 0);
				GLCheckError("divisor");
			}
		}

		m_pCurrentVertexFormat = pVertexFormat;
	}
}

// Changes the vertex buffer
void ShaderAPIGL::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer, int nStream, const intptr offset)
{
	CVertexBufferGL* pVB = (CVertexBufferGL*)pVertexBuffer;
	GLuint vbo = pVB ? pVB->GetCurrentBuffer() : 0;
	CVertexFormatGL* currentFormat = (CVertexFormatGL*)m_pCurrentVertexFormat;

	static const GLsizei glTypes[] = {
		GL_NONE,
		GL_UNSIGNED_BYTE,
		GL_HALF_FLOAT,
		GL_FLOAT,
	};

	static const GLboolean glNormTypes[] = {
		GL_FALSE,
		GL_TRUE,
		GL_FALSE,
		GL_FALSE,
	};

	const bool instanceBuffer = (nStream > 0) && pVB != nullptr && (pVB->GetFlags() & VERTBUFFER_FLAG_INSTANCEDATA);
	const bool vboChanged = m_currentGLVB[nStream] != vbo;

	const bool instancingChanged = !instanceBuffer && m_boundInstanceStream == nStream ||
									instanceBuffer && m_boundInstanceStream != nStream;

	if (vboChanged || offset != m_nCurrentOffsets[nStream] || m_pCurrentVertexFormat != m_pActiveVertexFormat[nStream] ||
		instanceBuffer && m_boundInstanceStream != nStream)
	{
		bool anyEnabled = false;

		if (currentFormat && vbo != 0)
		{
			for (int i = 0; i < m_caps.maxVertexAttributes; i++)
			{
				const CVertexFormatGL::eqGLVertAttrDesc_t& attrib = currentFormat->m_genericAttribs[i];
				if (attrib.streamId == nStream && attrib.count)
					anyEnabled = true;
			}
		}

		if (anyEnabled)
		{
			const int vertexSize = currentFormat->GetVertexSize(nStream);
			const char* base = (char*)(offset * vertexSize);

			glBindBuffer(GL_ARRAY_BUFFER, m_currentGLVB[nStream] = vbo);
			GLCheckError("bind array");

			for (int i = 0; i < m_caps.maxVertexAttributes; i++)
			{
				const CVertexFormatGL::eqGLVertAttrDesc_t& attrib = currentFormat->m_genericAttribs[i];

				if (attrib.streamId == nStream && attrib.count)
				{
					glEnableVertexAttribArray(i);
					GLCheckError("enable vtx attrib");

					glVertexAttribPointer(i, attrib.count, glTypes[attrib.attribFormat], glNormTypes[attrib.attribFormat], vertexSize, base + attrib.offsetInBytes);
					GLCheckError("attribpointer");

					glVertexAttribDivisor(i, instanceBuffer ? 1 : 0);
					GLCheckError("divisor");
				}
			}
		}

		m_pActiveVertexFormat[nStream] = m_pCurrentVertexFormat;
		m_pCurrentVertexBuffers[nStream] = pVertexBuffer;
		m_nCurrentOffsets[nStream] = offset;
	}

	if(pVertexBuffer && instancingChanged)
	{
		if (!instanceBuffer && m_boundInstanceStream != -1)
			m_boundInstanceStream = -1;
		else if (instanceBuffer && m_boundInstanceStream == -1)
			m_boundInstanceStream = nStream;
		else if (instanceBuffer && m_boundInstanceStream != -1 && nStream != m_boundInstanceStream)
		{
			ASSERT_FAIL("Already bound instancing stream at %d!!!", m_boundInstanceStream);
		}
	}
}

// Changes the index buffer
void ShaderAPIGL::ChangeIndexBuffer(IIndexBuffer *pIndexBuffer)
{
	CIndexBufferGL* pIB = (CIndexBufferGL*)pIndexBuffer;
	uint ibo = pIB ? pIB->GetCurrentBuffer() : 0;

	if (pIndexBuffer != m_pCurrentIndexBuffer || (ibo != m_currentGLIB))
	{
		if (pIndexBuffer == nullptr)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			GLCheckError("bind elem array 0");
		}
		else
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_currentGLIB = ibo);
			GLCheckError("bind elem array");
		}

		m_pCurrentIndexBuffer = pIndexBuffer;
	}
}

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

// Creates shader class for needed ShaderAPI
IShaderProgramPtr ShaderAPIGL::CreateNewShaderProgram(const char* pszName, const char* query)
{
	CGLShaderProgram* pNewProgram = PPNew CGLShaderProgram();
	pNewProgram->SetName((_Es(pszName)+query).GetData());

	CScopedMutex scoped(g_sapi_ShaderMutex);

	ASSERT_MSG(m_ShaderList.find(pNewProgram->m_nameHash) == m_ShaderList.end(), "Shader %s was already added", pNewProgram->GetName());
	m_ShaderList.insert(pNewProgram->m_nameHash, pNewProgram);

	return IShaderProgramPtr(pNewProgram);
}

// Destroy all shader
void ShaderAPIGL::FreeShaderProgram(IShaderProgram* pShaderProgram)
{
	CGLShaderProgram* pShader = (CGLShaderProgram*)(pShaderProgram);

	if(!pShader)
		return;

	{
		CScopedMutex m(g_sapi_ShaderMutex);
		auto it = m_ShaderList.find(pShader->m_nameHash);
		if (it.atEnd())
			return;
		m_ShaderList.remove(it);
	}

	g_renderWorker.Execute(__func__, [program = pShader->m_program]() {
		if (program)
		{
			glDeleteProgram(program);
			GLCheckError("delete gl program");
		}
		return 0;
	});
	pShader->m_program = GL_NONE;
}

void ShaderAPIGL::PreloadShadersFromCache()
{
	if (!r_preloadShaderCache.GetBool())
		return;

	int numShaders = 0;
	CFileSystemFind fsFind(SHADERCACHE_FOLDER "/*.scache", SP_ROOT);
	while (fsFind.Next())
	{
		const EqString filename = EqString::Format(SHADERCACHE_FOLDER "/%s", fsFind.GetPath());

		IFilePtr pStream = g_fileSystem->Open(filename.ToCString(), "rb", SP_ROOT);
		if (!pStream)
			continue;

		shaderCacheHdr_t scHdr;
		pStream->Read(&scHdr, 1, sizeof(shaderCacheHdr_t));

		if (scHdr.ident != SHADERCACHE_IDENT)
			continue;

		char nameStr[2048];
		ASSERT(scHdr.nameLen < sizeof(nameStr));
		pStream->Read(nameStr, scHdr.nameLen, 1);
		nameStr[scHdr.nameLen] = 0;

		pStream->Seek(0, VS_SEEK_SET);

		DevMsg(DEVMSG_RENDER, "Restoring shader '%s'\n", nameStr);

		CGLShaderProgram* pNewProgram = PPNew CGLShaderProgram();
		pNewProgram->SetName(nameStr);
		if (InitShaderFromCache(pNewProgram, pStream))
		{
			CScopedMutex scoped(g_sapi_ShaderMutex);

			ASSERT_MSG(m_ShaderList.find(pNewProgram->m_nameHash) == m_ShaderList.end(), "Shader %s was already added", pNewProgram->GetName());
			m_ShaderList.insert(pNewProgram->m_nameHash, pNewProgram);
			numShaders++;

			pNewProgram->Ref_Grab();
		}
		else
			delete pNewProgram;
	}

	Msg("Shader cache: %d shaders loaded\n", numShaders);
}

bool ShaderAPIGL::InitShaderFromCache(IShaderProgram* pShaderOutput, IVirtualStream* pStream, uint32 checksum)
{
	CGLShaderProgram* pShader = (CGLShaderProgram*)(pShaderOutput);

	if (!pShader)
		return false;

	// read pixel shader
	shaderCacheHdr_t scHdr;
	pStream->Read(&scHdr, 1, sizeof(shaderCacheHdr_t));

	if (checksum != 0 && checksum != scHdr.checksum || scHdr.ident != SHADERCACHE_IDENT)
	{
		MsgWarning("Shader cache for '%s' outdated\n", pShaderOutput->GetName());
		return false;
	}

	// skip name
	pStream->Seek(scHdr.nameLen, VS_SEEK_CUR);
#ifdef USE_GLES2
	// skip extra text
	pStream->Seek(scHdr.extraLen, VS_SEEK_CUR);

	// read program binary
	char* shaderText = (char*)PPAlloc(scHdr.psSize);
	pStream->Read(shaderText, 1, scHdr.psSize);
#else
	char* extraText = nullptr;
	if(scHdr.extraLen)
	{
		extraText = (char*)PPAlloc(scHdr.extraLen);
		pStream->Read(extraText, 1, scHdr.extraLen);
	}

	// read compressed shader text
	char* compressedData = (char*)PPAlloc(scHdr.psSize);
	pStream->Read(compressedData, 1, scHdr.psSize);

	char* shaderText = (char*)PPAlloc(scHdr.vsSize);
	const int dataSize = LZ4_decompress_safe(compressedData, (char*)shaderText, scHdr.psSize, scHdr.vsSize);

	if (dataSize != scHdr.vsSize)
	{
		MsgError("Unable to init shader from cache, please delete shader cache and try again\n");
		PPFree(extraText);
		return false;
	}

	shaderText[scHdr.vsSize-1] = 0;

	PPFree(compressedData);
#endif

	// read samplers and constants
	Array<GLShaderSampler_t> samplers(PP_SL);
	Array<GLShaderConstant_t> uniforms(PP_SL);
	samplers.setNum(scHdr.numSamplers);
	uniforms.setNum(scHdr.numConstants);

	pStream->Read(samplers.ptr(), scHdr.numSamplers, sizeof(GLShaderSampler_t));
	pStream->Read(uniforms.ptr(), scHdr.numConstants, sizeof(GLShaderConstant_t));

	int result = g_renderWorker.WaitForExecute(__func__, [&]() {

		GLint vsResult, fsResult, linkResult;

		// create GL program
		pShader->m_program = glCreateProgram();
		if (!GLCheckError("create program"))
			return -1;

#ifdef USE_GLES2
		glProgramBinary(pShader->m_program, scHdr.vsSize, shaderText, scHdr.psSize);
		if (!GLCheckError("set program binary"))
			return -1;
#else
		// vertex
		{
			EqString shaderString;
			shaderString.Append("#version 330\r\n");
			shaderString.Append("#define VERTEX\r\n");

			if (extraText != nullptr)
				shaderString.Append(extraText);

			const GLchar* sStr[] = {
				shaderString.ToCString(), "\r\n",
				shaderText
			};

			GLhandleARB	vertexShader = glCreateShader(GL_VERTEX_SHADER);
			if (!GLCheckError("create vertex shader"))
				return -1;

			glShaderSource(vertexShader, sizeof(sStr) / sizeof(sStr[0]), (const GLchar**)sStr, nullptr);
			glCompileShader(vertexShader);
			GLCheckError("compile vert shader");

			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vsResult);

			if (vsResult)
			{
				glAttachShader(pShader->m_program, vertexShader);
				GLCheckError("attach vert shader");

				glDeleteShader(vertexShader);
				GLCheckError("delete shaders");
			}
			else
			{
				char infoLog[2048];
				GLint len;

				glGetShaderInfoLog(vertexShader, sizeof(infoLog), &len, infoLog);
				MsgError("Vertex shader %s error:\n%s\n", pShader->GetName(), infoLog);

				// don't even report errors
				return -1;
			}
		}

		// fragment
		{
			EqString shaderString;
			shaderString.Append("#version 330\r\n");
			shaderString.Append("#define FRAGMENT\r\n");

			if (extraText != nullptr)
				shaderString.Append(extraText);

			const GLchar* sStr[] = {
				shaderString.ToCString(),"\r\n",
				shaderText
			};

			GLhandleARB	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			if (!GLCheckError("create fragment shader"))
				return -1;

			glShaderSource(fragmentShader, sizeof(sStr) / sizeof(sStr[0]), (const GLchar**)sStr, nullptr);
			glCompileShader(fragmentShader);
			GLCheckError("compile frag shader");

			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fsResult);

			if (fsResult)
			{
				glAttachShader(pShader->m_program, fragmentShader);
				GLCheckError("attach frag shader");

				glDeleteShader(fragmentShader);
				GLCheckError("delete shaders");
			}
			else
			{
				char infoLog[2048];
				GLint len;

				glGetShaderInfoLog(fragmentShader, sizeof(infoLog), &len, infoLog);
				MsgError("Fragment shader %s error:\n%s\n", pShader->GetName(), infoLog);

				// don't even report errors
				return -1;
			}
		}

		// link
		{
			// link program and go
			glLinkProgram(pShader->m_program);
			GLCheckError("link program");

			glGetProgramiv(pShader->m_program, GL_LINK_STATUS, &linkResult);

			if (!linkResult)
			{
				// don't even report errors
				return -1;
			}
		}
#endif

		// use freshly generated program to retirieve constants (uniforms) and samplers
		glUseProgram(pShader->m_program);
		GLCheckError("test use program");

		// intel buggygl fix
		if (m_vendor == VENDOR_INTEL)
		{
			glUseProgram(0);
			glUseProgram(pShader->m_program);
		}

		Map<int, GLShaderSampler_t>& samplerMap = pShader->m_samplers;
		Map<int, GLShaderConstant_t>& constantMap = pShader->m_constants;

		// assign
		for (int i = 0; i < samplers.numElem(); ++i)
		{
			samplers[i].index = i;
			glUniform1i(samplers[i].uniformLoc, i);
			samplerMap.insert(samplers[i].nameHash, samplers[i]);
		}

		for (int i = 0; i < uniforms.numElem(); ++i)
		{
			GLShaderConstant_t& uni = uniforms[i];
			uni.data = PPNew ubyte[uni.size];
			memset(uni.data, 0, uni.size);

			constantMap.insert(uniforms[i].nameHash, uni);
		}

		return 0;
	});

#ifndef USE_GLES2
	PPFree(extraText);
#endif
	PPFree(shaderText);

	if (result == -1)
	{
		MsgError("Unable to init shader from cache, please delete shader cache and try again\n");
		return false;
	}

	pShader->m_cacheChecksum = scHdr.checksum;

	return true;
}

// Load any shader from stream
bool ShaderAPIGL::CompileShadersFromStream(IShaderProgramPtr pShaderOutput,const ShaderProgCompileInfo& info, const char* extra)
{
	if(!pShaderOutput)
		return false;

	if (!m_caps.shadersSupportedFlags)
	{
		MsgError("CompileShadersFromStream - shaders unsupported\n");
		return false;
	}

	CGLShaderProgram* pShader = (CGLShaderProgram*)pShaderOutput.Ptr();

	const bool shaderCacheEnabled = !info.disableCache && !r_skipShaderCache.GetBool();
	const EqString cacheFileName = EqString::Format(SHADERCACHE_FOLDER "/%x.scache", pShader->m_nameHash);
	{
		bool needsCompile = true;
		if (shaderCacheEnabled)
		{
			IFilePtr pStream = g_fileSystem->Open(cacheFileName.ToCString(), "rb", SP_ROOT);
			
			if (pStream)
			{
				if (InitShaderFromCache(pShader, pStream, info.data.checksum))
					needsCompile = false;
			}
		}

		if (!needsCompile)
			return true;
	}

	if (!info.data.text)
		return false;

	EqString precision = "mediump";
	if (info.apiPrefs) 
	{
		precision = KV_GetValueString(info.apiPrefs->FindSection("precision"), 0, "mediump");
	}

	struct compileData
	{
		CGLShaderProgram* prog;
		GLint vsResult, fsResult, linkResult;

		// intermediates
		GLhandleARB	vertexShader;
		GLhandleARB	fragmentShader;
		// GLhandleARB geomShader;
		// GLhandleARB hullShader;

		int		programSize = 0;
		char*	programData = nullptr;
		GLenum	programFormat = GL_NONE;
	} cdata;

	cdata.prog = pShader;

	int result;

	// compile vertex
	if(info.data.text)
	{
		EqString shaderString;

#ifdef USE_GLES2
		shaderString.Append("#version 300 es\r\n");
		shaderString.Append("precision " + precision + " float;\r\n");
		shaderString.Append("precision mediump int;\r\n");
#else
		shaderString.Append("#version 330\r\n");
#endif // USE_GLES2
		shaderString.Append("#define VERTEX\r\n");

		if (extra != nullptr)
			shaderString.Append(extra);

		const GLchar* sStr[] = { 
			shaderString.ToCString(), "\r\n",
			info.data.boilerplate, "\r\n",
			"#line 1 0\r\n",
			info.data.text
		};

		result = g_renderWorker.WaitForExecute(__func__, [&cdata, sStr]() {

			// create GL program
			cdata.prog->m_program = glCreateProgram();
			if (!GLCheckError("create program"))
				return -1;

			cdata.vertexShader = glCreateShader(GL_VERTEX_SHADER);
			if (!GLCheckError("create vertex shader"))
				return -1;

			glShaderSource(cdata.vertexShader, sizeof(sStr) / sizeof(sStr[0]), (const GLchar **)sStr, nullptr);
			glCompileShader(cdata.vertexShader);
			GLCheckError("compile vert shader");

			glGetShaderiv(cdata.vertexShader, GL_COMPILE_STATUS, &cdata.vsResult);

			if (cdata.vsResult)
			{
				glAttachShader(cdata.prog->m_program, cdata.vertexShader);
				GLCheckError("attach vert shader");

				glDeleteShader(cdata.vertexShader);
				GLCheckError("delete shaders");
			}
			else
			{
				char infoLog[2048];
				GLint len;

				glGetShaderInfoLog(cdata.vertexShader, sizeof(infoLog), &len, infoLog);
				MsgError("Vertex shader %s error:\n%s\n", cdata.prog->GetName(), infoLog);

				return -1;
			}

			return 0;
		});

		if (result == -1)
		{
			MsgInfo("Shader files dump:");
			for (int i = 0; i < info.data.includes.numElem(); i++)
				MsgInfo("\t%d : %s\n", i + 1, info.data.includes[i].ToCString());

			return false;
		}
			
	}
	else
		return false; // vertex shader is required

	// compile fragment
	if(info.data.text)
	{
		EqString shaderString;

#ifdef USE_GLES2
		shaderString.Append("#version 300 es\r\n");
		shaderString.Append("precision " + precision + " float;\r\n");
		shaderString.Append("precision mediump int;\r\n");
#else
		shaderString.Append("#version 330\r\n");
#endif // USE_GLES2
		shaderString.Append("#define FRAGMENT\r\n");

		if (extra != nullptr)
			shaderString.Append(extra);

		const GLchar* sStr[] = { 
			shaderString.ToCString(),"\r\n",
			info.data.boilerplate,"\r\n",
			"#line 1 0\r\n",
			info.data.text
		};

		result = g_renderWorker.WaitForExecute(__func__, [&cdata, sStr]() {

			cdata.fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			if (!GLCheckError("create fragment shader"))
				return -1;

			glShaderSource(cdata.fragmentShader, sizeof(sStr) / sizeof(sStr[0]), (const GLchar**)sStr, nullptr);
			glCompileShader(cdata.fragmentShader);
			GLCheckError("compile frag shader");

			glGetShaderiv(cdata.fragmentShader, GL_COMPILE_STATUS, &cdata.fsResult);

			if (cdata.fsResult)
			{
				glAttachShader(cdata.prog->m_program, cdata.fragmentShader);
				GLCheckError("attach frag shader");

				glDeleteShader(cdata.fragmentShader);
				GLCheckError("delete shaders");
			}
			else
			{
				char infoLog[2048];
				GLint len;

				glGetShaderInfoLog(cdata.fragmentShader, sizeof(infoLog), &len, infoLog);
				MsgError("Pixel shader %s error:\n%s\n", cdata.prog->GetName(), infoLog);
				return -1;
			}

			return 0;
		});

		if (result == -1)
		{
			MsgInfo("Shader files dump:");
			for (int i = 0; i < info.data.includes.numElem(); i++)
				MsgInfo("\t%d : %s\n", i + 1, info.data.includes[i].ToCString());

			return false;
		}

	}
	else
		cdata.fsResult = GL_TRUE;

	Array<GLShaderSampler_t> samplers(PP_SL);
	Array<GLShaderConstant_t> uniforms(PP_SL);

	if(cdata.fsResult && cdata.vsResult)
	{
		const ShaderProgCompileInfo* pInfo = &info;

		EGraphicsVendor vendor = m_vendor;

		result = g_renderWorker.WaitForExecute(__func__, [&cdata, &samplers, &uniforms, &vendor, shaderCacheEnabled]() {
			// link program and go
			glLinkProgram(cdata.prog->m_program);
			GLCheckError("link program");

			glGetProgramiv(cdata.prog->m_program, GL_LINK_STATUS, &cdata.linkResult);

			if (!cdata.linkResult)
			{
				char infoLog[2048];
				GLint len;

				glGetProgramInfoLog(cdata.prog->m_program, sizeof(infoLog), &len, infoLog);
				MsgError("Shader '%s' link error: %s\n", cdata.prog->GetName(), infoLog);
				return -1;
			}

			// use freshly generated program to retirieve constants (uniforms) and samplers
			glUseProgram(cdata.prog->m_program);
			GLCheckError("test use program");

			// intel buggygl fix
			if (vendor == VENDOR_INTEL)
			{
				glUseProgram(0);
				glUseProgram(cdata.prog->m_program);
			}

			GLint uniformCount, maxLength;
			glGetProgramiv(cdata.prog->m_program, GL_ACTIVE_UNIFORMS, &uniformCount);
			glGetProgramiv(cdata.prog->m_program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

			if (maxLength == 0 && (uniformCount > 0 || uniformCount > 256))
			{
				if (vendor == VENDOR_INTEL)
					DevMsg(DEVMSG_RENDER, "Guess who? It's Intel! uniformCount to be zeroed\n");
				else
					DevMsg(DEVMSG_RENDER, "I... didn't... expect... that! uniformCount to be zeroed\n");

				uniformCount = 0;
			}

			samplers.resize(uniformCount);
			uniforms.resize(uniformCount);

			char* tmpName = PPNew char[maxLength+1];

			DevMsg(DEVMSG_RENDER, "[DEBUG] getting UNIFORMS from '%s'\n", cdata.prog->GetName());

			for (int i = 0; i < uniformCount; i++)
			{
				GLenum type;
				GLint length, size;

				glGetActiveUniform(cdata.prog->m_program, i, maxLength, &length, &size, &type, tmpName);

	#ifdef USE_GLES2
				if (type >= GL_SAMPLER_2D && type <= GL_SAMPLER_CUBE_SHADOW)
	#else
				if (type >= GL_SAMPLER_1D && type <= GL_SAMPLER_2D_RECT_SHADOW)
	#endif // USE_GLES3
				{
					const int samplerNum = samplers.numElem();

					// Assign samplers to image units
					const GLint location = glGetUniformLocation(cdata.prog->m_program, tmpName);
					glUniform1i(location, samplerNum);

					DevMsg(DEVMSG_RENDER, "[DEBUG] retrieving sampler '%s' at %d (location = %d)\n", tmpName, samplerNum, location);

					GLShaderSampler_t& sp = samplers.append();
					sp.index = samplerNum;
					sp.uniformLoc = location;
					strcpy(sp.name, tmpName);
				}
				else
				{
					// Store all non-gl uniforms
					if (!strncmp(tmpName, "gl_", 3))
						continue;

					const int uniformNum = uniforms.numElem();

					DevMsg(DEVMSG_RENDER, "[DEBUG] retrieving uniform '%s' at %d\n", tmpName, uniformNum);

					// also cut off name at the bracket
					char* bracket = strchr(tmpName, '[');
					if (!bracket || (bracket[1] == '0' && bracket[2] == ']'))
					{
						if (bracket)
							length = (GLint)(bracket - tmpName);

						GLShaderConstant_t& uni = uniforms.append();

						strncpy(uni.name, tmpName, length);
						uni.name[length] = 0;

						uni.uniformLoc = glGetUniformLocation(cdata.prog->m_program, tmpName);
						uni.type = GetConstantType(type);
							
						int totalElements = 1;

						// detect the array size by getting next valid uniforms
						if (bracket)
						{
							for (int j = i + 1; j < uniformCount; j++)
							{
								GLint tmpLength, tmpSize;
								GLenum tmpType;
								glGetActiveUniform(cdata.prog->m_program, j, maxLength, &tmpLength, &tmpSize, &tmpType, tmpName);

								// we only expect arrays
								bracket = strchr(tmpName, '[');
								if(bracket)
								{
									*bracket = '\0';
									if(!stricmp(uni.name, tmpName))
										totalElements++;
									else
										break;
								}
								else
								{
									break;
								}
							}

							i += totalElements-1;
						} // bracket

						uni.nElements = size * totalElements;
					}
				}// Sampler types?
			}

#ifdef USE_GLES2
			if (shaderCacheEnabled)
			{
				glGetProgramiv(cdata.prog->m_program, GL_PROGRAM_BINARY_LENGTH, &cdata.programSize);
				GLCheckError("get program size");

				cdata.programData = (char*)PPAlloc(cdata.programSize);

				glGetProgramBinary(cdata.prog->m_program, cdata.programSize, &cdata.programSize, &cdata.programFormat, cdata.programData);
				GLCheckError("get program binary");
			}
#endif

			delete [] tmpName;

			return 0;
		});

		if (result == -1)
			return false;

		Map<int, GLShaderSampler_t>& samplerMap = cdata.prog->m_samplers;
		Map<int, GLShaderConstant_t>& constantMap = cdata.prog->m_constants;

		// build a map
		for (int i = 0; i < samplers.numElem(); ++i)
		{
			samplers[i].nameHash = StringToHash(samplers[i].name);
			samplerMap.insert(samplers[i].nameHash, samplers[i]);
		}

		for (int i = 0; i < uniforms.numElem(); ++i)
		{
			GLShaderConstant_t& uni = uniforms[i];
			uni.nameHash = StringToHash(uniforms[i].name);

			// init uniform data
			uni.size = s_constantTypeSizes[uni.type] * uni.nElements;
			uni.data = PPNew ubyte[uni.size];
			memset(uni.data, 0, uni.size);
			uni.dirty = false;

			constantMap.insert(uniforms[i].nameHash, uniforms[i]);
		}
	}
	else
		return false;

	// store the shader cache data
	if (shaderCacheEnabled && info.data.text != nullptr)
	{
		IFilePtr pStream = g_fileSystem->Open(cacheFileName.ToCString(), "wb", SP_ROOT);
		if (pStream)
		{
			shaderCacheHdr_t scHdr;
			scHdr.ident = SHADERCACHE_IDENT;
			scHdr.checksum = info.data.checksum;
			scHdr.numSamplers = samplers.numElem();
			scHdr.numConstants = uniforms.numElem();
#ifdef USE_GLES2
			scHdr.vsSize = cdata.programFormat;
			scHdr.psSize = cdata.programSize;
#else
			// we really don't have any binary in GL3x
			// compress and write full text instead
			EqString shaderWithBoilerplate;
			shaderWithBoilerplate.Append(info.data.boilerplate);
			shaderWithBoilerplate.Append("\r\n");
			shaderWithBoilerplate.Append("#line 1 0\r\n");
			shaderWithBoilerplate.Append(info.data.text);

			const int dataSize = shaderWithBoilerplate.Length() + 1;
			char* compressedData = (char*)PPAlloc(dataSize + 128);
			const int compressedSize = LZ4_compress_fast(shaderWithBoilerplate.GetData(), compressedData, dataSize, dataSize + 128, 1);

			scHdr.vsSize = dataSize;
			scHdr.psSize = compressedSize;
#endif
			scHdr.nameLen = pShader->m_szName.Length();
			scHdr.extraLen = extra ? (strlen(extra) + 1) : 0;

			pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));
			pStream->Write(pShader->m_szName.GetData(), scHdr.nameLen, 1);

			if(extra)
				pStream->Write(extra, scHdr.extraLen, 1);

#ifdef USE_GLES2
			pStream->Write(cdata.programData, cdata.programSize, 1);
			PPFree(cdata.programData);
#else
			pStream->Write(compressedData, compressedSize, 1);
			PPFree(compressedData);
#endif

			pStream->Write(samplers.ptr(), samplers.numElem(), sizeof(GLShaderSampler_t));
			pStream->Write(uniforms.ptr(), uniforms.numElem(), sizeof(GLShaderConstant_t));
		}
		else
			MsgError("ERROR: Cannot create shader cache file for %s\n", pShaderOutput->GetName());
	}

	return true;
}

// Set current shader for rendering
void ShaderAPIGL::SetShader(IShaderProgramPtr pShader)
{
	m_pSelectedShader = pShader;
}

// RAW Constant (Used for structure types, etc.)
void ShaderAPIGL::SetShaderConstantRaw(int nameHash, const void *data, int nSize)
{
	if (data == nullptr || nSize == 0)
		return;

	CGLShaderProgram* prog = (CGLShaderProgram*)m_pSelectedShader.Ptr();

	if (!prog)
		return;

	const Map<int, GLShaderConstant_t>& constantsMap = prog->m_constants;
	auto it = constantsMap.find(nameHash);
	if (it.atEnd())
		return;

	GLShaderConstant_t& uni = *it;

	const uint maxSize = min((uint)nSize, uni.size);

	if (memcmp(uni.data, data, maxSize))
	{
		memcpy(uni.data, data, maxSize);
		uni.dirty = true;
	}

	//MsgError("[SHADER] error: constant '%s' not found\n", pszName);
}

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

IVertexFormat* ShaderAPIGL::CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc)
{
	CVertexFormatGL *pVertexFormat = PPNew CVertexFormatGL(name, formatDesc);

	{
		CScopedMutex m(g_sapi_VBMutex);
		m_VFList.append(pVertexFormat);
	}

	return pVertexFormat;
}

IVertexBuffer* ShaderAPIGL::CreateVertexBuffer(const BufferInfo& bufferInfo)
{
	CVertexBufferGL* pVB = PPNew CVertexBufferGL();

	pVB->m_bufElemCapacity = bufferInfo.elementCapacity;
	pVB->m_bufElemSize = bufferInfo.elementSize;
	pVB->m_access = bufferInfo.accessType;
	pVB->m_flags = bufferInfo.flags;

	DevMsg(DEVMSG_RENDER,"Creating VBO with size %i KB\n", pVB->GetSizeInBytes() / 1024);

	const int numBuffers = (bufferInfo.accessType == BUFFER_DYNAMIC) ? MAX_VB_SWITCHING : 1;
	const int sizeInBytes = pVB->m_bufElemCapacity * pVB->m_bufElemSize;

	int result = g_renderWorker.WaitForExecute(__func__, 
		[numBuffers, sizeInBytes, pVB, bufAccess = bufferInfo.accessType, data = bufferInfo.data, dataSize = bufferInfo.dataSize]() {

		glGenBuffers(numBuffers, pVB->m_rhiBuffer);

		if (!GLCheckError("gen vert buffer"))
			return -1;

		if (data)
		{
			for (int i = 0; i < numBuffers; i++)
			{
				glBindBuffer(GL_ARRAY_BUFFER, pVB->m_rhiBuffer[i]);
				GLCheckError("bind buffer");

				glBufferData(GL_ARRAY_BUFFER, min(dataSize, sizeInBytes), data, g_gl_bufferUsages[bufAccess]);
				GLCheckError("upload vtx data");
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		return 0;
	});

	for (int i = 0; i < numBuffers; i++)
	{
		ASSERT_MSG(pVB->m_rhiBuffer[i] != 0, "Vertex buffer %d is not generated, requested count: %d", i, numBuffers);
	}

	if (result == -1)
	{
		ASSERT_FAIL("Vertex buffer creation failed!");
		delete pVB;
		return nullptr;
	}

	{
		CScopedMutex m(g_sapi_VBMutex);
		m_VBList.append(pVB);
	}

	return pVB;
}

IIndexBuffer* ShaderAPIGL::CreateIndexBuffer(const BufferInfo& bufferInfo)
{
	ASSERT(bufferInfo.elementSize >= sizeof(short));
	ASSERT(bufferInfo.elementSize <= sizeof(int));

	CIndexBufferGL* pIB = PPNew CIndexBufferGL();

	pIB->m_bufElemCapacity = bufferInfo.elementCapacity;
	pIB->m_bufElemSize = bufferInfo.elementSize;
	pIB->m_access = bufferInfo.accessType;

	DevMsg(DEVMSG_RENDER,"Creating IBO with size %i KB\n", (bufferInfo.elementSize * bufferInfo.elementCapacity) / 1024);

	const int sizeInBytes = pIB->m_bufElemCapacity * pIB->m_bufElemSize;
	const int numBuffers = (bufferInfo.accessType == BUFFER_DYNAMIC) ? MAX_IB_SWITCHING : 1;

	int result = g_renderWorker.WaitForExecute(__func__,
		[numBuffers, sizeInBytes, pIB, bufAccess = bufferInfo.accessType, data = bufferInfo.data, dataSize = bufferInfo.dataSize]() {

		glGenBuffers(numBuffers, pIB->m_rhiBuffer);
		if (!GLCheckError("gen idx buffer"))
			return -1;

		if (data)
		{
			for (int i = 0; i < numBuffers; i++)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIB->m_rhiBuffer[i]);
				GLCheckError("bind buff");

				glBufferData(GL_ELEMENT_ARRAY_BUFFER, min(dataSize, sizeInBytes), data, g_gl_bufferUsages[bufAccess]);
				GLCheckError("upload idx data");
			}

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		return 0;
	});

	for (int i = 0; i < numBuffers; i++)
	{
		ASSERT_MSG(pIB->m_rhiBuffer[i] != 0, "Index buffer %d is not generated, requested count: %d", i, numBuffers);
	}

	if (result == -1)
	{
		ASSERT_FAIL("Index buffer creation failed!");
		delete pIB;
		return nullptr;
	}

	{
		CScopedMutex m(g_sapi_IBMutex);
		m_IBList.append(pIB);
	}

	return pIB;
}

// Destroy vertex format
void ShaderAPIGL::DestroyVertexFormat(IVertexFormat* pFormat)
{
	CVertexFormatGL* pVF = (CVertexFormatGL*)(pFormat);
	if (!pVF)
		return;

	bool deleted = false;
	{
		CScopedMutex m(g_sapi_VBMutex);
		deleted = m_VFList.remove(pVF);
	}

	if(deleted)
	{
		DevMsg(DEVMSG_RENDER, "Destroying vertex format\n");
		delete pVF;
	}
}

// Destroy vertex buffer
void ShaderAPIGL::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	CVertexBufferGL* pVB = (CVertexBufferGL*)(pVertexBuffer);
	if (!pVB)
		return;

	bool deleted = false;
	{
		CScopedMutex m(g_sapi_VBMutex);
		deleted = m_VBList.remove(pVB);
	}

	if(deleted)
	{
		const int numBuffers = (pVB->m_access == BUFFER_DYNAMIC) ? MAX_VB_SWITCHING : 1;
		uint* tempArray = PPNew uint[numBuffers];
		memcpy(tempArray, pVB->m_rhiBuffer, numBuffers * sizeof(uint));

		delete pVB;

		g_renderWorker.Execute(__func__, [numBuffers, tempArray]() {

			glDeleteBuffers(numBuffers, tempArray);
			GLCheckError("delete vertex buffer");

			delete [] tempArray;

			return 0;
		});
	}
}

// Destroy index buffer
void ShaderAPIGL::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	CIndexBufferGL* pIB = (CIndexBufferGL*)(pIndexBuffer);

	if (!pIB)
		return;

	bool deleted = false;
	{
		CScopedMutex m(g_sapi_IBMutex);
		deleted = m_IBList.remove(pIB);
	}

	if(deleted)
	{
		const int numBuffers = (pIB->m_access == BUFFER_DYNAMIC) ? MAX_IB_SWITCHING : 1;
		uint* tempArray = PPNew uint[numBuffers];
		memcpy(tempArray, pIB->m_rhiBuffer, numBuffers * sizeof(uint));

		delete pIndexBuffer;

		g_renderWorker.Execute(__func__, [numBuffers, tempArray]() {
			
			glDeleteBuffers(numBuffers, tempArray);
			GLCheckError("delete index buffer");

			delete [] tempArray;

			return 0;
		});		
	}
}

//-------------------------------------------------------------
// Primitive drawing
//-------------------------------------------------------------

#define BUFFER_OFFSET(i) ((char *) nullptr + (i))

// Indexed primitive drawer
void ShaderAPIGL::DrawIndexedPrimitives(EPrimTopology nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
	ASSERT(nVertices > 0);

	if(nIndices <= 2)
		return;

	if (!m_pCurrentIndexBuffer)
		return;

	int numInstances = 0;
	if(m_boundInstanceStream != -1 && m_pCurrentVertexBuffers[m_boundInstanceStream])
		numInstances = m_pCurrentVertexBuffers[m_boundInstanceStream]->GetVertexCount();

	glBindVertexArray(m_drawVAO);
	GLCheckError("bind VAO");

	const uint indexSize = m_pCurrentIndexBuffer->GetIndexSize();

	if (nBaseVertex > 0)
	{
		ASSERT_MSG(numInstances == 0, "nBaseVertex is not supported when drawing instanced in OpenGL :(")

		glDrawRangeElements(g_gl_primitiveType[nType], nBaseVertex, nVertices, nIndices, indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, BUFFER_OFFSET(indexSize * nFirstIndex));
	}
	else
	{
		if (numInstances)
			glDrawElementsInstanced(g_gl_primitiveType[nType], nIndices, indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, BUFFER_OFFSET(indexSize * nFirstIndex), numInstances);
		else
			glDrawElements(g_gl_primitiveType[nType], nIndices, indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, BUFFER_OFFSET(indexSize * nFirstIndex));
	}

	GLCheckError("draw elements");

	glBindVertexArray(0);

	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += s_primCount[nType](nIndices);
}

// Draw elements
void ShaderAPIGL::DrawNonIndexedPrimitives(EPrimTopology nType, int nFirstVertex, int nVertices)
{
	if(m_pCurrentVertexFormat == nullptr)
		return;

	if (nVertices <= 2)
		return;

	int numInstances = 0;
	if(m_boundInstanceStream != -1 && m_pCurrentVertexBuffers[m_boundInstanceStream])
		numInstances = m_pCurrentVertexBuffers[m_boundInstanceStream]->GetVertexCount();

	glBindVertexArray(m_drawVAO);
	GLCheckError("bind VAO");

	if(numInstances)
		glDrawArraysInstanced(g_gl_primitiveType[nType], nFirstVertex, nVertices, numInstances);
	else
		glDrawArrays(g_gl_primitiveType[nType], nFirstVertex, nVertices);
	GLCheckError("draw arrays");

	glBindVertexArray(0);

	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += s_primCount[nType](nVertices);
}

bool ShaderAPIGL::IsDeviceActive() const
{
	return !m_deviceLost;
}

//-------------------------------------------------------------
// State manipulation
//-------------------------------------------------------------

// creates blending state
IRenderState* ShaderAPIGL::CreateBlendingState( const BlendStateParams &blendDesc )
{
	CGLBlendingState* pState = nullptr;

	for(int i = 0; i < m_BlendStates.numElem(); i++)
	{
		pState = (CGLBlendingState*)m_BlendStates[i];

		if(blendDesc.enable == pState->m_params.enable)
		{
			if(blendDesc.enable == true)
			{
				if(blendDesc.srcFactor == pState->m_params.srcFactor &&
					blendDesc.dstFactor == pState->m_params.dstFactor &&
					blendDesc.blendFunc == pState->m_params.blendFunc &&
					blendDesc.mask == pState->m_params.mask)
				{
					pState->Ref_Grab();
					return pState;
				}
			}
			else
			{
				pState->Ref_Grab();
				return pState;
			}
		}
	}

	pState = PPNew CGLBlendingState;
	pState->m_params = blendDesc;

	m_BlendStates.append(pState);

	pState->Ref_Grab();

	return pState;
}

// creates depth/stencil state
IRenderState* ShaderAPIGL::CreateDepthStencilState( const DepthStencilStateParams &depthDesc )
{
	CGLDepthStencilState* pState = nullptr;

	for(int i = 0; i < m_DepthStates.numElem(); i++)
	{
		pState = (CGLDepthStencilState*)m_DepthStates[i];

		if(depthDesc.depthWrite == pState->m_params.depthWrite &&
			depthDesc.depthTest == pState->m_params.depthTest &&
			depthDesc.depthFunc == pState->m_params.depthFunc &&
			depthDesc.stencilTest == pState->m_params.stencilTest &&
			depthDesc.useDepthBias == pState->m_params.useDepthBias)
		{
			// if we searching for stencil test
			if(depthDesc.stencilTest)
			{
				if (depthDesc.stencilFront.depthFailOp == pState->m_params.stencilFront.depthFailOp &&
					depthDesc.stencilFront.failOp == pState->m_params.stencilFront.failOp &&
					depthDesc.stencilFront.passOp == pState->m_params.stencilFront.passOp &&
					depthDesc.stencilFront.compareFunc == pState->m_params.stencilFront.compareFunc &&
					depthDesc.stencilMask == pState->m_params.stencilMask &&
					depthDesc.stencilWriteMask == pState->m_params.stencilWriteMask &&
					depthDesc.stencilRef == pState->m_params.stencilRef)
				{
					pState->Ref_Grab();
					return pState;
				}
			}
			else
			{
				pState->Ref_Grab();
				return pState;
			}
		}
	}

	pState = PPNew CGLDepthStencilState;
	pState->m_params = depthDesc;

	m_DepthStates.append(pState);

	pState->Ref_Grab();

	return pState;
}

// creates rasterizer state
IRenderState* ShaderAPIGL::CreateRasterizerState( const RasterizerStateParams &rasterDesc )
{
	CGLRasterizerState* pState = nullptr;

	for(int i = 0; i < m_RasterizerStates.numElem(); i++)
	{
		pState = (CGLRasterizerState*)m_RasterizerStates[i];

		if(rasterDesc.cullMode == pState->m_params.cullMode &&
			rasterDesc.fillMode == pState->m_params.fillMode &&
			rasterDesc.multiSample == pState->m_params.multiSample &&
			rasterDesc.scissor == pState->m_params.scissor)
		{
			pState->Ref_Grab();
			return pState;
		}
	}

	pState = PPNew CGLRasterizerState();
	pState->m_params = rasterDesc;

	pState->Ref_Grab();

	m_RasterizerStates.append(pState);

	return pState;
}

// completely destroys shader
void ShaderAPIGL::DestroyRenderState( IRenderState* pState, bool removeAllRefs )
{
	if(!pState)
		return;

	if(!pState->Ref_Drop() && !removeAllRefs)
	{
		return;
	}

	CScopedMutex scoped(g_sapi_Mutex);

	switch(pState->GetType())
	{
		case RENDERSTATE_BLENDING:
			delete ((CGLBlendingState*)pState);
			m_BlendStates.remove(pState);
			break;
		case RENDERSTATE_RASTERIZER:
			delete ((CGLRasterizerState*)pState);
			m_RasterizerStates.remove(pState);
			break;
		case RENDERSTATE_DEPTHSTENCIL:
			delete ((CGLDepthStencilState*)pState);
			m_DepthStates.remove(pState);
			break;
		default:
			break;
	}
}

// sets viewport
void ShaderAPIGL::SetViewport(const IAARectangle& rect)
{
	ShaderAPI_Base::SetViewport(rect);

	const IVector2D rectSize = rect.GetSize();
	glViewport(rect.leftTop.x, rect.leftTop.y, rectSize.x, rectSize.y);
	GLCheckError("set viewport");
}

// sets scissor rectangle
void ShaderAPIGL::SetScissorRectangle( const IAARectangle &rect )
{
    IAARectangle scissor(rect);

    scissor.leftTop.y = m_backbufferSize.y - scissor.leftTop.y;
    scissor.rightBottom.y = m_backbufferSize.y - scissor.rightBottom.y;
    QuickSwap(scissor.leftTop.y, scissor.rightBottom.y);

    const IVector2D scissorSize = scissor.GetSize();
	glScissor( scissor.leftTop.x, scissor.leftTop.y, scissorSize.x, scissorSize.y);
	GLCheckError("set scissor");
}

// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
void ShaderAPIGL::SetTexture(int nameHash, const ITexturePtr& pTexture )
{
	if (nameHash == 0)
	{
		ASSERT_FAIL("SetTexture requires name");
		return;
	}

	if (!m_pSelectedShader)
		return;

	const Map<int, GLShaderSampler_t>& samplerMap = ((CGLShaderProgram*)m_pSelectedShader.Ptr())->m_samplers;

	auto it = samplerMap.find(nameHash);
	if (it.atEnd())
		return;

	const int unitIndex = (*it).index;

	if (unitIndex == -1)
		return;

	SetTextureAtIndex(pTexture, unitIndex);
}

void ShaderAPIGL::StepProgressiveLodTextures()
{
	if (!m_progressiveTextures.size())
		return;

	g_sapi_ProgressiveTextureMutex.Lock();
	auto it = m_progressiveTextures.begin();
	g_sapi_ProgressiveTextureMutex.Unlock();

	int numTransferred = 0;
	while (!it.atEnd() && numTransferred < TEXTURE_TRANSFER_MAX_TEXTURES_PER_FRAME)
	{
		CGLTexture* nextTexture = nullptr;
		{
			CScopedMutex m(g_sapi_ProgressiveTextureMutex);
			nextTexture = it.key();
			++it;
		}

		EProgressiveStatus status = nextTexture->StepProgressiveLod();
		if (status == PROGRESSIVE_STATUS_COMPLETED)
		{
			CScopedMutex m(g_sapi_ProgressiveTextureMutex);
			m_progressiveTextures.remove(nextTexture);
			++numTransferred;
		}

		if (status == PROGRESSIVE_STATUS_DID_UPLOAD)
			++numTransferred;
	}
}