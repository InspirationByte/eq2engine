//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "ShaderAPIGL.h"

#include "CGLTexture.h"
#include "VertexFormatGL.h"
#include "VertexBufferGL.h"
#include "IndexBufferGL.h"
#include "GLShaderProgram.h"
#include "GLRenderState.h"
#include "GLOcclusionQuery.h"

#include "DebugInterface.h"

#include "shaderapigl_def.h"

#include "imaging/ImageLoader.h"

#include "IConCommandFactory.h"
#include "utils/strtools.h"
#include "utils/KeyValues.h"

#include "utils/eqthread.h"

#include <functional>

extern ShaderAPIGL g_shaderApi;

#define WORK_PENDING_MARKER 0x1d1d0001

class GLWorkerThread : CEqThread
{
public:
	GLWorkerThread()
	{
	}

	int Do(std::function<int()> f);

protected:
	int Run();

	struct work_t
	{
		work_t(std::function<int()> f, uint id)
		{
			func = f;
			result = WORK_PENDING_MARKER;
			workId = id;
		}

		std::function<int()> func;
		volatile int result;
		uint workId;
	};

	int WaitForResult(uint workId);

	uint m_workCounter;
	DkList<work_t*> m_pendingWork;
	work_t* m_currentWork;
	CEqMutex m_workMutex;
};

int GLWorkerThread::WaitForResult(uint workId)
{
	// find by work id
	m_workMutex.Lock();
	work_t* work = nullptr;
	for (int i = 0; i < m_pendingWork.numElem(); i++)
	{
		if (m_pendingWork[i]->workId == workId)
		{
			work = m_pendingWork[i];
			break;
		}
	}
	m_workMutex.Unlock();

	ASSERT(work);

	Msg("awaiting %d\n", work->workId);

	// wait
	while (work->result == WORK_PENDING_MARKER)
	{
		Platform_Sleep(1);
	}

	// remove from list
	m_workMutex.Lock();
	m_pendingWork.fastRemove(work);
	m_workMutex.Unlock();

	// retrieve result and delete
	int result = work->result;

	Msg("got result from %d\n", work->workId);

	delete work;

	//return the result
	return result;
}

int GLWorkerThread::Do(std::function<int()> f)
{
	//uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	//if (thisThreadId == g_shaderApi.m_mainThreadId) // not required for main thread
	//	return (f)();

	if (!IsRunning())
		StartWorkerThread("ShaderAPIGLWorker");

	uint workId = 0xFFFFFFFF;

	// set the new worker and signal to start...
	{
		CScopedMutex m(m_workMutex);

		workId = m_workCounter++;

		work_t* work = new work_t(f, workId);
		m_pendingWork.append(work);
	}

	// set the worker function
	SignalWork();

	return WaitForResult(workId);
}

int GLWorkerThread::Run()
{
	{
		CScopedMutex m(m_workMutex);

		// find some work
		while (!m_currentWork && m_pendingWork.numElem())
		{
			for (int i = 0; i < m_pendingWork.numElem(); i++)
			{
				work_t* work = m_pendingWork[i];

				if (work->result == WORK_PENDING_MARKER)
				{
					m_currentWork = work;
					break;
				}
			}
		}
	}

	if (m_currentWork)
	{
		// get and quickly dispose
		work_t* cur = m_currentWork;
		m_currentWork = nullptr;

		g_shaderApi.BeginAsyncOperation(GetThreadID());
		g_shaderApi.GL_CRITICAL();

		// run work
		int result = cur->func();

		g_shaderApi.EndAsyncOperation();

		m_workMutex.Lock();
		cur->result = result;
		m_workMutex.Unlock();
	}

	return 0;
}

GLWorkerThread glWorker;

HOOK_TO_CVAR(r_loadmiplevel);

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

        EqString errString = varargs("code %x", lastError);

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
#ifndef USE_GLES2
            case GL_STACK_UNDERFLOW:
                errString = "GL_STACK_UNDERFLOW";
                break;
            case GL_STACK_OVERFLOW:
                errString = "GL_STACK_OVERFLOW";
                break;
