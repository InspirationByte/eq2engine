//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "ShaderAPIGL.h"

#include "CGLTexture.h"
#include "VertexFormatGL.h"
#include "VertexBufferGL.h"
#include "IndexBufferGL.h"
#include "GLShaderProgram.h"
#include "GLRenderState.h"
#include "GLOcclusionQuery.h"

#include "shaderapigl_def.h"

#include "imaging/ImageLoader.h"

#include "core/DebugInterface.h"
#include "core/IConsoleCommands.h"

#include "utils/strtools.h"
#include "utils/KeyValues.h"
#include "utils/eqthread.h"
#include "ds/function.h"

#include <atomic>

extern ShaderAPIGL g_shaderApi;

#define WORK_PENDING_MARKER 0x1d1d0001

class GLWorkerThread : CEqThread
{
	friend class ShaderAPIGL;

public:
	using FUNC_TYPE = EqFunction<int()>;

	GLWorkerThread()
	{
		m_started = false;
	}

	void Init()
	{
		StartWorkerThread("GLWorker");
		m_started = true;
	}

	void Shutdown()
	{
		m_started = false;
		SignalWork();
		StopThread();

		// delete work
		work_t* work = m_pendingWork;
		while(work)
		{
			work_t* nextWork = work->next;
			delete work;
			work = nextWork;
		}
		m_pendingWork = nullptr;
	}

	// syncronous execution
	int WaitForExecute(const char* name, FUNC_TYPE f)
	{
		return AddWork(name, f, true);
	}

	// asyncronous execution
	void Execute(const char* name, FUNC_TYPE f)
	{
		AddWork(name, f, false);
	}

protected:
	int Run();

	int AddWork(const char* name, FUNC_TYPE f, bool blocking);

	struct work_t
	{
		work_t(const char* _name, FUNC_TYPE f, uint id, bool block) :
			func(f)
		{
			name = _name;
			result = WORK_PENDING_MARKER;
			workId = id;
			blocking = block;
		}

		std::atomic<work_t*>	next{ nullptr };
		FUNC_TYPE				func;

		const char*				name;
		
		volatile int			result;
		uint					workId;
		bool					blocking;
	};

	int WaitForResult(work_t* work);

	std::atomic<work_t*>	m_pendingWork;

	uint					m_workCounter;
	bool					m_started;
};

int GLWorkerThread::WaitForResult(work_t* work)
{
	ASSERT(work);

	DevMsg(DEVMSG_SHADERAPI, "WaitForResult for %s (workId %d)\n", work->name, work->workId);

	// wait
	do
	{
		if (work->result != WORK_PENDING_MARKER)
			return  work->result;

		// HACK: weird hack to make this unfreeze itself
		if(IsWorkDone() && !m_pendingWork)
		{
			m_pendingWork = work;
			SignalWork();
		}

		Platform_Sleep(1);
	} while (true);

	return 0;
}

int GLWorkerThread::AddWork(const char* name, FUNC_TYPE f, bool blocking)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (blocking && thisThreadId == g_shaderApi.m_mainThreadId) // not required for main thread
		return f();

	work_t* work = new work_t(name, f, m_workCounter++, blocking);

	// chain link
	work->next = m_pendingWork.load();
	m_pendingWork = work;

	SignalWork();

	if (blocking)
		return WaitForResult(work);

	return 0;
}

int GLWorkerThread::Run()
{
	work_t* currentWork = nullptr;

	// find some work
	while (!currentWork)
	{
		// search for work
		{
			currentWork = m_pendingWork.load();
			if(currentWork)
			{
				m_pendingWork = currentWork->next.load();
				currentWork->next = nullptr;
			}
			else
			{
				m_pendingWork = nullptr;
				break;
			}
		}

		if (currentWork && currentWork->func)
		{
			DevMsg(DEVMSG_SHADERAPI, "BeginAsyncOperation for %s (workId %d)\n", currentWork->name, currentWork->workId);
			g_shaderApi.BeginAsyncOperation(GetThreadID());

			// run work
			currentWork->result = currentWork->func();

			DevMsg(DEVMSG_SHADERAPI, "EndAsyncOperation for %s (workId %d)\n", currentWork->name, currentWork->workId);
			g_shaderApi.EndAsyncOperation();

			delete currentWork;
			currentWork = nullptr;
		}

		if (!m_started)
			break;
	}

	return 0;
}

GLWorkerThread glWorker;

#ifdef PLAT_LINUX
#include "glx_caps.hpp"
#endif // PLAT_LINUX

ConVar gl_report_errors("gl_report_errors", "0");
ConVar gl_break_on_error("gl_break_on_error", "0");
ConVar gl_bypass_errors("gl_bypass_errors", "0");

bool GLCheckError(const char* op)
{
	GLenum lastError = glGetError();
	if(lastError != GL_NO_ERROR)
	{
		ASSERT(!gl_break_on_error.GetBool());

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

		if(gl_report_errors.GetBool())
			MsgError("*OGL* error occured while '%s' (%s)\n", op, errString.ToCString());

		return gl_bypass_errors.GetBool();
	}

	return true;
}

typedef GLvoid (APIENTRY *UNIFORM_FUNC)(GLint location, GLsizei count, const void *value);
typedef GLvoid (APIENTRY *UNIFORM_MAT_FUNC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

ER_ConstantType GetConstantType(GLenum type)
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

	return (ER_ConstantType) -1;
}

void* s_uniformFuncs[CONSTANT_TYPE_COUNT] = {};

ShaderAPIGL::~ShaderAPIGL()
{

}

ShaderAPIGL::ShaderAPIGL() : ShaderAPI_Base()
{
	Msg("Initializing OpenGL Shader API...\n");

	m_nCurrentRenderTargets = 0;

	m_nCurrentFrontFace = 0;

	m_nCurrentSrcFactor = BLENDFACTOR_ONE;
	m_nCurrentDstFactor = BLENDFACTOR_ZERO;
	m_nCurrentBlendFunc = BLENDFUNC_ADD;

	m_nCurrentDepthFunc = COMP_LEQUAL;
	m_bCurrentDepthTestEnable = false;
	m_bCurrentDepthWriteEnable = false;

	m_bCurrentMultiSampleEnable = false;
	m_bCurrentScissorEnable = false;
	m_nCurrentCullMode = CULL_BACK;
	m_nCurrentFillMode = FILL_SOLID;

	m_fCurrentDepthBias = 0.0f;
	m_fCurrentSlopeDepthBias = 0.0f;

	m_nCurrentMask = COLORMASK_ALL;
	m_bCurrentBlendEnable = false;

	m_frameBuffer = 0;
	m_depthBuffer = 0;

	m_nCurrentMatrixMode = MATRIXMODE_VIEW;

	m_boundInstanceStream = -1;
	memset(m_currentGLVB, 0, sizeof(m_currentGLVB));
	memset(m_currentGLTextures, 0, sizeof(m_currentGLTextures));
	m_currentGLIB = 0;

	m_asyncOperationActive = false;
}

