//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Direct3D 9 ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include <d3dx9.h>

#include "core/DebugInterface.h"
#include "core/IConsoleCommands.h"
#include "core/IFileSystem.h"

#include "ShaderAPID3DX9.h"
#include "CD3D9Texture.h"
#include "d3dx9_def.h"

#include "VertexFormatD3DX9.h"
#include "VertexBufferD3DX9.h"
#include "IndexBufferD3DX9.h"

#include "D3D9ShaderProgram.h"
#include "D3D9OcclusionQuery.h"

#include "imaging/ImageLoader.h"

#include "ds/VirtualStream.h"
#include "utils/strtools.h"
#include "utils/KeyValues.h"

bool InternalCreateRenderTarget(LPDIRECT3DDEVICE9 dev, CD3D9Texture *tex, int nFlags);

// only needed for unmanaged textures
#define DEVICE_SPIN_WAIT while(m_bDeviceAtReset){if(!m_bDeviceAtReset) break;}

#pragma warning(disable:4838)

ShaderAPID3DX9::~ShaderAPID3DX9()
{
	
}

ShaderAPID3DX9::ShaderAPID3DX9() : ShaderAPI_Base()
{
	Msg("Initializing Direct3D9 Shader API...\n");

	m_pEventQuery = NULL;

	m_pD3DDevice = NULL;

	m_fbColorTexture = NULL;
	m_fbDepthTexture = NULL;

	m_nCurrentSrcFactor = BLENDFACTOR_ONE;
	m_nCurrentDstFactor = BLENDFACTOR_ZERO;
	m_nCurrentBlendMode = BLENDFUNC_ADD;

	m_nCurrentDepthFunc = COMP_LEQUAL;
	m_bCurrentDepthTestEnable = false;
	m_bCurrentDepthWriteEnable = false;

	m_bDoStencilTest = false;
	m_nStencilMask = 0xFF;
	m_nStencilFunc = COMP_ALWAYS,
	m_nStencilFail = STENCILFUNC_KEEP;
	m_nDepthFail = STENCILFUNC_KEEP;
	m_nStencilPass = STENCILFUNC_KEEP;

	m_bCurrentMultiSampleEnable = false;
	m_bCurrentScissorEnable = false;
	m_nCurrentCullMode = CULL_BACK;
	m_nCurrentFillMode = FILL_SOLID;

	m_nCurrentMask = COLORMASK_ALL;
	m_bCurrentBlendEnable = false;
	m_bCurrentAlphaTestEnabled = false;
	m_fCurrentAlphaTestRef = 0.9f;

	//m_nCurrentSampleMask = ~0;
	m_nSelectedSampleMask = ~0;

	memset(m_vsRegs,0,sizeof(m_vsRegs));
	memset(m_psRegs,0,sizeof(m_psRegs));

	m_nMinVSDirty = 256;
	m_nMaxVSDirty = -1;
	m_nMinPSDirty = 224;
	m_nMaxPSDirty = -1;

	memset(m_pSelectedSamplerStates,0,sizeof(m_pSelectedSamplerStates));
	memset(m_pSelectedVertexSamplerStates,0,sizeof(m_pSelectedVertexSamplerStates));

	
	for(int i = 0; i < MAX_VERTEXSTREAM; i++)
		m_nSelectedStreamParam[i] = 1;

	m_fCurrentDepthBias = 0.0f;
	m_fCurrentSlopeDepthBias = 0.0f;

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

	m_nCurrentSamplerStateDirty = 0xffffffff;
	m_nCurrentVertexSamplerStateDirty = 0xffffffff;
}

// Only in D3DX9 Renderer
#ifdef USE_D3DEX
void ShaderAPID3DX9::SetD3DDevice(LPDIRECT3DDEVICE9EX d3ddev, D3DCAPS9 &d3dcaps)
#else
void ShaderAPID3DX9::SetD3DDevice(LPDIRECT3DDEVICE9 d3ddev, D3DCAPS9 &d3dcaps)
#endif
{
	m_pD3DDevice = d3ddev;
	m_hCaps = d3dcaps;
}

//-----------------------------------------------------------------------------
// Check for device lost
//-----------------------------------------------------------------------------

void ShaderAPID3DX9::CheckDeviceResetOrLost(HRESULT hr)
{
	if (hr == D3DERR_DEVICELOST)
	{
		if(!m_bDeviceIsLost)
			MsgWarning("DIRECT3D9 device lost.\n");

		m_bDeviceIsLost = true;
	}
	else if (FAILED(hr) && hr != D3DERR_INVALIDCALL)
	{
		MsgWarning("DIRECT3D9 present failed.\n");
		return;
	}
}

bool ShaderAPID3DX9::ResetDevice( D3DPRESENT_PARAMETERS &d3dpp )
{
	HRESULT hr;

	CScopedMutex scoped(m_Mutex);

	if(!m_bDeviceAtReset)
	{
		m_bDeviceAtReset = true;

		if (m_pEventQuery)
			m_pEventQuery->Release();

		m_pEventQuery = NULL;

		Reset();
		Apply();

		// release back buffer and depth first
		ReleaseD3DFrameBufferSurfaces();

		for (int i = 0; i < m_VBList.numElem(); i++)
		{
			CVertexBufferD3DX9* pVB = (CVertexBufferD3DX9*)m_VBList[i];

			pVB->ReleaseForRestoration();
		}

		for (int i = 0; i < m_IBList.numElem(); i++)
		{
			CIndexBufferD3DX9* pIB = (CIndexBufferD3DX9*)m_IBList[i];

			pIB->ReleaseForRestoration();
		}

		for (int i = 0; i < m_OcclusionQueryList.numElem(); i++)
		{
			CD3D9OcclusionQuery* query = (CD3D9OcclusionQuery*)m_OcclusionQueryList[i];
			query->Destroy();
		}

		// relesase texture surfaces
		for (auto it = m_TextureList.begin(); it != m_TextureList.end(); ++it)
		{
			CD3D9Texture* pTex = (CD3D9Texture*)*it;

			bool is_managed = (pTex->GetFlags() & TEXFLAG_MANAGED);

			// release unmanaged textures and rts
			if (!is_managed)
			{
				DevMsg(DEVMSG_SHADERAPI, "RESET: releasing %s\n", pTex->GetName());
				pTex->Release();
			}
		}

		DevMsg(DEVMSG_SHADERAPI, "Device objects releasing done, resetting...\n");
	}

	// Reset the device before restoring everything
	if (FAILED(hr = m_pD3DDevice->Reset(&d3dpp)))
	{
		if (hr == D3DERR_DEVICELOST)
		{
			m_bDeviceIsLost = true;
			MsgWarning("Restoring failed due to device lost.\n");
		}
		else if(hr == D3DERR_INVALIDCALL)
		{
			m_bDeviceIsLost = true;
			MsgWarning("Restoring failed -  D3DERR_INVALIDCALL\n");
		}
		else
			MsgWarning("Restoring failed (%d)\n", hr);

		return false;
	}

	if(m_bDeviceAtReset)
	{
		DevMsg(DEVMSG_SHADERAPI, "Restoring states...\n");

		m_bDeviceIsLost = false;

		m_pCurrentShader = NULL;
		m_pCurrentBlendstate = NULL;
		m_pCurrentDepthState = NULL;
		m_pCurrentRasterizerState = NULL;

		m_pCurrentShader = NULL;
		m_pSelectedShader = NULL;

		m_pSelectedBlendstate = NULL;
		m_pSelectedDepthState = NULL;
		m_pSelectedRasterizerState = NULL;

		// VF selectoin
		m_pSelectedVertexFormat = NULL;
		m_pCurrentVertexFormat = NULL;

		// Index buffer
		m_pSelectedIndexBuffer = NULL;
		m_pCurrentIndexBuffer = NULL;

		// Vertex buffer
		memset(m_pSelectedVertexBuffers, 0, sizeof(m_pSelectedVertexBuffers));
		memset(m_pCurrentVertexBuffers, 0, sizeof(m_pCurrentVertexBuffers));

		memset(m_pActiveVertexFormat, 0, sizeof(m_pActiveVertexFormat));

		memset(m_nCurrentOffsets, 0, sizeof(m_nCurrentOffsets));
		memset(m_nSelectedOffsets, 0, sizeof(m_nSelectedOffsets));

		// Index buffer
		m_pSelectedIndexBuffer = NULL;
		m_pCurrentIndexBuffer = NULL;

		memset(m_pSelectedTextures, 0, sizeof(m_pSelectedTextures));
		memset(m_pCurrentTextures, 0, sizeof(m_pCurrentTextures));

		m_nCurrentSrcFactor = BLENDFACTOR_ONE;
		m_nCurrentDstFactor = BLENDFACTOR_ZERO;
		m_nCurrentBlendMode = BLENDFUNC_ADD;

		m_nCurrentDepthFunc = COMP_LEQUAL;
		m_bCurrentDepthTestEnable = false;
		m_bCurrentDepthWriteEnable = false;

		m_bCurrentMultiSampleEnable = false;
		m_bCurrentScissorEnable = false;
		m_nCurrentCullMode = CULL_BACK;
		m_nCurrentFillMode = FILL_SOLID;

		m_nCurrentMask = COLORMASK_ALL;
		m_bCurrentBlendEnable = false;
		m_bCurrentAlphaTestEnabled = false;
		m_fCurrentAlphaTestRef = 0.9f;

		//m_nCurrentSampleMask = ~0;
		m_nSelectedSampleMask = ~0;

		// Set some of my preferred defaults
		m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
		m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		memset(m_vsRegs, 0, sizeof(m_vsRegs));
		memset(m_psRegs, 0, sizeof(m_psRegs));

		memset(m_pSelectedSamplerStates, 0, sizeof(m_pSelectedSamplerStates));
		memset(m_pCurrentSamplerStates, 0, sizeof(m_pCurrentSamplerStates));

		memset(m_pSelectedTextures, 0, sizeof(m_pSelectedTextures));
		memset(m_pCurrentTextures, 0, sizeof(m_pCurrentTextures));

		memset(m_pCurrentColorRenderTargets, NULL, sizeof(m_pCurrentColorRenderTargets));
		memset(m_nCurrentCRTSlice, 0, sizeof(m_nCurrentCRTSlice));

		Reset();
		Apply();

		DevMsg(DEVMSG_SHADERAPI, "Restoring VBs...\n");
		for (int i = 0; i < m_VBList.numElem(); i++)
		{
			CVertexBufferD3DX9* pVB = (CVertexBufferD3DX9*)m_VBList[i];

			pVB->Restore();
		}

		DevMsg(DEVMSG_SHADERAPI, "Restoring IBs...\n");
		for (int i = 0; i < m_IBList.numElem(); i++)
		{
			CIndexBufferD3DX9* pIB = (CIndexBufferD3DX9*)m_IBList[i];

			pIB->Restore();
		}

		DevMsg(DEVMSG_SHADERAPI, "Restoring query...\n");
		for (int i = 0; i < m_OcclusionQueryList.numElem(); i++)
		{
			CD3D9OcclusionQuery* query = (CD3D9OcclusionQuery*)m_OcclusionQueryList[i];
			query->Init();
		}

		m_pD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pEventQuery);

		DevMsg(DEVMSG_SHADERAPI, "Restoring RTs...\n");

		// create texture surfaces
		for (auto it = m_TextureList.begin(); it != m_TextureList.end(); ++it)
		{
			CD3D9Texture* pTex = (CD3D9Texture*)*it;

			if(pTex->GetFlags() & TEXFLAG_FOREIGN)
				continue;

			bool is_rendertarget = (pTex->GetFlags() & TEXFLAG_RENDERTARGET);
			bool is_managed = (pTex->GetFlags() & TEXFLAG_MANAGED);

			// restore unmanaged texture
			if (!is_managed && !is_rendertarget)
			{
				DevMsg(DEVMSG_SHADERAPI, "Restoring texture %s\n", pTex->GetName());
				RestoreTextureInternal(pTex);
			}
			else if (!is_managed && is_rendertarget)
			{
				DevMsg(DEVMSG_SHADERAPI, "Restoring rentertarget %s\n", pTex->GetName());
				InternalCreateRenderTarget(m_pD3DDevice, pTex, pTex->GetFlags(), m_caps);
			}
		}

		DevMsg(DEVMSG_SHADERAPI, "Restoring backbuffer...\n");

		// this is a last operation because we
		CreateD3DFrameBufferSurfaces();

		m_bDeviceAtReset = false;
	}

	return true;
}