#endif // USE_GLES2
        }

		if(gl_report_errors.GetBool())
			MsgError("*OGL* error occured while '%s' (%s)\n", op, errString.c_str());

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
	m_currentGLIB = 0;
}

void ShaderAPIGL::PrintAPIInfo()
{
	Msg("ShaderAPI: ShaderAPIGL\n");

	MsgInfo("------ Loaded textures ------\n");

	Msg("Active workers: %d\n", m_activeWorkers.numElem());
	for(int i = 0; i < m_activeWorkers.numElem(); i++)
	{
		MsgInfo("  worker TID=%d numWorks=%d active=%d\n", m_activeWorkers[i].threadId, m_activeWorkers[i].numWorks, m_activeWorkers[i].active == true);
	}

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

	m_mainThreadId = Threading::GetCurrentThreadID();

	m_contextBound = true;

	// don't wait on first commands
	m_busySignal.Raise();

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
			ASSERTMSG(false, varargs("Uniform function for '%d' is not ok, pls check extensions\n", i));
	}
}

void ShaderAPIGL::Shutdown()
{
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
	for (int i = 0; i < m_caps.maxTextureUnits; i++)
	{
		CGLTexture* pSelectedTexture = (CGLTexture*)m_pSelectedTextures[i];
		CGLTexture* pCurrentTexture = (CGLTexture*)m_pCurrentTextures[i];
		
		if(pSelectedTexture != pCurrentTexture)
		{
			// Set the active texture unit and bind the selected texture to target
			glActiveTexture(GL_TEXTURE0 + i);

			if (pSelectedTexture == NULL)
			{
				if(pCurrentTexture != NULL)
				{
					glBindTexture(pCurrentTexture->glTarget, 0);
				}
			}
			else
			{
				if (pCurrentTexture == NULL)
				{
					// bind texture
					glBindTexture(pSelectedTexture->glTarget, pSelectedTexture->GetCurrentTexture().glTexID);
#ifndef USE_GLES2
					glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, pSelectedTexture->m_flLod);
#endif // USE_GLES2
				}
				else
				{
#ifndef USE_GLES2
					if (pSelectedTexture->m_flLod != pCurrentTexture->m_flLod)
						glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, pSelectedTexture->m_flLod);
#endif // USE_GLES2

					// bind our texture
					glBindTexture(pSelectedTexture->glTarget, pSelectedTexture->GetCurrentTexture().glTexID);
				}
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

			#pragma todo("GL: stencil func")
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
					const float POLYGON_OFFSET_SCALE = 32.0f;	// FIXME: is a depth buffer bit depth?

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
		glUseProgram(selectedShader ? selectedShader->m_program : 0);
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

	GL_CRITICAL();

	CGLOcclusionQuery* occQuery = new CGLOcclusionQuery();

	m_Mutex.Lock();
	m_OcclusionQueryList.append( occQuery );
	m_Mutex.Unlock();

	return occQuery;
}

// removal of occlusion query object
void ShaderAPIGL::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
	GL_CRITICAL();

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

	GL_CRITICAL();

	if(pTex->Ref_Count() == 0)
		MsgWarning("texture %s refcount==0\n",pTex->GetName());

	//ASSERT(pTex->numReferences > 0);

	CScopedMutex scoped(m_Mutex);

	pTex->Ref_Drop();

	if(pTex->Ref_Count() <= 0)
	{
		DevMsg(DEVMSG_SHADERAPI,"Texture unloaded: %s\n",pTex->GetName());

		m_TextureList.remove(pTexture);
		delete pTex;
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
	return CreateNamedRenderTarget("__rt_001", width, height, nRTFormat, textureFilterType,textureAddress,comparison,nFlags);
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

	int tex_flags = nFlags;

	tex_flags |= TEXFLAG_RENDERTARGET;

	pTexture->SetFlags(tex_flags);
	pTexture->SetName(pszName);

	pTexture->glTarget = (tex_flags & TEXFLAG_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	CScopedMutex scoped(m_Mutex);

	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);

	pTexture->SetSamplerState(texSamplerParams);

	Finish();

	pTexture->textures.setNum(1);

	glGenTextures(1, &pTexture->textures[0].glTexID);
	GLCheckError("gen tex");

	glBindTexture(pTexture->glTarget, pTexture->textures[0].glTexID);

	SetupGLSamplerState(pTexture->glTarget, texSamplerParams);

	// this generates the render target
	ResizeRenderTarget(pTexture, width,height);

	m_TextureList.append(pTexture);
	return pTexture;
}

void ShaderAPIGL::ResizeRenderTarget(ITexture* pRT, int newWide, int newTall)
{
	CGLTexture* pTex = (CGLTexture*)pRT;
	ETextureFormat format = pTex->GetFormat();

	pTex->SetDimensions(newWide,newTall);

	if (pTex->glTarget == GL_RENDERBUFFER)
	{
		// Bind render buffer
		glBindRenderbuffer(GL_RENDERBUFFER, pTex->glDepthID);
		glRenderbufferStorage(GL_RENDERBUFFER, internalFormats[format], newWide, newTall);

		// Restore renderbuffer
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	else
	{
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
		}
		else
		{
			glTexImage2D(pTex->glTarget, 0, internalFormat, newWide, newTall, 0, srcFormat, srcType, NULL);
		}

		glBindTexture(pTex->glTarget, 0);
	}
}

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

	const GLenum glTarget = glTexTargetType[texture.type];

	// set our referenced params
	wide = pSrc->GetWidth();
	tall = pSrc->GetHeight();

	GLTextureRef_t* texturePtr = &texture;

	int result = glWorker.Do([=]() {
		// Generate a texture
		glGenTextures(1, &texturePtr->glTexID);

		if (!GLCheckError("gen tex"))
			return -1;

		glBindTexture(glTarget, texturePtr->glTexID);
		GLCheckError("bind tex");

		// Setup the sampler state
		SetupGLSamplerState(glTarget, sampler, numMipmaps);

		if (!UpdateGLTextureFromImage(*texturePtr, pSrc, nQuality))
		{
			glBindTexture(glTarget, 0);
			GLCheckError("tex unbind");

			glDeleteTextures(1, &texture.glTexID);
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

#ifdef USE_GLES2
	glTexParameteri(texTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
#else
	glTexParameteri(texTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
#endif // USE_GLES2
	GLCheckError("smp cmpmode");

	glTexParameteri(texTarget, GL_TEXTURE_COMPARE_FUNC, depthConst[sampler.compareFunc]);
	GLCheckError("smp cmpfunc");

#ifndef USE_GLES2
	// Setup anisotropic filtering
	if (sampler.aniso > 1 && GLAD_GL_EXT_texture_filter_anisotropic)
	{
		glTexParameteri(texTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.aniso);
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

	glBlitFramebuffer(0, 0, m_nViewportWidth, m_nViewportHeight, 0, pTargetTexture->GetHeight(), pTargetTexture->GetWidth(), 0, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

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
	// FIXME: use glCopyTexSubImage2D

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

		int &nCubeFace = nCubemapFaces[i];

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

	CGLTexture* pDepth = (CGLTexture*)pDepthTarget;

	if (pDepth != m_pCurrentDepthRenderTarget)
	{
		if (pDepth != NULL && pDepth->glTarget != GL_RENDERBUFFER)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pDepth->textures[0].glTexID, 0);
			if (IsStencilFormat(pDepth->GetFormat()))
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, pDepth->textures[0].glTexID, 0);
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			}
		}
		else
		{
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, (pDepth == NULL) ? 0 : pDepth->textures[0].glTexID);

			if (pDepth != NULL && IsStencilFormat(pDepth->GetFormat()))
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pDepth->textures[0].glTexID);
			}
			else
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}
		}

		m_pCurrentDepthRenderTarget = pDepth;
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

	glBindFramebuffer(GL_FRAMEBUFFER,0);
	SetViewport(0, 0, m_nViewportWidth, m_nViewportHeight);

	int numRTs = m_nCurrentRenderTargets;

	for (int i = 0; i < numRTs; i++)
	{
		if (m_pCurrentColorRenderTargets[i] != NULL)
			m_pCurrentColorRenderTargets[i] = NULL;
	}

	if (m_pCurrentDepthRenderTarget != NULL)
		m_pCurrentDepthRenderTarget = NULL;
}

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

