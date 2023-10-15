//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Direct3D 9 ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"
#include "core/IFileSystem.h"
#include "imaging/ImageLoader.h"
#include "utils/KeyValues.h"

#include "shaderapid3d9_def.h"

#include "ShaderAPID3D9.h"
#include "D3D9Texture.h"
#include "D3D9VertexFormat.h"
#include "D3D9VertexBuffer.h"
#include "D3D9IndexBuffer.h"
#include "D3D9ShaderProgram.h"
#include "D3D9OcclusionQuery.h"
#include "D3D9RenderState.h"

#define SHADERCACHE_FOLDER		"ShaderCache/D3D9"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;
extern CEqMutex	g_sapi_ShaderMutex;
extern CEqMutex	g_sapi_VBMutex;
extern CEqMutex	g_sapi_IBMutex;
extern CEqMutex	g_sapi_Mutex;

extern CEqMutex	g_sapi_ProgressiveTextureMutex;

DECLARE_CVAR(r_preloadShaderCache, "1", nullptr, 0);
DECLARE_CVAR(r_skipShaderCache, "0", "Shader debugging purposes", 0);

#pragma warning(disable:4838)

ShaderAPID3D9::~ShaderAPID3D9()
{
	
}

ShaderAPID3D9::ShaderAPID3D9() : ShaderAPI_Base()
{
	Msg("Initializing Direct3D9 Shader API...\n");

	for(int i = 0; i < MAX_VERTEXSTREAM; i++)
		m_nSelectedStreamParam[i] = 1;

	m_defaultSamplerState.magFilter = TEXFILTER_NEAREST;
	m_defaultSamplerState.minFilter = TEXFILTER_NEAREST;
	//m_defaultSamplerState.mipFilter = TEXFILTER_NEAREST;
	m_defaultSamplerState.wrapS = TEXADDRESS_WRAP;
	m_defaultSamplerState.wrapT = TEXADDRESS_WRAP;
	m_defaultSamplerState.wrapR = TEXADDRESS_WRAP;
	m_defaultSamplerState.aniso = 1;

	for (int i = 0; i < MAX_SAMPLERSTATE; i++)
		m_pCurrentSamplerStates[i] = m_defaultSamplerState;

	for (int i = 0; i < MAX_SAMPLERSTATE; i++)
		m_pCurrentVertexSamplerStates[i] = m_defaultSamplerState;
}

void ShaderAPID3D9::SetD3DDevice(LPDIRECT3DDEVICE9 d3ddev, D3DCAPS9 &d3dcaps)
{
	m_pD3DDevice = d3ddev;
	m_hCaps = d3dcaps;
}

//-----------------------------------------------------------------------------
// Check for device lost
//-----------------------------------------------------------------------------

void ShaderAPID3D9::OnDeviceLost()
{
	if (m_deviceIsLost)
		return;

	m_deviceIsLost = true;

	if (m_pEventQuery)
		m_pEventQuery->Release();
	m_pEventQuery = nullptr;

	// release back buffer and depth first
	ReleaseD3DFrameBufferSurfaces();

	{
		CScopedMutex scoped(g_sapi_VBMutex);
		for (int i = 0; i < m_VBList.numElem(); i++)
		{
			CD3D9VertexBuffer* pVB = (CD3D9VertexBuffer*)m_VBList[i];
			pVB->ReleaseForRestoration();
		}
	}

	{
		CScopedMutex scoped(g_sapi_IBMutex);
		for (int i = 0; i < m_IBList.numElem(); i++)
		{
			CD3D9IndexBuffer* pIB = (CD3D9IndexBuffer*)m_IBList[i];
			pIB->ReleaseForRestoration();
		}
	}

	for (int i = 0; i < m_OcclusionQueryList.numElem(); i++)
	{
		CD3D9OcclusionQuery* query = (CD3D9OcclusionQuery*)m_OcclusionQueryList[i];
		query->Destroy();
	}

	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		// relesase texture surfaces
		for (auto it = m_TextureList.begin(); !it.atEnd(); ++it)
		{
			CD3D9Texture* pTex = (CD3D9Texture*)*it;

			const bool isManaged = (pTex->m_pool == D3DPOOL_MANAGED);
			if (!isManaged)
			{
				if (pTex->GetFlags() & TEXFLAG_RENDERTARGET)
				{
					// don't care - we'll recreate it
					pTex->Release();
				}
				else
				{
					DevMsg(DEVMSG_RENDER, "RESET: releasing %s\n", pTex->GetName());
					pTex->ReleaseForRestoration();
				}
			}
		}
	}

	DevMsg(DEVMSG_RENDER, "Device objects releasing done\n");
}

bool ShaderAPID3D9::ResetDevice(D3DPRESENT_PARAMETERS& d3dpp)
{
	ASSERT_MSG(m_deviceIsLost, "To gracefully reset device use OnDeviceLost");

	if (m_deviceAtReset)
		return true;
	m_deviceAtReset = true;

	HRESULT hr;

	m_pD3DDevice->EvictManagedResources();

	// Reset the device before restoring everything
	if (FAILED(hr = m_pD3DDevice->Reset(&d3dpp)))
	{
		if (hr == D3DERR_DEVICELOST)
		{
			m_deviceIsLost = true;
			m_deviceAtReset = false;
		}
		else if (hr == D3DERR_INVALIDCALL)
		{
			MsgError("D3DERR_INVALIDCALL happened while Reset\n");
			m_deviceIsLost = true;
			m_deviceAtReset = false;
		}
		else
			ASSERT_FAIL("Reset failed (%d)\n", hr);

		return false;
	}

	return true;
}

bool ShaderAPID3D9::RestoreDevice()
{
	if (!m_deviceAtReset)
		return true;

	m_deviceAtReset = false; 
	m_deviceIsLost = false;

	DevMsg(DEVMSG_RENDER, "Restoring device...\n");

	// reset all the stuff so we could re-apply
	Reset();

	m_pCurrentBlendstate = nullptr;
	m_pCurrentDepthState = nullptr;
	m_pCurrentRasterizerState = nullptr;
	m_pCurrentShader = nullptr;
	m_pCurrentVertexFormat = nullptr;
	m_pCurrentIndexBuffer = nullptr;

	for (int i = 0; i < MAX_TEXTUREUNIT; ++i)
	{
		m_pCurrentTextures[i] = nullptr;
		m_pCurrentVertexTextures[i] = nullptr;
	}

	memset(m_vsRegs, 0, sizeof(m_vsRegs));
	memset(m_psRegs, 0, sizeof(m_psRegs));
	memset(m_pCurrentSamplerStates, 0, sizeof(m_pCurrentSamplerStates));

	m_nCurrentSamplerStateDirty = -1;
	m_nCurrentVertexSamplerStateDirty = -1;

	for (int i = 0; i < MAX_VERTEXSTREAM; i++)
		m_nSelectedStreamParam[i] = 1;

	memset(m_pCurrentVertexBuffers, 0, sizeof(m_pCurrentVertexBuffers));
	memset(m_pActiveVertexFormat, 0, sizeof(m_pActiveVertexFormat));
	memset(m_nCurrentOffsets, 0, sizeof(m_nCurrentOffsets));

	m_nCurrentSrcFactor = BLENDFACTOR_ONE;
	m_nCurrentDstFactor = BLENDFACTOR_ZERO;
	m_nCurrentBlendMode = BLENDFUNC_ADD;
	m_pCurrentDepthSurface = nullptr;

	m_nCurrentDepthFunc = COMPFUNC_LEQUAL;
	m_bCurrentDepthTestEnable = false;
	m_bCurrentDepthWriteEnable = false;

	m_bCurrentMultiSampleEnable = false;
	m_bCurrentScissorEnable = false;
	m_nCurrentCullMode = CULL_BACK;
	m_nCurrentFillMode = FILL_SOLID;

	m_nCurrentMask = COLORMASK_ALL;
	m_bCurrentBlendEnable = false;

	// Set some of my preferred defaults
	m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	m_pD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pEventQuery);

	DevMsg(DEVMSG_RENDER, "Restoring VBs...\n");
	for (int i = 0; i < m_VBList.numElem(); i++)
	{
		CD3D9VertexBuffer* pVB = (CD3D9VertexBuffer*)m_VBList[i];
		pVB->Restore();
	}

	DevMsg(DEVMSG_RENDER, "Restoring IBs...\n");
	for (int i = 0; i < m_IBList.numElem(); i++)
	{
		CD3D9IndexBuffer* pIB = (CD3D9IndexBuffer*)m_IBList[i];
		pIB->Restore();
	}

	DevMsg(DEVMSG_RENDER, "Restoring query...\n");
	for (int i = 0; i < m_OcclusionQueryList.numElem(); i++)
	{
		CD3D9OcclusionQuery* query = (CD3D9OcclusionQuery*)m_OcclusionQueryList[i];
		query->Init();
	}

	DevMsg(DEVMSG_RENDER, "Restoring RTs...\n");

	// create texture surfaces
	for (auto it = m_TextureList.begin(); !it.atEnd(); ++it)
	{
		CD3D9Texture* pTex = (CD3D9Texture*)*it;

		// skip hardcoded
		if (pTex == m_fbColorTexture || pTex == m_fbDepthTexture)
			continue;

		const bool isManaged = (pTex->m_pool == D3DPOOL_MANAGED);

		// restore unmanaged texture
		if (!isManaged)
		{
			if (pTex->GetFlags() & TEXFLAG_RENDERTARGET)
			{
				DevMsg(DEVMSG_RENDER, "Restoring rentertarget %s\n", pTex->GetName());
				InternalCreateRenderTarget(m_pD3DDevice, pTex, pTex->GetFlags(), m_caps);
			}
			else
			{
				DevMsg(DEVMSG_RENDER, "Restoring texture %s\n", pTex->GetName());
				pTex->Restore();
			}
		}
	}

	DevMsg(DEVMSG_RENDER, "Restoring backbuffer...\n");

	CreateD3DFrameBufferSurfaces();
	ChangeRenderTargetToBackBuffer();

	// apply everything back
	Apply();

	return true;
}