void ShaderAPIGL::PrintAPIInfo()
{
	Msg("ShaderAPI: ShaderAPIGL\n");

	Msg("  Maximum texture anisotropy: %d\n", m_caps.maxTextureAnisotropicLevel);
	Msg("  Maximum drawable textures: %d\n", m_caps.maxTextureUnits);
	Msg("  Maximum vertex textures: %d\n", m_caps.maxVertexTextureUnits);
	Msg("  Maximum texture size: %d x %d\n", m_caps.maxTextureSize, m_caps.maxTextureSize);

	Msg("  Instancing supported: %d\n", m_caps.isInstancingSupported);

	MsgInfo("------ Loaded textures ------\n");

	CScopedMutex scoped(m_Mutex);
	for(int i = 0; i < m_TextureList.numElem(); i++)
	{
		CGLTexture* pTexture = (CGLTexture*)m_TextureList[i];

		MsgInfo("     %s (%d) - %dx%d\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(),pTexture->GetHeight());
	}
}

// Init + Shurdown
void ShaderAPIGL::Init( shaderAPIParams_t &params)
{
	const char* vendorStr = (const char *) glGetString(GL_VENDOR);

	if(xstristr(vendorStr, "nvidia"))
		m_vendor = VENDOR_NV;
	else if(xstristr(vendorStr, "ati") || xstristr(vendorStr, "amd") || xstristr(vendorStr, "radeon"))
		m_vendor = VENDOR_ATI;
	else if(xstristr(vendorStr, "intel"))
		m_vendor = VENDOR_INTEL;
	else
		m_vendor = VENDOR_OTHER;

	DevMsg(DEVMSG_SHADERAPI, "[DEBUG] ShaderAPIGL vendor: %d\n", m_vendor);

	// Set some of my preferred defaults
	glEnable(GL_DEPTH_TEST);
	GLCheckError("def param GL_DEPTH_TEST");

	glDepthFunc(GL_LEQUAL);
	GLCheckError("def param GL_LEQUAL");

	glFrontFace(GL_CW);
	GLCheckError("def param GL_CW");

	glPixelStorei(GL_PACK_ALIGNMENT,   1);
	GLCheckError("def param GL_PACK_ALIGNMENT");

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLCheckError("def param GL_UNPACK_ALIGNMENT");

	for (int i = 0; i < m_caps.maxRenderTargets; i++)
		m_drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;

	m_currentGLDepth = GL_NONE;

	m_mainThreadId = Threading::GetCurrentThreadID();

	// init worker thread
	glWorker.Init();

	// Init the base shader API
	ShaderAPI_Base::Init(params);

	// all shaders supported, nothing to report

	kvkeybase_t baseMeshBufferParams;
	kvkeybase_t* attr = baseMeshBufferParams.AddKeyBase("attribute", "input_vPos");
	attr->AddValue(0);

	attr = baseMeshBufferParams.AddKeyBase("attribute", "input_texCoord");
	attr->AddValue(1);

	attr = baseMeshBufferParams.AddKeyBase("attribute", "input_color");
	attr->AddValue(3);

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

	for(int i = 0; i < CONSTANT_TYPE_COUNT; i++)
	{
		if(s_uniformFuncs[i] == NULL)
			ASSERTMSG(false, EqString::Format("Uniform function for '%d' is not ok, pls check extensions\n", i).ToCString());
	}
}

void ShaderAPIGL::Shutdown()
{
	glDeleteFramebuffers(1, &m_frameBuffer);
	m_frameBuffer = 0;

	// shutdown worker and detach context
	glWorker.Shutdown();
	ShaderAPI_Base::Shutdown();
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
	int i;

	for (i = 0; i < m_caps.maxTextureUnits; i++)
	{
		CGLTexture* pSelectedTexture = (CGLTexture*)m_pSelectedTextures[i];
		ITexture* pCurrentTexture = m_pCurrentTextures[i];
		GLTextureRef_t& currentGLTexture = m_currentGLTextures[i];
		
		if(pSelectedTexture != m_pCurrentTextures[i])
		{
			// Set the active texture unit and bind the selected texture to target
			glActiveTexture(GL_TEXTURE0 + i);

			if (pSelectedTexture == NULL)
			{
				//if(pCurrentTexture != NULL)
				glBindTexture(glTexTargetType[currentGLTexture.type], 0);
			}
			else
			{
				currentGLTexture = pSelectedTexture->GetCurrentTexture();
				glBindTexture(glTexTargetType[currentGLTexture.type], currentGLTexture.glTexID);
			}

			m_pCurrentTextures[i] = pSelectedTexture;
			
		}
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
	
		if (pSelectedState == NULL)
		{
			if (m_bCurrentBlendEnable)
			{
				glDisable(GL_BLEND);
 				m_bCurrentBlendEnable = false;
			}
		}
		else
		{
			BlendStateParam_t& state = pSelectedState->m_params;
		
			if (state.blendEnable)
			{
				if (!m_bCurrentBlendEnable)
				{
					glEnable(GL_BLEND);
					m_bCurrentBlendEnable = true;
				}

				if (state.srcFactor != m_nCurrentSrcFactor || state.dstFactor != m_nCurrentDstFactor)
				{
					glBlendFunc(blendingConsts[m_nCurrentSrcFactor = state.srcFactor],
								blendingConsts[m_nCurrentDstFactor = state.dstFactor]);
				}

				if (state.blendFunc != m_nCurrentBlendFunc)
					glBlendEquation(blendingModes[m_nCurrentBlendFunc = state.blendFunc]);
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
	CGLDepthStencilState* pSelectedState = (CGLDepthStencilState*)m_pSelectedDepthState;

	if (pSelectedState != m_pCurrentDepthState)
	{
		if (pSelectedState == NULL)
		{
			if (!m_bCurrentDepthTestEnable)
			{
				glEnable(GL_DEPTH_TEST);
				m_bCurrentDepthTestEnable = true;
			}

			if (!m_bCurrentDepthWriteEnable)
			{
				glDepthMask(GL_TRUE);
				m_bCurrentDepthWriteEnable = true;
			}

			if (m_nCurrentDepthFunc != COMP_LEQUAL)
				glDepthFunc(depthConst[m_nCurrentDepthFunc = COMP_LEQUAL]);
		}
		else
		{
			DepthStencilStateParams_t& state = pSelectedState->m_params;
		
			if (state.depthTest)
			{
				if (!m_bCurrentDepthTestEnable)
				{
					glEnable(GL_DEPTH_TEST);
					m_bCurrentDepthTestEnable = true;
				}

				if (state.depthWrite != m_bCurrentDepthWriteEnable)
				{
					glDepthMask((state.depthWrite)? GL_TRUE : GL_FALSE);
					m_bCurrentDepthWriteEnable = state.depthWrite;
				}

				if (state.depthFunc != m_nCurrentDepthFunc)
					glDepthFunc(depthConst[m_nCurrentDepthFunc = state.depthFunc]);
			}
			else
			{
				if (m_bCurrentDepthTestEnable)
				{
					glDisable(GL_DEPTH_TEST);
					m_bCurrentDepthTestEnable = false;
				}
			}

			#pragma todo(GL: stencil tests)
		}

		m_pCurrentDepthState = pSelectedState;
	}
}

void ShaderAPIGL::ApplyRasterizerState()
{
	CGLRasterizerState* pSelectedState = (CGLRasterizerState*)m_pSelectedRasterizerState;

	if (m_pCurrentRasterizerState != pSelectedState)
	{
		if (pSelectedState == NULL)
		{
			if (CULL_BACK != m_nCurrentCullMode)
			{
				if (m_nCurrentCullMode == CULL_NONE)
					glEnable(GL_CULL_FACE);

				glCullFace(cullConst[m_nCurrentCullMode = CULL_BACK]);
			}

#ifndef USE_GLES2
			if (FILL_SOLID != m_nCurrentFillMode)
				glPolygonMode(GL_FRONT_AND_BACK, fillConst[m_nCurrentFillMode = FILL_SOLID]);

			if (false != m_bCurrentMultiSampleEnable)
			{
				glDisable(GL_MULTISAMPLE);
				m_bCurrentMultiSampleEnable = false;
			}
#endif // USE_GLES2
			if (false != m_bCurrentScissorEnable)
			{
				glDisable(GL_SCISSOR_TEST);
				m_bCurrentScissorEnable = false;
			}

			if(m_fCurrentDepthBias != 0.0f || m_fCurrentSlopeDepthBias != 0.0f)
			{
				glDisable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(0.0f, 0.0f);

				m_fCurrentDepthBias = 0.0f;
				m_fCurrentSlopeDepthBias = 0.0f;
			}			
		}
		else
		{
			RasterizerStateParams_t& state = pSelectedState->m_params;
		
			if (state.cullMode != m_nCurrentCullMode)
			{
				if (state.cullMode > CULL_NONE)
				{
					if (m_nCurrentCullMode == CULL_NONE)
						glEnable(GL_CULL_FACE);

					glCullFace(cullConst[state.cullMode]);
				}
				else
					glDisable(GL_CULL_FACE);

				m_nCurrentCullMode = state.cullMode;
			}

#ifndef USE_GLES2
			if (state.fillMode != m_nCurrentFillMode)
				glPolygonMode(GL_FRONT_AND_BACK, fillConst[m_nCurrentFillMode = state.fillMode]);

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

			if (state.useDepthBias != false)
			{
				if(m_fCurrentDepthBias != state.depthBias || m_fCurrentSlopeDepthBias != state.slopeDepthBias)
				{
					const float POLYGON_OFFSET_SCALE = 320.0f;	// FIXME: is a depth buffer bit depth?

					glEnable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset((m_fCurrentDepthBias = state.depthBias) * POLYGON_OFFSET_SCALE, (m_fCurrentSlopeDepthBias = state.slopeDepthBias) * POLYGON_OFFSET_SCALE);
				}
			}
			else
			{
				if(m_fCurrentDepthBias != 0.0f || m_fCurrentSlopeDepthBias != 0.0f)
				{
					glDisable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset(0.0f, 0.0f);

					m_fCurrentDepthBias = 0.0f;
					m_fCurrentSlopeDepthBias = 0.0f;
				}
			}
		}
	}

	m_pCurrentRasterizerState = pSelectedState;
}

void ShaderAPIGL::ApplyShaderProgram()
{
	CGLShaderProgram* selectedShader = (CGLShaderProgram*)m_pSelectedShader;

	if (selectedShader != m_pCurrentShader)
	{
		GLhandleARB program = selectedShader ? selectedShader->m_program : 0;

		glUseProgram(program);
		m_pCurrentShader = selectedShader;
	}
}

void ShaderAPIGL::ApplyConstants()
{
	CGLShaderProgram* currentShader = (CGLShaderProgram*)m_pCurrentShader;

	if (!currentShader)
		return;

	for (int i = 0; i < currentShader->m_numConstants; i++)
	{
		GLShaderConstant_t& uni = currentShader->m_constants[i];

		if (!uni.dirty)
			continue;

		uni.dirty = false;

		if (uni.type >= CONSTANT_MATRIX2x2)
			((UNIFORM_MAT_FUNC) s_uniformFuncs[uni.type])(uni.index, uni.nElements, GL_TRUE, (float *) uni.data);
		else
			((UNIFORM_FUNC) s_uniformFuncs[uni.type])(uni.index, uni.nElements, (float *) uni.data);
	}
}


void ShaderAPIGL::Clear(bool bClearColor,
						bool bClearDepth,
						bool bClearStencil,
						const ColorRGBA &fillColor,
						float fDepth,
						int nStencil)
{
	GLbitfield clearBits = 0;

	if (bClearColor)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		GLCheckError("clr mask");

		glClearColor(fillColor.x, fillColor.y, fillColor.z, fillColor.w);
		GLCheckError("clr color");
	}

	if (bClearDepth)
	{
		clearBits |= GL_DEPTH_BUFFER_BIT;

		glDepthMask(GL_TRUE);
		GLCheckError("clr depth msk");

#ifndef USE_GLES2
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
	}
}
//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

// Device vendor and version
const char* ShaderAPIGL::GetDeviceNameString() const
{
	return (const char*)glGetString(GL_VENDOR);
}

// Renderer string (ex: OpenGL, D3D9)
const char* ShaderAPIGL::GetRendererName() const
{
#ifdef USE_GLES2
	return "OpenGLES";
#else
	return "OpenGL";
#endif // USE_GLES2
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
		return NULL;

	CGLOcclusionQuery* occQuery = new CGLOcclusionQuery();

	m_Mutex.Lock();
	m_OcclusionQueryList.append( occQuery );
	m_Mutex.Unlock();

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

// Unload the texture and free the memory
void ShaderAPIGL::FreeTexture(ITexture* pTexture)
{
	CGLTexture* pTex = (CGLTexture*)pTexture;

	if(pTex == NULL)
		return;

	bool deleted = false;
	{
		CScopedMutex scoped(m_Mutex);

		if (pTex->Ref_Count() == 0)
			MsgWarning("texture %s refcount==0\n", pTexture->GetName());

		//ASSERT(pTex->numReferences > 0);

		pTex->Ref_Drop();

		if (pTex->Ref_Count() <= 0)
			deleted = m_TextureList.remove(pTexture);
	}

	if(deleted)
	{
		glWorker.Execute(__FUNCTION__, [pTex]() {
			delete pTex;

			return 0;
		});
	}
}

// It will add new rendertarget
ITexture* ShaderAPIGL::CreateRenderTarget(	int width, int height,
											ETextureFormat nRTFormat,
											ER_TextureFilterMode textureFilterType,
											ER_TextureAddressMode textureAddress,
											ER_CompareFunc comparison,
											int nFlags)
{
	return CreateNamedRenderTarget(EqString::Format("_sapi_rt_%d", m_TextureList.numElem()).ToCString(), width, height, nRTFormat, textureFilterType,textureAddress,comparison,nFlags);
}

// It will add new rendertarget
ITexture* ShaderAPIGL::CreateNamedRenderTarget(	const char* pszName,
												int width, int height,
												ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType,
												ER_TextureAddressMode textureAddress,
												ER_CompareFunc comparison,
												int nFlags)
{
	CGLTexture *pTexture = new CGLTexture;

	pTexture->SetDimensions(width,height);
	pTexture->SetFormat(nRTFormat);
	pTexture->SetFlags(nFlags | TEXFLAG_RENDERTARGET);
	pTexture->SetName(pszName);

	pTexture->glTarget = (nFlags & TEXFLAG_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);

	pTexture->SetSamplerState(texSamplerParams);

	m_Mutex.Lock();

	if (nFlags & TEXFLAG_RENDERDEPTH)
	{
		glGenTextures(1, &pTexture->glDepthID);
		GLCheckError("gen depth tex");

		glBindTexture(GL_TEXTURE_2D, pTexture->glDepthID);
		SetupGLSamplerState(GL_TEXTURE_2D, texSamplerParams);
	}

	// create texture
	GLTextureRef_t texture;

	if (nFlags & TEXFLAG_CUBEMAP)
		texture.type = GLTEX_TYPE_CUBETEXTURE;
	else
		texture.type = GLTEX_TYPE_TEXTURE;

	if(nRTFormat != FORMAT_NONE)
	{
		glGenTextures(1, &texture.glTexID);
		GLCheckError("gen tex");

		glBindTexture(pTexture->glTarget, texture.glTexID);
		SetupGLSamplerState(pTexture->glTarget, texSamplerParams);

		pTexture->textures.append(texture);

		// this generates the render target
		ResizeRenderTarget(pTexture, width, height);
	}
	else
	{
		texture.glTexID = 0;
		pTexture->textures.append(texture);
	}

	
	m_TextureList.append(pTexture);
	m_Mutex.Unlock();

	return pTexture;
}

void ShaderAPIGL::ResizeRenderTarget(ITexture* pRT, int newWide, int newTall)
{
	CGLTexture* pTex = (CGLTexture*)pRT;
	ETextureFormat format = pTex->GetFormat();

	pTex->SetDimensions(newWide,newTall);

	if (format == FORMAT_NONE)
		return;

	if (pTex->glTarget == GL_RENDERBUFFER)
	{
		// Bind render buffer
		glBindRenderbuffer(GL_RENDERBUFFER, pTex->glDepthID);
		glRenderbufferStorage(GL_RENDERBUFFER, internalFormats[format], newWide, newTall);
		GLCheckError("gen tex renderbuffer storage");

		// Restore renderbuffer
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	else
	{
		if (pTex->glDepthID != GL_NONE)
		{
			// make a depth texture first
			// use glDepthID
			ETextureFormat depthFmt = FORMAT_D16;
			GLint depthInternalFormat = internalFormats[depthFmt];
			GLenum depthSrcFormat = IsStencilFormat(depthFmt) ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT;
			GLenum depthSrcType = chanTypePerFormat[depthFmt];

			glBindTexture(GL_TEXTURE_2D, pTex->glDepthID);
			glTexImage2D(GL_TEXTURE_2D, 0, depthInternalFormat, newWide, newTall, 0, depthSrcFormat, depthSrcType, NULL);
			GLCheckError("gen tex image depth");
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		GLint internalFormat = internalFormats[format];
		GLenum srcFormat = chanCountTypes[GetChannelCount(format)];
		GLenum srcType = chanTypePerFormat[format];

		if (IsDepthFormat(format))
		{
			if (IsStencilFormat(format))
				srcFormat = GL_DEPTH_STENCIL;
			else
				srcFormat = GL_DEPTH_COMPONENT;
		}

		// Allocate all required surfaces.
		glBindTexture(pTex->glTarget, pTex->textures[0].glTexID);

		if (pTex->GetFlags() & TEXFLAG_CUBEMAP)
		{
			for (int i = 0; i < 6; i++)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, newWide, newTall, 0, srcFormat, srcType, NULL);
			GLCheckError("gen tex image cube");
		}
		else
		{
			glTexImage2D(pTex->glTarget, 0, internalFormat, newWide, newTall, 0, srcFormat, srcType, NULL);
			GLCheckError("gen tex image");
		}

		glBindTexture(pTex->glTarget, 0);
	}
}

static ConVar gl_skipTextures("gl_skipTextures", "0", nullptr, CV_CHEAT);

GLTextureRef_t ShaderAPIGL::CreateGLTextureFromImage(CImage* pSrc, const SamplerStateParam_t& sampler, int& wide, int& tall, int nFlags)
{
	GLTextureRef_t noTexture = { 0, GLTEX_TYPE_ERROR };

	if(!pSrc)
		return noTexture;

	GLint internalFormat = PickGLInternalFormat(pSrc->GetFormat());

	if (internalFormat == 0)
	{
		MsgError("'%s' has unsupported image format (%d)\n", pSrc->GetName(), pSrc->GetFormat());
		return noTexture;
	}

	HOOK_TO_CVAR(r_loadmiplevel);
	int nQuality = r_loadmiplevel->GetInt();

	// force quality to best
	if(nFlags & TEXFLAG_NOQUALITYLOD)
		nQuality = 0;

	bool bMipMaps = (pSrc->GetMipMapCount() > 1);

	if(!bMipMaps)
		nQuality = 0;

	int numMipmaps = (pSrc->GetMipMapCount() - nQuality);
	numMipmaps = max(0, numMipmaps);

	// create texture
	GLTextureRef_t texture;
	if (pSrc->Is3D())
		texture.type = GLTEX_TYPE_VOLUMETEXTURE;
	else if (pSrc->IsCube())
		texture.type = GLTEX_TYPE_CUBETEXTURE;
	else
		texture.type = GLTEX_TYPE_TEXTURE;


	// set our referenced params
	wide = pSrc->GetWidth();
	tall = pSrc->GetHeight();

	GLTextureRef_t* texturePtr = &texture;
	
	if(gl_skipTextures.GetBool())
		return noTexture;

	int result = glWorker.WaitForExecute(__FUNCTION__,[=]() {
		// Generate a texture
		glGenTextures(1, &texturePtr->glTexID);

		if (!GLCheckError("gen tex"))
			return -1;

		const GLenum glTarget = glTexTargetType[texturePtr->type];

		glBindTexture(glTarget, texturePtr->glTexID);
		GLCheckError("bind tex");

		// Setup the sampler state
		SetupGLSamplerState(glTarget, sampler, numMipmaps);

		if (!UpdateGLTextureFromImage(*texturePtr, pSrc, nQuality))
		{
			glBindTexture(glTarget, 0);
			GLCheckError("tex unbind");

			glDeleteTextures(1, &texturePtr->glTexID);
			GLCheckError("del tex");

			return -1;
		}

		return 0;
	});

	if (result == -1)
		return noTexture;

	return texture;
}

void ShaderAPIGL::CreateTextureInternal(ITexture** pTex, const DkList<CImage*>& pImages, const SamplerStateParam_t& sampler,int nFlags)
{
	if(!pImages.numElem())
		return;

	CGLTexture* pTexture = NULL;

	// get or create
	if(*pTex)
		pTexture = (CGLTexture*)*pTex;
	else
		pTexture = new CGLTexture();

	int wide = 0, tall = 0;
	int mipCount = 0;

	HOOK_TO_CVAR(r_loadmiplevel);

	for(int i = 0; i < pImages.numElem(); i++)
	{
		SamplerStateParam_t ss = sampler;

		// setup sampler correctly
		if(pImages[i]->GetMipMapCount() == 1)
		{
			if(ss.minFilter > TEXFILTER_NEAREST)
				ss.minFilter = TEXFILTER_LINEAR;

			if(ss.magFilter > TEXFILTER_NEAREST)
				ss.magFilter = TEXFILTER_LINEAR;
		}

		// TODO: Async loading
		GLTextureRef_t textureRef = CreateGLTextureFromImage(pImages[i], ss, wide, tall, nFlags);

		if(textureRef.type > GLTEX_TYPE_ERROR)
		{
			if(pTexture->glTarget == 0)
				pTexture->glTarget = glTexTargetType[textureRef.type];

			int nQuality = r_loadmiplevel->GetInt();

			// force quality to best
			if((nFlags & TEXFLAG_NOQUALITYLOD) || pImages[i]->GetMipMapCount() == 1)
				nQuality = 0;

			mipCount += pImages[i]->GetMipMapCount()-nQuality;

			pTexture->m_texSize += pImages[i]->GetMipMappedSize(nQuality);
			pTexture->textures.append(textureRef);
		}
	}

	if(!pTexture->textures.numElem())
	{
		if(!(*pTex))
			delete pTexture;
		else
			FreeTexture(pTexture);

		return;
	}

	pTexture->m_numAnimatedTextureFrames = pTexture->textures.numElem();

	// Bind this sampler state to texture
	pTexture->SetSamplerState(sampler);
	pTexture->SetDimensions(wide, tall);
	pTexture->SetMipCount(mipCount);
	pTexture->SetFormat(pImages[0]->GetFormat());
	pTexture->SetFlags(nFlags | TEXFLAG_MANAGED);
	pTexture->SetName( pImages[0]->GetName() );

	pTexture->m_flLod = sampler.lod;

	// if this is a new texture, add
	if(!(*pTex))
	{
		m_Mutex.Lock();
		m_TextureList.append(pTexture);
		m_Mutex.Unlock();
	}

	// set for output
	*pTex = pTexture;
}

void ShaderAPIGL::SetupGLSamplerState(uint texTarget, const SamplerStateParam_t& sampler, int mipMapCount)
{
	// Set requested wrapping modes
	glTexParameteri(texTarget, GL_TEXTURE_WRAP_S, addressModes[sampler.wrapS]);
	GLCheckError("smp w s");

#ifndef USE_GLES2
	if (texTarget != GL_TEXTURE_1D)
#endif // USE_GLES2
		glTexParameteri(texTarget, GL_TEXTURE_WRAP_T, addressModes[sampler.wrapT]);
		GLCheckError("smp w t");

	if (texTarget == GL_TEXTURE_3D)
	{
		glTexParameteri(texTarget, GL_TEXTURE_WRAP_R, addressModes[sampler.wrapR]);
		GLCheckError("smp w r");
	}

	// Set requested filter modes
	glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, minFilters[sampler.magFilter]);
	GLCheckError("smp mag");

	glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, minFilters[sampler.minFilter]);
	GLCheckError("smp min");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1);
	GLCheckError("smp mip");

	glTexParameteri(texTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	GLCheckError("smp cmpmode");

	glTexParameteri(texTarget, GL_TEXTURE_COMPARE_FUNC, depthConst[sampler.compareFunc]);
	GLCheckError("smp cmpfunc");

#ifndef USE_GLES2
	// Setup anisotropic filtering
	if (sampler.aniso > 1 && GLAD_GL_ARB_texture_filter_anisotropic)
	{
		glTexParameteri(texTarget, GL_TEXTURE_MAX_ANISOTROPY, sampler.aniso);
		GLCheckError("smp aniso");
	}
#endif // USE_GLES2
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

// Copy render target to texture
void ShaderAPIGL::CopyFramebufferToTexture(ITexture* pTargetTexture)
{
	// store the current rendertarget states
	ITexture* currentRenderTarget[MAX_MRTS];
	int	currentCRTSlice[MAX_MRTS];
	ITexture* currentDepthTarget = m_pCurrentDepthRenderTarget;
	m_pCurrentDepthRenderTarget = NULL;

	int currentNumRTs = 0;
	for(; currentNumRTs < MAX_MRTS;)
	{
		ITexture* rt = m_pCurrentColorRenderTargets[currentNumRTs];

		if(!rt)
			break;

		currentRenderTarget[currentNumRTs] = rt;
		currentCRTSlice[currentNumRTs] = m_nCurrentCRTSlice[currentNumRTs];

		m_pCurrentColorRenderTargets[currentNumRTs] = NULL;
		m_nCurrentCRTSlice[currentNumRTs] = 0;

		currentNumRTs++;
	}

	ChangeRenderTarget(pTargetTexture);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_frameBuffer);

	// FIXME: this could lead to regression problem, so states must be revisited
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);

	glBlitFramebuffer(0, 0, m_nViewportWidth, m_nViewportHeight, 0, 0, pTargetTexture->GetWidth(), pTargetTexture->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// restore render targets back
	// or to backbuffer if no RTs were set
	if(currentNumRTs)
		ChangeRenderTargets(currentRenderTarget, currentNumRTs, currentCRTSlice, currentDepthTarget);
	else
		ChangeRenderTargetToBackBuffer();
}

// Copy render target to texture
void ShaderAPIGL::CopyRendertargetToTexture(ITexture* srcTarget, ITexture* destTex, IRectangle* srcRect, IRectangle* destRect)
{
	// BUG BUG:
	// this process is very bugged as fuck
	// TODO: double-check main framebuffer attachments from de-attaching

	// store the current rendertarget states
	ITexture* currentRenderTarget[MAX_MRTS];
	int	currentCRTSlice[MAX_MRTS];
	ITexture* currentDepthTarget = m_pCurrentDepthRenderTarget;
	m_pCurrentDepthRenderTarget = NULL;

	int currentNumRTs = 0;
	for(; currentNumRTs < MAX_MRTS;)
	{
		ITexture* rt = m_pCurrentColorRenderTargets[currentNumRTs];

		if (!rt)
			break;

		currentRenderTarget[currentNumRTs] = rt;
		currentCRTSlice[currentNumRTs] = m_nCurrentCRTSlice[currentNumRTs];

		m_pCurrentColorRenderTargets[currentNumRTs] = NULL;
		m_nCurrentCRTSlice[currentNumRTs] = 0;

		currentNumRTs++;
	}

	// 1. preserve old render targets
	// 2. set shader
	// 3. render to texture
	CGLTexture* srcTexture = (CGLTexture*)srcTarget;
	CGLTexture* destTexture = (CGLTexture*)destTex;

	IRectangle _srcRect(0,0,srcTexture->GetWidth(), srcTexture->GetHeight());
	IRectangle _destRect(0,0,destTexture->GetWidth(), destTexture->GetHeight());

	if(srcRect)
		_srcRect = *srcRect;

	if(destRect)
		_destRect = *destRect;

	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

	// FIXME: this could lead to regression problem, so states must be revisited
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GL_NONE);

	// setup read from texture
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srcTexture->textures[0].glTexID, 0);

	// setup write to texture
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, destTexture->textures[0].glTexID, 0);

	// setup GL_COLOR_ATTACHMENT1 as destination
	GLenum drawBufferSetting[2] = {GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBufferSetting);
	GLCheckError("glDrawBuffers att1");

	// copy
	glBlitFramebuffer(	_srcRect.vleftTop.x, _srcRect.vleftTop.y, _srcRect.vrightBottom.x, _srcRect.vrightBottom.y,
						_destRect.vleftTop.x, _destRect.vleftTop.y, _destRect.vrightBottom.x, _destRect.vrightBottom.y, 
						GL_COLOR_BUFFER_BIT, GL_NEAREST);
	GLCheckError("blit");

	// reset
	glDrawBuffers(1, drawBufferSetting+1);
	GLCheckError("glDrawBuffer rst");

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// change render targets back
	if(currentNumRTs)
		ChangeRenderTargets(currentRenderTarget, currentNumRTs, currentCRTSlice, currentDepthTarget);
	else
		ChangeRenderTargetToBackBuffer();
}