bool ShaderAPID3DX9::CreateD3DFrameBufferSurfaces()
{
	m_pCurrentDepthSurface = NULL;

	if(!m_fbColorTexture)
	{
		m_fbColorTexture = new CD3D9Texture();
		m_fbColorTexture->SetName("rhi_fb_color");
		m_fbColorTexture->SetDimensions(0, 0);
		m_fbColorTexture->SetFlags(TEXFLAG_RENDERTARGET | TEXFLAG_FOREIGN | TEXFLAG_NOQUALITYLOD);
		m_fbColorTexture->Ref_Grab();

		CScopedMutex m(m_Mutex);
		ASSERTMSG(m_TextureList.find(m_fbColorTexture->m_nameHash) == m_TextureList.end(), "Texture %s was already added", m_fbColorTexture->GetName());
		m_TextureList.insert(m_fbColorTexture->m_nameHash, m_fbColorTexture);
	}

	if (!m_fbDepthTexture)
	{
		m_fbDepthTexture = new CD3D9Texture();
		m_fbDepthTexture->SetName("rhi_fb_depth");
		m_fbDepthTexture->SetDimensions(0, 0);
		m_fbDepthTexture->SetFlags(TEXFLAG_RENDERDEPTH | TEXFLAG_FOREIGN | TEXFLAG_NOQUALITYLOD);
		m_fbDepthTexture->Ref_Grab();

		CScopedMutex m(m_Mutex);
		ASSERTMSG(m_TextureList.find(m_fbDepthTexture->m_nameHash) == m_TextureList.end(), "Texture %s was already added", m_fbDepthTexture->GetName());
		m_TextureList.insert(m_fbDepthTexture->m_nameHash, m_fbDepthTexture);
	}

	IDirect3DSurface9* fbColorSurface;
	if (m_pD3DDevice->GetRenderTarget(0, &fbColorSurface) != D3D_OK)
		return false;

	m_fbColorTexture->surfaces.setNum(1);
	m_fbColorTexture->surfaces[0] = fbColorSurface;

	IDirect3DSurface9* fbDepthSurface;
	if (m_pD3DDevice->GetDepthStencilSurface(&fbDepthSurface) != D3D_OK)
		return false;

	m_fbDepthTexture->surfaces.setNum(1);
	m_fbDepthTexture->surfaces[0] = fbDepthSurface;

	return true;
}

void ShaderAPID3DX9::ReleaseD3DFrameBufferSurfaces()
{
	m_fbColorTexture->surfaces[0]->Release();
	m_fbColorTexture->surfaces.clear();

	m_fbDepthTexture->surfaces[0]->Release();
	m_fbDepthTexture->surfaces.clear();
}

// Init + Shurdown
void ShaderAPID3DX9::Init( shaderAPIParams_t &params )
{
	m_bDeviceIsLost = false;
	m_bDeviceAtReset = false;

	CreateD3DFrameBufferSurfaces();

	m_nCurrentMatrixMode = D3DTS_VIEW;

	// Set some of my preferred defaults
	m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	m_pD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pEventQuery);

	// set the anisotropic level
	if (m_caps.maxTextureAnisotropicLevel > 1)
	{
		for (int i = 0; i < m_caps.maxTextureUnits; i++)
			m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, m_caps.maxTextureAnisotropicLevel);
	}

	const char *vsprofile = D3DXGetVertexShaderProfile(m_pD3DDevice);
	const char *psprofile = D3DXGetPixelShaderProfile(m_pD3DDevice);

	MsgAccept(" \n*Max pixel shader profile: %s\n*Max vertex shader profile: %s\n",psprofile,vsprofile);

	ShaderAPI_Base::Init(params);
}

void ShaderAPID3DX9::PrintAPIInfo()
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

	int allTexturesSize = 0;

	CScopedMutex scoped(m_Mutex);
	for (auto it = m_TextureList.begin(); it != m_TextureList.end(); ++it)
	{
		CD3D9Texture* pTexture = (CD3D9Texture*)*it;

		ETextureFormat texFmt = pTexture->GetFormat();

		float textureSize = 0;
		
		if(IsCompressedFormat(texFmt))
			textureSize = pTexture->m_texSize;
		else
			textureSize = pTexture->GetWidth() * pTexture->GetHeight() * pTexture->GetMipCount() * GetBytesPerPixel(texFmt);

		allTexturesSize += textureSize / 1024;

		MsgInfo("     %s (%d) - %dx%d (~%.2f kb)\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(),pTexture->GetHeight(), (textureSize / 1024.0f));
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

void ShaderAPID3DX9::SetViewport(int x, int y, int w, int h)
{
	D3DVIEWPORT9 vp = { x, y, w, h, 0.0f, 1.0f };
	m_pD3DDevice->SetViewport(&vp);
}

void ShaderAPID3DX9::GetViewport(int &x, int &y, int &w, int &h)
{
	D3DVIEWPORT9 vp;
	m_pD3DDevice->GetViewport(&vp);

	x = vp.X;
	y = vp.Y;
	w = vp.Width;
	h = vp.Height;
}

bool ShaderAPID3DX9::IsDeviceActive()
{
	return !m_bDeviceIsLost;
}

void ShaderAPID3DX9::Shutdown()
{
	ReleaseD3DFrameBufferSurfaces();
	ShaderAPI_Base::Shutdown();

	if(m_pEventQuery)
		m_pEventQuery->Release();

	m_pEventQuery = NULL;
}

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------
void ShaderAPID3DX9::Reset(int nResetType)
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

void ShaderAPID3DX9::ApplyTextures()
{
	for(int i = 0; i < MAX_TEXTUREUNIT; i++)
	{
		CD3D9Texture* pTexture = (CD3D9Texture*)m_pSelectedTextures[i];
		if (pTexture != m_pCurrentTextures[i])
		{
			if (pTexture == NULL)
			{
				m_pD3DDevice->SetTexture(i, NULL);
				m_pSelectedSamplerStates[i] = NULL;
			} 
			else 
			{
#ifdef EQ_DEBUG
				if (pTexture->textures.numElem() == 0)
				{
					ASSERTMSG(false, "D3D9 renderer error: texture has no surfaces\n");
				}
#endif

				m_pD3DDevice->SetTexture(i, pTexture->GetCurrentTexture());
				m_pSelectedSamplerStates[i] = (SamplerStateParam_t*)&pTexture->GetSamplerState();

				// changed texture means changed sampler state
				m_nCurrentSamplerStateDirty |= (1 << i);
			}

			m_pCurrentTextures[i] = pTexture;
		}
	}

	for(int i = 0; i < m_caps.maxVertexTextureUnits; i++)
	{
		CD3D9Texture* pTexture = (CD3D9Texture*)m_pSelectedVertexTextures[i];
		if (pTexture != m_pCurrentVertexTextures[i])
		{
			if (pTexture == NULL)
			{
				m_pD3DDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+i, NULL);
				m_pSelectedVertexSamplerStates[i] = NULL;
			} 
			else 
			{
				m_pD3DDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+i, pTexture->GetCurrentTexture());
				m_pSelectedVertexSamplerStates[i] = (SamplerStateParam_t*)&pTexture->GetSamplerState();

				// changed texture means changed sampler state
				m_nCurrentVertexSamplerStateDirty |= (1 << i);
			}

			m_pCurrentVertexTextures[i] = pTexture;
		}
	}
}