bool ShaderAPID3D9::CreateD3DFrameBufferSurfaces()
{
	if(!m_fbColorTexture)
	{
		m_fbColorTexture = PPNew CD3D9Texture();
		m_fbColorTexture->SetName("rhi_fb_color");
		m_fbColorTexture->SetDimensions(0, 0);
		m_fbColorTexture->SetFlags(TEXFLAG_RENDERTARGET | TEXFLAG_NOQUALITYLOD);
		m_fbColorTexture->Ref_Grab();

		CScopedMutex m(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(m_fbColorTexture);
		m_TextureList.insert(m_fbColorTexture->m_nameHash, m_fbColorTexture);
	}

	if (!m_fbDepthTexture)
	{
		m_fbDepthTexture = PPNew CD3D9Texture();
		m_fbDepthTexture->SetName("rhi_fb_depth");
		m_fbDepthTexture->SetDimensions(0, 0);
		m_fbDepthTexture->SetFlags(TEXFLAG_RENDERDEPTH | TEXFLAG_NOQUALITYLOD);
		m_fbDepthTexture->Ref_Grab();

		CScopedMutex m(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(m_fbDepthTexture);
		m_TextureList.insert(m_fbDepthTexture->m_nameHash, m_fbDepthTexture);
	}

	IDirect3DSurface9* fbColorSurface;
	if (m_pD3DDevice->GetRenderTarget(0, &fbColorSurface) != D3D_OK)
		return false;

	m_fbColorTexture->m_surfaces.setNum(1);
	m_fbColorTexture->m_surfaces[0] = fbColorSurface;

	IDirect3DSurface9* fbDepthSurface;
	if (m_pD3DDevice->GetDepthStencilSurface(&fbDepthSurface) != D3D_OK)
		return false;

	m_fbDepthTexture->m_surfaces.setNum(1);
	m_fbDepthTexture->m_surfaces[0] = fbDepthSurface;

	return true;
}

void ShaderAPID3D9::ReleaseD3DFrameBufferSurfaces()
{
	if (m_fbColorTexture)
	{
		m_fbColorTexture->m_surfaces[0]->Release();
		m_fbColorTexture->m_surfaces.clear();
	}

	if (m_fbDepthTexture)
	{
		m_fbDepthTexture->m_surfaces[0]->Release();
		m_fbDepthTexture->m_surfaces.clear();
	}
}

// Init + Shurdown
void ShaderAPID3D9::Init( const ShaderAPIParams &params )
{
	HOOK_TO_CVAR(r_anisotropic);
	const int desiredAnisotropicLevel = min(r_anisotropic->GetInt(), m_caps.maxTextureAnisotropicLevel);

	// set the anisotropic level
	if (desiredAnisotropicLevel > 1)
	{
		for (int i = 0; i < m_caps.maxTextureUnits; i++)
			m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, desiredAnisotropicLevel);
	}

	m_caps.maxTextureAnisotropicLevel = desiredAnisotropicLevel;

	const char *vsprofile = D3DXGetVertexShaderProfile(m_pD3DDevice);
	const char *psprofile = D3DXGetPixelShaderProfile(m_pD3DDevice);

	MsgAccept(" \n*Max pixel shader profile: %s\n*Max vertex shader profile: %s\n",psprofile,vsprofile);

	g_fileSystem->MakeDir(SHADERCACHE_FOLDER, SP_ROOT);
	PreloadShadersFromCache();

	ShaderAPI_Base::Init(params);
}

void ShaderAPID3D9::PrintAPIInfo() const
{
	Msg("ShaderAPI: ShaderAPID3DX9\n");
	Msg("Direct3D 9 SDK version: %d\n \n", D3D_SDK_VERSION);

	const char *vsprofile = D3DXGetVertexShaderProfile(m_pD3DDevice);
	const char *psprofile = D3DXGetPixelShaderProfile(m_pD3DDevice);

	Msg("Max pixel shader profile: %s\n*Max vertex shader profile: %s\n",psprofile,vsprofile);

	uint32 tex_memory = m_pD3DDevice->GetAvailableTextureMem();

	Msg("  Available texture/mesh memory: %d mb\n", (tex_memory / 1024) / 1024);

	Msg("  Maximum FFP lights: %d\n", m_hCaps.MaxActiveLights);
	Msg("  Maximum Anisotropy: %d\n", m_hCaps.MaxAnisotropy);
	Msg("  Maximum NPatch tesselation level: %f\n", m_hCaps.MaxNpatchTessellationLevel);
	Msg("  Maximum Pixel Shader 3 instruction slots: %d\n", m_hCaps.MaxPixelShader30InstructionSlots);
	Msg("  Maximum point size: %f\n", m_hCaps.MaxPointSize);
	Msg("  Maximum primitives per DrawPrimitive call: %d\n", m_hCaps.MaxPrimitiveCount);
	Msg("  Maximum pixel shader executed instructions: %d\n", m_hCaps.MaxPShaderInstructionsExecuted);
	Msg("  Maximum vertex shader executed instructions: %d\n", m_hCaps.MaxVShaderInstructionsExecuted);
	Msg("  Maximum drawable textures: %d\n", m_caps.maxTextureUnits);
	Msg("  Maximum VBO streams per draw: %d\n", m_hCaps.MaxStreams);
	Msg("  Maximum VBO stream vertex stride size: %d\n", m_hCaps.MaxStreamStride);
	Msg("  Maximum texture size: %d x %d\n", m_hCaps.MaxTextureWidth, m_hCaps.MaxTextureHeight);
	Msg("  Maximum vertex index: %d\n", m_hCaps.MaxVertexIndex);

	MsgInfo("------ Loaded textures ------");

	uint64 allTexturesSize = 0;

	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		for (auto it = m_TextureList.begin(); !it.atEnd(); ++it)
		{
			CD3D9Texture* pTexture = (CD3D9Texture*)*it;

			ETextureFormat texFmt = pTexture->GetFormat();

			float textureSize = 0;

			if (IsCompressedFormat(texFmt))
				textureSize = pTexture->m_texSize;
			else
				textureSize = pTexture->GetWidth() * pTexture->GetHeight() * pTexture->GetMipCount() * GetBytesPerPixel(texFmt);

			allTexturesSize += textureSize / 1024;

			MsgInfo("     %s (%d) - %dx%d (~%.2f kb)\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(), pTexture->GetHeight(), (textureSize / 1024.0f));
		}
	}

	Msg("Texture memory: %.2f MB\n", ((float)allTexturesSize / 1024.0f));

	int allBuffersSize = 0;

	// get vb's size
	for(int i = 0; i < m_VBList.numElem(); i++)
		allBuffersSize += (float)m_VBList[i]->GetSizeInBytes() / 1024.0f;

	// get ib's size
	for(int i = 0; i < m_IBList.numElem(); i++)
		allBuffersSize += (float)(m_IBList[i]->GetIndicesCount() * m_IBList[i]->GetIndexSize()) / 1024.0f;

	Msg("VBO memory: %.2f MB\n", ((float)allBuffersSize / 1024.0f));

	Msg("TOTAL USAGE: %g MB\n", ((float)(allTexturesSize+allBuffersSize) / 1024.0f));
}

void ShaderAPID3D9::SetViewport(int x, int y, int w, int h)
{
	D3DVIEWPORT9 vp = { x, y, w, h, 0.0f, 1.0f };
	m_pD3DDevice->SetViewport(&vp);
}

bool ShaderAPID3D9::IsDeviceActive() const
{
	return !m_deviceAtReset;
}

void ShaderAPID3D9::Shutdown()
{
	ReleaseD3DFrameBufferSurfaces();

	if(m_fbColorTexture)
		m_fbColorTexture->Ref_Drop();

	if(m_fbDepthTexture)
		m_fbDepthTexture->Ref_Drop();

	ShaderAPI_Base::Shutdown();

	if(m_pEventQuery)
		m_pEventQuery->Release();

	m_pEventQuery = nullptr;
}

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------
void ShaderAPID3D9::Reset(int nResetType)
{
	ShaderAPI_Base::Reset(nResetType);

	if(nResetType & STATE_RESET_SHADERCONST)
	{
		memset(m_vsRegs,0,sizeof(m_vsRegs));
		memset(m_psRegs,0,sizeof(m_psRegs));

		m_nMinVSDirty = 256;
		m_nMaxVSDirty = -1;
		m_nMinPSDirty = 224;
		m_nMaxPSDirty = -1;
	}
}

void ShaderAPID3D9::ApplyTextures()
{
	for(int i = 0; i < MAX_TEXTUREUNIT; i++)
	{
		CD3D9Texture* pTexture = (CD3D9Texture*)m_pSelectedTextures[i].Ptr();
		if (pTexture != m_pCurrentTextures[i])
		{
			if (pTexture == nullptr)
			{
				m_pD3DDevice->SetTexture(i, nullptr);
				m_pSelectedSamplerStates[i] = nullptr;
			} 
			else 
			{
#ifdef EQ_DEBUG
				if (pTexture->m_textures.numElem() == 0)
				{
					ASSERT_FAIL("D3D9 renderer error: texture has no surfaces\n");
				}
#endif

				m_pD3DDevice->SetTexture(i, pTexture->GetCurrentTexture());
				m_pSelectedSamplerStates[i] = (SamplerStateParams*)&pTexture->GetSamplerState();

				// changed texture means changed sampler state
				m_nCurrentSamplerStateDirty |= (1 << i);
			}

			m_pCurrentTextures[i] = ITexturePtr(pTexture);
		}
	}

	for(int i = 0; i < m_caps.maxVertexTextureUnits; i++)
	{
		CD3D9Texture* pTexture = (CD3D9Texture*)m_pSelectedVertexTextures[i].Ptr();
		if (pTexture != m_pCurrentVertexTextures[i])
		{
			if (pTexture == nullptr)
			{
				m_pD3DDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+i, nullptr);
				m_pSelectedVertexSamplerStates[i] = nullptr;
			} 
			else 
			{
				m_pD3DDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+i, pTexture->GetCurrentTexture());
				m_pSelectedVertexSamplerStates[i] = (SamplerStateParams*)&pTexture->GetSamplerState();

				// changed texture means changed sampler state
				m_nCurrentVertexSamplerStateDirty |= (1 << i);
			}

			m_pCurrentVertexTextures[i] = ITexturePtr(pTexture);
		}
	}
}