// Changes render target (MRT)
void ShaderAPIGL::ChangeRenderTargets(ITexture** pRenderTargets, int nNumRTs, int* nCubemapFaces, ITexture* pDepthTarget, int nDepthSlice)
{
	uint glFrameBuffer = m_frameBuffer;

	if (glFrameBuffer == 0)
	{
		glGenFramebuffers(1, &glFrameBuffer);
		m_frameBuffer = glFrameBuffer;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, glFrameBuffer);

	for (int i = 0; i < nNumRTs; i++)
	{
		CGLTexture* colorRT = (CGLTexture*)pRenderTargets[i];

		const int nCubeFace = nCubemapFaces ? nCubemapFaces[i] : 0;

		if (colorRT->GetFlags() & TEXFLAG_CUBEMAP)
		{
			if (colorRT != m_pCurrentColorRenderTargets[i] || m_nCurrentCRTSlice[i] != nCubeFace)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER,
						GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + nCubeFace, colorRT->textures[0].glTexID, 0);

				m_nCurrentCRTSlice[i] = nCubeFace;
			}
		}
		else
		{
			if (colorRT != m_pCurrentColorRenderTargets[i])
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorRT->textures[0].glTexID, 0);
			}
		}

		m_pCurrentColorRenderTargets[i] = colorRT;
	}

	if (nNumRTs != m_nCurrentRenderTargets)
	{
		for (int i = nNumRTs; i < m_nCurrentRenderTargets; i++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
			m_pCurrentColorRenderTargets[i] = NULL;
			m_nCurrentCRTSlice[i] = -1;
		}

		if (nNumRTs == 0)
		{
			glDrawBuffers(0, NULL);
			glReadBuffer(GL_NONE);
		}
		else
		{
			glDrawBuffers(nNumRTs, m_drawBuffers);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
		}

		m_nCurrentRenderTargets = nNumRTs;
	}

	GLuint bestDepth = nNumRTs ? ((CGLTexture*)pRenderTargets[0])->glDepthID : GL_NONE;
	GLuint bestTarget = GL_TEXTURE_2D;
	ETextureFormat bestFormat = nNumRTs ? FORMAT_D16 : FORMAT_NONE;
	
	CGLTexture* pDepth = (CGLTexture*)pDepthTarget;

	if (pDepth != m_pCurrentDepthRenderTarget)
	{
		if(pDepth)
		{
			bestDepth = (pDepth->glTarget == GL_RENDERBUFFER) ? pDepth->glDepthID : pDepth->textures[0].glTexID;
			bestTarget = pDepth->glTarget == GL_RENDERBUFFER ? GL_RENDERBUFFER : GL_TEXTURE_2D;
			bestFormat = pDepth->GetFormat();
		}
		
		m_pCurrentDepthRenderTarget = pDepth;
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
	if (m_nCurrentRenderTargets > 0 && m_pCurrentColorRenderTargets[0] != NULL)
	{
		SetViewport(0, 0, m_pCurrentColorRenderTargets[0]->GetWidth(), m_pCurrentColorRenderTargets[0]->GetHeight());
	}
	else if(m_pCurrentDepthRenderTarget != NULL)
	{
		SetViewport(0, 0, m_pCurrentDepthRenderTarget->GetWidth(), m_pCurrentDepthRenderTarget->GetHeight());
	}
}