void ShaderAPID3DX9::ApplySamplerState()
{
	for (int i = 0; i < MAX_SAMPLERSTATE; i++)
	{
		SamplerStateParam_t* pSelectedSamplerState = m_pSelectedSamplerStates[i];

		if (m_nCurrentSamplerStateDirty & (1 << i))
		{
			SamplerStateParam_t &ss = pSelectedSamplerState ? *pSelectedSamplerState : m_defaultSamplerState;
			SamplerStateParam_t &css = m_pCurrentSamplerStates[i];

			if (ss.minFilter != css.minFilter)
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MINFILTER, d3dFilterType[css.minFilter = ss.minFilter]);

			if (ss.magFilter != css.magFilter)
			{
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, d3dFilterType[css.magFilter = ss.magFilter]);
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, d3dFilterType[ss.magFilter]);	// FIXME: separate selector for MIP?
			}

			if (ss.wrapS != css.wrapS) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSU, d3dAddressMode[css.wrapS = ss.wrapS]);
			if (ss.wrapT != css.wrapT) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSV, d3dAddressMode[css.wrapT = ss.wrapT]);
			if (ss.wrapR != css.wrapR) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSW, d3dAddressMode[css.wrapR = ss.wrapR]);

			if (ss.aniso != css.aniso) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, css.aniso = ss.aniso);

			if (ss.lod != css.lod) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *(DWORD *) &(css.lod = ss.lod));
		}
	}
	m_nCurrentSamplerStateDirty = 0;

	// Vertex texture samplers
	for (int i = 0; i < MAX_VERTEXTEXTURES; i++)
	{
		SamplerStateParam_t* pSelectedSamplerState = m_pSelectedVertexSamplerStates[i];

		if (m_nCurrentVertexSamplerStateDirty & (1 << i))
		{
			SamplerStateParam_t &ss = pSelectedSamplerState ? *pSelectedSamplerState : m_defaultSamplerState;
			SamplerStateParam_t &css = m_pCurrentVertexSamplerStates[i];

			if (ss.minFilter != css.minFilter)
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MINFILTER, d3dFilterType[css.minFilter = ss.minFilter]);

			if (ss.magFilter != css.magFilter)
			{
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, d3dFilterType[css.magFilter = ss.magFilter]);
				m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, d3dFilterType[ss.magFilter]);	// FIXME: separate selector for MIP?
			}

			if (ss.wrapS != css.wrapS) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSU, d3dAddressMode[css.wrapS = ss.wrapS]);
			if (ss.wrapT != css.wrapT) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSV, d3dAddressMode[css.wrapT = ss.wrapT]);
			if (ss.wrapR != css.wrapR) m_pD3DDevice->SetSamplerState(i, D3DSAMP_ADDRESSW, d3dAddressMode[css.wrapR = ss.wrapR]);

			if (ss.aniso != css.aniso) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, css.aniso = ss.aniso);

			if (ss.lod != css.lod) m_pD3DDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&(css.lod = ss.lod));
		}
	}
	m_nCurrentVertexSamplerStateDirty = 0;
}

void ShaderAPID3DX9::ApplyBlendState()
{
	CD3D9BlendingState* pSelectedState = (CD3D9BlendingState*)m_pSelectedBlendstate;

	int mask = COLORMASK_ALL;
	bool blendingEnabled = pSelectedState != NULL && pSelectedState->m_params.blendEnable;

	// switch the blending on/off
	if (m_bCurrentBlendEnable != blendingEnabled)
	{
		m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD)blendingEnabled);
		m_bCurrentBlendEnable = blendingEnabled;
	}

	if(pSelectedState != NULL)
	{
		BlendStateParam_t& state = pSelectedState->m_params;

		// enable alphatest
		if (state.alphaTest != m_bCurrentAlphaTestEnabled)
		{
			m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, (DWORD)state.alphaTest);
			m_pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			m_bCurrentAlphaTestEnabled = state.alphaTest;
		}

		if (state.alphaTestRef != m_fCurrentAlphaTestRef)
		{
			m_pD3DDevice->SetRenderState(D3DRS_ALPHAREF, (DWORD)(255.0f*state.alphaTestRef));
			m_fCurrentAlphaTestRef = state.alphaTestRef;
		}

		// handle blending params if blending is enabled
		if (state.blendEnable)
		{
			if (state.srcFactor != m_nCurrentSrcFactor)
			{
				m_nCurrentSrcFactor = state.srcFactor;
				m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, blendingConsts[state.srcFactor]);
			}

			if (state.dstFactor != m_nCurrentDstFactor)
			{
				m_nCurrentDstFactor = state.dstFactor;
				m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, blendingConsts[state.dstFactor]);
			}
			if (state.blendFunc != m_nCurrentBlendMode)
			{
				m_nCurrentBlendMode = state.blendFunc;
				m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, blendingModes[state.blendFunc]);
			}
		}

		mask = state.mask;
	}
	else
	{
		// disable alpha testing
		if(m_bCurrentAlphaTestEnabled)
			m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, m_bCurrentAlphaTestEnabled = false);
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

void ShaderAPID3DX9::ApplyDepthState()
{
	CD3D9DepthStencilState* pSelectedState = (CD3D9DepthStencilState*)m_pSelectedDepthState;

	if (pSelectedState == NULL)
	{
		if (!m_bCurrentDepthTestEnable)
		{
			m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, m_bCurrentDepthTestEnable = true);
		}

		if (!m_bCurrentDepthWriteEnable)
		{
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, m_bCurrentDepthWriteEnable = true);
		}

		if (m_nCurrentDepthFunc != COMP_LESS)
			m_pD3DDevice->SetRenderState(D3DRS_ZFUNC, depthConst[m_nCurrentDepthFunc = COMP_LESS]);

		if (m_bDoStencilTest != false)
			m_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, m_bDoStencilTest = false);
	} 
	else 
	{
		DepthStencilStateParams_t& state = pSelectedState->m_params;

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
				m_pD3DDevice->SetRenderState(D3DRS_ZFUNC, depthConst[m_nCurrentDepthFunc = state.depthFunc]);
		
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
					m_pD3DDevice->SetRenderState(D3DRS_STENCILFUNC, stencilConst[m_nStencilFunc = state.nStencilFunc]);

				if(m_nStencilFail != state.nStencilFail)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILFAIL, stencilConst[m_nStencilFail = state.nStencilFail]);

				if(m_nStencilFunc != state.nStencilFunc)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILREF, depthConst[m_nStencilFunc = state.nStencilFunc]);

				if(m_nStencilPass != state.nStencilPass)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILPASS, stencilConst[m_nStencilPass = state.nStencilPass]);

				if(m_nDepthFail != state.nDepthFail)
					m_pD3DDevice->SetRenderState(D3DRS_STENCILZFAIL, stencilConst[m_nDepthFail = state.nDepthFail]);
			}
		}
	}

	m_pCurrentDepthState = pSelectedState;
}

void ShaderAPID3DX9::ApplyRasterizerState()
{
	CD3D9RasterizerState* pSelectedState = (CD3D9RasterizerState*)m_pSelectedRasterizerState;

	if (pSelectedState == NULL)
	{
		if (m_nCurrentCullMode != CULL_BACK)
			m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, cullConst[m_nCurrentCullMode = CULL_BACK]);

		if (m_nCurrentFillMode != FILL_SOLID)
			m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, fillConst[m_nCurrentFillMode = FILL_SOLID]);

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
		RasterizerStateParams_t& state = pSelectedState->m_params;

		if (state.cullMode != m_nCurrentCullMode)
			m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, cullConst[m_nCurrentCullMode = state.cullMode]);

		if (state.fillMode != m_nCurrentFillMode)
			m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, fillConst[m_nCurrentFillMode = state.fillMode]);

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

void ShaderAPID3DX9::ApplyShaderProgram()
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)m_pSelectedShader;

	if (pShader != m_pCurrentShader)
	{
		if (pShader == NULL)
		{
			m_pD3DDevice->SetVertexShader(NULL);
			m_pD3DDevice->SetPixelShader(NULL);
		} 
		else 
		{
			m_pD3DDevice->SetVertexShader(pShader->m_pVertexShader);
			m_pD3DDevice->SetPixelShader(pShader->m_pPixelShader);
		}
		m_pCurrentShader = pShader;
	}
}

void ShaderAPID3DX9::ApplyConstants()
{
	{
		int minVSDirty = m_nMinVSDirty;
		int maxVSDirty = m_nMaxVSDirty;

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
		int minPSDirty = m_nMinPSDirty;
		int maxPSDirty = m_nMaxPSDirty;

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

void ShaderAPID3DX9::Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const ColorRGBA &fillColor,float fDepth, int nStencil)
{
	// clear the back buffer
	m_pD3DDevice->Clear(0, NULL, (bClearColor ? D3DCLEAR_TARGET : 0) | (bClearDepth ? D3DCLEAR_ZBUFFER : 0) | (bClearStencil ? D3DCLEAR_STENCIL : 0),
		D3DCOLOR_ARGB((int)(fillColor.w*255), (int)(fillColor.x*255),(int)(fillColor.y*255),(int)(fillColor.z*255)), fDepth, nStencil);
}

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

// Device vendor and version
const char* ShaderAPID3DX9::GetDeviceNameString() const
{
	return "malfunction";
}

// Renderer string (ex: OpenGL, D3D9)
const char* ShaderAPID3DX9::GetRendererName() const
{
	return "Direct3D9";
}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

// Synchronization
void ShaderAPID3DX9::Flush()
{
	LPDIRECT3DQUERY9 query = m_pEventQuery;

	if(!query)
		return;

	query->Issue(D3DISSUE_END);
	query->GetData(NULL, 0, D3DGETDATA_FLUSH);
}

void ShaderAPID3DX9::Finish()
{
	LPDIRECT3DQUERY9 query = m_pEventQuery;

	if(!query)
		return;

	query->Issue(D3DISSUE_END);

	while (query->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
	{
		// Spin-wait
	}
}

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

// creates occlusion query object
IOcclusionQuery* ShaderAPID3DX9::CreateOcclusionQuery()
{
	CD3D9OcclusionQuery* occQuery = new CD3D9OcclusionQuery(m_pD3DDevice);

	m_Mutex.Lock();
	m_OcclusionQueryList.append( occQuery );
	m_Mutex.Unlock();

	return occQuery;
}

// removal of occlusion query object
void ShaderAPID3DX9::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
	if(pQuery)
		delete pQuery;

	m_Mutex.Lock();
	m_OcclusionQueryList.fastRemove( pQuery );
	m_Mutex.Unlock();
}

//-------------------------------------------------------------
// State manipulation 
//-------------------------------------------------------------

// creates blending state
IRenderState* ShaderAPID3DX9::CreateBlendingState( const BlendStateParam_t &blendDesc )
{
	CD3D9BlendingState* pState = NULL;

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

	pState = new CD3D9BlendingState;
	pState->m_params = blendDesc;

	m_BlendStates.append(pState);

	pState->AddReference();

	return pState;
}
	
// creates depth/stencil state
IRenderState* ShaderAPID3DX9::CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc )
{
	CD3D9DepthStencilState* pState = NULL;
	
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
	
	pState = new CD3D9DepthStencilState;
	pState->m_params = depthDesc;

	m_DepthStates.append(pState);

	pState->AddReference();

	return pState;
}