void ShaderAPID3D9::ApplySamplerState()
{
	for (int i = 0; i < m_caps.maxTextureUnits; i++)
	{
		SamplerStateParams* pSelectedSamplerState = m_pSelectedSamplerStates[i];

		if (m_nCurrentSamplerStateDirty & (1 << i))
		{
			SamplerStateParams &ss = pSelectedSamplerState ? *pSelectedSamplerState : m_defaultSamplerState;
			SamplerStateParams &css = m_pCurrentSamplerStates[i];

			if (ss.minFilter != css.minFilter)
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MINFILTER, g_d3d9_texFilterType[css.minFilter = ss.minFilter]);

			if (ss.magFilter != css.magFilter)
			{
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, g_d3d9_texFilterType[css.magFilter = ss.magFilter]);
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, g_d3d9_texFilterType[ss.magFilter]);	// FIXME: separate selector for MIP?
			}

			if (ss.wrapS != css.wrapS) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSU, g_d3d9_texAddressMode[css.wrapS = ss.wrapS]);
			if (ss.wrapT != css.wrapT) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSV, g_d3d9_texAddressMode[css.wrapT = ss.wrapT]);
			if (ss.wrapR != css.wrapR) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSW, g_d3d9_texAddressMode[css.wrapR = ss.wrapR]);

			if (ss.aniso != css.aniso) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, css.aniso = ss.aniso);

			if (ss.lod != css.lod) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *(DWORD *) &(css.lod = ss.lod));
		}
	}
	m_nCurrentSamplerStateDirty = 0;

	// Vertex texture samplers
	for (int i = 0; i < m_caps.maxVertexTextureUnits; i++)
	{
		SamplerStateParams* pSelectedSamplerState = m_pSelectedVertexSamplerStates[i];

		if (m_nCurrentVertexSamplerStateDirty & (1 << i))
		{
			SamplerStateParams &ss = pSelectedSamplerState ? *pSelectedSamplerState : m_defaultSamplerState;
			SamplerStateParams &css = m_pCurrentVertexSamplerStates[i];

			if (ss.minFilter != css.minFilter)
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MINFILTER, g_d3d9_texFilterType[css.minFilter = ss.minFilter]);

			if (ss.magFilter != css.magFilter)
			{
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, g_d3d9_texFilterType[css.magFilter = ss.magFilter]);
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, g_d3d9_texFilterType[ss.magFilter]);	// FIXME: separate selector for MIP?
			}

			if (ss.wrapS != css.wrapS) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSU, g_d3d9_texAddressMode[css.wrapS = ss.wrapS]);
			if (ss.wrapT != css.wrapT) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSV, g_d3d9_texAddressMode[css.wrapT = ss.wrapT]);
			if (ss.wrapR != css.wrapR) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSW, g_d3d9_texAddressMode[css.wrapR = ss.wrapR]);

			if (ss.aniso != css.aniso) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, css.aniso = ss.aniso);

			if (ss.lod != css.lod) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&(css.lod = ss.lod));
		}
	}
	m_nCurrentVertexSamplerStateDirty = 0;
}

void ShaderAPID3D9::ApplyBlendState()
{
	CD3D9BlendingState* pSelectedState = (CD3D9BlendingState*)m_pSelectedBlendstate;

	int mask = COLORMASK_ALL;
	bool blendingEnabled = pSelectedState != nullptr && pSelectedState->m_params.blendEnable;

	// switch the blending on/off
	if (m_bCurrentBlendEnable != blendingEnabled)
	{
		m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD)blendingEnabled);
		m_bCurrentBlendEnable = blendingEnabled;
	}

	if(pSelectedState != nullptr)
	{
		BlendStateParams& state = pSelectedState->m_params;

		// handle blending params if blending is enabled
		if (state.blendEnable)
		{
			if (state.srcFactor != m_nCurrentSrcFactor)
			{
				m_nCurrentSrcFactor = state.srcFactor;
				m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, g_d3d9_blendingConsts[state.srcFactor]);
			}

			if (state.dstFactor != m_nCurrentDstFactor)
			{
				m_nCurrentDstFactor = state.dstFactor;
				m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, g_d3d9_blendingConsts[state.dstFactor]);
			}
			if (state.blendFunc != m_nCurrentBlendMode)
			{
				m_nCurrentBlendMode = state.blendFunc;
				m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, g_d3d9_blendingModes[state.blendFunc]);
			}
		}

		mask = state.mask;
	}

	// change the mask
	if (mask != m_nCurrentMask)
	{
		m_nCurrentMask = mask;

		// FIXME: use all MRTs feature, not just global value
		m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, mask);
		m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, mask);
		m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, mask);
		m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, mask);
	}
	/*
	if (m_nSelectedSampleMask != m_nCurrentSampleMask)
	{
		m_pD3DDevice->SetRenderState(D3DRS_MULTISAMPLEMASK, m_nSelectedSampleMask);
		m_nCurrentSampleMask = m_nSelectedSampleMask;
	}
	*/
	
	// state was set up
	m_pCurrentBlendstate = pSelectedState;
}

void ShaderAPID3D9::ApplyDepthState()
{
	CD3D9DepthStencilState* pSelectedState = (CD3D9DepthStencilState*)m_pSelectedDepthState;

	if (pSelectedState == nullptr)
	{
		if (!m_bCurrentDepthTestEnable)
		{
			m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, m_bCurrentDepthTestEnable = true);
		}

		if (!m_bCurrentDepthWriteEnable)
		{
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, m_bCurrentDepthWriteEnable = true);
		}

		if (m_nCurrentDepthFunc != COMPFUNC_LESS)
			m_pD3DDevice->SetRenderState(D3DRS_ZFUNC, g_d3d9_depthConst[m_nCurrentDepthFunc = COMPFUNC_LESS]);

		if (m_bDoStencilTest != false)
			m_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, m_bDoStencilTest = false);
	} 
	else 
	{
		DepthStencilStateParams& state = pSelectedState->m_params;

		if (state.depthTest)
		{
			if (!m_bCurrentDepthTestEnable)
			{
				m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
				m_bCurrentDepthTestEnable = true;
			}

			if (state.depthWrite != m_bCurrentDepthWriteEnable)
				m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, (m_bCurrentDepthWriteEnable = state.depthWrite)? TRUE : FALSE);

			if (state.depthFunc != m_nCurrentDepthFunc)
				m_pD3DDevice->SetRenderState(D3DRS_ZFUNC, g_d3d9_depthConst[m_nCurrentDepthFunc = state.depthFunc]);
		
		} 
		else 
		{
			if (m_bCurrentDepthTestEnable)
			{
				m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, m_bCurrentDepthTestEnable = false);
			}
		}

		if(state.doStencilTest != m_bDoStencilTest)
		{
			m_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, m_bDoStencilTest = state.doStencilTest);

			if(state.doStencilTest)
			{
				if(m_nStencilMask != state.nStencilMask)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILMASK, m_nStencilMask = state.nStencilMask);

				if(m_nStencilWriteMask != state.nStencilWriteMask)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILREF, m_nStencilWriteMask = state.nStencilWriteMask);

				if(m_nStencilRef != state.nStencilRef)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILREF, m_nStencilRef = state.nStencilRef);

				if(m_nStencilFunc != state.nStencilFunc)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILFUNC, g_d3d9_stencilConst[m_nStencilFunc = state.nStencilFunc]);

				if(m_nStencilFail != state.nStencilFail)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILFAIL, g_d3d9_stencilConst[m_nStencilFail = state.nStencilFail]);

				if(m_nStencilFunc != state.nStencilFunc)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILREF, g_d3d9_depthConst[m_nStencilFunc = state.nStencilFunc]);

				if(m_nStencilPass != state.nStencilPass)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILPASS, g_d3d9_stencilConst[m_nStencilPass = state.nStencilPass]);

				if(m_nDepthFail != state.nDepthFail)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILZFAIL, g_d3d9_stencilConst[m_nDepthFail = state.nDepthFail]);
			}
		}
	}

	m_pCurrentDepthState = pSelectedState;
}

void ShaderAPID3D9::ApplyRasterizerState()
{
	CD3D9RasterizerState* pSelectedState = (CD3D9RasterizerState*)m_pSelectedRasterizerState;

	if (pSelectedState == nullptr)
	{
		if (m_nCurrentCullMode != CULL_BACK)
			m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, g_d3d9_cullConst[m_nCurrentCullMode = CULL_BACK]);

		if (m_nCurrentFillMode != FILL_SOLID)
			m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, g_d3d9_fillConst[m_nCurrentFillMode = FILL_SOLID]);

		if (m_bCurrentMultiSampleEnable != true)
			m_pD3DDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, m_bCurrentMultiSampleEnable = true);
		

		if (m_bCurrentScissorEnable != false)
			m_pD3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, m_bCurrentScissorEnable = false);

		if(m_fCurrentDepthBias != 0.0f)
		{
			m_pD3DDevice->SetRenderState( D3DRS_DEPTHBIAS, 0 ); 
			m_fCurrentDepthBias = 0.0f;
		}

		if(m_fCurrentSlopeDepthBias != 0.0f)
		{
			m_pD3DDevice->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, 0 );
			m_fCurrentSlopeDepthBias = 0.0f;
		}
	}
	else
	{
		RasterizerStateParams& state = pSelectedState->m_params;

		if (state.cullMode != m_nCurrentCullMode)
			m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, g_d3d9_cullConst[m_nCurrentCullMode = state.cullMode]);

		if (state.fillMode != m_nCurrentFillMode)
			m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, g_d3d9_fillConst[m_nCurrentFillMode = state.fillMode]);

		if (state.multiSample != m_bCurrentMultiSampleEnable)
			m_pD3DDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, m_bCurrentMultiSampleEnable = state.multiSample);
		
		if (state.scissor != m_bCurrentScissorEnable)
		{
			m_bCurrentScissorEnable = state.scissor;
			m_pD3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, m_bCurrentScissorEnable );
		}

		if (state.useDepthBias != false)
		{
			if(m_fCurrentDepthBias != state.depthBias)
				m_pD3DDevice->SetRenderState( D3DRS_DEPTHBIAS, *((DWORD*) (&(m_fCurrentDepthBias = state.depthBias)) ));

			if(m_fCurrentSlopeDepthBias != state.slopeDepthBias)
				m_pD3DDevice->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, *((DWORD*) (&(m_fCurrentSlopeDepthBias = state.slopeDepthBias)))); 
		}
		else
		{
			if(m_fCurrentDepthBias != 0.0f)
			{
				m_pD3DDevice->SetRenderState( D3DRS_DEPTHBIAS, 0 ); 
				m_fCurrentDepthBias = 0.0f;
			}

			if(m_fCurrentSlopeDepthBias != 0.0f)
			{
				m_pD3DDevice->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, 0 );
				m_fCurrentSlopeDepthBias = 0.0f;
			}
		}
	}

	m_pCurrentRasterizerState = pSelectedState;
}