// Matrix mode
void ShaderAPIGL::SetMatrixMode(ER_MatrixMode nMatrixMode)
{
#ifndef USE_GLES2
	glMatrixMode( matrixModeConst[nMatrixMode] );
#endif // USE_GLES2

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
#ifndef USE_GLES2
	glLoadIdentity();
#endif // USE_GLES2

	m_matrices[m_nCurrentMatrixMode] = identity4();
}

// Load custom matrix
void ShaderAPIGL::LoadMatrix(const Matrix4x4 &matrix)
{
#ifndef USE_GLES2
	if(m_nCurrentMatrixMode == MATRIXMODE_WORLD)
	{
		glMatrixMode( GL_MODELVIEW );
		glLoadMatrixf( transpose(m_matrices[MATRIXMODE_VIEW] * matrix) );
	}
	else
		glLoadMatrixf( transpose(matrix) );
#endif // USE_GLES2

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
	static CVertexFormatGL zero(nullptr, 0);

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

				glVertexAttribDivisorARB(i, 0);
				GLCheckError("divisor");
			}
			/*else if(shouldEnable)
			{
				glEnableVertexAttribArray(i);
				GLCheckError("enable vtx attrib");
			}*/
		}

		m_pCurrentVertexFormat = pVertexFormat;
	}
}