// creates rasterizer state
IRenderState* ShaderAPID3DX9::CreateRasterizerState( const RasterizerStateParams_t &rasterDesc )
{
	CD3D9RasterizerState* pState = NULL;

	for(int i = 0; i < m_RasterizerStates.numElem(); i++)
	{
		pState = (CD3D9RasterizerState*)m_RasterizerStates[i];

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

	pState = new CD3D9RasterizerState;
	pState->m_params = rasterDesc;

	pState->AddReference();

	m_RasterizerStates.append(pState);

	return pState;
}

// completely destroys shader
void ShaderAPID3DX9::DestroyRenderState( IRenderState* pState, bool removeAllRefs)
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

// Unload the texture and free the memory
void ShaderAPID3DX9::FreeTexture(ITexture* pTexture)
{
	CD3D9Texture* pTex = (CD3D9Texture*)(pTexture);

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
		{
			auto it = m_TextureList.find(pTex->m_nameHash);
			if (it != m_TextureList.end())
			{
				deleted = true;
				m_TextureList.remove(it);
			}
		}
	}

	if (deleted)
	{
		DevMsg(DEVMSG_SHADERAPI, "Texture unloaded: %s\n", pTexture->GetName());
		delete pTex;
	}
}

static LPDIRECT3DSURFACE9* CreateSurfaces(int num)
{
	return new LPDIRECT3DSURFACE9[num];
}

bool ShaderAPID3DX9::InternalCreateRenderTarget(LPDIRECT3DDEVICE9 dev, CD3D9Texture *tex, int nFlags, const ShaderAPICaps_t& caps)
{
	if (caps.INTZSupported && caps.INTZFormat == tex->GetFormat())
	{
		LPDIRECT3DBASETEXTURE9 pTexture = NULL;

		tex->usage = D3DUSAGE_DEPTHSTENCIL;
		tex->m_pool = D3DPOOL_DEFAULT;

		DevMsg(DEVMSG_SHADERAPI, "InternalCreateRenderTarget: creating INTZ render target single texture for %s\n", tex->GetName());
		if (dev->CreateTexture(tex->GetWidth(), tex->GetHeight(), tex->GetMipCount(), tex->usage, formats[tex->GetFormat()], tex->m_pool, (LPDIRECT3DTEXTURE9*)&pTexture, NULL) != D3D_OK)
		{
			MsgError("!!! Couldn't create '%s' INTZ render target with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
			ASSERT(!"Couldn't create INTZ render target");
			return false;
		}

		tex->textures.append(pTexture);

		LPDIRECT3DSURFACE9 pSurface = NULL;

		HRESULT hr = ((LPDIRECT3DTEXTURE9)tex->textures[0])->GetSurfaceLevel(0, &pSurface);
		if (!FAILED(hr))
			tex->surfaces.append(pSurface);
	}
	else if (IsDepthFormat(tex->GetFormat()))
	{
		DevMsg(DEVMSG_SHADERAPI, "InternalCreateRenderTarget: creating depth/stencil surface for %s\n", tex->GetName());

		LPDIRECT3DSURFACE9 pSurface = NULL;

		tex->m_pool = D3DPOOL_DEFAULT;

		if (dev->CreateDepthStencilSurface( tex->GetWidth(), tex->GetHeight(), formats[tex->GetFormat()], D3DMULTISAMPLE_NONE, 0, FALSE, &pSurface, NULL) != D3D_OK )
		{
			MsgError("!!! Couldn't create create '%s' depth surface with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
			ASSERT(!"Couldn't create depth surface");
			return false;
		}

		tex->surfaces.append(pSurface);
	}
	else 
	{
		if(nFlags & TEXFLAG_RENDERDEPTH)
		{
			DevMsg(DEVMSG_SHADERAPI, "InternalCreateRenderTarget: creating depth for %s\n", tex->GetName());
			if (dev->CreateDepthStencilSurface(tex->GetWidth(), tex->GetHeight(), D3DFMT_D16,
				D3DMULTISAMPLE_NONE, 0, TRUE, &tex->m_dummyDepth, NULL) != D3D_OK)
			{
				MsgError("!!! Couldn't create '%s' depth surface for RT with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
				ASSERT(!"Couldn't create depth surface for RT");
				return false;
			}
		}
		
		if (nFlags & TEXFLAG_CUBEMAP)
		{
			tex->m_pool = D3DPOOL_DEFAULT;

			LPDIRECT3DBASETEXTURE9 pTexture = NULL;

			DevMsg(DEVMSG_SHADERAPI, "InternalCreateRenderTarget: creating cubemap target for %s\n", tex->GetName());
			if (dev->CreateCubeTexture(tex->GetWidth(), tex->GetMipCount(), tex->usage, formats[tex->GetFormat()], tex->m_pool, (LPDIRECT3DCUBETEXTURE9 *) &pTexture, NULL) != D3D_OK)
			{
				MsgError("!!! Couldn't create '%s' cubemap render target with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
				ASSERT(!"Couldn't create cubemap render target");
				return false;
			}

			tex->textures.append(pTexture);

			for (uint i = 0; i < 6; i++)
			{
				LPDIRECT3DSURFACE9 pSurface = NULL;

				HRESULT hr = ((LPDIRECT3DCUBETEXTURE9) tex->textures[0])->GetCubeMapSurface((D3DCUBEMAP_FACES) i, 0, &pSurface);

				if(!FAILED(hr))
					tex->surfaces.append(pSurface);
			}
		}
		else 
		{
			LPDIRECT3DBASETEXTURE9 pTexture = NULL;

			tex->m_pool = D3DPOOL_DEFAULT;

			DevMsg(DEVMSG_SHADERAPI, "InternalCreateRenderTarget: creating render target single texture for %s\n", tex->GetName());
			if (dev->CreateTexture(tex->GetWidth(), tex->GetHeight(), tex->GetMipCount(), tex->usage, formats[tex->GetFormat()], tex->m_pool, (LPDIRECT3DTEXTURE9 *) &pTexture, NULL) != D3D_OK)
			{
				MsgError("!!! Couldn't create '%s' render target with size %d %d\n", tex->GetName(), tex->GetWidth(), tex->GetHeight());
				ASSERT(!"Couldn't create render target");
				return false;
			}

			tex->textures.append(pTexture);

			LPDIRECT3DSURFACE9 pSurface = NULL;

			HRESULT hr = ((LPDIRECT3DTEXTURE9) tex->textures[0])->GetSurfaceLevel(0, &pSurface);
			if(!FAILED(hr))
				tex->surfaces.append(pSurface);
		}
	}

	return true;
}

// It will add new rendertarget
ITexture* ShaderAPID3DX9::CreateRenderTarget(int width, int height, ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType, ER_TextureAddressMode textureAddress, ER_CompareFunc comparison, int nFlags)
{
	CD3D9Texture *pTexture = new CD3D9Texture;

	pTexture->SetDimensions(width,height);
	pTexture->SetFormat(nRTFormat);

	pTexture->usage = D3DUSAGE_RENDERTARGET;

	pTexture->SetFlags(nFlags | TEXFLAG_RENDERTARGET);
	pTexture->SetName(EqString::Format("_sapi_rt_%d", m_TextureList.size()).ToCString());

	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);

	pTexture->SetSamplerState(texSamplerParams);

	// do spin wait (if in other thread)
	DEVICE_SPIN_WAIT

	if (InternalCreateRenderTarget(m_pD3DDevice, pTexture, nFlags, m_caps))
	{
		CScopedMutex scoped(m_Mutex);

		ASSERTMSG(m_TextureList.find(pTexture->m_nameHash) == m_TextureList.end(), "Texture %s was already added", pTexture->GetName());
		m_TextureList.insert(pTexture->m_nameHash, pTexture);
		return pTexture;
	} 
	else 
	{
		delete pTexture;
		return NULL;
	}
}

// It will add new rendertarget
ITexture* ShaderAPID3DX9::CreateNamedRenderTarget(const char* pszName,int width, int height,ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType, ER_TextureAddressMode textureAddress, ER_CompareFunc comparison, int nFlags)
{
	CD3D9Texture *pTexture = new CD3D9Texture;

	pTexture->SetDimensions(width,height);
	pTexture->SetFormat(nRTFormat);

	pTexture->usage = D3DUSAGE_RENDERTARGET;

	pTexture->SetFlags(nFlags | TEXFLAG_RENDERTARGET);
	pTexture->SetName(pszName);

	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);

	pTexture->SetSamplerState(texSamplerParams);

	if (InternalCreateRenderTarget(m_pD3DDevice, pTexture, nFlags, m_caps))
	{
		CScopedMutex scoped(m_Mutex);
		ASSERTMSG(m_TextureList.find(pTexture->m_nameHash) == m_TextureList.end(), "Texture %s was already added", pTexture->GetName());
		m_TextureList.insert(pTexture->m_nameHash, pTexture);
		return pTexture;
	} 
	else 
	{
		delete pTexture;
		return NULL;
	}
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

// saves rendertarget to texture, you can also save screenshots
void ShaderAPID3DX9::SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName)
{
	if(pTargetTexture && pTargetTexture->GetFlags() & TEXFLAG_RENDERTARGET)
	{
		CD3D9Texture* pTexture = (CD3D9Texture*)pTargetTexture;

		if(pTexture->GetFlags() & TEXFLAG_CUBEMAP)
			D3DXSaveTextureToFileA(pFileName, D3DXIFF_DDS, pTexture->textures[0], NULL);
		else
			D3DXSaveSurfaceToFileA(pFileName, D3DXIFF_DDS, pTexture->surfaces[0], NULL,NULL);
	}
}

// Copy render target to texture
void ShaderAPID3DX9::CopyFramebufferToTexture(ITexture* pTargetTexture)
{
	CD3D9Texture* dest = (CD3D9Texture*)(pTargetTexture);
	if(!dest)
		return;

	if(dest->textures.numElem() <= 0)
		return;

	LPDIRECT3DSURFACE9 srcSurface;
	HRESULT hr = m_pD3DDevice->GetRenderTarget( 0, &srcSurface );

	if (FAILED(hr))
		return;

	LPDIRECT3DTEXTURE9 destD3DTex = ( LPDIRECT3DTEXTURE9 )dest->textures[0];
	ASSERT( destD3DTex );

	// get target surface to copy
	LPDIRECT3DSURFACE9 destSurface;
	hr = destD3DTex->GetSurfaceLevel( 0, &destSurface );

	ASSERT( !FAILED( hr ) );
	if( FAILED( hr ) )
		return;

	hr = m_pD3DDevice->StretchRect( srcSurface, NULL, destSurface, NULL, D3DTEXF_NONE );
	ASSERT( !FAILED( hr ) );

	destSurface->Release();
	srcSurface->Release();
}

// Copy render target to texture with resizing
void ShaderAPID3DX9::CopyRendertargetToTexture(ITexture* srcTarget, ITexture* destTex, IRectangle* srcRect, IRectangle* destRect)
{
	CD3D9Texture* src = (CD3D9Texture*)(srcTarget);
	CD3D9Texture* dest = (CD3D9Texture*)(destTex);

	if(!src || !dest)
		return;

	if(dest->textures.numElem() <= 0)
		return;

	if(src->surfaces.numElem() <= 0)
		return;

	int numLevels = src->surfaces.numElem();

	LPDIRECT3DTEXTURE9 destD3DTex = ( LPDIRECT3DTEXTURE9 )dest->textures[0];

	RECT dxSrcRect, dxDestRect;

	if(srcRect)
	{
		dxSrcRect.left = srcRect->vleftTop.x;
		dxSrcRect.top = srcRect->vleftTop.y;
		dxSrcRect.right = srcRect->vrightBottom.x;
		dxSrcRect.bottom = srcRect->vrightBottom.y;
	}

	if(destRect)
	{
		dxDestRect.left = destRect->vleftTop.x;
		dxDestRect.top = destRect->vleftTop.y;
		dxDestRect.right = destRect->vrightBottom.x;
		dxDestRect.bottom = destRect->vrightBottom.y;
	}

	bool isCubemap = dest->GetFlags() & TEXFLAG_CUBEMAP;

	for(int i = 0; i < numLevels; i++)
	{
		LPDIRECT3DSURFACE9 srcSurface = src->surfaces[i];

		LPDIRECT3DSURFACE9 destSurface;
		HRESULT hr;
		
		if (isCubemap)
			hr = ((LPDIRECT3DCUBETEXTURE9) destD3DTex)->GetCubeMapSurface((D3DCUBEMAP_FACES) i, 0, &destSurface);
		else
			hr = destD3DTex->GetSurfaceLevel( i, &destSurface );

		if (FAILED(hr))
		{
			Msg("CopyRendertargetToTexture failed to GetSurfaceLevel\n");
			return;
		}

		ASSERT(destSurface);

		hr = m_pD3DDevice->StretchRect( srcSurface, srcRect ? &dxSrcRect : NULL, destSurface, destRect ? &dxDestRect : NULL, D3DTEXF_NONE );
		//ASSERT( !FAILED( hr ) );

		//if(FAILED( hr ))
		//	Msg("CopyRendertargetToTexture error\n");

		destSurface->Release();
	}
}

// Changes render target (MRT)
void ShaderAPID3DX9::ChangeRenderTargets(ITexture** pRenderTargets, int nNumRTs, int* nCubemapFaces, ITexture* pDepthTarget, int nDepthSlice)
{
	for (int i = 0; i < nNumRTs; i++)
	{
		CD3D9Texture* pRenderTarget = (CD3D9Texture*)pRenderTargets[i];

		const int nCubeFace = nCubemapFaces ? nCubemapFaces[i] : 0;

		if (pRenderTarget != m_pCurrentColorRenderTargets[i] || nCubeFace != m_nCurrentCRTSlice[i])
		{
			m_pD3DDevice->SetRenderTarget(i, pRenderTarget->surfaces[nCubeFace]);

			m_pCurrentColorRenderTargets[i] = pRenderTarget;
			m_nCurrentCRTSlice[i] = nCubeFace;
		}
	}

	for (int i = nNumRTs; i < m_caps.maxRenderTargets; i++)
	{
		if (m_pCurrentColorRenderTargets[i] != NULL)
		{
			m_pD3DDevice->SetRenderTarget(i, NULL);
			m_pCurrentColorRenderTargets[i] = NULL;
		}
	}

	LPDIRECT3DSURFACE9 bestDepth = nNumRTs ? ((CD3D9Texture*)pRenderTargets[0])->m_dummyDepth : NULL;

	if (pDepthTarget != m_pCurrentDepthRenderTarget)
	{
		CD3D9Texture* pDepthRenderTarget = (CD3D9Texture*)(pDepthTarget);

		if (pDepthRenderTarget)
			bestDepth = pDepthRenderTarget->surfaces[0 /*nDepthSlice ??? */];

		m_pCurrentDepthRenderTarget = pDepthRenderTarget;
	}

	if(!bestDepth)
		bestDepth = nullptr;

	if (m_pCurrentDepthSurface != bestDepth)
	{
		m_pD3DDevice->SetDepthStencilSurface(bestDepth);
		m_pCurrentDepthSurface = bestDepth;
	}
}

// Changes back to backbuffer
void ShaderAPID3DX9::ChangeRenderTargetToBackBuffer()
{
	// we can do it simplier, but there is a lack in depth, so keep an old this method...
	
	if (m_pCurrentColorRenderTargets[0] != NULL)
	{
		m_pD3DDevice->SetRenderTarget(0, m_fbColorTexture->surfaces[0]);
		m_pCurrentColorRenderTargets[0] = NULL;
	}

	for (int i = 1; i < m_caps.maxRenderTargets; i++)
	{
		if (m_pCurrentColorRenderTargets[i] != NULL)
		{
			m_pD3DDevice->SetRenderTarget(i, NULL);
			m_pCurrentColorRenderTargets[i] = NULL;
		}
	}

	if (m_pCurrentDepthSurface != NULL)
	{
		m_pD3DDevice->SetDepthStencilSurface(m_fbDepthTexture->surfaces[0]);
		m_pCurrentDepthSurface = NULL;
	}
}

// resizes render target
void ShaderAPID3DX9::ResizeRenderTarget(ITexture* pRT, int newWide, int newTall)
{
	if(pRT->GetWidth() == newWide && pRT->GetHeight() == newTall)
		return;

	CD3D9Texture* pRenderTarget = (CD3D9Texture*)(pRT);

	pRenderTarget->Release();

	pRenderTarget->SetDimensions(newWide, newTall);

	InternalCreateRenderTarget(m_pD3DDevice, pRenderTarget, pRenderTarget->GetFlags(), m_caps);
}


// fills the current rendertarget buffers
void ShaderAPID3DX9::GetCurrentRenderTargets(ITexture* pRenderTargets[MAX_MRTS], int *numRTs, ITexture** pDepthTarget, int cubeNumbers[MAX_MRTS])
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

			if(m_pCurrentColorRenderTargets[i] == NULL)
				break;
		}
	}

	if(pDepthTarget)
		*pDepthTarget = m_pCurrentDepthRenderTarget;

	*numRTs = nRts;
}

