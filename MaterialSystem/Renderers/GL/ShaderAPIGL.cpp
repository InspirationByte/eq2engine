//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
#include "GLMeshBuilder.h"
#include "GLOcclusionQuery.h"

#include "Platform.h"
#include "DebugInterface.h"

#include "shaderapigl_def.h"

#include "Imaging/ImageLoader.h"

#pragma todo("Rewrite with OpenGL ES 2.0 specification support")

static char s_FFPMeshBuilder_VertexProgram[] = 
"attribute vec4 input_vPos;									\
attribute vec2 input_texCoord;								\
attribute vec4 input_color;									\
varying vec2 texCoord;										\
varying vec4 color;											\
void main()													\
{															\
	gl_Position = gl_ModelViewProjectionMatrix * input_vPos;\
	color = input_color;									\
	texCoord = input_texCoord;								\
}";

static char s_FFPMeshBuilder_NoTexture_PixelProgram[] = 
"varying vec4 color;									\
void main()												\
{														\
	gl_FragColor = color;		\
}";

static char s_FFPMeshBuilder_Textured_PixelProgram[] = 
"varying vec2 texCoord;									\
varying vec4 color;										\
uniform sampler2D Base;									\
void main()												\
{														\
	gl_FragColor = texture2D(Base, texCoord)*color;		\
}";