void ShaderAPID3D9::ApplyShaderProgram()
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)m_pSelectedShader;

	if (pShader != m_pCurrentShader)
	{
		if (pShader == nullptr)
		{
			m_pD3DDevice->SetVertexShader(nullptr);
			m_pD3DDevice->SetPixelShader(nullptr);
		} 
		else 
		{
			m_pD3DDevice->SetVertexShader(pShader->m_pVertexShader);
			m_pD3DDevice->SetPixelShader(pShader->m_pPixelShader);
		}
		m_pCurrentShader = pShader;
	}
}

void ShaderAPID3D9::ApplyConstants()
{
	{
		const int minVSDirty = m_nMinVSDirty;
		const int maxVSDirty = m_nMaxVSDirty;

		// Apply vertex shader constants
		if (minVSDirty < maxVSDirty)
		{
			m_pD3DDevice->SetVertexShaderConstantF(minVSDirty, (const float *)(m_vsRegs + minVSDirty), maxVSDirty - minVSDirty + 1);
			//m_pD3DDevice->SetVertexShaderConstantF(0, (const float *) m_vsRegs, 256);
			m_nMinVSDirty = 256;
			m_nMaxVSDirty = -1;
		}
	}

	{
		const int minPSDirty = m_nMinPSDirty;
		const int maxPSDirty = m_nMaxPSDirty;

		// apply pixel shader constants
		if (minPSDirty < maxPSDirty)
		{
			m_pD3DDevice->SetPixelShaderConstantF(minPSDirty, (const float *) (m_psRegs + minPSDirty), maxPSDirty - minPSDirty + 1);
			//m_pD3DDevice->SetPixelShaderConstantF(0, (const float *)m_psRegs, 224);
			m_nMinPSDirty = 224;
			m_nMaxPSDirty = -1;
		}
	}
}

void ShaderAPID3D9::Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const ColorRGBA &fillColor,float fDepth, int nStencil)
{
	// clear the back buffer
	m_pD3DDevice->Clear(0, nullptr, (bClearColor ? D3DCLEAR_TARGET : 0) | (bClearDepth ? D3DCLEAR_ZBUFFER : 0) | (bClearStencil ? D3DCLEAR_STENCIL : 0),
		toBGRA(fillColor), fDepth, nStencil);
}

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

// Device vendor and version
const char* ShaderAPID3D9::GetDeviceNameString() const
{
	return "malfunction";
}

// Renderer string (ex: OpenGL, D3D9)
const char* ShaderAPID3D9::GetRendererName() const
{
	return "Direct3D9";
}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

// Synchronization
void ShaderAPID3D9::Flush()
{
	LPDIRECT3DQUERY9 query = m_pEventQuery;

	if(!query)
		return;

	query->Issue(D3DISSUE_END);
	query->GetData(nullptr, 0, D3DGETDATA_FLUSH);
}

void ShaderAPID3D9::Finish()
{
	LPDIRECT3DQUERY9 query = m_pEventQuery;

	if(!query)
		return;

	query->Issue(D3DISSUE_END);

	while (query->GetData(nullptr, 0, D3DGETDATA_FLUSH) == S_FALSE)
	{
		// Spin-wait
	}
}

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

// creates occlusion query object
IOcclusionQuery* ShaderAPID3D9::CreateOcclusionQuery()
{
	CD3D9OcclusionQuery* occQuery = PPNew CD3D9OcclusionQuery(m_pD3DDevice);

	{
		CScopedMutex m(g_sapi_Mutex);
		m_OcclusionQueryList.append(occQuery);
	}

	return occQuery;
}

// removal of occlusion query object
void ShaderAPID3D9::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
	bool canDelete;
	{
		canDelete = m_OcclusionQueryList.fastRemove(pQuery);
		CScopedMutex m(g_sapi_Mutex);
	}

	if(canDelete)
		delete pQuery;
}

//-------------------------------------------------------------
// State manipulation 
//-------------------------------------------------------------