// returns current size of backbuffer surface
void ShaderAPID3DX9::GetViewportDimensions(int &wide, int &tall)
{
	D3DVIEWPORT9 vp;
	m_pD3DDevice->GetViewport(&vp);

	wide = vp.Width;
	tall = vp.Height;
}

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

// Matrix mode
void ShaderAPID3DX9::SetMatrixMode(ER_MatrixMode nMatrixMode)
{
	m_nCurrentMatrixMode = d3dmatrixmodes[nMatrixMode];
}

// Will save matrix
void ShaderAPID3DX9::PushMatrix()
{
	// TODO: implement!
}

// Will reset matrix
void ShaderAPID3DX9::PopMatrix()
{
	// TODO: implement!
}

// Load identity matrix
void ShaderAPID3DX9::LoadIdentityMatrix()
{
	// It's may be invalid
	D3DXMATRIX Identity;
	D3DXMatrixIdentity(&Identity);

	m_pD3DDevice->SetTransform(m_nCurrentMatrixMode,&Identity);
}

// Load custom matrix
void ShaderAPID3DX9::LoadMatrix(const Matrix4x4 &matrix)
{
	m_pD3DDevice->SetTransform(m_nCurrentMatrixMode,(D3DXMATRIX*)(const float *)transpose(matrix));
}

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

// Set Depth range for next primitives
void ShaderAPID3DX9::SetDepthRange(float fZNear,float fZFar)
{
	D3DVIEWPORT9 view;
	m_pD3DDevice->GetViewport(&view);
	view.MinZ = fZNear;
	view.MaxZ = fZFar;
	m_pD3DDevice->SetViewport(&view);
}

// sets scissor rectangle
void ShaderAPID3DX9::SetScissorRectangle( const IRectangle &rect )
{
	RECT scissorRect;
	scissorRect.left = rect.vleftTop.x;
	scissorRect.top = rect.vleftTop.y;
	scissorRect.right = rect.vrightBottom.x;
	scissorRect.bottom = rect.vrightBottom.y;

	m_pD3DDevice->SetScissorRect(&scissorRect);
}

// Changes the vertex format
void ShaderAPID3DX9::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	CVertexFormatD3DX9* pFormat = (CVertexFormatD3DX9*)pVertexFormat;

	if (pFormat != m_pCurrentVertexFormat)
	{
		if (pFormat != NULL)
		{
			m_pD3DDevice->SetVertexDeclaration(pFormat->m_pVertexDecl);

			CVertexFormatD3DX9* pCurrentFormat = (CVertexFormatD3DX9*)m_pCurrentVertexFormat;
			if (pCurrentFormat != NULL)
			{
				for (int i = 0; i < MAX_VERTEXSTREAM; i++)
				{
					if (pFormat->m_streamStride[i] != pCurrentFormat->m_streamStride[i])
						m_pCurrentVertexBuffers[i] = NULL;
				}
			}
		}

		m_pCurrentVertexFormat = pFormat;
	}
}