// fills the current rendertarget buffers
void ShaderAPIGL::GetCurrentRenderTargets(ITexture* pRenderTargets[MAX_MRTS], int *numRTs, ITexture** pDepthTarget, int cubeNumbers[MAX_MRTS])
{
	int nRts = 0;

	if(pRenderTargets)
	{
		for (int i = 0; i < m_caps.maxRenderTargets; i++)
		{
			nRts++;

			pRenderTargets[i] = m_pCurrentColorRenderTargets[i];

			if(cubeNumbers)
				cubeNumbers[i] = m_nCurrentCRTSlice[i];

			if(!pRenderTargets[i])
				break;
		}
	}

	if(pDepthTarget)
		*pDepthTarget = m_pCurrentDepthRenderTarget;

	*numRTs = nRts;
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
	SetViewport(0, 0, m_nViewportWidth, m_nViewportHeight);

	int numRTs = m_nCurrentRenderTargets;

	for (int i = 0; i < numRTs; i++)
		m_pCurrentColorRenderTargets[i] = NULL;

	m_pCurrentDepthRenderTarget = NULL;
	m_currentGLDepth = GL_NONE;
}

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

// Matrix mode
void ShaderAPIGL::SetMatrixMode(ER_MatrixMode nMatrixMode)
{
#if 0
	glMatrixMode( matrixModeConst[nMatrixMode] );
#endif // OpenGL 2.1

	m_nCurrentMatrixMode = nMatrixMode;
}