// creates blending state
IRenderState* ShaderAPID3D9::CreateBlendingState( const BlendStateParams &blendDesc )
{
	CD3D9BlendingState* pState = nullptr;

	for(int i = 0; i < m_BlendStates.numElem(); i++)
	{
		pState = (CD3D9BlendingState*)m_BlendStates[i];

		if(blendDesc.blendEnable == pState->m_params.blendEnable)
		{
			if(blendDesc.blendEnable == true)
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

	pState = PPNew CD3D9BlendingState;
	pState->m_params = blendDesc;

	m_BlendStates.append(pState);

	pState->Ref_Grab();

	return pState;
}
	
// creates depth/stencil state
IRenderState* ShaderAPID3D9::CreateDepthStencilState( const DepthStencilStateParams &depthDesc )
{
	CD3D9DepthStencilState* pState = nullptr;
	
	for(int i = 0; i < m_DepthStates.numElem(); i++)
	{
		pState = (CD3D9DepthStencilState*)m_DepthStates[i];

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
	
	pState = PPNew CD3D9DepthStencilState;
	pState->m_params = depthDesc;

	m_DepthStates.append(pState);

	pState->Ref_Grab();

	return pState;
}

// creates rasterizer state
IRenderState* ShaderAPID3D9::CreateRasterizerState( const RasterizerStateParams &rasterDesc )
{
	CD3D9RasterizerState* pState = nullptr;

	for(int i = 0; i < m_RasterizerStates.numElem(); i++)
	{
		pState = (CD3D9RasterizerState*)m_RasterizerStates[i];

		if(rasterDesc.cullMode == pState->m_params.cullMode &&
			rasterDesc.fillMode == pState->m_params.fillMode &&
			rasterDesc.multiSample == pState->m_params.multiSample &&
			rasterDesc.scissor == pState->m_params.scissor &&
			rasterDesc.useDepthBias == pState->m_params.useDepthBias)
		{
			pState->Ref_Grab();
			return pState;
		}
	}

	pState = PPNew CD3D9RasterizerState;
	pState->m_params = rasterDesc;

	pState->Ref_Grab();

	m_RasterizerStates.append(pState);

	return pState;
}

// completely destroys shader
void ShaderAPID3D9::DestroyRenderState( IRenderState* pState, bool removeAllRefs)
{
	if(!pState)
		return;

	CScopedMutex scoped(g_sapi_Mutex);

	if(!pState->Ref_Drop() && !removeAllRefs)
	{
		return;
	}
	
	switch(pState->GetType())
	{
		case RENDERSTATE_BLENDING:
			delete ((CD3D9BlendingState*)pState);
			m_BlendStates.remove(pState);
			break;
		case RENDERSTATE_RASTERIZER:
			delete ((CD3D9RasterizerState*)pState);
			m_RasterizerStates.remove(pState);
			break;
		case RENDERSTATE_DEPTHSTENCIL:
			delete ((CD3D9DepthStencilState*)pState);
			m_DepthStates.remove(pState);
			break;
	}
}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

bool ShaderAPID3D9::InternalCreateRenderTarget(LPDIRECT3DDEVICE9 dev, CD3D9Texture *tex, int nFlags, const ShaderAPICaps& caps)
{
	if (caps.INTZSupported && caps.INTZFormat == tex->GetFormat())
	{
		LPDIRECT3DBASETEXTURE9 pTexture = nullptr;

		tex->m_usage = D3DUSAGE_DEPTHSTENCIL;
		tex->m_pool = D3DPOOL_DEFAULT;

		DevMsg(DEVMSG_RENDER, "InternalCreateRenderTarget: creating INTZ render target single texture for %s\n", tex->GetName());
		if (dev->CreateTexture(tex->GetWidth(), tex->GetHeight(), tex->GetMipCount(), tex->m_usage, g_d3d9_imageFormats[tex->GetFormat()], (D3DPOOL)tex->m_pool, (LPDIRECT3DTEXTURE9*)&pTexture, nullptr) != D3D_OK)
		{
			MsgError("!!! Couldn't create '%s' INTZ render target with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
			ASSERT(!"Couldn't create INTZ render target");
			return false;
		}

		tex->m_textures.append(pTexture);

		LPDIRECT3DSURFACE9 pSurface = nullptr;

		HRESULT hr = ((LPDIRECT3DTEXTURE9)tex->m_textures[0])->GetSurfaceLevel(0, &pSurface);
		if (!FAILED(hr))
			tex->m_surfaces.append(pSurface);
	}
	else if (IsDepthFormat(tex->GetFormat()))
	{
		DevMsg(DEVMSG_RENDER, "InternalCreateRenderTarget: creating depth/stencil surface for %s\n", tex->GetName());

		LPDIRECT3DSURFACE9 pSurface = nullptr;

		tex->m_usage = D3DUSAGE_RENDERTARGET;
		tex->m_pool = D3DPOOL_DEFAULT;

		if (dev->CreateDepthStencilSurface( tex->GetWidth(), tex->GetHeight(), g_d3d9_imageFormats[tex->GetFormat()], D3DMULTISAMPLE_NONE, 0, FALSE, &pSurface, nullptr) != D3D_OK )
		{
			MsgError("!!! Couldn't create create '%s' depth surface with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
			ASSERT(!"Couldn't create depth surface");
			return false;
		}

		tex->m_surfaces.append(pSurface);
	}
	else 
	{
		if(nFlags & TEXFLAG_RENDERDEPTH)
		{
			DevMsg(DEVMSG_RENDER, "InternalCreateRenderTarget: creating depth for %s\n", tex->GetName());
			if (dev->CreateDepthStencilSurface(tex->GetWidth(), tex->GetHeight(), D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &tex->m_dummyDepth, nullptr) != D3D_OK)
			{
				MsgError("!!! Couldn't create '%s' depth surface for RT with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
				ASSERT(!"Couldn't create depth surface for RT");
				return false;
			}
		}
		
		if (nFlags & TEXFLAG_CUBEMAP)
		{
			tex->m_usage = D3DUSAGE_RENDERTARGET;
			tex->m_pool = D3DPOOL_DEFAULT;

			LPDIRECT3DBASETEXTURE9 pTexture = nullptr;

			DevMsg(DEVMSG_RENDER, "InternalCreateRenderTarget: creating cubemap target for %s\n", tex->GetName());
			if (dev->CreateCubeTexture(tex->GetWidth(), tex->GetMipCount(), tex->m_usage, g_d3d9_imageFormats[tex->GetFormat()], (D3DPOOL)tex->m_pool, (LPDIRECT3DCUBETEXTURE9 *) &pTexture, nullptr) != D3D_OK)
			{
				MsgError("!!! Couldn't create '%s' cubemap render target with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
				ASSERT(!"Couldn't create cubemap render target");
				return false;
			}

			tex->m_textures.append(pTexture);

			for (uint i = 0; i < 6; i++)
			{
				LPDIRECT3DSURFACE9 pSurface = nullptr;

				HRESULT hr = ((LPDIRECT3DCUBETEXTURE9) tex->m_textures[0])->GetCubeMapSurface((D3DCUBEMAP_FACES) i, 0, &pSurface);

				if(!FAILED(hr))
					tex->m_surfaces.append(pSurface);
			}
		}
		else 
		{
			LPDIRECT3DBASETEXTURE9 pTexture = nullptr;

			tex->m_usage = D3DUSAGE_RENDERTARGET;
			tex->m_pool = D3DPOOL_DEFAULT;

			DevMsg(DEVMSG_RENDER, "InternalCreateRenderTarget: creating render target single texture for %s\n", tex->GetName());
			if (dev->CreateTexture(tex->GetWidth(), tex->GetHeight(), tex->GetMipCount(), tex->m_usage, g_d3d9_imageFormats[tex->GetFormat()], (D3DPOOL)tex->m_pool, (LPDIRECT3DTEXTURE9 *) &pTexture, nullptr) != D3D_OK)
			{
				MsgError("!!! Couldn't create '%s' render target with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
				ASSERT(!"Couldn't create render target");
				return false;
			}

			tex->m_textures.append(pTexture);

			LPDIRECT3DSURFACE9 pSurface = nullptr;

			HRESULT hr = ((LPDIRECT3DTEXTURE9) tex->m_textures[0])->GetSurfaceLevel(0, &pSurface);
			if(!FAILED(hr))
				tex->m_surfaces.append(pSurface);
		}
	}

	return true;
}

// It will add new rendertarget
ITexturePtr ShaderAPID3D9::CreateRenderTarget(const char* pszName,int width, int height,ETextureFormat nRTFormat, ETexFilterMode textureFilterType, ETexAddressMode textureAddress, ECompareFunc comparison, int nFlags)
{
	// TODO: use CreateTextureResource

	CRefPtr<CD3D9Texture> pTexture = CRefPtr_new(CD3D9Texture);

	pTexture->SetDimensions(width,height);
	pTexture->SetFormat(nRTFormat);
	pTexture->SetFlags(nFlags | TEXFLAG_RENDERTARGET);
	pTexture->SetName(pszName);

	SamplerStateParams texSamplerParams;
	SamplerStateParams_Make(texSamplerParams, g_renderAPI->GetCaps(), textureFilterType, textureAddress, textureAddress, textureAddress);

	pTexture->SetSamplerState(texSamplerParams);

	if (InternalCreateRenderTarget(m_pD3DDevice, pTexture, nFlags, m_caps))
	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(pTexture);
		m_TextureList.insert(pTexture->m_nameHash, pTexture);
		return ITexturePtr(pTexture);
	} 

	return nullptr;
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

#if 0
// saves rendertarget to texture, you can also save screenshots
void ShaderAPID3D9::SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName)
{
	if(pTargetTexture && pTargetTexture->GetFlags() & TEXFLAG_RENDERTARGET)
	{
		CD3D9Texture* pTexture = (CD3D9Texture*)pTargetTexture;

		if(pTexture->GetFlags() & TEXFLAG_CUBEMAP)
			D3DXSaveTextureToFileA(pFileName, D3DXIFF_DDS, pTexture->m_textures[0], nullptr);
		else
			D3DXSaveSurfaceToFileA(pFileName, D3DXIFF_DDS, pTexture->m_surfaces[0], nullptr, nullptr);
	}
}
#endif

// Copy render target to texture
void ShaderAPID3D9::CopyFramebufferToTexture(const ITexturePtr& pTargetTexture)
{
	CD3D9Texture* dest = (CD3D9Texture*)pTargetTexture.Ptr();
	if(!dest)
		return;

	if(dest->m_textures.numElem() <= 0)
		return;

	LPDIRECT3DSURFACE9 srcSurface;
	HRESULT hr = m_pD3DDevice->GetRenderTarget( 0, &srcSurface );

	if (FAILED(hr))
		return;

	LPDIRECT3DTEXTURE9 destD3DTex = ( LPDIRECT3DTEXTURE9 )dest->m_textures[0];
	ASSERT( destD3DTex );

	// get target surface to copy
	LPDIRECT3DSURFACE9 destSurface;
	hr = destD3DTex->GetSurfaceLevel( 0, &destSurface );

	ASSERT( !FAILED( hr ) );
	if( FAILED( hr ) )
		return;

	hr = m_pD3DDevice->StretchRect( srcSurface, nullptr, destSurface, nullptr, D3DTEXF_NONE );
	ASSERT( !FAILED( hr ) );

	destSurface->Release();
	srcSurface->Release();
}

// Copy render target to texture with resizing
void ShaderAPID3D9::CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect, IAARectangle* destRect)
{
	CD3D9Texture* src = (CD3D9Texture*)srcTarget.Ptr();
	CD3D9Texture* dest = (CD3D9Texture*)destTex.Ptr();

	if(!src || !dest)
		return;

	if(dest->m_textures.numElem() <= 0)
		return;

	if(src->m_surfaces.numElem() <= 0)
		return;

	LPDIRECT3DTEXTURE9 destD3DTex = ( LPDIRECT3DTEXTURE9 )dest->m_textures[0];

	const RECT dxSrcRect = srcRect ? IRectangleToD3DRECT(*srcRect) : RECT{};
	const RECT dxDestRect = destRect ? IRectangleToD3DRECT(*destRect) : RECT{};

	const bool isCubemap = dest->GetFlags() & TEXFLAG_CUBEMAP;

	const int numFaces = src->m_surfaces.numElem();
	for(int i = 0; i < numFaces; i++)
	{
		LPDIRECT3DSURFACE9 srcSurface = src->m_surfaces[i];

		LPDIRECT3DSURFACE9 destSurface;
		HRESULT hr;
		
		if (isCubemap)
			hr = ((LPDIRECT3DCUBETEXTURE9) destD3DTex)->GetCubeMapSurface((D3DCUBEMAP_FACES) i, 0, &destSurface);
		else
			hr = destD3DTex->GetSurfaceLevel( i, &destSurface );

		if (FAILED(hr))
		{
			Msg("CopyRendertargetToTexture failed to GetSurfaceLevel for %s\n", destTex->GetName());
			return;
		}

		ASSERT(destSurface);

		hr = m_pD3DDevice->StretchRect( srcSurface, srcRect ? &dxSrcRect : nullptr, destSurface, destRect ? &dxDestRect : nullptr, D3DTEXF_NONE );
		//ASSERT( !FAILED( hr ) );

		if(FAILED( hr ))
			MsgError("CopyRendertargetToTexture(%s -> %s) failed (%x)\n", srcTarget->GetName(), destTex->GetName(), hr);

		destSurface->Release();
	}
}

// Changes render target (MRT)
void ShaderAPID3D9::ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets, ArrayCRef<int> rtSlice, const ITexturePtr& depthTarget, int depthSlice)
{
	ASSERT_MSG(!rtSlice.ptr() || renderTargets.numElem() == rtSlice.numElem(), "ChangeRenderTargets - renderTargets and rtSlice must be equal");

	for (int i = 0; i < renderTargets.numElem(); i++)
	{
		CD3D9Texture* pRenderTarget = (CD3D9Texture*)renderTargets[i].Ptr();

		const int rtSliceIdx = rtSlice.ptr() ? rtSlice[i] : 0;

		if (pRenderTarget != m_pCurrentColorRenderTargets[i] || rtSliceIdx != m_nCurrentCRTSlice[i])
		{
			m_pD3DDevice->SetRenderTarget(i, pRenderTarget->m_surfaces[rtSliceIdx]);

			m_pCurrentColorRenderTargets[i] = renderTargets[i];
			m_nCurrentCRTSlice[i] = rtSliceIdx;
		}
	}

	for (int i = renderTargets.numElem(); i < m_caps.maxRenderTargets; i++)
	{
		if (m_pCurrentColorRenderTargets[i] != nullptr)
		{
			m_pD3DDevice->SetRenderTarget(i, nullptr);
			m_pCurrentColorRenderTargets[i] = nullptr;
		}
	}

	LPDIRECT3DSURFACE9 bestDepth = renderTargets.numElem() ? ((CD3D9Texture*)renderTargets[0].Ptr())->m_dummyDepth : nullptr;

	if (depthTarget.Ptr() != m_pCurrentDepthRenderTarget)
	{
		CD3D9Texture* pDepthRenderTarget = (CD3D9Texture*)depthTarget.Ptr();

		if (pDepthRenderTarget)
			bestDepth = pDepthRenderTarget->m_surfaces[0 /*nDepthSlice ??? */];

		m_pCurrentDepthRenderTarget = depthTarget;
	}

	if(!bestDepth)
		bestDepth = m_fbDepthTexture->m_surfaces[0];

	if (m_pCurrentDepthSurface != bestDepth)
	{
		m_pD3DDevice->SetDepthStencilSurface(bestDepth);
		m_pCurrentDepthSurface = bestDepth;
	}
}

// Changes back to backbuffer
void ShaderAPID3D9::ChangeRenderTargetToBackBuffer()
{
	// we can do it simplier, but there is a lack in depth, so keep an old this method...
	
	if (m_pCurrentColorRenderTargets[0] != nullptr)
	{
		m_pD3DDevice->SetRenderTarget(0, m_fbColorTexture->m_surfaces[0]);
		m_pCurrentColorRenderTargets[0] = nullptr;
	}

	for (int i = 1; i < m_caps.maxRenderTargets; i++)
	{
		if (m_pCurrentColorRenderTargets[i] != nullptr)
		{
			m_pD3DDevice->SetRenderTarget(i, nullptr);
			m_pCurrentColorRenderTargets[i] = nullptr;
		}
	}

	if (m_pCurrentDepthSurface != nullptr)
	{
		m_pD3DDevice->SetDepthStencilSurface(m_fbDepthTexture->m_surfaces[0]);
		m_pCurrentDepthSurface = nullptr;
	}
}

// resizes render target
void ShaderAPID3D9::ResizeRenderTarget(const ITexturePtr& renderTarget, int newWide, int newTall)
{
	if(renderTarget->GetWidth() == newWide && renderTarget->GetHeight() == newTall)
		return;

	CD3D9Texture* pRenderTarget = (CD3D9Texture*)renderTarget.Ptr();

	pRenderTarget->Release();
	pRenderTarget->SetDimensions(newWide, newTall);

	InternalCreateRenderTarget(m_pD3DDevice, pRenderTarget, pRenderTarget->GetFlags(), m_caps);
}

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

// Set Depth range for next primitives
void ShaderAPID3D9::SetDepthRange(float fZNear,float fZFar)
{
	D3DVIEWPORT9 view;
	m_pD3DDevice->GetViewport(&view);
	view.MinZ = fZNear;
	view.MaxZ = fZFar;
	m_pD3DDevice->SetViewport(&view);
}

// sets scissor rectangle
void ShaderAPID3D9::SetScissorRectangle( const IAARectangle &rect )
{
	const RECT scissorRect = IRectangleToD3DRECT(rect);
	m_pD3DDevice->SetScissorRect(&scissorRect);
}

// Changes the vertex format
void ShaderAPID3D9::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	CD3D9VertexFormat* pFormat = (CD3D9VertexFormat*)pVertexFormat;

	if (pFormat == m_pCurrentVertexFormat)
		return;

	if (pFormat != nullptr)
	{
		m_pD3DDevice->SetVertexDeclaration(pFormat->m_pVertexDecl);

		CD3D9VertexFormat* pCurrentFormat = (CD3D9VertexFormat*)m_pCurrentVertexFormat;
		if (pCurrentFormat != nullptr)
		{
			for (int i = 0; i < MAX_VERTEXSTREAM; i++)
			{
				if (pFormat->m_streamStride[i] != pCurrentFormat->m_streamStride[i])
					m_pCurrentVertexBuffers[i] = nullptr;
			}
		}
	}

	m_pCurrentVertexFormat = pFormat;
}

// Changes the vertex buffer
void ShaderAPID3D9::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset)
{
	UINT nStreamParam1 = 1;
	UINT nStreamParam2 = 1;

	CD3D9VertexBuffer* pVB = (CD3D9VertexBuffer*)(pVertexBuffer);

	if (nStream == 2)	// FIXME: instance stream ID
	{
		if (pVB && (pVB->GetFlags() & VERTBUFFER_FLAG_INSTANCEDATA))
		{
			uint numInstances = pVB->GetVertexCount();

			nStreamParam1 = (D3DSTREAMSOURCE_INDEXEDDATA | numInstances);
			nStreamParam2 = (D3DSTREAMSOURCE_INSTANCEDATA | 1);
		}

		if (m_nSelectedStreamParam[0] != nStreamParam1 || m_nSelectedStreamParam[nStream] != nStreamParam2)
		{
			m_pD3DDevice->SetStreamSourceFreq(0, nStreamParam1);
			m_pD3DDevice->SetStreamSourceFreq(nStream, nStreamParam2);

			m_nSelectedStreamParam[0] = nStreamParam1;
			m_nSelectedStreamParam[nStream] = nStreamParam2;
		}
	}

	if (pVB != m_pCurrentVertexBuffers[nStream] || m_nCurrentOffsets[nStream] != offset)
	{
		if (pVB == nullptr)
			m_pD3DDevice->SetStreamSource(nStream, nullptr, 0, 0 );
		else 
			m_pD3DDevice->SetStreamSource(nStream, pVB->m_pVertexBuffer, (UINT)offset*pVB->GetStrideSize(), pVB->GetStrideSize());

		m_pCurrentVertexBuffers[nStream] = pVertexBuffer;
		m_nCurrentOffsets[nStream] = offset;
	}
}

// Changes the index buffer
void ShaderAPID3D9::ChangeIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	if (pIndexBuffer != m_pCurrentIndexBuffer)
	{
		CD3D9IndexBuffer* pIB = (CD3D9IndexBuffer*)(pIndexBuffer);

		if (pIB == nullptr)
			m_pD3DDevice->SetIndices(nullptr);
		else
			m_pD3DDevice->SetIndices(pIB->m_pIndexBuffer);

		m_pCurrentIndexBuffer = pIndexBuffer;
	}
}

// Destroy vertex format
void ShaderAPID3D9::DestroyVertexFormat(IVertexFormat* pFormat)
{
	CD3D9VertexFormat* pVF = (CD3D9VertexFormat*)(pFormat);
	if(!pVF)
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
void ShaderAPID3D9::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	CD3D9VertexBuffer* pVB = (CD3D9VertexBuffer*)(pVertexBuffer);
	if(!pVB)
		return;

	bool deleted = false;
	{
		CScopedMutex m(g_sapi_VBMutex);
		deleted = m_VBList.remove(pVB);
	}

	if(deleted)
	{
		// reset if in use
		DevMsg(DEVMSG_RENDER, "Destroying vertex buffer\n");
		delete pVB;
	}
}

// Destroy index buffer
void ShaderAPID3D9::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	CD3D9IndexBuffer* pIB = (CD3D9IndexBuffer*)(pIndexBuffer);

	if(!pIB)
		return;

	bool deleted = false;
	{
		CScopedMutex m(g_sapi_IBMutex);
		deleted = m_IBList.remove(pIB);
	}

	if (deleted)
	{
		DevMsg(DEVMSG_RENDER, "Destroying index buffer\n");
		delete pIB;
	}
}

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------


// Creates shader class for needed ShaderAPI
IShaderProgram* ShaderAPID3D9::CreateNewShaderProgram(const char* pszName, const char* query)
{
	CD3D9ShaderProgram* pNewProgram = PPNew CD3D9ShaderProgram();
	pNewProgram->SetName((_Es(pszName)+query).GetData());

	CScopedMutex scoped(g_sapi_ShaderMutex);

	ASSERT_MSG(m_ShaderList.find(pNewProgram->m_nameHash) == m_ShaderList.end(), "Shader %s was already added", pNewProgram->GetName());
	m_ShaderList.insert(pNewProgram->m_nameHash, pNewProgram);

	return pNewProgram;
}

// Destroy all shader
void ShaderAPID3D9::DestroyShaderProgram(IShaderProgram* pShaderProgram)
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(pShaderProgram);

	if(!pShader)
		return;

	{
		CScopedMutex m(g_sapi_ShaderMutex);
		auto it = m_ShaderList.find(pShader->m_nameHash);
		if (it.atEnd())
			return;

		// remove it if reference is zero
		if (!pShader->Ref_Drop())
			return;

		m_ShaderList.remove(it);
	}
	delete pShader;
}