// Changes the vertex buffer
void ShaderAPID3DX9::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset)
{
	UINT nStreamParam1 = 1;
	UINT nStreamParam2 = 1;

	CVertexBufferD3DX9* pVB = (CVertexBufferD3DX9*)(pVertexBuffer);

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
		if (pVB == NULL)
			m_pD3DDevice->SetStreamSource(nStream, NULL, 0, 0 );
		else 
			m_pD3DDevice->SetStreamSource(nStream, pVB->m_pVertexBuffer, (UINT)offset*pVB->GetStrideSize(), pVB->GetStrideSize());

		m_pCurrentVertexBuffers[nStream] = pVertexBuffer;
		m_nCurrentOffsets[nStream] = offset;
	}
}

// Changes the index buffer
void ShaderAPID3DX9::ChangeIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	if (pIndexBuffer != m_pCurrentIndexBuffer)
	{
		CIndexBufferD3DX9* pIB = (CIndexBufferD3DX9*)(pIndexBuffer);

		if (pIB == NULL)
			m_pD3DDevice->SetIndices(NULL);
		else
			m_pD3DDevice->SetIndices(pIB->m_pIndexBuffer);

		m_pCurrentIndexBuffer = pIndexBuffer;
	}
}

// Destroy vertex format
void ShaderAPID3DX9::DestroyVertexFormat(IVertexFormat* pFormat)
{
	CVertexFormatD3DX9* pVF = (CVertexFormatD3DX9*)(pFormat);
	if(!pVF)
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
void ShaderAPID3DX9::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	CVertexBufferD3DX9* pVB = (CVertexBufferD3DX9*)(pVertexBuffer);
	if(!pVB)
		return;

	bool deleted = false;
	{
		CScopedMutex m(m_Mutex);
		deleted = m_VBList.remove(pVB);
	}

	if(deleted)
	{
		// reset if in use
		DevMsg(DEVMSG_SHADERAPI, "Destroying vertex buffer\n");
		delete pVB;
	}
}

// Destroy index buffer
void ShaderAPID3DX9::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	CIndexBufferD3DX9* pIB = (CIndexBufferD3DX9*)(pIndexBuffer);

	if(!pIB)
		return;

	bool deleted = false;
	{
		CScopedMutex m(m_Mutex);
		deleted = m_IBList.remove(pIB);
	}

	if (deleted)
	{
		DevMsg(DEVMSG_SHADERAPI, "Destroying index buffer\n");
		delete pIB;
	}
}

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------


// Creates shader class for needed ShaderAPI
IShaderProgram* ShaderAPID3DX9::CreateNewShaderProgram(const char* pszName, const char* query)
{
	CD3D9ShaderProgram* pNewProgram = new CD3D9ShaderProgram();
	pNewProgram->SetName((_Es(pszName)+query).GetData());

	CScopedMutex scoped(m_Mutex);

	ASSERTMSG(m_ShaderList.find(pNewProgram->m_nameHash) == m_ShaderList.end(), "Shader %s was already added", pNewProgram->GetName());
	m_ShaderList.insert(pNewProgram->m_nameHash, pNewProgram);

	return pNewProgram;
}

// Destroy all shader
void ShaderAPID3DX9::DestroyShaderProgram(IShaderProgram* pShaderProgram)
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(pShaderProgram);

	if(!pShader)
		return;

	bool deleted = false;
	{
		CScopedMutex m(m_Mutex);
		pShader->Ref_Drop(); // decrease references to this shader

		// remove it if reference is zero
		if (pShader->Ref_Count() <= 0)
		{
			auto it = m_ShaderList.find(pShader->m_nameHash);
			if (it != m_ShaderList.end())
			{
				deleted = true;
				m_ShaderList.remove(it);
			}
		}
	}

	if (deleted)
		delete pShader;
}

int SamplerComp(const void *s0, const void *s1)
{
	return strcmp(((DX9Sampler_t*) s0)->name, ((DX9Sampler_t*) s1)->name);
}

int ConstantComp(const void *s0, const void *s1)
{
	return strcmp(((DX9ShaderConstant *) s0)->name, ((DX9ShaderConstant *) s1)->name);
}

ConVar r_skipShaderCache("r_skipShaderCache", "0", "Shader debugging purposes", 0);

struct shaderCacheHdr_t
{
	int		ident;			// EQSC

	long	checksum;		// file crc32

	int		psSize;
	int		vsSize;

	int		numConstants;
	int		numSamplers;
};

#define SHADERCACHE_IDENT		MCHAR4('E','Q','S','C')