// Will save matrix
void ShaderAPIGL::PushMatrix()
{
	//ThreadMakeCurrent();
	//glPushMatrix();
}

// Will reset matrix
void ShaderAPIGL::PopMatrix()
{
	//ThreadMakeCurrent();
	//glPopMatrix();
}

// Load identity matrix
void ShaderAPIGL::LoadIdentityMatrix()
{
#if 0
	glLoadIdentity();
#endif // OpenGL 2.1

	m_matrices[m_nCurrentMatrixMode] = identity4();
}

// Load custom matrix
void ShaderAPIGL::LoadMatrix(const Matrix4x4 &matrix)
{
#if 0
	if(m_nCurrentMatrixMode == MATRIXMODE_WORLD)
	{
		glMatrixMode( GL_MODELVIEW );
		glLoadMatrixf( transpose(m_matrices[MATRIXMODE_VIEW] * matrix) );
	}
	else
		glLoadMatrixf( transpose(matrix) );
#endif // OpenGL 2.1

	m_matrices[m_nCurrentMatrixMode] = matrix;
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

// Changes the vertex format
void ShaderAPIGL::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	static CVertexFormatGL zero("", nullptr, 0);

	CVertexFormatGL* pSelectedFormat = pVertexFormat ? (CVertexFormatGL*)pVertexFormat : &zero;
	CVertexFormatGL* pCurrentFormat = m_pCurrentVertexFormat ? (CVertexFormatGL*)m_pCurrentVertexFormat : &zero;

	if( pVertexFormat != pCurrentFormat)
	{
		for (int i = 0; i < m_caps.maxVertexGenericAttributes; i++)
		{
			eqGLVertAttrDesc_t& selDesc = pSelectedFormat->m_genericAttribs[i];
			eqGLVertAttrDesc_t& curDesc = pCurrentFormat->m_genericAttribs[i];

			bool shouldDisable = !selDesc.sizeInBytes && curDesc.sizeInBytes;
			bool shouldEnable = selDesc.sizeInBytes && !curDesc.sizeInBytes;

			if (shouldDisable)
			{
				glDisableVertexAttribArray(i);
				GLCheckError("disable vtx attrib");

				glVertexAttribDivisor(i, 0);
				GLCheckError("divisor");
			}
			else if (shouldEnable)
			{
				glEnableVertexAttribArray(i);
				GLCheckError("enable vtx attrib");
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

	const GLsizei glTypes[] = {
		GL_FLOAT,
		GL_HALF_FLOAT,
		GL_UNSIGNED_BYTE,
	};

	bool instanceBuffer = (nStream > 0) && pVB != nullptr && (pVB->GetFlags() & VERTBUFFER_FLAG_INSTANCEDATA);
	bool vboChanged = m_currentGLVB[nStream] != vbo;

	bool instancingChanged = !instanceBuffer && m_boundInstanceStream == nStream ||
							  instanceBuffer && m_boundInstanceStream != nStream;

	if (vboChanged || offset != m_nCurrentOffsets[nStream] || m_pCurrentVertexFormat != m_pActiveVertexFormat[nStream] ||
		instanceBuffer && m_boundInstanceStream != nStream)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_currentGLVB[nStream] = vbo);
		GLCheckError("bind array");

		if (currentFormat)
		{
			const int vertexSize = currentFormat->m_streamStride[nStream];

			const char* base = (char*)(offset * vertexSize);

			for (int i = 0; i < m_caps.maxVertexGenericAttributes; i++)
			{
				const eqGLVertAttrDesc_t& attrib = currentFormat->m_genericAttribs[i];

				if (attrib.streamId == nStream && attrib.sizeInBytes)
				{
					glVertexAttribPointer(i, attrib.sizeInBytes, glTypes[attrib.attribFormat], GL_FALSE, vertexSize, base + attrib.offsetInBytes);
					GLCheckError("attribpointer");

					glVertexAttribDivisor(i, instanceBuffer ? 1 : 0);
					GLCheckError("divisor");
				}
			}
		}

		m_pCurrentVertexBuffers[nStream] = pVertexBuffer;
		m_nCurrentOffsets[nStream] = offset;
		m_pActiveVertexFormat[nStream] = m_pCurrentVertexFormat;
	}

	if(pVertexBuffer)
	{
		if (!instanceBuffer && m_boundInstanceStream != -1)
			m_boundInstanceStream = -1;
		else if (instanceBuffer && m_boundInstanceStream == -1)
			m_boundInstanceStream = nStream;
		else if (instanceBuffer && m_boundInstanceStream != -1)
		{
			ASSERTMSG(false, EqString::Format("Already bound instancing stream at %d!!!", m_boundInstanceStream));
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
		if (pIndexBuffer == NULL)
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
IShaderProgram* ShaderAPIGL::CreateNewShaderProgram(const char* pszName, const char* query)
{
	CGLShaderProgram* pNewProgram = new CGLShaderProgram();
	pNewProgram->SetName((_Es(pszName)+query).GetData());

	CScopedMutex scoped(m_Mutex);

	m_ShaderList.append(pNewProgram);

	return pNewProgram;
}

// Destroy all shader
void ShaderAPIGL::DestroyShaderProgram(IShaderProgram* pShaderProgram)
{
	CGLShaderProgram* pShader = (CGLShaderProgram*)(pShaderProgram);

	if(!pShader)
		return;

	bool deleted = false;
	{
		CScopedMutex m(m_Mutex);
		pShader->Ref_Drop(); // decrease references to this shader

		// remove it if reference is zero
		if (pShader->Ref_Count() <= 0)
			deleted = m_ShaderList.remove(pShader);
	}

	// remove it if reference is zero
	if(deleted)
	{
		glWorker.Execute(__FUNCTION__, [pShader]() {
			delete pShader;

			return 0;
		});
	}
}

#define SHADER_HELPERS_STRING \
	"#define saturate(x) clamp(x,0.0,1.0)\r\n"	\
	"#define lerp mix\r\n"						\
	"#define mul(a,b) ((a)*(b))\r\n"			\
	"#define frac fract\r\n"					\
	"#define float2 vec2\r\n"					\
	"#define float3 vec3\r\n"					\
	"#define float4 vec4\r\n"					\
	"#define float2x2 mat2\r\n"					\
	"#define float3x3 mat3\r\n"					\
	"#define float4x4 mat4\r\n"					\
	"#define tex2D texture\r\n"					\
	"#define tex3D texture\r\n"					\
	"#define texCUBE texture\r\n"				\
	"#define samplerCUBE samplerCube\r\n"

#define SHADER_HELPERS_STRING_VERTEX	\
	"float clip(float x) {return 1.0;}\r\n" \
	"#define ddx(x) (x)\r\n"				\
	"#define ddy(x) (x)\r\n"

#define SHADER_HELPERS_STRING_FRAGMENT	\
	"#define clip(x) if((x) < 0.0) discard\r\n" \
	"#define ddx dFdx\r\n"						\
	"#define ddy dFdy\r\n"

#define GLSL_VERTEX_ATTRIB_START 0	// this is compatibility only

int constantComparator(const void *s0, const void *s1)
{
	return strcmp(((GLShaderConstant_t *) s0)->name, ((GLShaderConstant_t *) s1)->name);
}

int samplerComparator(const void *sampler0, const void *sampler1)
{
	return strcmp(((GLShaderSampler_t *) sampler0)->name, ((GLShaderSampler_t *) sampler1)->name);
}

// Load any shader from stream
bool ShaderAPIGL::CompileShadersFromStream(	IShaderProgram* pShaderOutput,const shaderProgramCompileInfo_t& info, const char* extra)
{
	if(!pShaderOutput)
		return false;

	if (!m_caps.shadersSupportedFlags)
	{
		MsgError("CompileShadersFromStream - shaders unsupported\n");
		return false;
	}

	if (!info.data.text)
		return false;

	CGLShaderProgram* prog = (CGLShaderProgram*)pShaderOutput;
	GLint vsResult, fsResult, linkResult;

	int result;

	// compile vertex
	if(info.data.text)
	{
		GLint* pvsResult = &vsResult;

		EqString shaderString;

#if !defined(USE_GLES2)
		shaderString.Append("#version 330\r\n");
		shaderString.Append("#define VERTEX\r\n");
		// append useful HLSL replacements
		shaderString.Append(SHADER_HELPERS_STRING);
		shaderString.Append(SHADER_HELPERS_STRING_VERTEX);
#endif // USE_GLES2

		if (extra != NULL)
			shaderString.Append(extra);

		const GLchar* sStr[] = { 
			shaderString.ToCString(),
			info.data.text
		};

		result = glWorker.WaitForExecute(__FUNCTION__, [prog, pvsResult, sStr]() {

			// create GL program
			prog->m_program = glCreateProgram();

			if (!GLCheckError("create program"))
				return -1;

			prog->m_vertexShader = glCreateShader(GL_VERTEX_SHADER);

			if (!GLCheckError("create vertex shader"))
				return -1;

			glShaderSource(prog->m_vertexShader, sizeof(sStr) / sizeof(sStr[0]), (const GLchar **)sStr, NULL);
			glCompileShader(prog->m_vertexShader);

			GLCheckError("compile vert shader");

			glGetShaderiv(prog->m_vertexShader, GL_COMPILE_STATUS, pvsResult);

			if (*pvsResult)
			{
				glAttachShader(prog->m_program, prog->m_vertexShader);
				GLCheckError("attach vert shader");
			}
			else
			{
				char infoLog[2048];
				GLint len;

				glGetShaderInfoLog(prog->m_vertexShader, sizeof(infoLog), &len, infoLog);
				MsgError("Vertex shader %s error:\n%s\n", prog->GetName(), infoLog);

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

#if !defined(USE_GLES2)
		shaderString.Append("#version 330\r\n");
		shaderString.Append("#define FRAGMENT\r\n");
		// append useful HLSL replacements
		shaderString.Append(SHADER_HELPERS_STRING);
		shaderString.Append(SHADER_HELPERS_STRING_FRAGMENT);
#endif // USE_GLES2

		if (extra != NULL)
			shaderString.Append(extra);

		const GLchar* sStr[] = { 
			shaderString.ToCString(),
			info.data.text
		};

		GLint* pfsResult = &fsResult;

		result = glWorker.WaitForExecute(__FUNCTION__, [prog, pfsResult, sStr]() {
			prog->m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

			if (!GLCheckError("create fragment shader"))
				return -1;

			glShaderSource(prog->m_fragmentShader, sizeof(sStr) / sizeof(sStr[0]), (const GLchar**)sStr, NULL);
			glCompileShader(prog->m_fragmentShader);
			glGetShaderiv(prog->m_fragmentShader, GL_COMPILE_STATUS, pfsResult);

			GLCheckError("compile frag shader");

			if (*pfsResult)
			{
				glAttachShader(prog->m_program, prog->m_fragmentShader);
				GLCheckError("attach frag shader");
			}
			else
			{
				char infoLog[2048];
				GLint len;

				glGetShaderInfoLog(prog->m_fragmentShader, sizeof(infoLog), &len, infoLog);
				MsgError("Pixel shader %s error:\n%s\n", prog->GetName(), infoLog);
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
		fsResult = GL_TRUE;

	if(fsResult && vsResult)
	{
		GLint* plinkResult = &linkResult;
		const shaderProgramCompileInfo_t* pInfo = &info;

		// get current set program
		GLuint currProgram = (m_pCurrentShader == NULL) ? 0 : ((CGLShaderProgram*)m_pCurrentShader)->m_program;

		EGraphicsVendor vendor = m_vendor;

		result = glWorker.WaitForExecute(__FUNCTION__, [prog, pInfo, plinkResult, vendor, currProgram]() {
			// link program and go
			glLinkProgram(prog->m_program);
			glGetProgramiv(prog->m_program, GL_LINK_STATUS, plinkResult);

			GLCheckError("link program");

			if (!(*plinkResult))
			{
				char infoLog[2048];
				GLint len;

				glGetProgramInfoLog(prog->m_program, sizeof(infoLog), &len, infoLog);
				MsgError("Shader '%s' link error: %s\n", prog->GetName(), infoLog);
				return -1;
			}

			// use freshly generated program to retirieve constants (uniforms) and samplers
			glUseProgram(prog->m_program);

			GLCheckError("test use program");

			// intel buggygl fix
			if (vendor == VENDOR_INTEL)
			{
				glUseProgram(0);
				glUseProgram(prog->m_program);
			}

			GLint uniformCount, maxLength;
			glGetProgramiv(prog->m_program, GL_ACTIVE_UNIFORMS, &uniformCount);
			glGetProgramiv(prog->m_program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

			if (maxLength == 0 && (uniformCount > 0 || uniformCount > 256))
			{
				if (vendor == VENDOR_INTEL)
					DevMsg(DEVMSG_SHADERAPI, "Guess who? It's Intel! uniformCount to be zeroed\n");
				else
					DevMsg(DEVMSG_SHADERAPI, "I... didn't... expect... that! uniformCount to be zeroed\n");

				uniformCount = 0;
			}

			GLShaderSampler_t*	samplers = (GLShaderSampler_t  *)malloc(uniformCount * sizeof(GLShaderSampler_t));
			GLShaderConstant_t*	uniforms = (GLShaderConstant_t *)malloc(uniformCount * sizeof(GLShaderConstant_t));

			int nSamplers = 0;
			int nUniforms = 0;

			char* tmpName = new char[maxLength+1];

			for (int i = 0; i < uniformCount; i++)
			{
				GLenum type;
				GLint length, size;

				glGetActiveUniform(prog->m_program, i, maxLength, &length, &size, &type, tmpName);

	#ifdef USE_GLES2
				if (type >= GL_SAMPLER_2D && type <= GL_SAMPLER_CUBE_SHADOW)
	#else
				if (type >= GL_SAMPLER_1D && type <= GL_SAMPLER_2D_RECT_SHADOW)
	#endif // USE_GLES3
				{
					// Assign samplers to image units
					GLint location = glGetUniformLocation(prog->m_program, tmpName);
					glUniform1i(location, nSamplers);

					DevMsg(DEVMSG_SHADERAPI, "[DEBUG] retrieving sampler '%s' at %d (location = %d)\n", tmpName, nSamplers, location);

					GLShaderSampler_t& sp = samplers[nSamplers];
					sp.index = nSamplers++;
					strcpy(sp.name, tmpName);
				}
				else
				{
					// Store all non-gl uniforms
					if (strncmp(tmpName, "gl_", 3) != 0)
					{
						DevMsg(DEVMSG_SHADERAPI, "[DEBUG] retrieving uniform '%s' at %d\n", tmpName, nUniforms);

						// also cut off name at the bracket
						char* bracket = strchr(tmpName, '[');
						if (bracket == NULL || (bracket[1] == '0' && bracket[2] == ']'))
						{
							if (bracket)
								length = (GLint)(bracket - tmpName);

							GLShaderConstant_t& uni = uniforms[nUniforms++];

							EqString uniformName(tmpName, length);

							strcpy(uni.name, uniformName);
							uni.index = glGetUniformLocation(prog->m_program, tmpName);
							uni.type = GetConstantType(type);
							
							int totalElements = 1;

							// detect the array size by getting next valid uniforms
							if (bracket)
							{
								for (int j = i + 1; j < uniformCount; j++)
								{
									GLint tmpLength, tmpSize;
									GLenum tmpType;
									glGetActiveUniform(prog->m_program, j, maxLength, &tmpLength, &tmpSize, &tmpType, tmpName);

									// we only expect arrays
									bracket = strchr(tmpName, '[');
									if(bracket)
									{
										*bracket = '\0';
										if(!uniformName.Compare(tmpName))
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
					} // cmp != "gl_"
				}// Sampler types?
			}

			delete [] tmpName;

			// restore current program we previously stored
			glUseProgram(currProgram);

			GLCheckError("restore use program");

			glDeleteShader(prog->m_fragmentShader);
			glDeleteShader(prog->m_vertexShader);

			GLCheckError("delete shaders");

			prog->m_fragmentShader = 0;
			prog->m_vertexShader = 0;

			// Shorten arrays to actual count
			samplers = (GLShaderSampler_t  *) realloc(samplers, nSamplers * sizeof(GLShaderSampler_t));
			uniforms = (GLShaderConstant_t *) realloc(uniforms, nUniforms * sizeof(GLShaderConstant_t));
			qsort(samplers, nSamplers, sizeof(GLShaderSampler_t),  samplerComparator);
			qsort(uniforms, nUniforms, sizeof(GLShaderConstant_t), constantComparator);

			for (int i = 0; i < nUniforms; i++)
			{
				GLShaderConstant_t& constant = uniforms[i];
				constant.size = s_constantTypeSizes[constant.type] * constant.nElements;
				constant.data = new ubyte[constant.size];
				memset(constant.data, 0, constant.size);
				constant.dirty = false;
			}

			// finally assign
			prog->m_samplers = samplers;
			prog->m_constants = uniforms;

			prog->m_numSamplers = nSamplers;
			prog->m_numConstants = nUniforms;

			return 0;
		});

		if (result == -1)
			return false;
	}
	else
		return false;

	return true;
}

// Set current shader for rendering
void ShaderAPIGL::SetShader(IShaderProgram* pShader)
{
	m_pSelectedShader = pShader;
}

// RAW Constant (Used for structure types, etc.)
int ShaderAPIGL::SetShaderConstantRaw(const char *pszName, const void *data, int nSize, int nConstId)
{
	if(!m_pSelectedShader)
		return -1;

	CGLShaderProgram* prog = (CGLShaderProgram*)m_pSelectedShader;

	int minUniform = 0;
	int maxUniform = prog->m_numConstants - 1;
	GLShaderConstant_t* uniforms = prog->m_constants;

	// Do a quick lookup in the sorted table with a binary search
	while (minUniform <= maxUniform)
	{
		int currUniform = (minUniform + maxUniform) >> 1;
		int res = strcmp(pszName, uniforms[currUniform].name);

		if (res == 0)
		{
			GLShaderConstant_t *uni = uniforms + currUniform;

			const int maxSize = min(nSize, (int)uni->size);

			if (memcmp(uni->data, data, maxSize))
			{
				memcpy(uni->data, data, maxSize);
				uni->dirty = true;
			}

			return currUniform;
		}
		else if (res > 0)
			minUniform = currUniform + 1;
		else
			maxUniform = currUniform - 1;
	}

	//MsgError("[SHADER] error: constant '%s' not found\n", pszName);

	return -1;
}

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

IVertexFormat* ShaderAPIGL::CreateVertexFormat(const char* name, VertexFormatDesc_s *formatDesc, int nAttribs)
{
	CVertexFormatGL *pVertexFormat = new CVertexFormatGL(name, formatDesc, nAttribs);

	m_Mutex.Lock();
	m_VFList.append(pVertexFormat);
	m_Mutex.Unlock();

	return pVertexFormat;
}

IVertexBuffer* ShaderAPIGL::CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData )
{
	CVertexBufferGL* pVB = new CVertexBufferGL();

	pVB->m_numVerts = nNumVerts;
	pVB->m_strideSize = strideSize;
	pVB->m_access = nBufAccess;

	DevMsg(DEVMSG_SHADERAPI,"Creating VBO with size %i KB\n", pVB->GetSizeInBytes() / 1024);

	const int numBuffers = (nBufAccess == BUFFER_DYNAMIC) ? MAX_VB_SWITCHING : 1;

	int result = glWorker.WaitForExecute(__FUNCTION__, [pVB, numBuffers, nBufAccess, pData]() {

		glGenBuffers(numBuffers, pVB->m_nGL_VB_Index);

		if (!GLCheckError("gen vert buffer"))
			return -1;

		for (int i = 0; i < numBuffers; i++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, pVB->m_nGL_VB_Index[i]);
			GLCheckError("bind buffer");

			glBufferData(GL_ARRAY_BUFFER, pVB->GetSizeInBytes(), pData, glBufferUsages[nBufAccess]);
			GLCheckError("upload vtx data");
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return 0;
	});

	if (result == -1)
	{
		delete pVB;
		return nullptr;
	}

	m_Mutex.Lock();
	m_VBList.append( pVB );
	m_Mutex.Unlock();

	return pVB;
}

IIndexBuffer* ShaderAPIGL::CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData )
{
	CIndexBufferGL* pIB = new CIndexBufferGL();

	pIB->m_nIndices = nIndices;
	pIB->m_nIndexSize = nIndexSize;
	pIB->m_access = nBufAccess;

	DevMsg(DEVMSG_SHADERAPI,"Creating IBO with size %i KB\n", (nIndices*nIndexSize) / 1024);

	int size = nIndices * nIndexSize;

	const int numBuffers = (nBufAccess == BUFFER_DYNAMIC) ? MAX_IB_SWITCHING : 1;

	int result = glWorker.WaitForExecute(__FUNCTION__, [pIB, numBuffers, nBufAccess, pData, size]() {
		glGenBuffers(numBuffers, pIB->m_nGL_IB_Index);

		if (!GLCheckError("gen idx buffer"))
			return -1;

		for (int i = 0; i < numBuffers; i++)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIB->m_nGL_IB_Index[i]);
			GLCheckError("bind buff");

			glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, pData, glBufferUsages[nBufAccess]);
			GLCheckError("upload idx data");
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		return 0;
	});

	if (result == -1)
	{
		delete pIB;
		return nullptr;
	}

	m_Mutex.Lock();
	m_IBList.append( pIB );
	m_Mutex.Unlock();

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
		CScopedMutex m(m_Mutex);
		deleted = m_VFList.remove(pVF);
	}

	if(deleted)
	{
		DevMsg(DEVMSG_SHADERAPI, "Destroying vertex format\n");
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
		CScopedMutex m(m_Mutex);
		deleted = m_VBList.remove(pVB);
	}

	if(deleted)
	{
		const int numBuffers = (pVB->m_access == BUFFER_DYNAMIC) ? MAX_VB_SWITCHING : 1;
		uint* tempArray = new uint[numBuffers];
		memcpy(tempArray, pVB->m_nGL_VB_Index, numBuffers * sizeof(uint));

		delete pVB;

		glWorker.Execute(__FUNCTION__, [numBuffers, tempArray]() {

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
		CScopedMutex m(m_Mutex);
		deleted = m_IBList.remove(pIB);
	}

	if(deleted)
	{
		const int numBuffers = (pIB->m_access == BUFFER_DYNAMIC) ? MAX_IB_SWITCHING : 1;
		uint* tempArray = new uint[numBuffers];
		memcpy(tempArray, pIB->m_nGL_IB_Index, numBuffers * sizeof(uint));

		delete pIndexBuffer;

		glWorker.Execute(__FUNCTION__, [numBuffers, tempArray]() {
			
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

IVertexFormat* pPlainFormat = NULL;

PRIMCOUNTER g_pGLPrimCounterCallbacks[] =
{
	PrimCount_TriangleList,
	PrimCount_TriangleFanStrip,
	PrimCount_TriangleFanStrip,
	PrimCount_QuadList,
	PrimCount_ListList,
	PrimCount_ListStrip,
	PrimCount_None,
	PrimCount_Points,
	PrimCount_None,
};

// Indexed primitive drawer
void ShaderAPIGL::DrawIndexedPrimitives(ER_PrimitiveType nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
	ASSERT(nVertices > 0);

	if(nIndices <= 2)
		return;

	if (!m_pCurrentIndexBuffer)
		return;

	int nTris = g_pGLPrimCounterCallbacks[nType](nIndices);

	uint indexSize = m_pCurrentIndexBuffer->GetIndexSize();

	int numInstances = 0;

	if(m_boundInstanceStream != -1 && m_pCurrentVertexBuffers[m_boundInstanceStream])
		numInstances = m_pCurrentVertexBuffers[m_boundInstanceStream]->GetVertexCount();

	if(numInstances)
		glDrawElementsInstanced(glPrimitiveType[nType], nIndices, indexSize == 2? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, BUFFER_OFFSET(indexSize * nFirstIndex), numInstances);
	else
		glDrawElements(glPrimitiveType[nType], nIndices, indexSize == 2? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, BUFFER_OFFSET(indexSize * nFirstIndex));

	GLCheckError("draw elements");

	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

// Draw elements
void ShaderAPIGL::DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices)
{
	if(m_pCurrentVertexFormat == NULL)
		return;

	if (nVertices <= 2)
		return;

	int nTris = g_pGLPrimCounterCallbacks[nType](nVertices);

	int numInstances = 0;

	if(m_boundInstanceStream != -1 && m_pCurrentVertexBuffers[m_boundInstanceStream])
		numInstances = m_pCurrentVertexBuffers[m_boundInstanceStream]->GetVertexCount();

	if(numInstances)
		glDrawArraysInstanced(glPrimitiveType[nType], nFirstVertex, nVertices, numInstances);
	else
		glDrawArrays(glPrimitiveType[nType], nFirstVertex, nVertices);

	GLCheckError("draw arrays");

	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

bool ShaderAPIGL::IsDeviceActive()
{
	return true;
}

void ShaderAPIGL::SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName)
{

}

//-------------------------------------------------------------
// State manipulation
//-------------------------------------------------------------

// creates blending state
IRenderState* ShaderAPIGL::CreateBlendingState( const BlendStateParam_t &blendDesc )
{
	CGLBlendingState* pState = NULL;

	for(int i = 0; i < m_BlendStates.numElem(); i++)
	{
		pState = (CGLBlendingState*)m_BlendStates[i];

		if(blendDesc.blendEnable == pState->m_params.blendEnable)
		{
			if(blendDesc.blendEnable == true)
			{
				if(blendDesc.srcFactor == pState->m_params.srcFactor &&
					blendDesc.dstFactor == pState->m_params.dstFactor &&
					blendDesc.blendFunc == pState->m_params.blendFunc &&
					blendDesc.mask == pState->m_params.mask &&
					blendDesc.alphaTest == pState->m_params.alphaTest)
				{

					if(blendDesc.alphaTest)
					{
						if(blendDesc.alphaTestRef == pState->m_params.alphaTestRef)
						{
							pState->AddReference();
							return pState;
						}
					}
					else
					{
						pState->AddReference();
						return pState;
					}
				}
			}
			else
			{
				pState->AddReference();
				return pState;
			}
		}
	}

	pState = new CGLBlendingState;
	pState->m_params = blendDesc;

	m_BlendStates.append(pState);

	pState->AddReference();

	return pState;
}

// creates depth/stencil state
IRenderState* ShaderAPIGL::CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc )
{
	CGLDepthStencilState* pState = NULL;

	for(int i = 0; i < m_DepthStates.numElem(); i++)
	{
		pState = (CGLDepthStencilState*)m_DepthStates[i];

		if(depthDesc.depthWrite == pState->m_params.depthWrite &&
			depthDesc.depthTest == pState->m_params.depthTest &&
			depthDesc.depthFunc == pState->m_params.depthFunc &&
			depthDesc.doStencilTest == pState->m_params.doStencilTest )
		{
			// if we searching for stencil test
			if(depthDesc.doStencilTest)
			{
				if(	depthDesc.nDepthFail == pState->m_params.nDepthFail &&
					depthDesc.nStencilFail == pState->m_params.nStencilFail &&
					depthDesc.nStencilFunc == pState->m_params.nStencilFunc &&
					depthDesc.nStencilMask == pState->m_params.nStencilMask &&
					depthDesc.nStencilMask == pState->m_params.nStencilWriteMask &&
					depthDesc.nStencilMask == pState->m_params.nStencilRef &&
					depthDesc.nStencilPass == pState->m_params.nStencilPass)
				{
					pState->AddReference();
					return pState;
				}
			}
			else
			{
				pState->AddReference();
				return pState;
			}
		}
	}

	pState = new CGLDepthStencilState;
	pState->m_params = depthDesc;

	m_DepthStates.append(pState);

	pState->AddReference();

	return pState;
}

// creates rasterizer state
IRenderState* ShaderAPIGL::CreateRasterizerState( const RasterizerStateParams_t &rasterDesc )
{
	CGLRasterizerState* pState = NULL;

	for(int i = 0; i < m_RasterizerStates.numElem(); i++)
	{
		pState = (CGLRasterizerState*)m_RasterizerStates[i];

		if(rasterDesc.cullMode == pState->m_params.cullMode &&
			rasterDesc.fillMode == pState->m_params.fillMode &&
			rasterDesc.multiSample == pState->m_params.multiSample &&
			rasterDesc.scissor == pState->m_params.scissor &&
			rasterDesc.useDepthBias == pState->m_params.useDepthBias)
		{
			pState->AddReference();
			return pState;
		}
	}

	pState = new CGLRasterizerState();
	pState->m_params = rasterDesc;

	pState->AddReference();

	m_RasterizerStates.append(pState);

	return pState;
}

// completely destroys shader
void ShaderAPIGL::DestroyRenderState( IRenderState* pState, bool removeAllRefs )
{
	if(!pState)
		return;

	CScopedMutex scoped(m_Mutex);

	pState->RemoveReference();

	if(pState->GetReferenceNum() > 0 && !removeAllRefs)
	{
		return;
	}

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
void ShaderAPIGL::SetViewport(int x, int y, int w, int h)
{
	// this is actually represents our viewport state
	m_viewPort = IRectangle(x,y,x+w,y+h);

	glViewport(x, y, w, h);
	GLCheckError("set viewport");
}

// returns viewport
void ShaderAPIGL::GetViewport(int &x, int &y, int &w, int &h)
{
	x = m_viewPort.vleftTop.x;
	y = m_viewPort.vleftTop.y;

	IVector2D size = m_viewPort.GetSize();

	w = size.x;
	h = size.y;
}

// returns current size of viewport
void ShaderAPIGL::GetViewportDimensions(int &wide, int &tall)
{
	IVector2D size = m_viewPort.GetSize();

	wide = size.x;
	tall = size.y;
}

// sets scissor rectangle
void ShaderAPIGL::SetScissorRectangle( const IRectangle &rect )
{
	IVector2D viewportSize = m_viewPort.GetSize();

    IRectangle scissor(rect);

    scissor.vleftTop.y = viewportSize.y - scissor.vleftTop.y;
    scissor.vrightBottom.y = viewportSize.y - scissor.vrightBottom.y;

    QuickSwap(scissor.vleftTop.y, scissor.vrightBottom.y);

    IVector2D size = scissor.GetSize();
	glScissor( scissor.vleftTop.x, scissor.vleftTop.y, size.x, size.y);
	GLCheckError("set scissor");
}

int ShaderAPIGL::GetSamplerUnit(CGLShaderProgram* prog, const char* samplerName)
{
	if(!prog || !samplerName)
		return -1;

	GLShaderSampler_t* samplers = prog->m_samplers;
	int minSampler = 0;
	int maxSampler = prog->m_numSamplers - 1;

	// Do a quick lookup in the sorted table with a binary search
	while (minSampler <= maxSampler)
	{
		int currSampler = (minSampler + maxSampler) >> 1;
        int res = strcmp(samplerName, samplers[currSampler].name);
		if (res == 0)
		{
			return samplers[currSampler].index;
		}
		else if(res > 0)
		{
            minSampler = currSampler + 1;
		}
		else
		{
            maxSampler = currSampler - 1;
		}
	}

	return -1;
}

// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
void ShaderAPIGL::SetTexture( ITexture* pTexture, const char* pszName, int index )
{
	int unit = GetSamplerUnit((CGLShaderProgram*)m_pSelectedShader, pszName);

	if (unit >= 0)
		index = unit;

	SetTextureOnIndex(pTexture, index);
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------


extern CGLRenderLib g_library;
#include "CGLRenderLib.h"

// prepares for async operation (required to be called in main thread)
void ShaderAPIGL::BeginAsyncOperation( uintptr_t threadId )
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (thisThreadId == m_mainThreadId) // not required for main thread
		return;

	ASSERT(m_asyncOperationActive == false);
	
	GL_CONTEXT sharedContext = g_library.GetSharedContext();

#ifdef USE_GLES2
	eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, sharedContext);
#elif _WIN32
	wglMakeCurrent(m_hdc, sharedContext);
#elif LINUX
	glXMakeCurrent(m_display, (Window)m_params->windowHandle, sharedContext);
#elif __APPLE__

#endif

	m_asyncOperationActive = true;
}

// completes for async operation (must be called in worker thread)
void  ShaderAPIGL::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if(thisThreadId == m_mainThreadId) // not required for main thread
	{
		ASSERTMSG(false, "EndAsyncOperation() cannot be called from main thread!");
		return;
	}

	ASSERT(m_asyncOperationActive == true);

	//glFinish();

#ifdef USE_GLES2
	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
#elif _WIN32
	wglMakeCurrent(NULL, NULL);
#elif LINUX
	glXMakeCurrent(m_display, None, NULL);
#elif __APPLE__

#endif

	m_asyncOperationActive = false;
}