// Changes the vertex buffer
void ShaderAPIGL::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer, int nStream, const intptr offset)
{
	CVertexBufferGL* pVB = (CVertexBufferGL*)pVertexBuffer;
	GLuint vbo = pVB ? pVB->GetCurrentBuffer() : 0;

#ifdef USE_GLES2
	const GLsizei glTypes[] = {
		GL_FLOAT,
		GL_HALF_FLOAT,
		GL_UNSIGNED_BYTE,
	};
#else
	const GLsizei glTypes[] = {
		GL_FLOAT,
		GL_HALF_FLOAT_ARB,
		GL_UNSIGNED_BYTE,
	};
#endif // USE_GLES2

	bool instanceBuffer = (nStream > 0) && pVB != NULL && (pVB->GetFlags() & VERTBUFFER_FLAG_INSTANCEDATA);

	// should be always rebound
	if (pVB != m_pCurrentVertexBuffers[nStream] || offset != m_nCurrentOffsets[nStream] || m_currentGLVB[nStream] != vbo)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_currentGLVB[nStream] = vbo);
		GLCheckError("bind array");

		CVertexFormatGL* currentFormat = (CVertexFormatGL*)m_pCurrentVertexFormat;

		if (currentFormat != NULL)
		{
			int vertexSize = currentFormat->m_streamStride[nStream];

			char* base = (char *)(offset * vertexSize);			

			for (int i = 0; i < m_caps.maxVertexGenericAttributes; i++)
			{
				eqGLVertAttrDesc_t& attrib = currentFormat->m_genericAttribs[i];

				if (attrib.streamId != nStream)
					continue;

				if(attrib.sizeInBytes)
				{
					// enable this attribute first
					glEnableVertexAttribArray(i);
					GLCheckError("enable vtx attrib");

					glVertexAttribPointer(i, attrib.sizeInBytes, glTypes[attrib.attribFormat], GL_FALSE, vertexSize, base + attrib.offsetInBytes);
					GLCheckError("attribpointer"); 

					// instance vertex attrib divisor
					int selStreamParam = instanceBuffer ? 1 : 0;

					glVertexAttribDivisorARB(i, selStreamParam);
					GLCheckError("divisor");
				}
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		GLCheckError("unbind array after vattrib");
	}

	if(pVertexBuffer)
	{
		if(!instanceBuffer && m_boundInstanceStream != -1)
			m_boundInstanceStream = -1;
		else if(instanceBuffer && m_boundInstanceStream == -1)
			m_boundInstanceStream = nStream;
		else if(instanceBuffer && m_boundInstanceStream != -1)
		{
			ASSERTMSG(false, "Already bound instancing stream at %d!!!");
		}
	}

	m_pCurrentVertexBuffers[nStream] = pVertexBuffer;
	m_nCurrentOffsets[nStream] = offset;
	m_pActiveVertexFormat[nStream] = m_pCurrentVertexFormat;
}