// Load any shader from stream
bool ShaderAPID3DX9::CompileShadersFromStream(	IShaderProgram* pShaderOutput,
												const shaderProgramCompileInfo_t& info,
												const char* extra)
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(pShaderOutput);

	if(!pShader)
		return false;

	g_fileSystem->MakeDir("ShaderCache_DX9", SP_ROOT);

	EqString cache_file_name(EqString::Format("ShaderCache_DX9/%s.scache", pShaderOutput->GetName()));

	IFile* pStream = NULL;

	bool needsCompile = true;

	if(!(info.disableCache || r_skipShaderCache.GetBool()))
	{
		pStream = g_fileSystem->Open(cache_file_name.GetData(), "rb", SP_ROOT);

		if(pStream)
		{
			// read pixel shader
			shaderCacheHdr_t scHdr;
			pStream->Read(&scHdr, 1, sizeof(shaderCacheHdr_t));

			if(	scHdr.ident == SHADERCACHE_IDENT &&
				scHdr.checksum == info.data.checksum)
			{
				// read vertex shader
				ubyte* pShaderMem = (ubyte*)malloc(scHdr.vsSize);
				pStream->Read(pShaderMem, 1, scHdr.vsSize);
		
				m_pD3DDevice->CreateVertexShader((DWORD *) pShaderMem, &pShader->m_pVertexShader);
				free(pShaderMem);

				// read pixel shader
				pShaderMem = (ubyte*)malloc(scHdr.psSize);
				pStream->Read(pShaderMem, 1, scHdr.psSize);
		
				m_pD3DDevice->CreatePixelShader((DWORD *) pShaderMem, &pShader->m_pPixelShader);
				free(pShaderMem);

				// read samplers and constants

				DX9Sampler_t*		samplers = (DX9Sampler_t*) malloc(scHdr.numSamplers * sizeof(DX9Sampler_t));
				DX9ShaderConstant*	constants = (DX9ShaderConstant*) malloc(scHdr.numConstants * sizeof(DX9ShaderConstant));

				pStream->Read(samplers, scHdr.numSamplers, sizeof(DX9Sampler_t));
				pStream->Read(constants, scHdr.numConstants, sizeof(DX9ShaderConstant));

				// assign them
				pShader->m_pConstants  = constants;
				pShader->m_pSamplers   = samplers;
				pShader->m_numSamplers  = scHdr.numSamplers;
				pShader->m_numConstants = scHdr.numConstants;

				needsCompile = false;
			}
			else
			{
				MsgWarning("Shader cache for '%s' broken and will be recompiled\n", pShaderOutput->GetName());
			}

			g_fileSystem->Close(pStream);
		}
	}

	if(needsCompile && info.data.text != NULL)
	{
		pStream = g_fileSystem->Open( cache_file_name.GetData(), "wb", SP_ROOT);

		if(!pStream)
			MsgError("ERROR: Cannot create shader cache file for %s\n", pShaderOutput->GetName());
	}
	else
	{
		return true;
	}

	shaderCacheHdr_t scHdr;
	scHdr.ident = SHADERCACHE_IDENT;
	scHdr.vsSize = 0;
	scHdr.psSize = 0;

	if(pStream) // write empty header
		pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));

	if (info.data.text != NULL)
	{
		LPD3DXBUFFER shaderBuf = NULL;
		LPD3DXBUFFER errorsBuf = NULL;

		EqString shaderString;

		if (extra  != NULL)
			shaderString.Append(extra);

		int maxVSVersion = D3DSHADER_VERSION_MAJOR(m_hCaps.VertexShaderVersion);
		EqString profile(D3DXGetVertexShaderProfile(m_pD3DDevice));
		EqString entry("vs_main");

		int vsVersion = maxVSVersion;

		if(info.apiPrefs)
		{
			profile = KV_GetValueString(info.apiPrefs->FindKeyBase("vs_profile"), 0, profile.ToCString());
			entry = KV_GetValueString(info.apiPrefs->FindKeyBase("EntryPoint"), 0, entry.ToCString());

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

		//shaderString.Append(EqString::Format("#line %d\n", params.vsLine + 1));
		shaderString.Append(info.data.text);

		HRESULT compileResult = D3DXCompileShader(shaderString.GetData(), shaderString.Length(),
			NULL, NULL, 
			entry.ToCString(), profile.ToCString(),
			D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR | D3DXSHADER_PARTIALPRECISION,
			&shaderBuf, &errorsBuf, &pShader->m_pVSConstants);

		if (compileResult == D3D_OK)
		{
			m_pD3DDevice->CreateVertexShader((DWORD *) shaderBuf->GetBufferPointer(), &pShader->m_pVertexShader);

			scHdr.vsSize = shaderBuf->GetBufferSize();

			if(pStream)
				pStream->Write(shaderBuf->GetBufferPointer(), 1, scHdr.vsSize);

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

	if (info.data.text != NULL)
	{
		LPD3DXBUFFER shaderBuf = NULL;
		LPD3DXBUFFER errorsBuf = NULL;

		EqString shaderString;

		if (extra  != NULL)
			shaderString.Append(extra);

		int maxPSVersion = D3DSHADER_VERSION_MAJOR(m_hCaps.PixelShaderVersion);
		EqString profile(D3DXGetPixelShaderProfile(m_pD3DDevice));
		EqString entry("ps_main");

		int psVersion = maxPSVersion;

		if(info.apiPrefs)
		{
			profile = KV_GetValueString(info.apiPrefs->FindKeyBase("ps_profile"), 0, profile.ToCString());
			entry = KV_GetValueString(info.apiPrefs->FindKeyBase("EntryPoint"), 0, entry.ToCString());

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

		//shaderString.Append(EqString::Format("#line %d\n", params.psLine + 1));
		shaderString.Append(info.data.text);

		HRESULT compileResult = D3DXCompileShader(
			shaderString.GetData(), shaderString.Length(),
			NULL, NULL, entry.ToCString(), profile.ToCString(),
			D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR,
			&shaderBuf, &errorsBuf,
			&pShader->m_pPSConstants);

		if (compileResult == D3D_OK)
		{
			m_pD3DDevice->CreatePixelShader((DWORD *) shaderBuf->GetBufferPointer(), &pShader->m_pPixelShader);

			scHdr.psSize = shaderBuf->GetBufferSize();

			if(pStream)
				pStream->Write(shaderBuf->GetBufferPointer(), 1, scHdr.psSize);

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

	if (pShader->m_pPixelShader == NULL || pShader->m_pVertexShader == NULL)
	{
		if(pStream)
		{
			scHdr.checksum = -1;
			scHdr.psSize = -1;
			scHdr.vsSize = -1;

			pStream->Seek(0,VS_SEEK_SET);
			pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));
			g_fileSystem->Close(pStream);
		}

		return false; // Don't do anything
	}

	if(pShader->m_pVSConstants == NULL || pShader->m_pPSConstants == NULL)
	{
		if(pStream)
		{
			scHdr.checksum = -1;
			scHdr.psSize = -1;
			scHdr.vsSize = -1;

			pStream->Seek(0,VS_SEEK_SET);
			pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));
			g_fileSystem->Close(pStream);
		}

		return false;
	}

	D3DXCONSTANTTABLE_DESC vsDesc, psDesc;
	pShader->m_pVSConstants->GetDesc(&vsDesc);
	pShader->m_pPSConstants->GetDesc(&psDesc);

	uint count = vsDesc.Constants + psDesc.Constants;

	DX9Sampler_t* samplers			= (DX9Sampler_t*) malloc(count * sizeof(DX9Sampler_t));
	DX9ShaderConstant* constants	= (DX9ShaderConstant *) malloc(count * sizeof(DX9ShaderConstant));

	uint nSamplers  = 0;
	uint nConstants = 0;

	D3DXCONSTANT_DESC cDesc;
	for (uint i = 0; i < vsDesc.Constants; i++)
	{
		UINT cnt = 1;
		pShader->m_pVSConstants->GetConstantDesc(pShader->m_pVSConstants->GetConstant(NULL, i), &cDesc, &cnt);

		//size_t length = strlen(cDesc.Name);
		if (cDesc.Type >= D3DXPT_SAMPLER && cDesc.Type <= D3DXPT_SAMPLERCUBE)
		{
			// TODO: Vertex samplers not yet supported ...
		} 
		else 
		{
			//constants[nConstants].name = new char[length + 1];
			strcpy(constants[nConstants].name, cDesc.Name);
			constants[nConstants].hash = -1;
			constants[nConstants].vsReg = cDesc.RegisterIndex;
			constants[nConstants].psReg = -1;
			constants[nConstants].constFlags = SCONST_VERTEX;
			//constants[nConstants].nElements = cDesc.RegisterCount;
			nConstants++;
		}
	}

	uint nVSConsts = nConstants;
	for (uint i = 0; i < psDesc.Constants; i++)
	{
		UINT cnt = 1;
		pShader->m_pPSConstants->GetConstantDesc(pShader->m_pPSConstants->GetConstant(NULL, i), &cDesc, &cnt);

		//size_t length = strlen(cDesc.Name);
		if (cDesc.Type >= D3DXPT_SAMPLER && cDesc.Type <= D3DXPT_SAMPLERCUBE)
		{
			//samplers[nSamplers].name = new char[length + 1];
			samplers[nSamplers].index = cDesc.RegisterIndex;
			strcpy(samplers[nSamplers].name, cDesc.Name);
			nSamplers++;
		} 
		else 
		{
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
				//constants[nConstants].name = new char[length + 1];
				strcpy(constants[nConstants].name, cDesc.Name);
				constants[nConstants].hash = -1;
				constants[nConstants].vsReg = -1;
				constants[nConstants].psReg = cDesc.RegisterIndex;
				//constants[nConstants].nElements = cDesc.RegisterCount;
				constants[nConstants].constFlags = SCONST_PIXEL;
			} 
			else 
			{
				constants[merge].psReg = cDesc.RegisterIndex;
				constants[merge].constFlags |= SCONST_PIXEL; // add flags
			}
			nConstants++;
		}
	}

	// Shorten arrays to actual count
	samplers  = (DX9Sampler_t*) realloc(samplers,  nSamplers  * sizeof(DX9Sampler_t));
	constants = (DX9ShaderConstant *) realloc(constants, nConstants * sizeof(DX9ShaderConstant));
	qsort(samplers,  nSamplers,  sizeof(DX9Sampler_t),  SamplerComp);
	qsort(constants, nConstants, sizeof(DX9ShaderConstant), ConstantComp);

	if(pStream)
	{
		scHdr.numSamplers = nSamplers;
		scHdr.numConstants = nConstants;
		scHdr.checksum = info.data.checksum;

		pStream->Write(samplers, nSamplers, sizeof(DX9Sampler_t));
		pStream->Write(constants, nConstants, sizeof(DX9ShaderConstant));

		pStream->Seek(0,VS_SEEK_SET);
		pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));
		g_fileSystem->Close(pStream);
	}

	pShader->m_pConstants  = constants;
	pShader->m_pSamplers   = samplers;
	pShader->m_numConstants = nConstants;
	pShader->m_numSamplers  = nSamplers;

	return true;
}

// Set current shader for rendering
void ShaderAPID3DX9::SetShader(IShaderProgram* pShader)
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