void ShaderAPID3D9::StepProgressiveLodTextures()
{
	int numTransferred = 0;

	g_sapi_ProgressiveTextureMutex.Lock();
	auto it = m_progressiveTextures.begin();
	g_sapi_ProgressiveTextureMutex.Unlock();

	while (!it.atEnd() && numTransferred < TEXTURE_TRANSFER_MAX_TEXTURES_PER_FRAME)
	{
		CD3D9Texture* nextTexture = nullptr;
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

		if(status == PROGRESSIVE_STATUS_DID_UPLOAD)
			++numTransferred;
	} 
}

void ShaderAPID3D9::PreloadShadersFromCache()
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
		
		CD3D9ShaderProgram* pNewProgram = PPNew CD3D9ShaderProgram();
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

bool ShaderAPID3D9::InitShaderFromCache(IShaderProgram* pShaderOutput, IVirtualStream* pStream, uint32 checksum)
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(pShaderOutput);

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

	// skip defines
	pStream->Seek(scHdr.extraLen, VS_SEEK_CUR);

	// read vertex shader
	ubyte* pShaderMem = (ubyte*)PPAlloc(scHdr.vsSize);
	pStream->Read(pShaderMem, 1, scHdr.vsSize);

	m_pD3DDevice->CreateVertexShader((DWORD*)pShaderMem, &pShader->m_pVertexShader);
	PPFree(pShaderMem);

	// read pixel shader
	pShaderMem = (ubyte*)PPAlloc(scHdr.psSize);
	pStream->Read(pShaderMem, 1, scHdr.psSize);

	m_pD3DDevice->CreatePixelShader((DWORD*)pShaderMem, &pShader->m_pPixelShader);
	PPFree(pShaderMem);

	// read samplers and constants
	Array<DX9Sampler_t> samplers(PP_SL);
	Array<DX9ShaderConstant_t> constants(PP_SL);
	samplers.setNum(scHdr.numSamplers);
	constants.setNum(scHdr.numConstants);

	pStream->Read(samplers.ptr(), scHdr.numSamplers, sizeof(DX9Sampler_t));
	pStream->Read(constants.ptr(), scHdr.numConstants, sizeof(DX9ShaderConstant_t));

	Map<int, DX9Sampler_t>& samplerMap = pShader->m_samplers;
	Map<int, DX9ShaderConstant_t>& constantMap = pShader->m_constants;

	// assign
	for (int i = 0; i < samplers.numElem(); ++i)
		samplerMap.insert(samplers[i].nameHash, samplers[i]);

	for (int i = 0; i < constants.numElem(); ++i)
		constantMap.insert(constants[i].nameHash, constants[i]);

	pShader->m_cacheChecksum = scHdr.checksum;

	return true;
}