typedef GLvoid (APIENTRY *UNIFORM_FUNC)(GLint location, GLsizei count, const void *value);
typedef GLvoid (APIENTRY *UNIFORM_MAT_FUNC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

ConstantType_e GetConstantType(GLenum type)
{
	switch (type)
	{
		case gl::FLOAT:			return CONSTANT_FLOAT;
		case gl::FLOAT_VEC2:	return CONSTANT_VECTOR2D;
		case gl::FLOAT_VEC3:	return CONSTANT_VECTOR3D;
		case gl::FLOAT_VEC4:	return CONSTANT_VECTOR4D;
		case gl::INT:			return CONSTANT_INT;
		case gl::INT_VEC2:		return CONSTANT_IVECTOR2D;
		case gl::INT_VEC3:		return CONSTANT_IVECTOR3D;
		case gl::INT_VEC4:		return CONSTANT_IVECTOR4D;
		case gl::BOOL:			return CONSTANT_BOOL;
		case gl::BOOL_VEC2:		return CONSTANT_BVECTOR2D;
		case gl::BOOL_VEC3:		return CONSTANT_BVECTOR3D;
		case gl::BOOL_VEC4:		return CONSTANT_BVECTOR4D;
		case gl::FLOAT_MAT2:	return CONSTANT_MATRIX2x2;
		case gl::FLOAT_MAT3:	return CONSTANT_MATRIX3x3;
		case gl::FLOAT_MAT4:	return CONSTANT_MATRIX4x4;
	}

	MsgError("Invalid constant type (%d)\n", type);

	return (ConstantType_e) -1;
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

	m_nCurrentMask = COLORMASK_ALL;
	m_bCurrentBlendEnable = false;

	m_nCurrentVBO = 0;

	m_frameBuffer = 0;
	m_depthBuffer = 0;

	m_nCurrentMatrixMode = MATRIXMODE_VIEW;

	m_pMeshBufferTexturedShader = NULL;
	m_pMeshBufferNoTextureShader = NULL;
}

void ShaderAPIGL::PrintAPIInfo()
{
	Msg("ShaderAPI: ShaderAPIGL LEGACY\n");

	MsgInfo("------ Loaded textures ------");

	CScopedMutex scoped(m_Mutex);
	for(int i = 0; i < m_TextureList.numElem(); i++)
	{
		CGLTexture* pTexture = (CGLTexture*)m_TextureList[i];

		MsgInfo("     %s (%d) - %dx%d\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(),pTexture->GetHeight());
	}
}

// Init + Shurdown
void ShaderAPIGL::Init(const shaderapiinitparams_t &params)
{
	m_mainThreadId = Threading::GetCurrentThreadID();
	m_currThreadId = m_mainThreadId;
	m_isSharing = false;
	m_isMainAtCritical = false;
	m_busySignal.Raise();

	// Set some of my preferred defaults
	gl::Enable(gl::DEPTH_TEST);
	gl::DepthFunc(gl::LEQUAL);
	gl::FrontFace(gl::CW);
	gl::PixelStorei(gl::PACK_ALIGNMENT,   1);
	gl::PixelStorei(gl::UNPACK_ALIGNMENT, 1);

	memset(&m_caps, 0, sizeof(m_caps));

	m_caps.maxTextureAnisotropicLevel = 1;

	if (gl::exts::var_EXT_texture_filter_anisotropic)
		gl::GetIntegerv(gl::MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_caps.maxTextureAnisotropicLevel);

	m_caps.isHardwareOcclusionQuerySupported = gl::exts::var_ARB_occlusion_query;
	m_caps.isInstancingSupported = gl::exts::var_ARB_instanced_arrays;

	gl::GetIntegerv(gl::MAX_TEXTURE_SIZE, &m_caps.maxTextureSize);

	m_caps.maxRenderTargets = MAX_MRTS;

	m_caps.maxVertexGenericAttributes = MAX_GENERIC_ATTRIB;
	m_caps.maxVertexTexcoordAttributes = MAX_TEXCOORD_ATTRIB;

	m_caps.shadersSupportedFlags = ((gl::exts::var_EXT_vertex_shader || gl::exts::var_ARB_shader_objects) ? SHADER_CAPS_VERTEX_SUPPORTED : 0)
								 | ((gl::exts::var_ARB_fragment_shader || gl::exts::var_ARB_shader_objects) ? SHADER_CAPS_PIXEL_SUPPORTED : 0);
	m_caps.maxTextureUnits = MAX_TEXTUREUNIT;
	m_caps.maxVertexStreams = MAX_VERTEXSTREAM;
	m_caps.maxVertexTextureUnits = MAX_VERTEXTEXTURES;

	m_caps.maxTextureUnits = 1;

	if (gl::exts::var_ARB_fragment_shader)
		gl::GetIntegerv(gl::MAX_TEXTURE_IMAGE_UNITS, &m_caps.maxTextureUnits);
	else 
		gl::GetIntegerv(gl::MAX_TEXTURE_UNITS, &m_caps.maxTextureUnits);

	if (gl::exts::var_ARB_draw_buffers)
	{
		m_caps.maxRenderTargets = 1;
		gl::GetIntegerv(gl::MAX_DRAW_BUFFERS_ARB, &m_caps.maxRenderTargets);
	}

	if (m_caps.maxRenderTargets > MAX_MRTS)
		m_caps.maxRenderTargets = MAX_MRTS;

	for (int i = 0; i < m_caps.maxRenderTargets; i++)
		m_drawBuffers[i] = gl::COLOR_ATTACHMENT0 + i;

	// Init the base shader API
	ShaderAPI_Base::Init(params);

	// all shaders supported, nothing to report

	kvkeybase_t baseMeshBufferParams;
	kvkeybase_t* attr = baseMeshBufferParams.AddKeyBase("attribute", "input_vPos");
	attr->AppendValue("0");

	attr = baseMeshBufferParams.AddKeyBase("attribute", "input_texCoord");
	attr->AppendValue("1");

	attr = baseMeshBufferParams.AddKeyBase("attribute", "input_color");
	attr->AppendValue("2");

	s_uniformFuncs[CONSTANT_FLOAT]		= (void *) gl::_detail::Uniform1fv;
	s_uniformFuncs[CONSTANT_VECTOR2D]	= (void *) gl::_detail::Uniform2fv;
	s_uniformFuncs[CONSTANT_VECTOR3D]	= (void *) gl::_detail::Uniform3fv;
	s_uniformFuncs[CONSTANT_VECTOR4D]	= (void *) gl::_detail::Uniform4fv;
	s_uniformFuncs[CONSTANT_INT]		= (void *) gl::_detail::Uniform1iv;
	s_uniformFuncs[CONSTANT_IVECTOR2D]	= (void *) gl::_detail::Uniform2iv;
	s_uniformFuncs[CONSTANT_IVECTOR3D]	= (void *) gl::_detail::Uniform3iv;
	s_uniformFuncs[CONSTANT_IVECTOR4D]	= (void *) gl::_detail::Uniform4iv;
	s_uniformFuncs[CONSTANT_BOOL]		= (void *) gl::_detail::Uniform1iv;
	s_uniformFuncs[CONSTANT_BVECTOR2D]	= (void *) gl::_detail::Uniform2iv;
	s_uniformFuncs[CONSTANT_BVECTOR3D]	= (void *) gl::_detail::Uniform3iv;
	s_uniformFuncs[CONSTANT_BVECTOR4D]	= (void *) gl::_detail::Uniform4iv;
	s_uniformFuncs[CONSTANT_MATRIX2x2]	= (void *) gl::_detail::UniformMatrix2fv;
	s_uniformFuncs[CONSTANT_MATRIX3x3]	= (void *) gl::_detail::UniformMatrix3fv;
	s_uniformFuncs[CONSTANT_MATRIX4x4]	= (void *) gl::_detail::UniformMatrix4fv;

	for(int i = 0; i < CONSTANT_TYPE_COUNT; i++)
	{
		if(s_uniformFuncs[i] == NULL)
			ASSERTMSG(false, varargs("Uniform function for '%d' is not ok, pls check extensions\n", i));
	}

	if(m_pMeshBufferTexturedShader == NULL)
	{
		m_pMeshBufferTexturedShader = CreateNewShaderProgram("MeshBuffer_Textured");

		shaderprogram_params_t sparams;
		memset(&sparams, 0, sizeof(sparams));

		sparams.pAPIPrefs = &baseMeshBufferParams;
		sparams.pszPSText = s_FFPMeshBuilder_Textured_PixelProgram;
		sparams.pszVSText = s_FFPMeshBuilder_VertexProgram;
		sparams.bDisableCache = true;

		CompileShadersFromStream(m_pMeshBufferTexturedShader, sparams);
	}

	if(m_pMeshBufferNoTextureShader == NULL)
	{
		m_pMeshBufferNoTextureShader = CreateNewShaderProgram("MeshBuffer_NoTexture");

		shaderprogram_params_t sparams;
		memset(&sparams, 0, sizeof(sparams));

		sparams.pAPIPrefs = &baseMeshBufferParams;
		sparams.pszPSText = s_FFPMeshBuilder_NoTexture_PixelProgram;
		sparams.pszVSText = s_FFPMeshBuilder_VertexProgram;
		sparams.bDisableCache = true;

		CompileShadersFromStream(m_pMeshBufferNoTextureShader,sparams);
	}
}

void ShaderAPIGL::Shutdown()
{
	GL_CRITICAL();

	ShaderAPI_Base::Shutdown();

	GL_END_CRITICAL();
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
	GL_CRITICAL();

	for (int i = 0; i < MAX_TEXTUREUNIT; i++)
	{
		CGLTexture* pCurrentTexture = (CGLTexture*)m_pCurrentTextures[i];
		CGLTexture* pSelectedTexture = (CGLTexture*)m_pSelectedTextures[i];
		
		if(pSelectedTexture != pCurrentTexture)
		{
			// Set the active texture to modify
			gl::ActiveTexture(gl::TEXTURE0 + i);

			if (pSelectedTexture == NULL)
			{
				if(pCurrentTexture != NULL)
				{
					gl::BindTexture(pCurrentTexture->glTarget, 0);
					gl::Disable(pCurrentTexture->glTarget);
				}
			} 
			else 
			{
				if (pCurrentTexture == NULL)
				{
					// enable texture
					gl::Enable( pSelectedTexture->glTarget );

					gl::BindTexture(pSelectedTexture->glTarget, pSelectedTexture->GetCurrentTexture().glTexID);

					gl::TexEnvf(gl::TEXTURE_FILTER_CONTROL, gl::TEXTURE_LOD_BIAS, pSelectedTexture->m_flLod);
				} 
				else
				{
					if (pSelectedTexture->glTarget != pCurrentTexture->glTarget)
					{
						// disable previously chosen target
						gl::Disable(pCurrentTexture->glTarget);

						gl::Enable(pSelectedTexture->glTarget);
					}

					if (pSelectedTexture->m_flLod != pCurrentTexture->m_flLod)
					{
						gl::TexEnvf(gl::TEXTURE_FILTER_CONTROL, gl::TEXTURE_LOD_BIAS, pSelectedTexture->m_flLod);
					}

					// bind our texture
					gl::BindTexture(pSelectedTexture->glTarget, pSelectedTexture->GetCurrentTexture().glTexID);
				}
			}

			m_pCurrentTextures[i] = m_pSelectedTextures[i];
		}
	}

	GL_END_CRITICAL();
}

void ShaderAPIGL::ApplySamplerState()
{
	//ASSERT(!"ShaderAPIGL::ApplySamplerState() not implemented!");
}

void ShaderAPIGL::ApplyBlendState()
{
	CGLBlendingState* pSelectedState = (CGLBlendingState*)m_pSelectedBlendstate;

	if (m_pCurrentBlendstate != m_pSelectedBlendstate)
	{
		GL_CRITICAL();

		if (m_pSelectedBlendstate == NULL)
		{
			if (m_bCurrentBlendEnable)
			{
				gl::Disable(gl::BLEND);
 				m_bCurrentBlendEnable = false;
			}
		}
		else
		{
			if (pSelectedState->m_params.blendEnable)
			{
				if (!m_bCurrentBlendEnable)
				{
					gl::Enable(gl::BLEND);
					m_bCurrentBlendEnable = true;
				}

				if (pSelectedState->m_params.srcFactor != m_nCurrentSrcFactor || pSelectedState->m_params.dstFactor != m_nCurrentDstFactor)
				{
					m_nCurrentSrcFactor = pSelectedState->m_params.srcFactor;
					m_nCurrentDstFactor = pSelectedState->m_params.dstFactor;

					gl::BlendFunc(blendingConsts[m_nCurrentSrcFactor],blendingConsts[m_nCurrentDstFactor]);
				}

				if (pSelectedState->m_params.blendFunc != m_nCurrentBlendFunc)
				{
					m_nCurrentBlendFunc = pSelectedState->m_params.blendFunc;

					gl::BlendEquation(blendingModes[m_nCurrentBlendFunc]);
				}
			}
			else
			{
				if (m_bCurrentBlendEnable)
				{
					gl::Disable(gl::BLEND);
 					m_bCurrentBlendEnable = false;
				}
			}

			if(pSelectedState->m_params.alphaTest)
			{
				gl::Enable(gl::ALPHA_TEST);
				gl::AlphaFunc(gl::GREATER,pSelectedState->m_params.alphaTestRef);
			}
			else
			{
				gl::Disable(gl::ALPHA_TEST);
			}
		}

		int mask = COLORMASK_ALL;
		if (m_pSelectedBlendstate != NULL)
		{
			mask = pSelectedState->m_params.mask;
		}

		if (mask != m_nCurrentMask)
		{
			gl::ColorMask((mask & COLORMASK_RED) ? 1 : 0, ((mask & COLORMASK_GREEN) >> 1) ? 1 : 0, ((mask & COLORMASK_BLUE) >> 2) ? 1 : 0, ((mask & COLORMASK_ALPHA) >> 3)  ? 1 : 0);

			m_nCurrentMask = mask;
		}

		m_pCurrentBlendstate = m_pSelectedBlendstate;

		GL_END_CRITICAL();
	}
}

void ShaderAPIGL::ApplyDepthState()
{
	// stencilRef currently not used
	CGLDepthStencilState* pSelectedState = (CGLDepthStencilState*)m_pSelectedDepthState;

	if (m_pSelectedDepthState != m_pCurrentDepthState)
	{
		GL_CRITICAL();

		if (m_pSelectedDepthState == NULL)
		{
			if (!m_bCurrentDepthTestEnable)
			{
				gl::Enable(gl::DEPTH_TEST);
				m_bCurrentDepthTestEnable = true;
			}

			if (!m_bCurrentDepthWriteEnable)
			{
				gl::DepthMask(gl::TRUE_);
				m_bCurrentDepthWriteEnable = true;
			}

			if (m_nCurrentDepthFunc != COMP_LEQUAL)
			{
				m_nCurrentDepthFunc = COMP_LEQUAL;
				gl::DepthFunc(depthConst[m_nCurrentDepthFunc]);
			}
		} 
		else 
		{
			if (pSelectedState->m_params.depthTest)
			{
				if (!m_bCurrentDepthTestEnable)
				{
					gl::Enable(gl::DEPTH_TEST);
					m_bCurrentDepthTestEnable = true;
				}
				if (pSelectedState->m_params.depthWrite != m_bCurrentDepthWriteEnable)
				{
					m_bCurrentDepthWriteEnable = pSelectedState->m_params.depthWrite;
					gl::DepthMask((m_bCurrentDepthWriteEnable)? gl::TRUE_ : gl::FALSE_);
				}
				if (pSelectedState->m_params.depthFunc != m_nCurrentDepthFunc)
				{
					m_nCurrentDepthFunc = pSelectedState->m_params.depthFunc;
					gl::DepthFunc(depthConst[m_nCurrentDepthFunc]);
				}
			} 
			else 
			{
				if (m_bCurrentDepthTestEnable)
				{
					gl::Disable(gl::DEPTH_TEST);
					m_bCurrentDepthTestEnable = false;
				}
			}

			#pragma todo("GL: stencil func")
		}

		m_pCurrentDepthState = m_pSelectedDepthState;

		GL_END_CRITICAL();
	}
}

void ShaderAPIGL::ApplyRasterizerState()
{
	CGLRasterizerState* pSelectedState = (CGLRasterizerState*)m_pSelectedRasterizerState;

	if (m_pCurrentRasterizerState != m_pSelectedRasterizerState)
	{
		GL_CRITICAL();

		if (pSelectedState == NULL)
		{
			if (CULL_BACK != m_nCurrentCullMode)
			{
				m_nCurrentCullMode = CULL_BACK;

				gl::CullFace(cullConst[m_nCurrentCullMode]);
			}

			if (FILL_SOLID != m_nCurrentFillMode)
			{
				m_nCurrentFillMode = FILL_SOLID;
				gl::PolygonMode(gl::FRONT_AND_BACK, fillConst[m_nCurrentFillMode]);
			}

			if (false != m_bCurrentMultiSampleEnable)
			{
				gl::Disable(gl::MULTISAMPLE);

				m_bCurrentMultiSampleEnable = false;
			}

			if (false != m_bCurrentScissorEnable)
			{
				gl::Disable(gl::SCISSOR_TEST);

				m_bCurrentScissorEnable = false;
			}

#pragma todo("GL: depth bias")
		}
		else
		{
			if (pSelectedState->m_params.cullMode != m_nCurrentCullMode)
			{
				if (pSelectedState->m_params.cullMode == CULL_NONE)
				{
					gl::Disable(gl::CULL_FACE);
				} 
				else 
				{
					if (m_nCurrentCullMode == CULL_NONE)
						gl::Enable(gl::CULL_FACE);
				
					gl::CullFace(cullConst[pSelectedState->m_params.cullMode]);
				}

				m_nCurrentCullMode = pSelectedState->m_params.cullMode;
			}

			if (pSelectedState->m_params.fillMode != m_nCurrentFillMode)
			{
				m_nCurrentFillMode = pSelectedState->m_params.fillMode;
				gl::PolygonMode(gl::FRONT_AND_BACK, fillConst[m_nCurrentFillMode]);
			}

			if (pSelectedState->m_params.multiSample != m_bCurrentMultiSampleEnable)
			{
				if (pSelectedState->m_params.multiSample)
				{
					gl::Enable(gl::MULTISAMPLE);
				} 
				else 
				{
					gl::Disable(gl::MULTISAMPLE);
				}
				m_bCurrentMultiSampleEnable = pSelectedState->m_params.multiSample;
			}

			if (pSelectedState->m_params.scissor != m_bCurrentScissorEnable)
			{
				if (pSelectedState->m_params.scissor)
				{
					gl::Enable(gl::SCISSOR_TEST);
				}
				else 
				{
					gl::Disable(gl::SCISSOR_TEST);
				}
				m_bCurrentScissorEnable = pSelectedState->m_params.scissor;
			}

		}

		GL_END_CRITICAL();
	}

	m_pCurrentRasterizerState = m_pSelectedRasterizerState;
}

void ShaderAPIGL::ApplyShaderProgram()
{
	if (m_pSelectedShader != m_pCurrentShader)
	{
		GL_CRITICAL();

		if (m_pSelectedShader == NULL)
		{
			gl::UseProgram(0);
		} 
		else 
		{
			CGLShaderProgram* prog = (CGLShaderProgram*)m_pSelectedShader;

			gl::UseProgram( prog->m_program );
		}

		m_pCurrentShader = m_pSelectedShader;

		GL_END_CRITICAL();
	}
}

void ShaderAPIGL::ApplyConstants()
{
	if (m_pCurrentShader != NULL)
	{
		GL_CRITICAL();

		CGLShaderProgram* prog = (CGLShaderProgram*)m_pCurrentShader;

		for (int i = 0; i < prog->m_numConstants; i++)
		{
			GLShaderConstant_t* uni = prog->m_constants + i;

			if (uni->dirty)
			{
				if (uni->type >= CONSTANT_MATRIX2x2)
					((UNIFORM_MAT_FUNC) s_uniformFuncs[uni->type])(uni->index, uni->nElements, gl::TRUE_, (float *) uni->data);
				else 
					((UNIFORM_FUNC) s_uniformFuncs[uni->type])(uni->index, uni->nElements, (float *) uni->data);

				uni->dirty = false;
			}
		}

		GL_END_CRITICAL();
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

	GL_CRITICAL();

	if (bClearColor)
	{
		gl::ColorMask(gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);
		clearBits |= gl::COLOR_BUFFER_BIT;
		gl::ClearColor(fillColor.x, fillColor.y, fillColor.z, 1.0f);
	}

	if (bClearDepth)
	{
		gl::DepthMask(gl::TRUE_);
		clearBits |= gl::DEPTH_BUFFER_BIT;
		gl::ClearDepth(fDepth);
	}

	if (bClearStencil)
	{
		gl::StencilMask(gl::TRUE_);
		clearBits |= gl::STENCIL_BUFFER_BIT;
		gl::ClearStencil(nStencil);
	}

	if (clearBits)
	{
		gl::Clear(clearBits);
	}

	GL_END_CRITICAL();
}
//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

// Device vendor and version
const char* ShaderAPIGL::GetDeviceNameString() const
{
	return (const char*)gl::GetString(gl::VENDOR);
}

// Renderer string (ex: OpenGL, D3D9)
const char* ShaderAPIGL::GetRendererName() const
{
	return "OpenGL";
}

// Pixel shader version
bool ShaderAPIGL::IsSupportsPixelShaders() const
{
	return gl::exts::var_ARB_fragment_shader;
}

// Vertex shader version
bool ShaderAPIGL::IsSupportsVertexShaders() const
{
	return gl::exts::var_EXT_vertex_shader;
}

// Geometry shader version
bool ShaderAPIGL::IsSupportsGeometryShaders() const
{
	// For now is not possible
	return false;
}

// The driver/hardware is supports Domain shaders?
bool ShaderAPIGL::IsSupportsDomainShaders() const
{
	return false;
}

// The driver/hardware is supports Hull (tessellator) shaders?
bool ShaderAPIGL::IsSupportsHullShaders() const
{
	return false;
}

// Render targetting support
bool ShaderAPIGL::IsSupportsRendertargetting() const
{
	return gl::exts::var_ARB_draw_buffers || gl::exts::var_EXT_draw_buffers2;
}

// Render targetting support for Multiple RTs
bool ShaderAPIGL::IsSupportsMRT() const
{
	return gl::exts::var_ARB_draw_buffers;
}

// Supports multitexturing???
bool ShaderAPIGL::IsSupportsMultitexturing() const
{
	return true;//gl::exts::var_EXT_multitexture;
}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

// Synchronization
void ShaderAPIGL::Flush()
{
	gl::Flush();
}

void ShaderAPIGL::Finish()
{
	gl::Finish();
}

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

// creates occlusion query object
IOcclusionQuery* ShaderAPIGL::CreateOcclusionQuery()
{
	ThreadingSharingRequest();

	CGLOcclusionQuery* occQuery = new CGLOcclusionQuery();

	m_Mutex.Lock();
	m_OcclusionQueryList.append( occQuery );
	m_Mutex.Unlock();

	ThreadingSharingRelease();

	return occQuery;
}

// removal of occlusion query object
void ShaderAPIGL::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
	ThreadingSharingRequest();
	if(pQuery)
		delete pQuery;

	m_OcclusionQueryList.fastRemove( pQuery );
	ThreadingSharingRelease();
}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

// Unload the texture and free the memory
void ShaderAPIGL::FreeTexture(ITexture* pTexture)
{
	CGLTexture* pTex = dynamic_cast<CGLTexture*>(pTexture);

	if(pTex == NULL)
		return;

	ThreadingSharingRequest();

	if(pTex->Ref_Count() == 0)
		MsgWarning("texture %s refcount==0\n",pTex->GetName());

	//ASSERT(pTex->numReferences > 0);

	CScopedMutex scoped(m_Mutex);

	pTex->Ref_Drop();

	if(pTex->Ref_Count() <= 0)
	{
		DevMsg(3,"Texture unloaded: %s\n",pTex->GetName());

		m_TextureList.remove(pTexture);
		delete pTex;
	}

	ThreadingSharingRelease();
}

// It will add new rendertarget
ITexture* ShaderAPIGL::CreateRenderTarget(	int width, int height,
											ETextureFormat nRTFormat,
											Filter_e textureFilterType, 
											AddressMode_e textureAddress, 
											CompareFunc_e comparison, 
											int nFlags)
{
	return CreateNamedRenderTarget("__rt_001", width, height, nRTFormat, textureFilterType,textureAddress,comparison,nFlags);
}

// It will add new rendertarget
ITexture* ShaderAPIGL::CreateNamedRenderTarget(	const char* pszName,
												int width, int height, 
												ETextureFormat nRTFormat, Filter_e textureFilterType,
												AddressMode_e textureAddress,
												CompareFunc_e comparison,
												int nFlags)
{
	CGLTexture *pTexture = new CGLTexture;

	pTexture->SetDimensions(width,height);
	pTexture->SetFormat(nRTFormat);

	int tex_flags = nFlags;

	tex_flags |= TEXFLAG_RENDERTARGET;

	pTexture->SetFlags(tex_flags);
	pTexture->mipMapped = false;
	pTexture->SetName(pszName);

	pTexture->glTarget = (tex_flags & TEXFLAG_CUBEMAP) ? gl::TEXTURE_CUBE_MAP : gl::TEXTURE_2D;

	CScopedMutex scoped(m_Mutex);

	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);

	pTexture->SetSamplerState(texSamplerParams);

	Finish();

	pTexture->textures.setNum(1);

	gl::GenTextures(1, &pTexture->textures[0].glTexID);
	gl::BindTexture(pTexture->glTarget, pTexture->textures[0].glTexID);

	InternalSetupSampler(pTexture->glTarget, texSamplerParams);

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

	if (pTex->glTarget == gl::RENDERBUFFER_EXT)
	{
		// Bind render buffer
		gl::BindRenderbufferEXT(gl::RENDERBUFFER_EXT, pTex->glDepthID);
		gl::RenderbufferStorageEXT(gl::RENDERBUFFER_EXT, internalFormats[format], newWide, newTall);

		// Restore renderbuffer
		gl::BindRenderbufferEXT(gl::RENDERBUFFER_EXT, 0);
	}
	else 
	{
		GLint internalFormat = internalFormats[format];
		GLenum srcFormat = srcFormats[GetChannelCount(format)];
		GLenum srcType = srcTypes[format];

		if (IsDepthFormat(format))
		{
			if (IsStencilFormat(format))
				srcFormat = gl::DEPTH_STENCIL_EXT;
			else 
				srcFormat = gl::DEPTH_COMPONENT;
		}

		if (IsFloatFormat(format))
			srcType = gl::FLOAT;

		// Allocate all required surfaces.
		gl::BindTexture(pTex->glTarget, pTex->textures[0].glTexID);

		if (pTex->GetFlags() & TEXFLAG_CUBEMAP)
		{
			for (int i = gl::TEXTURE_CUBE_MAP_POSITIVE_X; i <= gl::TEXTURE_CUBE_MAP_NEGATIVE_Z; i++)
				gl::TexImage2D(i, 0, internalFormat, newWide, newTall, 0, srcFormat, srcType, NULL);
		} 
		else
		{
			gl::TexImage2D(pTex->glTarget, 0, internalFormat, newWide, newTall, 0, srcFormat, srcType, NULL);
		}

		gl::BindTexture(pTex->glTarget, 0);
	}
}

GLuint ShaderAPIGL::CreateGLTextureFromImage(CImage* pSrc, GLuint gltarget, const SamplerStateParam_t& sampler, int& wide, int& tall, int nFlags)
{
	if(!pSrc)
		return 0;

	HOOK_TO_CVAR(r_loadmiplevel);

	int nQuality = r_loadmiplevel->GetInt();

	// force quality to best
	if(nFlags & TEXFLAG_NOQUALITYLOD)
		nQuality = 0;

	bool bMipMaps = (pSrc->GetMipMapCount() > 1);

	if(!bMipMaps)
		nQuality = 0;

	wide = pSrc->GetWidth();
	tall = pSrc->GetHeight();

	GLuint textureID = 0;

	if(nFlags & TEXFLAG_CUBEMAP)
	{
		pSrc->SetDepth(0);
	}

	// If the target hardware doesn't support the compressed texture format, just decompress it to a compatible format
	ETextureFormat format = pSrc->GetFormat();

	GLenum srcFormat = srcFormats[GetChannelCount(format)];
    GLenum srcType = srcTypes[format];
	GLint internalFormat = internalFormats[format];

	if(format >= FORMAT_I32F && format <= FORMAT_RGBA32F)
	{
        internalFormat = internalFormats[format - (FORMAT_I32F - FORMAT_I16F)];
	}

	ThreadingSharingRequest();

	// Generate a texture
	gl::GenTextures(1, &textureID);

	gl::Enable(gltarget);

	gl::BindTexture( gltarget, textureID );


	// Setup the sampler state
	InternalSetupSampler(gltarget, sampler);

	//Msg("Gen texture target=%d id=%d dim=%dx%d\n", gltarget, textureID, pSrc->GetWidth(nQuality), pSrc->GetHeight(nQuality));

	// Upload it all
	ubyte *src;
	int mipMapLevel = nQuality;
	while ((src = pSrc->GetPixels(mipMapLevel)) != NULL)
	{
		int size = pSrc->GetMipMappedSize(mipMapLevel, 1);

		int lockBoxLevel = mipMapLevel - nQuality;

		if (pSrc->IsCube())
		{
			size /= 6;

			for (uint i = 0; i < 6; i++)
			{
				if (IsCompressedFormat(format))
				{
					gl::CompressedTexImage2D(	gl::TEXTURE_CUBE_MAP_POSITIVE_X + i, 
											lockBoxLevel, 
											internalFormat,
											pSrc->GetWidth(mipMapLevel), pSrc->GetHeight(mipMapLevel),
											0,
											size,
											src + i * size);
				} 
				else 
				{
					gl::TexImage2D(	gl::TEXTURE_CUBE_MAP_POSITIVE_X + i,
									lockBoxLevel,
									internalFormat,
									pSrc->GetWidth(mipMapLevel), pSrc->GetHeight(mipMapLevel),
									0, 
									srcFormat,
									srcType,
									src + i * size);
				}
			}
		}
		else if (pSrc->Is3D())
		{
			if (IsCompressedFormat(format))
			{
				gl::CompressedTexImage3D(	gltarget,
										lockBoxLevel, 
										internalFormat, 
										pSrc->GetWidth(mipMapLevel), pSrc->GetHeight(mipMapLevel), pSrc->GetDepth(mipMapLevel),
										0, 
										pSrc->GetMipMappedSize(mipMapLevel, 1), 
										src);
			} 
			else 
			{
				gl::TexImage3D(	gltarget, 
								mipMapLevel - nQuality, 
								internalFormat, 
								pSrc->GetWidth(mipMapLevel), pSrc->GetHeight(mipMapLevel), pSrc->GetDepth(mipMapLevel), 
								0, 
								srcFormat, 
								srcType, 
								src);
			}
		} 
		else if (pSrc->Is2D())
		{
			if (IsCompressedFormat(format))
			{
				gl::CompressedTexImage2D(	gltarget, 
										lockBoxLevel, 
										internalFormat, 
										pSrc->GetWidth(mipMapLevel), pSrc->GetHeight(mipMapLevel), 
										0,
										size, 
										src);
			} 
			else 
			{
				gl::TexImage2D(	gltarget,
								lockBoxLevel,
								internalFormat, 
								pSrc->GetWidth(mipMapLevel), pSrc->GetHeight(mipMapLevel),
								0,
								srcFormat,
								srcType,
								src);
			}
		} 
		else 
		{
			gl::TexImage1D(	gltarget,
							mipMapLevel - nQuality,
							internalFormat,
							pSrc->GetWidth(mipMapLevel),
							0,
							srcFormat,
							srcType,
							src);
		}

		mipMapLevel++;
	}

	if(pSrc->IsCube())
		gl::Disable( gl::TEXTURE_CUBE_MAP );

	gl::BindTexture(gltarget, 0);

	ThreadingSharingRelease();

	return textureID;
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

	pTexture->glTarget = pImages[0]->IsCube()? gl::TEXTURE_CUBE_MAP : pImages[0]->Is3D()? gl::TEXTURE_3D : pImages[0]->Is2D()? gl::TEXTURE_2D : gl::TEXTURE_1D;

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

		GLuint nGlTex = CreateGLTextureFromImage(pImages[i], pTexture->glTarget, ss, wide, tall, nFlags);

		if(nGlTex)
		{
			eqGlTex tex;
			tex.glTexID = nGlTex;

			pTexture->textures.append( tex );
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

void ShaderAPIGL::InternalSetupSampler(uint texTarget, const SamplerStateParam_t& sampler)
{
	//GL_CRITICAL();
	
	// Set requested wrapping modes
	gl::TexParameteri(texTarget, gl::TEXTURE_WRAP_S, (sampler.wrapS == ADDRESSMODE_WRAP) ? gl::REPEAT : gl::CLAMP_TO_EDGE);

	if (texTarget != gl::TEXTURE_1D)
		gl::TexParameteri(texTarget, gl::TEXTURE_WRAP_T, (sampler.wrapT == ADDRESSMODE_WRAP) ? gl::REPEAT : gl::CLAMP_TO_EDGE);

	if (texTarget == gl::TEXTURE_3D)
		gl::TexParameteri(texTarget, gl::TEXTURE_WRAP_R, (sampler.wrapR == ADDRESSMODE_WRAP) ? gl::REPEAT : gl::CLAMP_TO_EDGE);

	// Set requested filter modes
	gl::TexParameteri(texTarget, gl::TEXTURE_MAG_FILTER, minFilters[sampler.magFilter]);
	gl::TexParameteri(texTarget, gl::TEXTURE_MIN_FILTER, minFilters[sampler.minFilter]);

	gl::TexParameteri(texTarget, gl::TEXTURE_COMPARE_MODE, gl::COMPARE_R_TO_TEXTURE);
	gl::TexParameteri(texTarget, gl::TEXTURE_COMPARE_FUNC, depthConst[sampler.nComparison]);

	// Setup anisotropic filtering
	if (sampler.aniso > 1 && gl::exts::var_EXT_texture_filter_anisotropic)
	{
		gl::TexParameteri(texTarget, gl::TEXTURE_MAX_ANISOTROPY_EXT, sampler.aniso);
	}

	//GL_END_CRITICAL();
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

// Copy render target to texture
void ShaderAPIGL::CopyFramebufferToTexture(ITexture* pTargetTexture)
{
	GL_CRITICAL();

	ChangeRenderTarget(pTargetTexture);

	gl::BindFramebufferEXT(gl::READ_FRAMEBUFFER, 0);
	gl::BindFramebufferEXT(gl::DRAW_FRAMEBUFFER, m_frameBuffer);

	gl::BlitFramebuffer(0, 0,m_nViewportWidth, m_nViewportHeight,0,pTargetTexture->GetHeight(),pTargetTexture->GetWidth(), 0, gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT, gl::NEAREST);
	//if(textures[rt].)

	gl::BindFramebufferEXT(gl::READ_FRAMEBUFFER, 0);
	gl::BindFramebufferEXT(gl::DRAW_FRAMEBUFFER, 0);

	ChangeRenderTargetToBackBuffer();

	GL_END_CRITICAL();
}

/*
// Copy render target to texture with resizing
void ShaderAPIGL::CopyFramebufferToTextureEx(ITexture* pTargetTexture,int srcX0, int srcY0,int srcX1, int srcY1,int destX0, int destY0,int destX1, int destY1)
{
	ASSERT(!"TODO: Implement ShaderAPIGL::CopyFramebufferToTextureEx()");
}
*/

// Changes render target (MRT)
void ShaderAPIGL::ChangeRenderTargets(ITexture** pRenderTargets, int nNumRTs, int* nCubemapFaces, ITexture* pDepthTarget, int nDepthSlice)
{
	GL_CRITICAL();

	if (m_frameBuffer == 0)
		gl::GenFramebuffersEXT(1, &m_frameBuffer);

	gl::BindFramebufferEXT(gl::FRAMEBUFFER_EXT, m_frameBuffer);

	for (int i = 0; i < nNumRTs; i++)
	{
		CGLTexture* colorRT = (CGLTexture*)pRenderTargets[i];

		int &nCubeFace = nCubemapFaces[i];

		if (colorRT->GetFlags() & TEXFLAG_CUBEMAP)
		{
			if (colorRT != m_pCurrentColorRenderTargets[i] || m_pCurrentRenderTargetsSlices[i] != nCubeFace)
			{
				gl::FramebufferTexture2DEXT(gl::FRAMEBUFFER_EXT, 
						gl::COLOR_ATTACHMENT0_EXT + i, gl::TEXTURE_CUBE_MAP_POSITIVE_X + nCubeFace, colorRT->textures[0].glTexID, 0);

				m_pCurrentRenderTargetsSlices[i] = nCubeFace;
			}
		}
		else 
		{
			if (colorRT != m_pCurrentColorRenderTargets[i])
			{
				gl::FramebufferTexture2DEXT(gl::FRAMEBUFFER_EXT, gl::COLOR_ATTACHMENT0_EXT + i, gl::TEXTURE_2D, colorRT->textures[0].glTexID, 0);
			}
		}

		m_pCurrentColorRenderTargets[i] = colorRT;
	}

	GL_END_CRITICAL();
	
	if (nNumRTs != m_nCurrentRenderTargets)
	{
		GL_CRITICAL();

		for (int i = nNumRTs; i < m_nCurrentRenderTargets; i++)
		{
			gl::FramebufferTexture2DEXT(gl::FRAMEBUFFER_EXT, gl::COLOR_ATTACHMENT0_EXT + i, gl::TEXTURE_2D, 0, 0);
			m_pCurrentRenderTargetsSlices[i] = NULL;
			m_pCurrentRenderTargetsSlices[i] = -1;
		}
		
		if (nNumRTs == 0)
		{
			gl::DrawBuffer(gl::NONE);
			gl::ReadBuffer(gl::NONE);
		} 
		else 
		{
			gl::DrawBuffers(nNumRTs, m_drawBuffers);
			gl::ReadBuffer(gl::COLOR_ATTACHMENT0_EXT);
		}

		m_nCurrentRenderTargets = nNumRTs;

		GL_END_CRITICAL();
	}

	CGLTexture* pDepth = (CGLTexture*)pDepthTarget;

	if (pDepth != m_pCurrentDepthRenderTarget)
	{
		GL_CRITICAL();

		if (pDepth != NULL && 
			pDepth->glTarget != gl::RENDERBUFFER_EXT)
		{
			gl::FramebufferTexture2DEXT(gl::FRAMEBUFFER_EXT, gl::DEPTH_ATTACHMENT_EXT, gl::TEXTURE_2D, pDepth->textures[0].glTexID, 0);
			if (IsStencilFormat(pDepth->GetFormat()))
			{
				gl::FramebufferTexture2DEXT(gl::FRAMEBUFFER_EXT, gl::STENCIL_ATTACHMENT_EXT, gl::TEXTURE_2D, pDepth->textures[0].glTexID, 0);
			}
			else 
			{
				gl::FramebufferTexture2DEXT(gl::FRAMEBUFFER_EXT, gl::STENCIL_ATTACHMENT_EXT, gl::TEXTURE_2D, 0, 0);
			}
		} 
		else 
		{
			gl::FramebufferRenderbufferEXT(gl::FRAMEBUFFER_EXT, gl::DEPTH_ATTACHMENT_EXT, gl::RENDERBUFFER_EXT, (pDepth == NULL) ? 0 : pDepth->textures[0].glTexID);
			if (pDepth != NULL &&
				IsStencilFormat(pDepth->GetFormat()))
			{
				gl::FramebufferRenderbufferEXT(gl::FRAMEBUFFER_EXT, gl::STENCIL_ATTACHMENT_EXT, gl::RENDERBUFFER_EXT, pDepth->textures[0].glTexID);
			}
			else 
			{
				gl::FramebufferRenderbufferEXT(gl::FRAMEBUFFER_EXT, gl::STENCIL_ATTACHMENT_EXT, gl::RENDERBUFFER_EXT, 0);
			}
		}

		m_pCurrentDepthRenderTarget = pDepth;

		GL_END_CRITICAL();
	}

	
	if (m_nCurrentRenderTargets > 0 && 
		m_pCurrentColorRenderTargets[0] != NULL)
	{
		GL_CRITICAL();

		// I still don't know why GL decided to be like that... damn
		if (m_pCurrentColorRenderTargets[0]->GetFlags() & TEXFLAG_CUBEMAP)
			InternalChangeFrontFace(gl::CCW);
		else 
			InternalChangeFrontFace(gl::CW);

		gl::Viewport(0, 0, m_pCurrentColorRenderTargets[0]->GetWidth(), m_pCurrentColorRenderTargets[0]->GetHeight());

		GL_END_CRITICAL();
	}
	else if(m_pCurrentDepthRenderTarget != NULL)
	{
		GL_CRITICAL();

		InternalChangeFrontFace(gl::CW);
		gl::Viewport(0, 0, m_pCurrentDepthRenderTarget->GetWidth(), m_pCurrentDepthRenderTarget->GetHeight());

		GL_END_CRITICAL();
	}
}

// fills the current rendertarget buffers
void ShaderAPIGL::GetCurrentRenderTargets(ITexture* pRenderTargets[MAX_MRTS], int *numRTs, ITexture** pDepthTarget, int cubeNumbers[MAX_MRTS])
{
	int nRts = 0;

	if(pRenderTargets)
	{
		for (register int i = 0; i < m_caps.maxRenderTargets; i++)
		{
			nRts++;

			pRenderTargets[i] = m_pCurrentColorRenderTargets[i];

			if(cubeNumbers)
				cubeNumbers[i] = m_nCurrentCRTSlice[i];

			if(m_pCurrentColorRenderTargets[i] == NULL)
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
	{
		gl::FrontFace(m_nCurrentFrontFace = nCullFaceMode);
	}
}

// Changes back to backbuffer
void ShaderAPIGL::ChangeRenderTargetToBackBuffer()
{
	if (m_frameBuffer == 0)
		return;

	GL_CRITICAL();
	
	gl::BindFramebufferEXT(gl::FRAMEBUFFER_EXT,0);
	gl::Viewport(0, 0, m_nViewportWidth, m_nViewportHeight);

	if (m_pCurrentColorRenderTargets[0] != NULL)
	{
		m_pCurrentColorRenderTargets[0] = NULL;
	}

	for (uint8 i = 1; i < m_nCurrentRenderTargets; i++)
	{
		if (m_pCurrentColorRenderTargets[i] != NULL)
		{
			m_pCurrentColorRenderTargets[i] = NULL;
		}
	}

	if (m_pCurrentDepthRenderTarget != NULL)
	{
		m_pCurrentDepthRenderTarget = NULL;
	}

	GL_END_CRITICAL();
}

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

// Matrix mode
void ShaderAPIGL::SetMatrixMode(MatrixMode_e nMatrixMode)
{
	GL_CRITICAL();

	gl::MatrixMode( matrixModeConst[nMatrixMode] );
	m_nCurrentMatrixMode = nMatrixMode;

	GL_END_CRITICAL();
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
	GL_CRITICAL();

	gl::LoadIdentity();
	m_matrices[m_nCurrentMatrixMode] = identity4();

	GL_END_CRITICAL();
}

// Load custom matrix
void ShaderAPIGL::LoadMatrix(const Matrix4x4 &matrix)
{
	GL_CRITICAL();

	if(m_nCurrentMatrixMode == MATRIXMODE_WORLD)
	{
		gl::MatrixMode( gl::MODELVIEW );
		gl::LoadMatrixf( transpose(m_matrices[MATRIXMODE_VIEW] * matrix) );
	}
	else
		gl::LoadMatrixf( transpose(matrix) );
	
	m_matrices[m_nCurrentMatrixMode] = matrix;
	//glPopMatrix();

	GL_END_CRITICAL();
}

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

// Set Depth range for next primitives
void ShaderAPIGL::SetDepthRange(float fZNear,float fZFar)
{
	gl::DepthRange(fZNear,fZFar);
}

// Changes the vertex format
void ShaderAPIGL::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	CVertexFormatGL* pCurrentFormat = NULL;
	CVertexFormatGL* pSelectedFormat = NULL;

	if( pVertexFormat != m_pCurrentVertexFormat )
	{
		GL_CRITICAL();

		static CVertexFormatGL* zero = new CVertexFormatGL();

		pCurrentFormat = zero;
		pSelectedFormat = zero;

		if (m_pCurrentVertexFormat != NULL)
			pCurrentFormat = (CVertexFormatGL*)m_pCurrentVertexFormat;

		if (pVertexFormat != NULL)
			pSelectedFormat = (CVertexFormatGL*)pVertexFormat;

		// Change array enables as needed
		if ( pSelectedFormat->m_hVertex.m_nSize && !pCurrentFormat->m_hVertex.m_nSize)
			gl::EnableClientState (gl::VERTEX_ARRAY);

		if (!pSelectedFormat->m_hVertex.m_nSize &&  pCurrentFormat->m_hVertex.m_nSize)
			gl::DisableClientState(gl::VERTEX_ARRAY);

		if ( pSelectedFormat->m_hNormal.m_nSize && !pCurrentFormat->m_hNormal.m_nSize)
			gl::EnableClientState (gl::NORMAL_ARRAY);

		if (!pSelectedFormat->m_hNormal.m_nSize &&  pCurrentFormat->m_hNormal.m_nSize)
			gl::DisableClientState(gl::NORMAL_ARRAY);

		if ( pSelectedFormat->m_hColor.m_nSize && !pCurrentFormat->m_hColor.m_nSize)
			gl::EnableClientState (gl::COLOR_ARRAY);

		if (!pSelectedFormat->m_hColor.m_nSize &&  pCurrentFormat->m_hColor.m_nSize)
			gl::DisableClientState(gl::COLOR_ARRAY);

		for (int i = 0; i < MAX_GL_GENERIC_ATTRIB; i++)
		{
			if ( pSelectedFormat->m_hGeneric[i].m_nSize && !pCurrentFormat->m_hGeneric[i].m_nSize)
				gl::EnableVertexAttribArray(i);

			if (!pSelectedFormat->m_hGeneric[i].m_nSize &&  pCurrentFormat->m_hGeneric[i].m_nSize)
				gl::DisableVertexAttribArray(i);
		}

		for (int i = 0; i < MAX_TEXCOORD_ATTRIB; i++)
		{
			if ((pSelectedFormat->m_hTexCoord[i].m_nSize > 0) ^ (pCurrentFormat->m_hTexCoord[i].m_nSize > 0))
			{
				gl::ClientActiveTexture(gl::TEXTURE0 + i);

				if (pSelectedFormat->m_hTexCoord[i].m_nSize > 0)
				{
					gl::EnableClientState(gl::TEXTURE_COORD_ARRAY);
				} 
				else 
				{
		            gl::DisableClientState(gl::TEXTURE_COORD_ARRAY);
				}
			}
		}

		m_pCurrentVertexFormat = pVertexFormat;

		GL_END_CRITICAL();
	}
}

// Changes the vertex buffer
void ShaderAPIGL::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer, int nStream, const intptr offset)
{
	CVertexBufferGL* pSelectedBuffer = (CVertexBufferGL*)pVertexBuffer;

	const GLsizei glTypes[] = {
		gl::FLOAT,
		gl::HALF_FLOAT_ARB,
		gl::UNSIGNED_BYTE,
	};

	GLuint vbo = 0;

	if (pSelectedBuffer != NULL)
	{
		vbo = pSelectedBuffer->m_nGL_VB_Index;
	}

	if( m_pCurrentVertexBuffers[nStream] != pVertexBuffer )
	{
		GL_CRITICAL();

		m_nCurrentVBO = vbo;
		gl::BindBuffer(gl::ARRAY_BUFFER, m_nCurrentVBO);

		GL_END_CRITICAL();
	}

	if (pSelectedBuffer != m_pCurrentVertexBuffers[nStream] || offset != m_nCurrentOffsets[nStream] || m_pCurrentVertexFormat != m_pActiveVertexFormat[nStream])
	{
		if (m_pCurrentVertexFormat != NULL)
		{
			GL_CRITICAL();

			char *base = (char *) offset;

			CVertexFormatGL* cvf = (CVertexFormatGL*)m_pCurrentVertexFormat;

			//int vertexSize = pSelectedBuffer ? pSelectedBuffer->GetStrideSize() : cvf->GetVertexSizePerStream(nStream);

			int vertexSize = cvf->m_nVertexSize[nStream];

			if (cvf->m_hVertex.m_nStream == nStream && cvf->m_hVertex.m_nSize)
			{
				gl::VertexPointer(cvf->m_hVertex.m_nSize, glTypes[cvf->m_hVertex.m_nFormat], vertexSize, base + cvf->m_hVertex.m_nOffset);
			}

			if (cvf->m_hNormal.m_nStream == nStream && cvf->m_hNormal.m_nSize)
			{
				gl::NormalPointer(glTypes[cvf->m_hNormal.m_nFormat], vertexSize, base + cvf->m_hNormal.m_nOffset);
			}

			for (int i = 0; i < MAX_GL_GENERIC_ATTRIB; i++)
			{
				if (cvf->m_hGeneric[i].m_nStream == nStream && cvf->m_hGeneric[i].m_nSize)
				{
					gl::VertexAttribPointer(i, cvf->m_hGeneric[i].m_nSize, glTypes[cvf->m_hGeneric[i].m_nFormat], gl::TRUE_, vertexSize, base + cvf->m_hGeneric[i].m_nOffset);
				}
			}

			for (int i = 0; i < MAX_TEXCOORD_ATTRIB; i++)
			{
				if (cvf->m_hTexCoord[i].m_nStream == nStream && cvf->m_hTexCoord[i].m_nSize)
				{
					gl::ClientActiveTexture(gl::TEXTURE0 + i);
					gl::TexCoordPointer(cvf->m_hTexCoord[i].m_nSize, glTypes[cvf->m_hTexCoord[i].m_nFormat], vertexSize, base + cvf->m_hTexCoord[i].m_nOffset);
				}
			}

		
			if (cvf->m_hColor.m_nStream == nStream && cvf->m_hColor.m_nSize)
			{
				gl::ColorPointer(cvf->m_hColor.m_nSize, glTypes[cvf->m_hColor.m_nFormat], vertexSize, base + cvf->m_hColor.m_nOffset);
			}
		
			GL_END_CRITICAL();
		}

		
	}

	if(pVertexBuffer)
	{
		// set bound stream
		((CVertexBufferGL*)pVertexBuffer)->m_boundStream = nStream;

		// unbind old
		if(m_pCurrentVertexBuffers[nStream])
		{
			((CVertexBufferGL*)m_pCurrentVertexBuffers[nStream])->m_boundStream = -1;
		}
	}

	m_pCurrentVertexBuffers[nStream] = pVertexBuffer;
	m_nCurrentOffsets[nStream] = offset;
	m_pActiveVertexFormat[nStream] = m_pCurrentVertexFormat;
}

// Changes the index buffer
void ShaderAPIGL::ChangeIndexBuffer(IIndexBuffer *pIndexBuffer)
{
	if (pIndexBuffer != m_pCurrentIndexBuffer)
	{
		GL_CRITICAL();

		if (pIndexBuffer == NULL)
		{
			gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, 0);
		}
		else 
		{
			CIndexBufferGL* pSelectedIndexBffer = (CIndexBufferGL*)pIndexBuffer;
			gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, pSelectedIndexBffer->m_nGL_IB_Index);
		}

		m_pCurrentIndexBuffer = pIndexBuffer;

		GL_END_CRITICAL();
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

	for(register int i = 0; i < m_ShaderList.numElem(); i++)
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

	CScopedMutex m(m_Mutex);

	// remove it if reference is zero
	if(pShader->Ref_Count() == 0)
	{
		// Cancel shader and destroy
		if(m_pCurrentShader == pShaderProgram)
		{
			Reset(STATE_RESET_SHADER);
			Apply();
		}

		m_ShaderList.remove(pShader);

		ThreadingSharingRequest();
		delete pShader;
		ThreadingSharingRelease();
	}
	else
		pShader->Ref_Drop(); // decrease references to this shader
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
bool ShaderAPIGL::CompileShadersFromStream(	IShaderProgram* pShaderOutput,const shaderprogram_params_t& params,const char* extra)
{
	if(!pShaderOutput)
		return false;

	if (!m_caps.shadersSupportedFlags)
	{
		MsgError("CompileShadersFromStream - shaders unsupported\n");
		return false;
	}

	if (params.pszVSText == NULL && params.pszPSText == NULL)
		return false;

	ThreadingSharingRequest();

	CGLShaderProgram* prog = (CGLShaderProgram*)pShaderOutput;
	GLint vsResult, fsResult, linkResult;

	// compile vertex
	if(params.pszVSText)
	{
		// create GL program
		prog->m_program = gl::CreateProgram();

		EqString shaderString;

#ifndef USE_OPENGL_ES
		//shaderString.Append("#version 120\r\n");
#endif // USE_OPENGL_ES

		if (extra  != NULL)
			shaderString.Append(extra);

		// append useful HLSL replacements
		shaderString.Append(SHADER_HELPERS_STRING);
		shaderString.Append(params.pszVSText);

		const char* sStr = shaderString.c_str();

		prog->m_vertexShader = gl::CreateShader(gl::VERTEX_SHADER);

		gl::ShaderSource(prog->m_vertexShader, 1, &sStr, NULL);
		gl::CompileShader(prog->m_vertexShader);
		gl::GetShaderiv(prog->m_vertexShader, gl::OBJECT_COMPILE_STATUS_ARB, &vsResult);

		if (vsResult)
		{
			gl::AttachShader(prog->m_program, prog->m_vertexShader);
		} 
		else 
		{
			char infoLog[2048];
			GLint len;

			gl::GetShaderInfoLog(prog->m_vertexShader, sizeof(infoLog), &len, infoLog);
			MsgError("Vertex shader '%s' error: %s\n", prog->GetName(), infoLog);
		}
	}
	else
		return false; // vertex shader is required

	// compile fragment
	if(params.pszPSText)
	{
		EqString shaderString;

#ifndef USE_OPENGL_ES
		//shaderString.Append("#version 120\r\n");
#endif // USE_OPENGL_ES

		if (extra  != NULL)
			shaderString.Append(extra);

		// append useful HLSL replacements
		shaderString.Append(SHADER_HELPERS_STRING);
		shaderString.Append(params.pszPSText);

		const char* sStr = shaderString.c_str();

		prog->m_fragmentShader = gl::CreateShader(gl::FRAGMENT_SHADER);

		gl::ShaderSource(prog->m_fragmentShader, 1, &sStr, NULL);
		gl::CompileShader(prog->m_fragmentShader);
		gl::GetShaderiv(prog->m_fragmentShader, gl::OBJECT_COMPILE_STATUS_ARB, &fsResult);

		if (vsResult)
		{
			gl::AttachShader(prog->m_program, prog->m_fragmentShader);
		} 
		else 
		{
			char infoLog[2048];
			GLint len;

			gl::GetShaderInfoLog(prog->m_vertexShader, sizeof(infoLog), &len, infoLog);
			MsgError("Pixel (fragment) shader '%s' error: %s\n", prog->GetName(), infoLog);
		}
	}
	else
		fsResult = gl::TRUE_;

	if(fsResult && vsResult)
	{
		for(int i = 0; i < params.pAPIPrefs->keys.numElem(); i++)
		{
			kvkeybase_t* kp = params.pAPIPrefs->keys[i];

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

				//Msg("Vertex attribute '%s' at %s\n", nameStr, locationStr);
				
				// bind attribute
				gl::BindAttribLocation(prog->m_program, attribIndex, nameStr);
			}
		}

		// link program and go
		gl::LinkProgram(prog->m_program);
		gl::GetProgramiv(prog->m_program, gl::OBJECT_LINK_STATUS_ARB, &linkResult);

		if( !linkResult )
		{
			char infoLog[2048];
			GLint len;

			gl::GetProgramInfoLog(prog->m_program, sizeof(infoLog), &len, infoLog);
			MsgError("Shader '%s' link error: %s\n", prog->GetName(), infoLog);
			return false;
		}

		// get current set program
		GLuint currProgram = (m_pCurrentShader == NULL)? 0 : ((CGLShaderProgram*)m_pCurrentShader)->m_program;

		// use freshly generated program to retirieve constants (uniforms) and samplers
		gl::UseProgram(prog->m_program);

		GLint uniformCount, maxLength;
		gl::GetProgramiv(prog->m_program, gl::OBJECT_ACTIVE_UNIFORMS_ARB, &uniformCount);
		gl::GetProgramiv(prog->m_program, gl::OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB, &maxLength);

		GLShaderSampler_t  *samplers = (GLShaderSampler_t  *) malloc(uniformCount * sizeof(GLShaderSampler_t));
		GLShaderConstant_t *uniforms = (GLShaderConstant_t *) malloc(uniformCount * sizeof(GLShaderConstant_t));

		//Msg("[SHADER] allocated %d samplers and uniforms\n", uniformCount);

		int nSamplers = 0;
		int nUniforms = 0;

		char* tmpName = new char[maxLength];

		for (int i = 0; i < uniformCount; i++)
		{
			GLenum type;
			GLint length, size;
			gl::GetActiveUniform(prog->m_program, i, maxLength, &length, &size, &type, tmpName);

			if (type >= gl::SAMPLER_1D && type <= gl::SAMPLER_2D_RECT_SHADOW_ARB)
			{
				GLShaderSampler_t* sp = &samplers[nSamplers];
				ASSERTMSG(sp, "WHAT?");

				// Assign samplers to image units
				GLint location = gl::GetUniformLocation(prog->m_program, tmpName);
				gl::Uniform1i(location, nSamplers);

				//Msg("[SHADER] retrieving sampler '%s' at %d (location = %d)\n", tmpName, nSamplers, location);

				sp->index = nSamplers;
				strcpy(sp->name, tmpName);
				nSamplers++;
			} 
			else 
			{
				// Store all non-gl uniforms
				if (strncmp(tmpName, "gl_", 3) != 0)
				{
					//Msg("[SHADER] retrieving uniform '%s' at %d\n", tmpName, nSamplers);

					char *bracket = strchr(tmpName, '[');
					if (bracket == NULL || (bracket[1] == '0' && bracket[2] == ']'))
					{
						if (bracket)
						{
							*bracket = '\0';
							length = (GLint) (bracket - tmpName);
						}

						uniforms[nUniforms].index = gl::GetUniformLocation(prog->m_program, tmpName);
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
		gl::UseProgram(currProgram);

		// Shorten arrays to actual count
		samplers = (GLShaderSampler_t  *) realloc(samplers, nSamplers * sizeof(GLShaderSampler_t));
		uniforms = (GLShaderConstant_t *) realloc(uniforms, nUniforms * sizeof(GLShaderConstant_t));
		qsort(samplers, nSamplers, sizeof(GLShaderSampler_t),  samplerComparator);
		qsort(uniforms, nUniforms, sizeof(GLShaderConstant_t), constantComparator);

		for (int i = 0; i < nUniforms; i++)
		{
			int constantSize = constantTypeSizes[uniforms[i].type] * uniforms[i].nElements;
			uniforms[i].data = new ubyte[constantSize];
			memset(uniforms[i].data, 0, constantSize);
			uniforms[i].dirty = false;
		}

		// finally assign
		prog->m_samplers = samplers;
		prog->m_constants = uniforms;

		prog->m_numSamplers = nSamplers;
		prog->m_numConstants = nUniforms;
	}
	else
		return false;

	ThreadingSharingRelease();

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

#define GL_NO_DEPRECATED_ATTRIBUTES

IVertexFormat* ShaderAPIGL::CreateVertexFormat(VertexFormatDesc_s *formatDesc, int nAttribs)
{
	CVertexFormatGL *pVertexFormat = new CVertexFormatGL();

	int nGeneric  = 0;
	int nTexCoord = 0;

#ifndef GL_NO_DEPRECATED_ATTRIBUTES
	// IT ALREADY DOES
	for (int i = 0; i < nAttribs; i++)
	{
		// Generic attribute 0 aliases with gl_Vertex
		if (formatDesc[i].m_nType == VERTEXTYPE_VERTEX)
		{
			nGeneric = 1;
			break;
		}
	}
#endif // GL_NO_DEPRECATED_ATTRIBUTES

	for (int i = 0; i < nAttribs; i++)
	{
		int stream = formatDesc[i].m_nStream;

		switch (formatDesc[i].m_nType)
		{
#ifndef GL_NO_DEPRECATED_ATTRIBUTES
			case VERTEXTYPE_NONE:
			case VERTEXTYPE_TANGENT:
			case VERTEXTYPE_BINORMAL:
				pVertexFormat->m_hGeneric[nGeneric].m_nStream = stream;
				pVertexFormat->m_hGeneric[nGeneric].m_nSize   = formatDesc[i].m_nSize;
				pVertexFormat->m_hGeneric[nGeneric].m_nOffset = pVertexFormat->m_nVertexSize[stream];
				pVertexFormat->m_hGeneric[nGeneric].m_nFormat = formatDesc[i].m_nFormat;
				nGeneric++;
				break;
			case VERTEXTYPE_VERTEX:
				pVertexFormat->m_hVertex.m_nStream = stream;
				pVertexFormat->m_hVertex.m_nSize   = formatDesc[i].m_nSize;
				pVertexFormat->m_hVertex.m_nOffset = pVertexFormat->m_nVertexSize[stream];
				pVertexFormat->m_hVertex.m_nFormat = formatDesc[i].m_nFormat;
				break;
			case VERTEXTYPE_NORMAL:
				pVertexFormat->m_hNormal.m_nStream = stream;
				pVertexFormat->m_hNormal.m_nSize   = formatDesc[i].m_nSize;
				pVertexFormat->m_hNormal.m_nOffset = pVertexFormat->m_nVertexSize[stream];
				pVertexFormat->m_hNormal.m_nFormat = formatDesc[i].m_nFormat;
				break;
			case VERTEXTYPE_TEXCOORD:
				pVertexFormat->m_hTexCoord[nTexCoord].m_nStream = stream;
				pVertexFormat->m_hTexCoord[nTexCoord].m_nSize   = formatDesc[i].m_nSize;
				pVertexFormat->m_hTexCoord[nTexCoord].m_nOffset = pVertexFormat->m_nVertexSize[stream];
				pVertexFormat->m_hTexCoord[nTexCoord].m_nFormat	= formatDesc[i].m_nFormat;
				nTexCoord++;
				break;
			case VERTEXTYPE_COLOR:
				pVertexFormat->m_hColor.m_nStream = stream;
				pVertexFormat->m_hColor.m_nSize   = formatDesc[i].m_nSize;
				pVertexFormat->m_hColor.m_nOffset = pVertexFormat->m_nVertexSize[stream];
				pVertexFormat->m_hColor.m_nFormat = formatDesc[i].m_nFormat;
				break;
#else
			case VERTEXTYPE_NONE:
			case VERTEXTYPE_TANGENT:
			case VERTEXTYPE_BINORMAL:
			case VERTEXTYPE_VERTEX:
			case VERTEXTYPE_NORMAL:
			case VERTEXTYPE_TEXCOORD:
			case VERTEXTYPE_COLOR:
				pVertexFormat->m_hGeneric[nGeneric].m_nStream = stream;
				pVertexFormat->m_hGeneric[nGeneric].m_nSize   = formatDesc[i].m_nSize;
				pVertexFormat->m_hGeneric[nGeneric].m_nOffset = pVertexFormat->m_nVertexSize[stream];
				pVertexFormat->m_hGeneric[nGeneric].m_nFormat = formatDesc[i].m_nFormat;
				nGeneric++;
				break;
#endif // GL_NO_DEPRECATED_ATTRIBUTES
		}
		
		pVertexFormat->m_nVertexSize[stream] += formatDesc[i].m_nSize * formatSize[formatDesc[i].m_nFormat];
	}

	pVertexFormat->m_nMaxGeneric = nGeneric;
	pVertexFormat->m_nMaxTexCoord = nTexCoord;

	m_VFList.append(pVertexFormat);

	return pVertexFormat;
}

IVertexBuffer* ShaderAPIGL::CreateVertexBuffer(BufferAccessType_e nBufAccess, int nNumVerts, int strideSize, void *pData )
{
	

	CVertexBufferGL* pGLVertexBuffer = new CVertexBufferGL();

	pGLVertexBuffer->m_numVerts = nNumVerts;
	pGLVertexBuffer->m_strideSize = strideSize;
	pGLVertexBuffer->m_usage = glBufferUsages[nBufAccess];

	DevMsg(3,"Creatting VBO with size %i KB\n", pGLVertexBuffer->GetSizeInBytes() / 1024);

	ThreadingSharingRequest();
	gl::GenBuffers(1, &pGLVertexBuffer->m_nGL_VB_Index);
	gl::BindBuffer(gl::ARRAY_BUFFER, pGLVertexBuffer->m_nGL_VB_Index);
	gl::BufferData(gl::ARRAY_BUFFER, pGLVertexBuffer->GetSizeInBytes(), pData, glBufferUsages[nBufAccess]);
	gl::BindBuffer(gl::ARRAY_BUFFER, 0);
	ThreadingSharingRelease();

	m_VBList.append( pGLVertexBuffer );

	return pGLVertexBuffer;
}

IIndexBuffer* ShaderAPIGL::CreateIndexBuffer(int nIndices, int nIndexSize, BufferAccessType_e nBufAccess, void *pData )
{
	CIndexBufferGL* pGLIndexBuffer = new CIndexBufferGL();

	pGLIndexBuffer->m_nIndices = nIndices;
	pGLIndexBuffer->m_nIndexSize = nIndexSize;
	pGLIndexBuffer->m_usage = glBufferUsages[nBufAccess];

	DevMsg(3,"Creatting IBO with size %i KB\n", (nIndices*nIndexSize) / 1024);

	int size = nIndices * nIndexSize;

	ThreadingSharingRequest();
	gl::GenBuffers(1, &pGLIndexBuffer->m_nGL_IB_Index);
	gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, pGLIndexBuffer->m_nGL_IB_Index);
	gl::BufferData(gl::ELEMENT_ARRAY_BUFFER, size, pData, glBufferUsages[nBufAccess]);
	gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, 0);
	ThreadingSharingRelease();

	m_IBList.append( pGLIndexBuffer );

	return pGLIndexBuffer;
}

// Destroy vertex format
void ShaderAPIGL::DestroyVertexFormat(IVertexFormat* pFormat)
{
	CVertexFormatGL* pVF = (CVertexFormatGL*)(pFormat);
	if (!pVF)
		return;

	CScopedMutex m(m_Mutex);

	// reset if in use
	if (m_pCurrentVertexFormat == pFormat)
	{
		Reset(STATE_RESET_VF);
		Apply();
	}

	m_VFList.remove(pFormat);
	delete pVF;
}

// Destroy vertex buffer
void ShaderAPIGL::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	CVertexBufferGL* pVB = (CVertexBufferGL*)(pVertexBuffer);
	if (!pVB)
		return;

	CScopedMutex m(m_Mutex);

	//Reset(STATE_RESET_VBO);
	//Apply();

	m_VBList.remove(pVertexBuffer);

	ThreadingSharingRequest();

	gl::DeleteBuffers(1, &pVB->m_nGL_VB_Index);

	ThreadingSharingRelease();

	delete pVB;
}

// Destroy index buffer
void ShaderAPIGL::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{	
	CIndexBufferGL* pIB = (CIndexBufferGL*)(pIndexBuffer);

	if (!pIB)
		return;

	CScopedMutex m(m_Mutex);

	//Reset(STATE_RESET_VBO);
	//Apply();

	m_IBList.remove(pIndexBuffer);

	ThreadingSharingRequest();

	gl::DeleteBuffers(1, &pIB->m_nGL_IB_Index);

	ThreadingSharingRelease();

	delete pIndexBuffer;
}

//-------------------------------------------------------------
// Primitive drawing
//-------------------------------------------------------------

IVertexFormat* pPlainFormat = NULL;

static CGLMeshBuilder s_MeshBuilder;

// Creates new mesh builder
IMeshBuilder* ShaderAPIGL::CreateMeshBuilder()
{
	return &s_MeshBuilder;//new D3D9MeshBuilder();
}

void ShaderAPIGL::DestroyMeshBuilder(IMeshBuilder* pBuilder)
{
	//delete pBuilder;
}

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
void ShaderAPIGL::DrawIndexedPrimitives(PrimitiveType_e nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
	ASSERT(m_pCurrentIndexBuffer != NULL);
	ASSERT(nVertices > 0);

	int nTris = g_pGLPrimCounterCallbacks[nType](nIndices);

	//m_pCurrentVertexFormat->GetVertexSizePerStream();
	uint indexSize = m_pCurrentIndexBuffer->GetIndexSize();

	GL_CRITICAL();
	gl::DrawElements(glPrimitiveType[nType], nIndices, indexSize == 2? gl::UNSIGNED_SHORT : gl::UNSIGNED_INT, BUFFER_OFFSET(indexSize * nFirstIndex));
	GL_END_CRITICAL();

	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

// Draw elements
void ShaderAPIGL::DrawNonIndexedPrimitives(PrimitiveType_e nType, int nFirstVertex, int nVertices)
{
	if(m_pCurrentVertexFormat == NULL)
		return;

	int nTris = g_pGLPrimCounterCallbacks[nType](nVertices);

	GL_CRITICAL();
	gl::DrawArrays(glPrimitiveType[nType], nFirstVertex, nVertices);
	GL_END_CRITICAL();

	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

// mesh buffer FFP emulation
void ShaderAPIGL::DrawMeshBufferPrimitives(PrimitiveType_e nType, int nVertices, int nIndices)
{
	if(m_pSelectedShader == NULL)
	{
		if(m_pCurrentTextures[0] == NULL)
			SetShader(m_pMeshBufferNoTextureShader);
		else
			SetShader(m_pMeshBufferTexturedShader);

		Matrix4x4 matrix = identity4() * m_matrices[MATRIXMODE_PROJECTION] * (m_matrices[MATRIXMODE_VIEW] * m_matrices[MATRIXMODE_WORLD]);

		SetShaderConstantMatrix4("WVP", matrix);
	}

	Apply();

	if(nIndices > 0)
		DrawIndexedPrimitives(nType, 0, nIndices, 0, nVertices);
	else
		DrawNonIndexedPrimitives(nType, 0, nVertices);
}

//-------------------------------------------------------------
// Fogging
//-------------------------------------------------------------

void ShaderAPIGL::SetupFog(FogInfo_t* fogparams)
{

}

bool ShaderAPIGL::IsFogEnabled()
{
	return false;
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
	}
}

// sets viewport
void ShaderAPIGL::SetViewport(int x, int y, int w, int h)
{
	m_viewPort = IRectangle(x,y, w,h);
	m_nViewportWidth = w;
	m_nViewportHeight = h;

	// TODO: set glviewport flipped by y
	gl::Viewport(x,y,w,h);
}

// returns viewport
void ShaderAPIGL::GetViewport(int &x, int &y, int &w, int &h)
{
	x = m_viewPort.vleftTop.x;
	y = m_viewPort.vleftTop.y;

	w = m_viewPort.vrightBottom.x;
	h = m_viewPort.vrightBottom.y;
}

// returns current size of viewport
void ShaderAPIGL::GetViewportDimensions(int &wide, int &tall)
{
	wide = m_viewPort.vrightBottom.x;
	tall = m_viewPort.vrightBottom.y;
}

// sets scissor rectangle
void ShaderAPIGL::SetScissorRectangle( const IRectangle &rect )
{
	//gl::SCISSOR_TEST
	//glScissor(
}

int ShaderAPIGL::GetSamplerUnit(CGLShaderProgram* prog, const char* samplerName)
{
	if(!prog)
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
void ShaderAPIGL::ThreadingSharingRequest()
{
	uintptr_t currThread = Threading::GetCurrentThreadID();

	if(m_mainThreadId == currThread)
		return;

	if(m_isSharing)
	{
		// other thread must wait quite some time
		m_busySignal.Wait();
	}

	if( m_isMainAtCritical )
		m_busySignal.Wait();

	m_currThreadId = currThread;

	m_isSharing = true;

#ifdef _WIN32
	wglMakeCurrent(m_hdc, m_glContext2);
#elif LINUX

#elif __APPLE__

#endif

	// don't fuck with the mutex
	m_busySignal.Clear();
}

// GL_CRITICAL is made for things that are called from main thread
bool ShaderAPIGL::GL_CRITICAL()
{
	uintptr_t currThread = Threading::GetCurrentThreadID();

	if(m_currThreadId == currThread)
		return false;

	// spin
	if(m_isSharing && m_mainThreadId == m_currThreadId)
	{
		//wglMakeCurrent(NULL, NULL);	// drop device here
		m_busySignal.Wait();
	}
		
	m_currThreadId = currThread;

#ifdef _WIN32
	wglMakeCurrent(m_hdc, m_glContext);
#elif LINUX

#elif __APPLE__

#endif

	m_isMainAtCritical = true;
	m_busySignal.Clear();

	return true;
}

// GL_END_CRITICAL releases main thread and lets other threads to take control over gl context
void ShaderAPIGL::GL_END_CRITICAL()
{
	if(!m_isMainAtCritical)
		return;

	m_isMainAtCritical = false;
	m_busySignal.Raise();

/*
	if(!m_isMainAtCritical)
		return;

	m_isMainAtCritical = false;
	m_busySignal.Raise();
	*/
}

// Release context sharing situation
void ShaderAPIGL::ThreadingSharingRelease()
{
	if( m_isMainAtCritical )
		m_busySignal.Wait();

	if(!m_isSharing)
		return;

	m_isSharing = false;

	// make sure it's fully done
	gl::Finish();

#ifdef _WIN32
	wglMakeCurrent(NULL, NULL);
#elif LINUX

#elif __APPLE__

#endif

	m_busySignal.Raise();
}