// Changes the index buffer
void ShaderAPIGL::ChangeIndexBuffer(IIndexBuffer *pIndexBuffer)
{
	CIndexBufferGL* pSelectedIndexBuffer = (CIndexBufferGL*)pIndexBuffer;
	uint ibo = pSelectedIndexBuffer ? pSelectedIndexBuffer->GetCurrentBuffer() : 0;

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
	IShaderProgram* pNewProgram = new CGLShaderProgram;
	pNewProgram->SetName((_Es(pszName)+query).GetData());

	CScopedMutex scoped(m_Mutex);

	m_ShaderList.append(pNewProgram);

	return pNewProgram;
}

// search for existing shader program
IShaderProgram* ShaderAPIGL::FindShaderProgram(const char* pszName, const char* query)
{
	CScopedMutex m(m_Mutex);

	for(int i = 0; i < m_ShaderList.numElem(); i++)
	{
		char findtext[1024];
		findtext[0] = '\0';
		strcpy(findtext, pszName);

		if(query)
			strcat(findtext, query);

		if(!stricmp(m_ShaderList[i]->GetName(), findtext))
		{
			return m_ShaderList[i];
		}
	}

	return NULL;
}

// Destroy all shader
void ShaderAPIGL::DestroyShaderProgram(IShaderProgram* pShaderProgram)
{
	CGLShaderProgram* pShader = (CGLShaderProgram*)(pShaderProgram);

	if(!pShader)
		return;

	CScopedMutex scoped(m_Mutex);

	pShader->Ref_Drop(); // decrease references to this shader

	// remove it if reference is zero
	if(pShader->Ref_Count() <= 0)
	{
		// Cancel shader and destroy
		if(m_pCurrentShader == pShaderProgram)
		{
			Reset(STATE_RESET_SHADER);
			Apply();
		}

		m_ShaderList.remove(pShader);

		GL_CRITICAL();
		delete pShader;

		GLCheckError("delete shader program");
	}
}