// Load any shader from stream
bool ShaderAPID3D9::CompileShadersFromStream(	IShaderProgram* pShaderOutput,
												const ShaderProgCompileInfo& info,
												const char* extra)
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(pShaderOutput);

	if(!pShader)
		return false;

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

	CMemoryStream vsMemStream(nullptr, VS_OPEN_WRITE, 2048, PP_SL);
	CMemoryStream psMemStream(nullptr, VS_OPEN_WRITE, 2048, PP_SL);

	if (info.data.text != nullptr)
	{
		LPD3DXBUFFER shaderBuf = nullptr;
		LPD3DXBUFFER errorsBuf = nullptr;

		EqString shaderString;

		if (extra  != nullptr)
			shaderString.Append(extra);

		int maxVSVersion = D3DSHADER_VERSION_MAJOR(m_hCaps.VertexShaderVersion);
		EqString profile(D3DXGetVertexShaderProfile(m_pD3DDevice));
		EqString entry("vs_main");

		int vsVersion = maxVSVersion;

		if(info.apiPrefs)
		{
			profile = KV_GetValueString(info.apiPrefs->FindSection("vs_profile"), 0, profile.ToCString());
			entry = KV_GetValueString(info.apiPrefs->FindSection("EntryPoint"), 0, entry.ToCString());

			char minor = '0';

			sscanf(profile.GetData(), "vs_%d_%c", &vsVersion, &minor);

			if(vsVersion > maxVSVersion)
			{
				MsgWarning("%s: vs version %s not supported\n", pShaderOutput->GetName(), profile.ToCString());
				vsVersion = maxVSVersion;
			}
		}
		// else default to maximum

		shaderString.Append(EqString::Format("#define COMPILE_VS_%d_0\n", vsVersion));
		shaderString.Append("#define VERTEX\n");
		shaderString.Append(info.data.boilerplate);
		shaderString.Append("\r\n#line 1 \"FXFile\"\r\n");
		shaderString.Append(info.data.text);

		HRESULT compileResult = D3DXCompileShader(shaderString.GetData(), shaderString.Length(),
			nullptr, nullptr,
			entry.ToCString(), profile.ToCString(),
			D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR | D3DXSHADER_PARTIALPRECISION,
			&shaderBuf, &errorsBuf, &pShader->m_pVSConstants);

		if (compileResult == D3D_OK)
		{
			m_pD3DDevice->CreateVertexShader((DWORD *) shaderBuf->GetBufferPointer(), &pShader->m_pVertexShader);

			if (shaderCacheEnabled)
				vsMemStream.Write(shaderBuf->GetBufferPointer(), 1, shaderBuf->GetBufferSize());

			shaderBuf->Release();
		}
		else 
		{
			char* d3dxShaderCompileErr = "Unknown\n";

			switch (compileResult)
			{
				case D3DERR_INVALIDCALL:
					d3dxShaderCompileErr = "D3DERR_INVALIDCALL";
					break;
				case D3DXERR_INVALIDDATA:
					d3dxShaderCompileErr = "D3DXERR_INVALIDDATA";
					break;
				case E_OUTOFMEMORY:
					d3dxShaderCompileErr = "E_OUTOFMEMORY";
					break;
			}

			MsgError("ERROR: Vertex shader '%s' CODE '%s'\n", pShader->GetName(), d3dxShaderCompileErr);
			if (errorsBuf)
			{
				MsgError("%s\n", (const char *)errorsBuf->GetBufferPointer());
				errorsBuf->Release();
			}

			MsgError("\n Profile: %s\n", profile.ToCString());
		}
	}

	if (info.data.text != nullptr)
	{
		LPD3DXBUFFER shaderBuf = nullptr;
		LPD3DXBUFFER errorsBuf = nullptr;

		EqString shaderString;

		if (extra  != nullptr)
			shaderString.Append(extra);

		int maxPSVersion = D3DSHADER_VERSION_MAJOR(m_hCaps.PixelShaderVersion);
		EqString profile(D3DXGetPixelShaderProfile(m_pD3DDevice));
		EqString entry("ps_main");

		int psVersion = maxPSVersion;

		if(info.apiPrefs)
		{
			profile = KV_GetValueString(info.apiPrefs->FindSection("ps_profile"), 0, profile.ToCString());
			entry = KV_GetValueString(info.apiPrefs->FindSection("EntryPoint"), 0, entry.ToCString());

			char minor = '0';

			sscanf(profile.GetData(), "ps_%d_%c", &psVersion, &minor);

			if(psVersion > maxPSVersion)
			{
				MsgWarning("%s: ps version %s not supported\n", pShaderOutput->GetName(), profile.GetData());
				psVersion = maxPSVersion;
			}
		}
		// else default to maximum

		shaderString.Append(EqString::Format("#define COMPILE_PS_%d_0\n", psVersion));
		shaderString.Append("#define PIXEL\n");
		shaderString.Append(info.data.boilerplate);
		shaderString.Append("\r\n#line 1 \"FXFile\"\r\n");
		shaderString.Append(info.data.text);

		HRESULT compileResult = D3DXCompileShader(
			shaderString.GetData(), shaderString.Length(),
			nullptr, nullptr, entry.ToCString(), profile.ToCString(),
			D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR,
			&shaderBuf, &errorsBuf,
			&pShader->m_pPSConstants);

		if (compileResult == D3D_OK)
		{
			m_pD3DDevice->CreatePixelShader((DWORD *) shaderBuf->GetBufferPointer(), &pShader->m_pPixelShader);

			if (shaderCacheEnabled)
				psMemStream.Write(shaderBuf->GetBufferPointer(), 1, shaderBuf->GetBufferSize());

			shaderBuf->Release();
		}
		else 
		{
			char* d3dxShaderCompileErr = "Unknown\n";

			switch (compileResult)
			{
			case D3DERR_INVALIDCALL:
				d3dxShaderCompileErr = "D3DERR_INVALIDCALL";
				break;
			case D3DXERR_INVALIDDATA:
				d3dxShaderCompileErr = "D3DXERR_INVALIDDATA";
				break;
			case E_OUTOFMEMORY:
				d3dxShaderCompileErr = "E_OUTOFMEMORY";
				break;
			}

			MsgError("ERROR: Pixel shader '%s' CODE '%s'\n", pShader->GetName(), d3dxShaderCompileErr);
			if (errorsBuf)
			{
				MsgError("%s\n", (const char *)errorsBuf->GetBufferPointer());
				errorsBuf->Release();
			}

			MsgError("\n Profile: %s\n",profile.ToCString());
		}
	}

	ID3DXConstantTable* d3dVSConstants = pShader->m_pVSConstants;
	ID3DXConstantTable* d3dPSConstants = pShader->m_pPSConstants;

	if (pShader->m_pPixelShader == nullptr || pShader->m_pVertexShader == nullptr ||
		d3dVSConstants == nullptr || d3dPSConstants == nullptr)
	{
		return false;
	}

	D3DXCONSTANTTABLE_DESC vsDesc, psDesc;
	d3dVSConstants->GetDesc(&vsDesc);
	d3dPSConstants->GetDesc(&psDesc);

	const uint count = vsDesc.Constants + psDesc.Constants;

	Array<DX9Sampler_t> samplers(PP_SL);
	Array<DX9ShaderConstant_t> constants(PP_SL);
	samplers.resize(count);
	constants.resize(count);

	// collect VS shader constants
	D3DXCONSTANT_DESC cDesc;
	for (uint i = 0; i < vsDesc.Constants; i++)
	{
		UINT cnt = 1;
		d3dVSConstants->GetConstantDesc(d3dVSConstants->GetConstant(nullptr, i), &cDesc, &cnt);

		//size_t length = strlen(cDesc.Name);
		if (cDesc.Type >= D3DXPT_SAMPLER && cDesc.Type <= D3DXPT_SAMPLERCUBE)
		{
			DX9Sampler_t& sampler = samplers.append();

			sampler.vsIndex = cDesc.RegisterIndex;
			strcpy(sampler.name, cDesc.Name);
		} 
		else 
		{
			DX9ShaderConstant_t& constant = constants.append();

			strcpy(constant.name, cDesc.Name);
			constant.vsReg = cDesc.RegisterIndex;
			//constant.nElements = cDesc.RegisterCount;
			constant.psReg = -1;
			constant.constFlags = SCONST_VERTEX;
		}
	}

	const uint nVSConsts = constants.numElem();
	const uint nVSSamplers = samplers.numElem();

	for (uint i = 0; i < psDesc.Constants; i++)
	{
		UINT cnt = 1;
		d3dPSConstants->GetConstantDesc(d3dPSConstants->GetConstant(NULL, i), &cDesc, &cnt);

		//size_t length = strlen(cDesc.Name);
		if (cDesc.Type >= D3DXPT_SAMPLER && cDesc.Type <= D3DXPT_SAMPLERCUBE)
		{
			// check for merging with VS sampler
			int merge = -1;
			for (uint j = 0; j < nVSSamplers; j++)
			{
				if (strcmp(samplers[j].name, cDesc.Name) == 0)
				{
					merge = j;
					break;
				}
			}

			if (merge < 0)
			{
				DX9Sampler_t& sampler = samplers.append();

				sampler.index = cDesc.RegisterIndex;
				strcpy(sampler.name, cDesc.Name);
			}
			else
			{
				samplers[merge].index = cDesc.RegisterIndex;
			}
		} 
		else 
		{
			// check for merging with VS constant
			int merge = -1;
			for (uint j = 0; j < nVSConsts; j++)
			{
				if (strcmp(constants[j].name, cDesc.Name) == 0)
				{
					merge = j;
					break;
				}
			}

			if (merge < 0)
			{
				DX9ShaderConstant_t& constant = constants.append();

				strcpy(constant.name, cDesc.Name);
				constant.vsReg = -1;
				constant.psReg = cDesc.RegisterIndex;
				//constant.nElements = cDesc.RegisterCount;
				constant.constFlags = SCONST_PIXEL;
			} 
			else 
			{
				constants[merge].psReg = cDesc.RegisterIndex;
				constants[merge].constFlags |= SCONST_PIXEL; // add flags
			}
		}
	}

	Map<int, DX9Sampler_t>& samplerMap = pShader->m_samplers;
	Map<int, DX9ShaderConstant_t>& constantMap = pShader->m_constants;

	// build a map
	for (int i = 0; i < samplers.numElem(); ++i)
	{
		samplers[i].nameHash = StringToHash(samplers[i].name);
		samplerMap.insert(samplers[i].nameHash, samplers[i]);
	}

	for (int i = 0; i < constants.numElem(); ++i)
	{
		constants[i].nameHash = StringToHash(constants[i].name);
		constantMap.insert(constants[i].nameHash, constants[i]);
	}

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
			scHdr.numConstants = constants.numElem();
			scHdr.vsSize = vsMemStream.Tell();
			scHdr.psSize = psMemStream.Tell();
			scHdr.nameLen = pShader->m_szName.Length();
			scHdr.extraLen = strlen(extra);

			pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));
			pStream->Write(pShader->m_szName.GetData(), scHdr.nameLen, 1);
			pStream->Write(extra, scHdr.extraLen, 1);

			vsMemStream.WriteToStream(pStream);
			psMemStream.WriteToStream(pStream);

			pStream->Write(samplers.ptr(), samplers.numElem(), sizeof(DX9Sampler_t));
			pStream->Write(constants.ptr(), constants.numElem(), sizeof(DX9ShaderConstant_t));
		}
		else
			MsgError("ERROR: Cannot create shader cache file for %s\n", pShaderOutput->GetName());
	}

	return true;
}