int	ShaderAPID3DX9::GetSamplerUnit(IShaderProgram* pProgram,const char* pszSamplerName)
{
	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(pProgram);
	if(!pShader)
		return -1;

	DX9Sampler_t* samplers = pShader->m_pSamplers;
	int minSampler = 0;
	int maxSampler = pShader->m_numSamplers - 1;

	// Do a quick lookup in the sorted table with a binary search
	while (minSampler <= maxSampler)
	{
		int currSampler = (minSampler + maxSampler) >> 1;
        int res = strcmp(pszSamplerName, samplers[currSampler].name);

		if (res == 0)
		{
			return samplers[currSampler].index;
		} 
		else if (res > 0)
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

void ShaderAPID3DX9::SetTexture( ITexture* pTexture, const char* pszName, int index )
{
	SetTextureOnIndex(pTexture, index);

	/*
	if(pszName == NULL)
	{
		SetTextureOnIndex(pTexture, index);
	}
	else
	{
		int unit = GetSamplerUnit( m_pSelectedShader, pszName );

		if(unit == -1)
			SetTextureOnIndex(pTexture, index);
		else
			SetTextureOnIndex(pTexture, unit);
	}
	*/
}

// RAW Constant (Used for structure types, etc.)
int ShaderAPID3DX9::SetShaderConstantRaw(const char *pszName, const void *data, int nSize, int nConstId)
{
	if(data == NULL || nSize == NULL)
		return nConstId;

	CD3D9ShaderProgram* pShader = (CD3D9ShaderProgram*)(m_pSelectedShader);

	if(!pShader)
		return -1;

	DX9ShaderConstant *constants = pShader->m_pConstants;
	/*
	if(nConstId != -1)
	{
		DX9ShaderConstant* c = constants + nConstId;

		if (c->vsReg >= 0)
		{
			if (memcmp(m_vsRegs + c->vsReg, data, nSize))
			{
				memcpy(m_vsRegs + c->vsReg, data, nSize);
					
				int r0 = c->vsReg;
				int r1 = c->vsReg + ((nSize + 15) >> 4);

				if (r0 < m_nMinVSDirty)
					m_nMinVSDirty = r0;

				if (r1 > m_nMaxVSDirty)
					m_nMaxVSDirty = r1;
			}
		}

		if (c->psReg >= 0)
		{
			if (memcmp(m_psRegs + c->psReg, data, nSize))
			{
				memcpy(m_psRegs + c->psReg, data, nSize);
					
				int r0 = c->psReg;
				int r1 = c->psReg + ((nSize + 15) >> 4);

				if (r0 < m_nMinPSDirty)
					m_nMinPSDirty = r0;

				if (r1 > m_nMaxPSDirty)
					m_nMaxPSDirty = r1;
			}
		}

		return nConstId;
	}*/

	int minConstant = 0;
	int maxConstant = pShader->m_numConstants - 1;

	// TODO: fix it up and do better
	
	// Do a quick lookup in the sorted table with a binary search
	while (minConstant <= maxConstant)
	{
		int currConstant = (minConstant + maxConstant) >> 1;

		int res = strcmp(pszName, constants[currConstant].name);

		if (res == 0)
		{
			DX9ShaderConstant *c = constants + currConstant;

			if (c->vsReg >= 0)
			{
				if (memcmp(m_vsRegs + c->vsReg, data, nSize))
				{
					memcpy(m_vsRegs + c->vsReg, data, nSize);

					int r0 = c->vsReg;
					int r1 = c->vsReg + ((nSize + 15) >> 4);

					if (r0 < m_nMinVSDirty)
						m_nMinVSDirty = r0;

					if (r1 > m_nMaxVSDirty)
						m_nMaxVSDirty = r1;
				}
			}

			if (c->psReg >= 0)
			{
				if (memcmp(m_psRegs + c->psReg, data, nSize))
				{
					memcpy(m_psRegs + c->psReg, data, nSize);
					
					int r0 = c->psReg;
					int r1 = c->psReg + ((nSize + 15) >> 4);

					if (r0 < m_nMinPSDirty)
						m_nMinPSDirty = r0;

					if (r1 > m_nMaxPSDirty)
						m_nMaxPSDirty = r1;
				}
			}

			return currConstant;
		} 
		else if (res > 0)
		{
			minConstant = currConstant + 1;
		}
		else 
		{
			maxConstant = currConstant - 1;
		}
	}

	return -1;
}

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

IVertexFormat* ShaderAPID3DX9::CreateVertexFormat(const char* name, VertexFormatDesc_s* formatDesc, int nAttribs)
{
	CVertexFormatD3DX9* pFormat = new CVertexFormatD3DX9(name, formatDesc, nAttribs);

	D3DVERTEXELEMENT9* vertexElements = new D3DVERTEXELEMENT9[nAttribs + 1];
	pFormat->GenVertexElement( vertexElements );

	HRESULT hr = m_pD3DDevice->CreateVertexDeclaration(vertexElements, &pFormat->m_pVertexDecl);
	delete [] vertexElements;

	if (hr != D3D_OK)
	{
		delete pFormat;
		MsgError("Couldn't create vertex declaration");
		ASSERT(!"Couldn't create vertex declaration");
		return NULL;
	}

	m_Mutex.Lock();
	m_VFList.append(pFormat);
	m_Mutex.Unlock();

	return pFormat;
}

IVertexBuffer* ShaderAPID3DX9::CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData)
{
	CVertexBufferD3DX9* pBuffer = new CVertexBufferD3DX9();
	pBuffer->m_nSize = nNumVerts*strideSize;
	pBuffer->m_nUsage = d3dbufferusages[nBufAccess];
	pBuffer->m_nNumVertices = nNumVerts;
	pBuffer->m_nStrideSize = strideSize;
	pBuffer->m_nInitialSize = nNumVerts*strideSize;

	DevMsg(DEVMSG_SHADERAPI,"Creating VBO with size %i KB\n", pBuffer->m_nSize / 1024);

	HRESULT hr = m_pD3DDevice->TestCooperativeLevel();

	while (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
	{
		// do loops if devise lost or needs reset
		hr = m_pD3DDevice->TestCooperativeLevel();
	}

	bool dynamic = (pBuffer->m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	if (m_pD3DDevice->CreateVertexBuffer(pBuffer->m_nInitialSize, pBuffer->m_nUsage, 0, dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &pBuffer->m_pVertexBuffer, NULL) != D3D_OK)
	{
		MsgError("Direct3D Error: Couldn't create vertex buffer with size %d\n", pBuffer->m_nSize);
        ASSERT(!"Direct3D Error: Couldn't create vertex buffer");
		return NULL;
	}

	// make first transfer operation
	void *dest;
	if (pData && pBuffer->m_pVertexBuffer->Lock(0, pBuffer->m_nSize, &dest, dynamic? D3DLOCK_DISCARD : 0) == D3D_OK)
	{
		memcpy(dest, pData, pBuffer->m_nSize);
		pBuffer->m_pVertexBuffer->Unlock();
	}

	m_Mutex.Lock();
	m_VBList.append(pBuffer);
	m_Mutex.Unlock();

	return pBuffer;

}
IIndexBuffer* ShaderAPID3DX9::CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData)
{
	ASSERT(nIndexSize >= 2);
	ASSERT(nIndexSize <= 4);

	CIndexBufferD3DX9* pBuffer = new CIndexBufferD3DX9();
	pBuffer->m_nIndices = nIndices;
	pBuffer->m_nIndexSize = nIndexSize;
	pBuffer->m_nInitialSize = nIndices*nIndexSize;
	pBuffer->m_nUsage = d3dbufferusages[nBufAccess];

	bool dynamic = (pBuffer->m_nUsage & D3DUSAGE_DYNAMIC) != 0;

	DevMsg(DEVMSG_SHADERAPI,"Creating IBO with size %i KB\n",(nIndices*nIndexSize) / 1024);

	HRESULT hr = m_pD3DDevice->TestCooperativeLevel();

	while (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
	{
		// do loops if devise lost or needs reset
		hr = m_pD3DDevice->TestCooperativeLevel();
	}

	if (m_pD3DDevice->CreateIndexBuffer(pBuffer->m_nInitialSize, pBuffer->m_nUsage, nIndexSize == 2? D3DFMT_INDEX16 : D3DFMT_INDEX32, dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &pBuffer->m_pIndexBuffer, NULL) != D3D_OK)
	{
		MsgError("Direct3D Error: Couldn't create index buffer with size %d\n", pBuffer->m_nInitialSize);
		ASSERT(!"Direct3D Error: Couldn't create index buffer\n");
		return NULL;
	}

	// make first transfer operation
	void *dest;
	if (pData && pBuffer->m_pIndexBuffer->Lock(0, pBuffer->m_nInitialSize, &dest, dynamic? D3DLOCK_DISCARD : 0) == D3D_OK)
	{
		memcpy(dest, pData, pBuffer->m_nInitialSize);
		pBuffer->m_pIndexBuffer->Unlock();
	} 

	m_Mutex.Lock();
	m_IBList.append(pBuffer);
	m_Mutex.Unlock();

	return pBuffer;
}

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

// Indexed primitive drawer
void ShaderAPID3DX9::DrawIndexedPrimitives(ER_PrimitiveType nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
	ASSERT(nVertices > 0);

	int nTris = s_DX9PrimitiveCounterFunctions[nType](nIndices);

	m_pD3DDevice->DrawIndexedPrimitive( d3dPrim[nType], nBaseVertex, nFirstVertex, nVertices, nFirstIndex, nTris );
	
	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

// Draw elements
void ShaderAPID3DX9::DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices)
{
	int nTris = s_DX9PrimitiveCounterFunctions[nType](nVertices);
	m_pD3DDevice->DrawPrimitive(d3dPrim[nType], nFirstVertex, nTris);

	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

//-------------------------------------------------------------------------------------------------------------------------
// Textures
//-------------------------------------------------------------------------------------------------------------------------

IDirect3DBaseTexture9* ShaderAPID3DX9::CreateD3DTextureFromImage(CImage* pSrc, int& wide, int& tall, int nFlags)
{
	if(!pSrc)
		return NULL;

	HOOK_TO_CVAR(r_loadmiplevel);
	int nQuality = r_loadmiplevel->GetInt();

	// force quality to best
	if(nFlags & TEXFLAG_NOQUALITYLOD)
		nQuality = 0;

	bool bMipMaps = (pSrc->GetMipMapCount() > 1);

	// force zero quality if no mipmaps
	if(!bMipMaps)
		nQuality = 0;

	int numMipmaps = (pSrc->GetMipMapCount() - nQuality);
	numMipmaps = max(0, numMipmaps);

	const D3DPOOL			nPool = D3DPOOL_MANAGED;
	const ETextureFormat	nFormat = pSrc->GetFormat();

	IDirect3DBaseTexture9* pTexture = NULL;

	if (pSrc->IsCube())
	{
		if (m_pD3DDevice->CreateCubeTexture(pSrc->GetWidth(nQuality),
											numMipmaps,
											0,
											formats[nFormat],
											nPool,
											(LPDIRECT3DCUBETEXTURE9 *)&pTexture, 
											NULL) != D3D_OK)
		{
			MsgError("D3D9 ERROR: Couldn't create cubemap texture '%s'\n", pSrc->GetName());

			return nullptr;
		}

		nFlags |= TEXFLAG_CUBEMAP;
	} 
	else if (pSrc->Is3D())
	{
		if (m_pD3DDevice->CreateVolumeTexture(	pSrc->GetWidth(nQuality), 
												pSrc->GetHeight(nQuality), 
												pSrc->GetDepth(nQuality), 
												numMipmaps, 
												0,
												formats[nFormat],
												nPool,
												(LPDIRECT3DVOLUMETEXTURE9 *)&pTexture, 
												NULL) != D3D_OK)
		{
			MsgError("D3D9 ERROR: Couldn't create volumetric texture '%s'\n", pSrc->GetName());

			return nullptr;
		}
	} 
	else 
	{
		if (m_pD3DDevice->CreateTexture(pSrc->GetWidth(nQuality),
										pSrc->GetHeight(nQuality), 
										numMipmaps, 
										0, 
										formats[nFormat], 
										nPool, 
										(LPDIRECT3DTEXTURE9 *)&pTexture, 
										NULL)!= D3D_OK)
		{
			MsgError("D3D9 ERROR: Couldn't create texture %s\n", pSrc->GetName());

			return nullptr;
		}
	}
	
	// set our referenced params
	wide = pSrc->GetWidth(nQuality);
	tall = pSrc->GetHeight(nQuality);

	// update texture
	if (!UpdateD3DTextureFromImage(pTexture, pSrc, nQuality, true))
	{
		pTexture->Release();
		return nullptr;
	}

	return pTexture;
}

void ShaderAPID3DX9::CreateTextureInternal(ITexture** pTex, const Array<CImage*>& pImages, const SamplerStateParam_t& sampler,int nFlags)
{
	if(!pImages.numElem())
		return;

	HOOK_TO_CVAR(r_loadmiplevel);

	CD3D9Texture* pTexture = NULL;

	// get or create
	if(*pTex)
		pTexture = (CD3D9Texture*)*pTex;
	else
		pTexture = new CD3D9Texture();

	int wide = 0, tall = 0;
	int numMips = 0;

	for(int i = 0; i < pImages.numElem(); i++)
	{
		IDirect3DBaseTexture9* pD3DTex = CreateD3DTextureFromImage(pImages[i], wide, tall, nFlags);

		if(pD3DTex)
		{
			int nQuality = r_loadmiplevel->GetInt();

			// force quality to best
			if((nFlags & TEXFLAG_NOQUALITYLOD) || pImages[i]->GetMipMapCount() == 1)
				nQuality = 0;

			numMips += pImages[i]->GetMipMapCount() - nQuality;

			pTexture->m_texSize += pImages[i]->GetMipMappedSize(nQuality);

			pTexture->textures.append(pD3DTex);
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
	pTexture->SetMipCount(numMips);
	pTexture->SetFormat(pImages[0]->GetFormat());
	pTexture->SetFlags(nFlags | TEXFLAG_MANAGED);
	pTexture->SetName( pImages[0]->GetName() );

	pTexture->m_pool = D3DPOOL_MANAGED;

	// if this is a new texture, add
	if(!(*pTex))
	{
		m_Mutex.Lock();
		ASSERTMSG(m_TextureList.find(pTexture->m_nameHash) == m_TextureList.end(), "Texture %s was already added", pTexture->GetName());
		m_TextureList.insert(pTexture->m_nameHash, pTexture);
		m_Mutex.Unlock();
	}

	// set for output
	*pTex = pTexture;
}