#define SHADER_HELPERS_STRING \
	"#define saturate(x) clamp(x,0.0,1.0)\r\n"	\
	"#define lerp mix\r\n"						\
	"#define float2 vec2\r\n"					\
	"#define float3 vec3\r\n"					\
	"#define float4 vec4\r\n"					\
	"#define float2x2 mat2\r\n"					\
	"#define float3x3 mat3\r\n"					\
	"#define float4x4 mat4\r\n"					\
	"#define mul(a,b) a*b\r\n"

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

	if (info.vs.text == NULL && info.ps.text == NULL)
		return false;

	if(!info.apiPrefs)
	{
		MsgError("Shader %s error: missing %s api preferences\n", pShaderOutput->GetName(), GetRendererName());
		return false;
	}

	CGLShaderProgram* prog = (CGLShaderProgram*)pShaderOutput;
	GLint vsResult, fsResult, linkResult;

	int result;

	// compile vertex
	if(info.vs.text)
	{
		GLint* pvsResult = &vsResult;

		EqString shaderString;

#ifndef USE_GLES2
		shaderString.Append("#version 120\r\n");
#endif // USE_GLES2

		if (extra != NULL)
			shaderString.Append(extra);

		// append useful HLSL replacements
		shaderString.Append(SHADER_HELPERS_STRING);
		shaderString.Append(info.vs.text);

		const GLchar* sStr[] = { shaderString.c_str() };

		result = glWorker.Do([prog, pvsResult, sStr]() {

			// create GL program
			prog->m_program = glCreateProgram();

			if (!GLCheckError("create program"))
				return -1;

			prog->m_vertexShader = glCreateShader(GL_VERTEX_SHADER);

			if (!GLCheckError("create vertex shader"))
				return -1;

			glShaderSource(prog->m_vertexShader, 1, (const GLchar**)sStr, NULL);
			glCompileShader(prog->m_vertexShader);

			GLCheckError("compile vert shader");

			glGetShaderiv(prog->m_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, pvsResult);

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
			for (int i = 0; i < info.vs.includes.numElem(); i++)
				MsgInfo("\t%d : %s\n", i + 1, info.vs.includes[i].c_str());

			return false;
		}
			
	}
	else
		return false; // vertex shader is required

	// compile fragment
	if(info.ps.text)
	{
		EqString shaderString;

#ifndef USE_GLES2
		shaderString.Append("#version 120\r\n");
#endif // USE_GLES2

		if (extra != NULL)
			shaderString.Append(extra);

		// append useful HLSL replacements
		shaderString.Append(SHADER_HELPERS_STRING);
		shaderString.Append(info.ps.text);

		const GLchar* sStr[] = { shaderString.c_str() };

		GLint* pfsResult = &fsResult;
		result = glWorker.Do([prog, pfsResult, sStr]() {
			prog->m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

			if (!GLCheckError("create fragment shader"))
				return -1;

			glShaderSource(prog->m_fragmentShader, 1, (const GLchar**)sStr, NULL);
			glCompileShader(prog->m_fragmentShader);
			glGetShaderiv(prog->m_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, pfsResult);

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
			for (int i = 0; i < info.ps.includes.numElem(); i++)
				MsgInfo("\t%d : %s\n", i + 1, info.ps.includes[i].c_str());

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

		result = glWorker.Do([prog, pInfo, plinkResult, vendor, currProgram]() {
			for(int i = 0; i < pInfo->apiPrefs->keys.numElem(); i++)
			{
				kvkeybase_t* kp = pInfo->apiPrefs->keys[i];

				if( !stricmp(kp->name, "attribute") )
				{
					const char* nameStr = KV_GetValueString(kp, 0, "INVALID");
					const char* locationStr = KV_GetValueString(kp, 1, "TYPE_TEXCOORD");

					int attribIndex = 0;	// all generic starts here

					// if starts with digit - this is an index
					if( isdigit(*locationStr) )
					{
						attribIndex = atoi(locationStr)+GLSL_VERTEX_ATTRIB_START;
					}
					else
					{
						// TODO: find corresponding attribute index for string types:
						// VERTEX0-VERTEX3	(4 parallel vertex buffers)
						// TEXCOORD0 - 7
					}

					// bind attribute
					glBindAttribLocation(prog->m_program, attribIndex, nameStr);
					GLCheckError("bind attrib");
				}
			}

			// link program and go
			glLinkProgram(prog->m_program);
			glGetProgramiv(prog->m_program, GL_OBJECT_LINK_STATUS_ARB, plinkResult);

			GLCheckError("link program");

			if( !(*plinkResult))
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
			if(vendor == VENDOR_INTEL )
			{
				glUseProgram(0);
				glUseProgram(prog->m_program);
			}

			GLint uniformCount, maxLength;
			glGetProgramiv(prog->m_program, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &uniformCount);
			glGetProgramiv(prog->m_program, GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB, &maxLength);

			if(maxLength == 0 && uniformCount > 0 || uniformCount > 256)
			{
				if(vendor == VENDOR_INTEL)
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
				if (type >= GL_SAMPLER_1D && type <= GL_SAMPLER_2D_RECT_SHADOW_ARB)
	#endif // USE_GLES3
				{
					GLShaderSampler_t* sp = &samplers[nSamplers];
					ASSERTMSG(sp, "WHAT?");

					// Assign samplers to image units
					GLint location = glGetUniformLocation(prog->m_program, tmpName);
					glUniform1i(location, nSamplers);

					DevMsg(DEVMSG_SHADERAPI, "[DEBUG] retrieving sampler '%s' at %d (location = %d)\n", tmpName, nSamplers, location);

					sp->index = nSamplers;
					strcpy(sp->name, tmpName);
					nSamplers++;
				}
				else
				{
					// Store all non-gl uniforms
					if (strncmp(tmpName, "gl_", 3) != 0)
					{
						DevMsg(DEVMSG_SHADERAPI, "[DEBUG] retrieving uniform '%s' at %d\n", tmpName, nUniforms);

						char *bracket = strchr(tmpName, '[');
						if (bracket == NULL || (bracket[1] == '0' && bracket[2] == ']'))
						{
							if (bracket)
							{
								*bracket = '\0';
								length = (GLint) (bracket - tmpName);
							}

							uniforms[nUniforms].index = glGetUniformLocation(prog->m_program, tmpName);
							uniforms[nUniforms].type = GetConstantType(type);
							uniforms[nUniforms].nElements = size;
							strcpy(uniforms[nUniforms].name, tmpName);
							nUniforms++;
						}
						else if (bracket != NULL && bracket[1] > '0')
						{
							*bracket = '\0';
							for (int j = nUniforms - 1; j >= 0; j--)
							{
								if (strcmp(uniforms[i].name, tmpName) == 0)
								{
									int index = atoi(bracket + 1) + 1;
									if (index > uniforms[j].nElements)
									{
										uniforms[j].nElements = index;
									}
								}
							}
						} // bracket
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
				int constantSize = s_constantTypeSizes[uniforms[i].type] * uniforms[i].nElements;
				uniforms[i].data = new ubyte[constantSize];
				memset(uniforms[i].data, 0, constantSize);
				uniforms[i].dirty = false;
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

			if (memcmp(uni->data, data, nSize))
			{
				memcpy(uni->data, data, nSize);
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

IVertexFormat* ShaderAPIGL::CreateVertexFormat(VertexFormatDesc_s *formatDesc, int nAttribs)
{
	CVertexFormatGL *pVertexFormat = new CVertexFormatGL(formatDesc, nAttribs);

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

	int result = glWorker.Do([pVB, numBuffers, nBufAccess, pData]() {

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

	int result = glWorker.Do([pIB, numBuffers, nBufAccess, pData, size]() {
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

	CScopedMutex m(m_Mutex);

	if(m_VFList.remove(pFormat))
	{
		// reset if in use
		if (m_pCurrentVertexFormat == pFormat)
		{
			Reset(STATE_RESET_VF);
			ApplyBuffers();
		}

		delete pVF;
	}
}

// Destroy vertex buffer
void ShaderAPIGL::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	CVertexBufferGL* pVB = (CVertexBufferGL*)(pVertexBuffer);
	if (!pVB)
		return;

	CScopedMutex m(m_Mutex);

	if(m_VBList.remove(pVertexBuffer))
	{
		GL_CRITICAL();

		Reset(STATE_RESET_VF | STATE_RESET_VB);
		ApplyBuffers();

		const int numBuffers = (pVB->m_access == BUFFER_DYNAMIC) ? MAX_VB_SWITCHING : 1;

		glDeleteBuffers(numBuffers, pVB->m_nGL_VB_Index);
		GLCheckError("delete vertex buffer");

		delete pVB;
	}
}

// Destroy index buffer
void ShaderAPIGL::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	CIndexBufferGL* pIB = (CIndexBufferGL*)(pIndexBuffer);

	if (!pIB)
		return;

	CScopedMutex m(m_Mutex);

	if(m_IBList.remove(pIndexBuffer))
	{
		Reset(STATE_RESET_IB);
		ApplyBuffers();

		GL_CRITICAL();

		const int numBuffers = (pIB->m_access == BUFFER_DYNAMIC) ? MAX_IB_SWITCHING : 1;

		glDeleteBuffers(numBuffers, pIB->m_nGL_IB_Index);
		GLCheckError("delete index buffer");

		delete pIndexBuffer;
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
	ASSERT(m_pCurrentIndexBuffer != NULL);
	ASSERT(nVertices > 0);

	int nTris = g_pGLPrimCounterCallbacks[nType](nIndices);

	uint indexSize = m_pCurrentIndexBuffer->GetIndexSize();

	int numInstances = 0;

	if(m_boundInstanceStream != -1 && m_pCurrentVertexBuffers[m_boundInstanceStream])
		numInstances = m_pCurrentVertexBuffers[m_boundInstanceStream]->GetVertexCount();

	if(numInstances)
		glDrawElementsInstancedARB(glPrimitiveType[nType], nIndices, indexSize == 2? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, BUFFER_OFFSET(indexSize * nFirstIndex), numInstances);
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

	int nTris = g_pGLPrimCounterCallbacks[nType](nVertices);

	int numInstances = 0;

	if(m_boundInstanceStream != -1 && m_pCurrentVertexBuffers[m_boundInstanceStream])
		numInstances = m_pCurrentVertexBuffers[m_boundInstanceStream]->GetVertexCount();

	if(numInstances)
		glDrawArraysInstancedARB(glPrimitiveType[nType], nFirstVertex, nVertices, numInstances);
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
		SetTextureOnIndex(pTexture, unit);
	else
		SetTextureOnIndex(pTexture, index);
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------

// Owns context for current execution thread
void ShaderAPIGL::GL_CRITICAL()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if(thisThreadId == m_mainThreadId) // not required for main thread
		return;

	int workerIdx = -1;

	m_Mutex.Lock();
	for(int i = 0; i < m_activeWorkers.numElem(); i++)
	{
		if( m_activeWorkers[i].threadId == thisThreadId )
		{
			workerIdx = i;
			break;
		}
	}

	ASSERTMSG(workerIdx != -1, "No BeginAsyncOperation() called for specified thread!");

	if(workerIdx != -1)
	{
		activeWorker_t& worker = m_activeWorkers[workerIdx];

		if( worker.active ) // don't make context again
		{
			m_Mutex.Unlock();
			return;
		}

		worker.active = true;

		//Msg("Apply context to thread\n");

#ifdef USE_GLES2
		eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, worker.context);
#elif _WIN32
		wglMakeCurrent(m_hdc, worker.context);
#elif LINUX
		glXMakeCurrent(m_display, (Window)m_params->windowHandle, worker.context);
#elif __APPLE__

#endif
	}

	m_Mutex.Unlock();
}

extern CGLRenderLib g_library;
#include "CGLRenderLib.h"

// prepares for async operation (required to be called in main thread)
void ShaderAPIGL::BeginAsyncOperation( uintptr_t threadId )
{
	m_Mutex.Lock();

	for(int i = 0; i < m_activeWorkers.numElem(); i++)
	{
		if( m_activeWorkers[i].threadId == threadId )
		{
			m_activeWorkers[i].numWorks++;

			//Msg("ShaderAPIGL::BeginAsyncOperation() duplicated\n");
			m_Mutex.Unlock();
			return; // already have one
		}
	}

	activeWorker_t aw;
	aw.threadId = threadId;
	aw.context = g_library.GetFreeSharedContext(threadId);
	aw.numWorks = 1;

	ASSERTMSG(aw.context ,"GetFreeSharedContext - no free contexts!");

	//Msg("ShaderAPIGL::BeginAsyncOperation() ok\n");

	m_activeWorkers.append(aw);
	m_Mutex.Unlock();
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

	int workerIdx = -1;

	m_Mutex.Lock();

	for(int i = 0; i < m_activeWorkers.numElem(); i++)
	{
		if( m_activeWorkers[i].threadId == thisThreadId )
		{
			workerIdx = i;
			break;
		}
	}

	ASSERTMSG(workerIdx != -1, "EndAsyncOperation() call requires BeginAsyncOperation() before this thread starts!");

	if(workerIdx != -1)
	{
		activeWorker_t& worker = m_activeWorkers[workerIdx];

		worker.numWorks--;

		//Msg("ShaderAPIGL::EndAsyncOperation() ok%s\n", worker.numWorks ? "" : " (no more work)");

		if(worker.numWorks <= 0)
		{
			glFinish();

#ifdef USE_GLES2
			eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
#elif _WIN32
			wglMakeCurrent(NULL, NULL);
#elif LINUX
			glXMakeCurrent(m_display, None, NULL);
#elif __APPLE__

#endif
			m_activeWorkers.fastRemoveIndex(workerIdx);
		}
	}

	m_Mutex.Unlock();
}