// Set current shader for rendering
void ShaderAPID3D9::SetShader(IShaderProgram* pShader)
{
	m_pSelectedShader = pShader;
	/*
	memset(m_vsRegs,0,sizeof(m_vsRegs));
	memset(m_psRegs,0,sizeof(m_psRegs));

	m_nMinVSDirty = 256;
	m_nMaxVSDirty = -1;
	m_nMinPSDirty = 224;
	m_nMaxPSDirty = -1;
	*/
}

void ShaderAPID3D9::SetTexture(int nameHash, const ITexturePtr& pTexture)
{
	if (nameHash == 0)
	{
		ASSERT_FAIL("SetTexture requires name");
		return;
	}

	if (!m_pSelectedShader)
		return;

	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(m_pSelectedShader);
	const Map<int, DX9Sampler_t>& samplerMap = pShader->m_samplers;

	auto it = samplerMap.find(nameHash);
	if (it.atEnd())
		return;

	const DX9Sampler_t& sampler = *it;

	if(sampler.index != -1)
		SetTextureAtIndex(pTexture, sampler.index);

	// NOTE: vertex shader index should be passed as negative
	if(sampler.vsIndex != -1)
		SetTextureAtIndex(pTexture, sampler.vsIndex | 0x8000);
}

// RAW Constant (Used for structure types, etc.)
void ShaderAPID3D9::SetShaderConstantRaw(int nameHash, const void *data, int nSize)
{
	if (data == nullptr || nSize == 0)
		return;

	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(m_pSelectedShader);

	if(!pShader)
		return;

	const Map<int, DX9ShaderConstant_t>& constantsMap = pShader->m_constants;
	auto it = constantsMap.find(nameHash);
	if (it.atEnd())
		return;

	const DX9ShaderConstant_t& constant = *it;

	if (constant.vsReg >= 0 && memcmp(m_vsRegs + constant.vsReg, data, nSize))
	{
		memcpy(m_vsRegs + constant.vsReg, data, nSize);

		const int r0 = constant.vsReg;
		const int r1 = constant.vsReg + ((nSize + 15) >> 4);

		if (r0 < m_nMinVSDirty)
			m_nMinVSDirty = r0;

		if (r1 > m_nMaxVSDirty)
			m_nMaxVSDirty = r1;
	}

	if (constant.psReg >= 0 && memcmp(m_psRegs + constant.psReg, data, nSize))
	{
		memcpy(m_psRegs + constant.psReg, data, nSize);

		const int r0 = constant.psReg;
		const int r1 = constant.psReg + ((nSize + 15) >> 4);

		if (r0 < m_nMinPSDirty)
			m_nMinPSDirty = r0;

		if (r1 > m_nMaxPSDirty)
			m_nMaxPSDirty = r1;
	}
}

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

IVertexFormat* ShaderAPID3D9::CreateVertexFormat(const char* name, ArrayCRef<VertexFormatDesc> formatDesc)
{
	CD3D9VertexFormat* pFormat = PPNew CD3D9VertexFormat(name, formatDesc.ptr(), formatDesc.numElem());

	D3DVERTEXELEMENT9* vertexElements = PPNew D3DVERTEXELEMENT9[formatDesc.numElem() + 1];
	pFormat->GenVertexElement( vertexElements );

	HRESULT hr = m_pD3DDevice->CreateVertexDeclaration(vertexElements, &pFormat->m_pVertexDecl);
	delete [] vertexElements;

	if (hr != D3D_OK)
	{
		delete pFormat;
		MsgError("Couldn't create vertex declaration");
		ASSERT(!"Couldn't create vertex declaration");
		return nullptr;
	}

	{
		CScopedMutex m(g_sapi_VBMutex);
		m_VFList.append(pFormat);
	}

	return pFormat;
}

IVertexBuffer* ShaderAPID3D9::CreateVertexBuffer(EBufferAccessType nBufAccess, int nNumVerts, int strideSize, void *pData)
{
	CD3D9VertexBuffer* pBuffer = PPNew CD3D9VertexBuffer();
	pBuffer->m_nSize = nNumVerts*strideSize;
	pBuffer->m_nUsage = g_d3d9_bufferUsages[nBufAccess];
	pBuffer->m_nNumVertices = nNumVerts;
	pBuffer->m_nStrideSize = strideSize;
	pBuffer->m_nInitialSize = nNumVerts*strideSize;

	DevMsg(DEVMSG_RENDER, "Creating VBO with size %i KB\n", pBuffer->m_nSize / 1024);

	const bool dynamic = (pBuffer->m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	if (m_pD3DDevice->CreateVertexBuffer(pBuffer->m_nInitialSize, pBuffer->m_nUsage, 0, dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &pBuffer->m_pVertexBuffer, nullptr) != D3D_OK)
	{
		MsgError("Direct3D Error: Couldn't create vertex buffer with size %d\n", pBuffer->m_nSize);
        ASSERT(!"Direct3D Error: Couldn't create vertex buffer");
		return nullptr;
	}

	// make first transfer operation
	void *dest;
	if (pData && pBuffer->m_pVertexBuffer->Lock(0, pBuffer->m_nSize, &dest, (dynamic ? D3DLOCK_DISCARD : 0) | D3DLOCK_NOSYSLOCK) == D3D_OK)
	{
		memcpy(dest, pData, pBuffer->m_nSize);
		pBuffer->m_pVertexBuffer->Unlock();
		pBuffer->m_pVertexBuffer->PreLoad();
	}

	{
		CScopedMutex m(g_sapi_VBMutex);
		m_VBList.append(pBuffer);
	}

	return pBuffer;

}
IIndexBuffer* ShaderAPID3D9::CreateIndexBuffer(int nIndices, int nIndexSize, EBufferAccessType nBufAccess, void *pData)
{
	ASSERT(nIndexSize >= 2);
	ASSERT(nIndexSize <= 4);

	CD3D9IndexBuffer* pBuffer = PPNew CD3D9IndexBuffer();
	pBuffer->m_nIndices = nIndices;
	pBuffer->m_nIndexSize = nIndexSize;
	pBuffer->m_nInitialSize = nIndices*nIndexSize;
	pBuffer->m_nUsage = g_d3d9_bufferUsages[nBufAccess];

	bool dynamic = (pBuffer->m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	DevMsg(DEVMSG_RENDER, "Creating IBO with size %i KB\n", (nIndices*nIndexSize) / 1024);

	if (m_pD3DDevice->CreateIndexBuffer(pBuffer->m_nInitialSize, pBuffer->m_nUsage, nIndexSize == 2? D3DFMT_INDEX16 : D3DFMT_INDEX32, dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &pBuffer->m_pIndexBuffer, nullptr) != D3D_OK)
	{
		MsgError("Direct3D Error: Couldn't create index buffer with size %d\n", pBuffer->m_nInitialSize);
		ASSERT(!"Direct3D Error: Couldn't create index buffer\n");
		return nullptr;
	}

	// make first transfer operation
	void *dest;
	if (pData && pBuffer->m_pIndexBuffer->Lock(0, pBuffer->m_nInitialSize, &dest, (dynamic ? D3DLOCK_DISCARD : 0) | D3DLOCK_NOSYSLOCK) == D3D_OK)
	{
		memcpy(dest, pData, pBuffer->m_nInitialSize);
		pBuffer->m_pIndexBuffer->Unlock();
		pBuffer->m_pIndexBuffer->PreLoad();
	} 

	{
		CScopedMutex m(g_sapi_IBMutex);
		m_IBList.append(pBuffer);
	}

	return pBuffer;
}

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

// Indexed primitive drawer
void ShaderAPID3D9::DrawIndexedPrimitives(EPrimTopology nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
	ASSERT(nVertices > 0);
	const int numPrimitives = g_d3d9_primCountFunc[nType](nIndices);
	m_pD3DDevice->DrawIndexedPrimitive( g_d3d9_primType[nType], nBaseVertex, nFirstVertex, nVertices, nFirstIndex, numPrimitives);
	
	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += numPrimitives;
}

// Draw elements
void ShaderAPID3D9::DrawNonIndexedPrimitives(EPrimTopology nType, int nFirstVertex, int nVertices)
{
	const int numPrimitives = g_d3d9_primCountFunc[nType](nVertices);
	m_pD3DDevice->DrawPrimitive(g_d3d9_primType[nType], nFirstVertex, numPrimitives);

	m_nDrawCalls++;
	m_nTrianglesCount += numPrimitives;
}

//-------------------------------------------------------------------------------------------------------------------------
// Textures
//-------------------------------------------------------------------------------------------------------------------------

ITexturePtr ShaderAPID3D9::CreateTextureResource(const char* pszName)
{
	CRefPtr<CD3D9Texture> texture = CRefPtr_new(CD3D9Texture);
	texture->SetName(pszName);

	m_TextureList.insert(texture->m_nameHash, texture);
	return ITexturePtr(texture);
}
