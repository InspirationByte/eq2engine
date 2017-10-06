//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Direct3D 10 ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "ShaderAPID3DX10.h"
#include "utils/strtools.h"

#include "DebugInterface.h"
#include "IConCommandFactory.h"
#include "D3D10Texture.h"

#include "d3dx10_def.h"

#include "VertexFormatD3DX10.h"
#include "VertexBufferD3DX10.h"
#include "IndexBufferD3DX10.h"
#include "D3D10ShaderProgram.h"
#include "D3D10RenderStates.h"

#include "Imaging/ImageLoader.h"

#include "VirtualStream.h"

ShaderAPID3DX10::~ShaderAPID3DX10()
{

}

ShaderAPID3DX10::ShaderAPID3DX10() : ShaderAPI_Base()
{
	Msg("Initializing Direct3D 10 Shader API...\n");

	m_pEventQuery = NULL;
	m_pD3DDevice = NULL;

	m_nCurrentSampleMask = ~0;
	m_nSelectedSampleMask = ~0;

	memset(m_pSelectedSamplerStatesVS,0,sizeof(m_pSelectedSamplerStatesVS));
	memset(m_pSelectedSamplerStatesGS,0,sizeof(m_pSelectedSamplerStatesGS));
	memset(m_pSelectedSamplerStatesPS,0,sizeof(m_pSelectedSamplerStatesPS));

	memset(m_pCurrentSamplerStatesVS,0,sizeof(m_pCurrentSamplerStatesVS));
	memset(m_pCurrentSamplerStatesGS,0,sizeof(m_pCurrentSamplerStatesGS));
	memset(m_pCurrentSamplerStatesPS,0,sizeof(m_pCurrentSamplerStatesPS));

	memset(m_pSelectedTextureSlicesVS,-1,sizeof(m_pSelectedTextureSlicesVS));
	memset(m_pSelectedTextureSlicesGS,-1,sizeof(m_pSelectedTextureSlicesGS));
	memset(m_pSelectedTextureSlicesPS,-1,sizeof(m_pSelectedTextureSlicesPS));

	memset(m_pCurrentTextureSlicesVS,-1,sizeof(m_pCurrentTextureSlicesVS));
	memset(m_pCurrentTextureSlicesGS,-1,sizeof(m_pCurrentTextureSlicesGS));
	memset(m_pCurrentTextureSlicesPS,-1,sizeof(m_pCurrentTextureSlicesPS));

	memset(m_pSelectedTexturesVS,0,sizeof(m_pSelectedTexturesVS));
	memset(m_pSelectedTexturesGS,0,sizeof(m_pSelectedTexturesGS));
	memset(m_pSelectedTexturesPS,0,sizeof(m_pSelectedTexturesPS));

	memset(m_pCurrentTexturesVS,0,sizeof(m_pCurrentTexturesVS));
	memset(m_pCurrentTexturesGS,0,sizeof(m_pCurrentTexturesGS));
	memset(m_pCurrentTexturesPS,0,sizeof(m_pCurrentTexturesPS));

	m_pDepthBufferTexture = NULL;
	m_pBackBufferTexture = NULL;

	//m_pBackBufferRTV = NULL;
	m_pDepthBufferDSV = NULL;
	//m_pBackBuffer = NULL;
	m_pDepthBuffer = NULL;

	m_pCustomSamplerState = new SamplerStateParam_t;

	m_pMeshBufferTexturedShader = NULL;
	m_pMeshBufferNoTextureShader = NULL;

	m_nCurrentMatrixMode = MATRIXMODE_VIEW;

	m_pD3DDevice = NULL;

	m_bDeviceIsLost = false;
}

void ShaderAPID3DX10::SetD3DDevice(ID3D10Device* d3ddev)
{
	ASSERT(d3ddev);

	m_pD3DDevice = d3ddev;

	m_pD3DDevice->AddRef();
}

// Init + Shurdown
void ShaderAPID3DX10::Init(const shaderAPIParams_t &params)
{
	HOOK_TO_CVAR(r_textureanisotrophy);

	memset(&m_caps, 0, sizeof(m_caps));

	m_caps.maxTextureAnisotropicLevel = clamp(r_textureanisotrophy->GetInt(), 1, 16);

	m_caps.isHardwareOcclusionQuerySupported = true;
	m_caps.isInstancingSupported = true;

	m_caps.maxTextureSize = 65535;
	m_caps.maxRenderTargets = MAX_MRTS;
	m_caps.maxVertexGenericAttributes = MAX_GENERIC_ATTRIB;
	m_caps.maxVertexTexcoordAttributes = MAX_TEXCOORD_ATTRIB;

	m_caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED | SHADER_CAPS_PIXEL_SUPPORTED | SHADER_CAPS_GEOMETRY_SUPPORTED;
	m_caps.maxTextureUnits = MAX_TEXTUREUNIT;
	m_caps.maxVertexStreams = MAX_VERTEXSTREAM;
	m_caps.maxVertexTextureUnits = MAX_VERTEXTEXTURES;

	// init base and critical section
	ShaderAPI_Base::Init( params );

	// all shaders supported, nothing to report

	if(m_pMeshBufferTexturedShader == NULL)
	{
		m_pMeshBufferTexturedShader = CreateNewShaderProgram("MeshBuffer_Textured");

		shaderProgramCompileInfo_t info;

		info.ps.text = s_FFPMeshBuilder_Textured_PixelProgram;
		info.vs.text = s_FFPMeshBuilder_VertexProgram;
		info.disableCache = true;

		CompileShadersFromStream(m_pMeshBufferTexturedShader, info);
	}

	if(m_pMeshBufferNoTextureShader == NULL)
	{
		m_pMeshBufferNoTextureShader = CreateNewShaderProgram("MeshBuffer_NoTexture");

		shaderProgramCompileInfo_t info;

		info.ps.text = s_FFPMeshBuilder_NoTexture_PixelProgram;
		info.vs.text = s_FFPMeshBuilder_VertexProgram;
		info.disableCache = true;

		CompileShadersFromStream(m_pMeshBufferNoTextureShader, info);
	}
}

void ShaderAPID3DX10::PrintAPIInfo()
{
	Msg("ShaderAPI: ShaderAPID3DX10\n");
	Msg("Direct3D 10 SDK version: %d\n \n", D3D10_SDK_VERSION);

	MsgInfo("------ Loaded textures ------");

	for(int i = 0; i < m_TextureList.numElem(); i++)
	{
		CD3D10Texture* pTexture = (CD3D10Texture*)m_TextureList[i];

		MsgInfo("     %s (%d) - %dx%d\n", pTexture->GetName(), pTexture->Ref_Count(), pTexture->GetWidth(),pTexture->GetHeight());
	}
}

void ShaderAPID3DX10::Shutdown()
{
	ShaderAPI_Base::Shutdown();
}

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------
void ShaderAPID3DX10::Reset(int nResetType)
{
	ShaderAPI_Base::Reset(nResetType);

	// i think is faster
	if (nResetType & STATE_RESET_TEX)
	{
		memset(m_pSelectedTexturesGS,0,sizeof(m_pSelectedTexturesGS));
		memset(m_pSelectedTexturesVS,0,sizeof(m_pSelectedTexturesVS));
		memset(m_pSelectedTexturesPS,0,sizeof(m_pSelectedTexturesPS));
	}
}

bool FillShaderResourceView(ID3D10SamplerState **samplers, ID3D10ShaderResourceView **dest, int &min, int &max, ITexture** selectedTextures, ITexture** currentTextures, const int selectedTextureSlices[], int currentTextureSlices[])
{
	min = 0;
	do 
	{
		if (selectedTextures[min] != currentTextures[min] || selectedTextureSlices[min] != currentTextureSlices[min])
		{
			max = MAX_TEXTUREUNIT;
			do 
			{
				max--;
			} 
			while (selectedTextures[max] == currentTextures[max] && selectedTextureSlices[max] != currentTextureSlices[max]);

			for (int i = min; i <= max; i++)
			{
				CD3D10Texture* pTexture = (CD3D10Texture*)selectedTextures[i];

				if (pTexture != NULL)
				{
					if (selectedTextureSlices[i] == -1)
					{
						CD3D10SamplerState* pSampler = (CD3D10SamplerState*)pTexture->m_pD3D10SamplerState;
						*samplers++ = pSampler->m_samplerState;
						*dest++ = pTexture->srv[pTexture->GetCurrentAnimatedTextureFrame()];
					}
					else 
					{
						CD3D10SamplerState* pSampler = (CD3D10SamplerState*)pTexture->m_pD3D10SamplerState;
						*samplers++ = pSampler->m_samplerState;
						*dest++ = pTexture->srv[selectedTextureSlices[i]];
					}
				}
				else 
				{
					*samplers++ = NULL;
					*dest++ = NULL;
				}

				currentTextures[i] = selectedTextures[i];
				currentTextureSlices[i] = selectedTextureSlices[i];
			}
			return true;
		}

		min++;
	}
	while (min < MAX_TEXTUREUNIT);

	return false;
}

void ShaderAPID3DX10::ApplyTextures()
{
	ID3D10SamplerState *samplers[MAX_SAMPLERSTATE];
	ID3D10ShaderResourceView *srViews[MAX_TEXTUREUNIT];

	int min, max;
	if (FillShaderResourceView(samplers, srViews, min, max, m_pSelectedTexturesVS, m_pCurrentTexturesVS, m_pSelectedTextureSlicesVS, m_pCurrentTextureSlicesVS))
	{
		m_pD3DDevice->VSSetShaderResources(min, max - min + 1, srViews);
		m_pD3DDevice->VSSetSamplers(min, max - min + 1, samplers);
	}
	if (FillShaderResourceView(samplers, srViews, min, max, m_pSelectedTexturesGS, m_pCurrentTexturesGS, m_pSelectedTextureSlicesGS, m_pCurrentTextureSlicesGS))
	{
		m_pD3DDevice->GSSetShaderResources(min, max - min + 1, srViews);
		m_pD3DDevice->GSSetSamplers(min, max - min + 1, samplers);
	}
	if (FillShaderResourceView(samplers, srViews, min, max, m_pSelectedTexturesPS, m_pCurrentTexturesPS, m_pSelectedTextureSlicesPS, m_pCurrentTextureSlicesPS))
	{
		m_pD3DDevice->PSSetShaderResources(min, max - min + 1, srViews);
		m_pD3DDevice->PSSetSamplers(min, max - min + 1, samplers);
	}
}

bool InternalFillSamplerState(ID3D10SamplerState **dest, int &min, int &max, ITexture** selectedTextures, ITexture** currentTextures)
{
	min = 0;
	do 
	{
		if (selectedTextures[min] != currentTextures[min])
		{
			max = MAX_SAMPLERSTATE;
			do 
			{
				max--;
			} 
			while (currentTextures[max] == selectedTextures[max]);

			for (int i = min; i <= max; i++)
			{
				CD3D10Texture* pTexture = (CD3D10Texture*)selectedTextures[i];

				if (selectedTextures[i] != NULL)
				{
					CD3D10SamplerState* pSampler = (CD3D10SamplerState*)pTexture->m_pD3D10SamplerState;
					*dest++ = pSampler->m_samplerState;
				}
				else 
					*dest++ = NULL;

				currentTextures[i] = selectedTextures[i];
			}

			return true;
		}
		min++;
	} 
	while (min < MAX_SAMPLERSTATE);

	return false;
}

void ShaderAPID3DX10::ApplySamplerState()
{
	/*
	ID3D10SamplerState *samplers[MAX_SAMPLERSTATE];

	int min, max = 0;
	if (InternalFillSamplerState(samplers, min, max, m_pSelectedTexturesVS, m_pCurrentTexturesVS))
	{
		m_pD3DDevice->VSSetSamplers(min, max - min + 1, samplers);
	}
	if (InternalFillSamplerState(samplers, min, max, m_pSelectedTexturesGS, m_pCurrentTexturesGS))
	{
		m_pD3DDevice->GSSetSamplers(min, max - min + 1, samplers);
	}
	if (InternalFillSamplerState(samplers, min, max, m_pSelectedTexturesPS, m_pCurrentTexturesPS))
	{
		m_pD3DDevice->PSSetSamplers(min, max - min + 1, samplers);
	}
	*/
}

void ShaderAPID3DX10::ApplyBlendState()
{
	CD3D10BlendingState* pSelectedState = (CD3D10BlendingState*)m_pSelectedBlendstate;

	if(!pSelectedState)
		m_pD3DDevice->OMSetBlendState(NULL, Vector4D(1, 1, 1, 1), 0u);
	else
		m_pD3DDevice->OMSetBlendState(pSelectedState->m_blendState, Vector4D(1, 1, 1, 1), 0u);

	m_pCurrentBlendstate = pSelectedState;
}

void ShaderAPID3DX10::ApplyDepthState()
{
	CD3D10DepthStencilState* pSelectedState = (CD3D10DepthStencilState*)m_pSelectedDepthState;

	if(!pSelectedState)
		m_pD3DDevice->OMSetDepthStencilState(NULL, 0);
	else
		m_pD3DDevice->OMSetDepthStencilState(pSelectedState->m_dsState, 0);

	m_pCurrentDepthState = m_pSelectedDepthState;
}

void ShaderAPID3DX10::ApplyRasterizerState()
{
	CD3D10RasterizerState* pSelectedState = (CD3D10RasterizerState*)m_pSelectedRasterizerState;

	if(!pSelectedState)
	{
		m_pD3DDevice->RSSetState(NULL);
	}
	else
	{
		if(m_pSelectedRasterizerState->GetType() != RENDERSTATE_RASTERIZER)
		{
			MsgError("Invalid render state type (%d), expected RASTERIZER\n", m_pSelectedRasterizerState->GetType());
			m_pD3DDevice->RSSetState(NULL);
			return;
		}

		m_pD3DDevice->RSSetState( pSelectedState->m_rsState );
	}

	m_pCurrentRasterizerState = pSelectedState;
}

void ShaderAPID3DX10::ApplyShaderProgram()
{
	if (m_pSelectedShader != m_pCurrentShader)
	{
		CD3D10ShaderProgram* pShader = (CD3D10ShaderProgram*)m_pSelectedShader;

		if (pShader == NULL)
		{
			m_pD3DDevice->VSSetShader(NULL);
			m_pD3DDevice->GSSetShader(NULL);
			m_pD3DDevice->PSSetShader(NULL);
		} 
		else 
		{
			m_pD3DDevice->VSSetShader(pShader->m_pVertexShader);
			m_pD3DDevice->GSSetShader(pShader->m_pGeomShader);
			m_pD3DDevice->PSSetShader(pShader->m_pPixelShader);

			if (pShader->m_nGSCBuffers)
				m_pD3DDevice->GSSetConstantBuffers(0, pShader->m_nGSCBuffers, pShader->m_pgsConstants);

			if (pShader->m_nPSCBuffers)
				m_pD3DDevice->PSSetConstantBuffers(0, pShader->m_nPSCBuffers, pShader->m_ppsConstants);

			if (pShader->m_nVSCBuffers)
				m_pD3DDevice->VSSetConstantBuffers(0, pShader->m_nVSCBuffers, pShader->m_pvsConstants);
		}
		m_pCurrentShader = m_pSelectedShader;
	}
}

void ShaderAPID3DX10::ApplyConstants()
{
	CD3D10ShaderProgram* pProgram = (CD3D10ShaderProgram*)m_pCurrentShader;

	if(!pProgram)
		return;

	for (uint i = 0; i <pProgram->m_nGSCBuffers; i++)
	{
		if (pProgram->m_pgsDirty[i])
		{
			m_pD3DDevice->UpdateSubresource(pProgram->m_pgsConstants[i], 0, NULL, pProgram->m_pgsConstMem[i], 0, 0);
			pProgram->m_pgsDirty[i] = false;
		}
	}

	for (uint i = 0; i < pProgram->m_nPSCBuffers; i++)
	{
		if (pProgram->m_ppsDirty[i])
		{
			m_pD3DDevice->UpdateSubresource(pProgram->m_ppsConstants[i], 0, NULL, pProgram->m_ppsConstMem[i], 0, 0);
			pProgram->m_ppsDirty[i] = false;
		}
	}

	for (uint i = 0; i < pProgram->m_nVSCBuffers; i++)
	{
		if (pProgram->m_pvsDirty[i])
		{
			m_pD3DDevice->UpdateSubresource(pProgram->m_pvsConstants[i], 0, NULL, pProgram->m_pvsConstMem[i], 0, 0);
			pProgram->m_pvsDirty[i] = false;
		}
	}
}

void ShaderAPID3DX10::Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const ColorRGBA &fillColor,float fDepth, int nStencil)
{


	if (bClearColor)
	{
		CD3D10Texture* pBackBuffer = (CD3D10Texture*)m_pBackBufferTexture;

		if (m_pCurrentColorRenderTargets[0] == NULL)
		{
			m_pD3DDevice->ClearRenderTargetView(pBackBuffer->rtv[0], fillColor);
		}

		for (int i = 0; i < MAX_MRTS; i++)
		{
			if (m_pCurrentColorRenderTargets[i] != NULL)
			{
				CD3D10Texture* pTargetTexture = (CD3D10Texture*)m_pCurrentColorRenderTargets[i];

				m_pD3DDevice->ClearRenderTargetView(pTargetTexture->rtv[m_nCurrentCRTSlice[i]], fillColor);
			}
		}
	}
	if (bClearDepth || bClearStencil)
	{
		UINT clearFlags = 0;
		if (bClearDepth)   clearFlags |= D3D10_CLEAR_DEPTH;
		if (bClearStencil) clearFlags |= D3D10_CLEAR_STENCIL;

		if (m_pCurrentDepthRenderTarget == m_pDepthBufferTexture)
		{
			m_pD3DDevice->ClearDepthStencilView(m_pDepthBufferDSV, clearFlags, fDepth, nStencil);
		}
		else if( m_pCurrentDepthRenderTarget == NULL )
		{
			return;
		}
		else
		{
			CD3D10Texture* pTargetTexture = (CD3D10Texture*)m_pCurrentDepthRenderTarget;

			/*
			if (currentDepthSlice == NO_SLICE)
			{
			*/
				m_pD3DDevice->ClearDepthStencilView(pTargetTexture->dsv[0], clearFlags, fDepth, nStencil);
				/*
			} 
			else 
			{
				m_pD3DDevice->ClearDepthStencilView(textures[currentDepthRT].dsvArray[currentDepthSlice], clearFlags, depth, stencil);
			}
			*/
		}
	}
}

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

// Device vendor and version
const char* ShaderAPID3DX10::GetDeviceNameString() const
{
	return "malfunction";
}

// Renderer string (ex: OpenGL, D3D9)
const char* ShaderAPID3DX10::GetRendererName() const
{
	return "Direct3D10";
}

// Render targetting support
bool ShaderAPID3DX10::IsSupportsRendertargetting() const
{
	return true;
}

// Render targetting support for Multiple RTs
bool ShaderAPID3DX10::IsSupportsMRT() const
{
	return true;
}

// Supports multitexturing???
bool ShaderAPID3DX10::IsSupportsMultitexturing() const
{
	return true;
}

// The driver/hardware is supports Pixel shaders?
bool ShaderAPID3DX10::IsSupportsPixelShaders() const
{
	return true;
}

// The driver/hardware is supports Vertex shaders?
bool ShaderAPID3DX10::IsSupportsVertexShaders() const
{
	return true;
}

// The driver/hardware is supports Geometry shaders?
bool ShaderAPID3DX10::IsSupportsGeometryShaders() const
{
	// D3D9 Does not support geometry shaders
	return true;
}

// The driver/hardware is supports Domain shaders?
bool ShaderAPID3DX10::IsSupportsDomainShaders() const
{
	return false;
}

// The driver/hardware is supports Hull (tessellator) shaders?
bool ShaderAPID3DX10::IsSupportsHullShaders() const
{
	return false;
}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

// Synchronization
void ShaderAPID3DX10::Flush()
{
	m_pD3DDevice->Flush();
}

void ShaderAPID3DX10::Finish()
{
	if (m_pEventQuery == NULL)
	{
		D3D10_QUERY_DESC desc;
		desc.Query = D3D10_QUERY_EVENT;
		desc.MiscFlags = 0;

		m_pD3DDevice->CreateQuery(&desc, &m_pEventQuery);
	}

	m_pEventQuery->End();

	m_pD3DDevice->Flush();

	BOOL result = FALSE;

	do
	{
		m_pEventQuery->GetData(&result, sizeof(BOOL), 0);
	}
	while (!result);
}

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------



//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

// Unload the texture and free the memory
void ShaderAPID3DX10::FreeTexture(ITexture* pTexture)
{
	CD3D10Texture* pTex = (CD3D10Texture*)pTexture;

	if(pTex == NULL)
		return;

	Finish();

	if(pTex->Ref_Count() == 0)
		MsgWarning("texture %s refcount==0\n",pTexture->GetName());

	//ASSERT(pTex->numReferences > 0);

	CScopedMutex scoped(m_Mutex);

	pTex->Ref_Drop();

	if(pTex->Ref_Count() <= 0)
	{
		DevMsg(DEVMSG_SHADERAPI,"Texture unloaded: %s\n",pTexture->GetName());

		Reset(STATE_RESET_TEX);
		ApplyTextures();

		if(pTex->m_pD3D10SamplerState)
			DestroyRenderState(pTex->m_pD3D10SamplerState);

		m_TextureList.remove(pTexture);
		delete pTex;
	}
}

void InternalCreateDepthTarget(CD3D10Texture* pTexture, ID3D10Device* pDevice)
{
	D3D10_TEXTURE2D_DESC desc;
	desc.Width  = pTexture->GetWidth();
	desc.Height = pTexture->GetHeight();
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D10_USAGE_DEFAULT;
	desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;

	if (pTexture->GetFlags() & TEXFLAG_CUBEMAP)
	{
		desc.ArraySize = 6;
		desc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
	} 
	else 
	{
		desc.ArraySize = pTexture->arraySize;
		desc.MiscFlags = 0;
	}

	// check depth sampling flag
	if (pTexture->GetFlags() & TEXFLAG_SAMPLEDEPTH)
	{
		switch (pTexture->dsvFormat)
		{
			case DXGI_FORMAT_D16_UNORM:
				pTexture->texFormat = DXGI_FORMAT_R16_TYPELESS;
				pTexture->srvFormat = DXGI_FORMAT_R16_UNORM;
				break;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				pTexture->texFormat = DXGI_FORMAT_R24G8_TYPELESS;
				pTexture->srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case DXGI_FORMAT_D32_FLOAT:
				pTexture->texFormat = DXGI_FORMAT_R32_TYPELESS;
				pTexture->srvFormat = DXGI_FORMAT_R32_FLOAT;
				break;
		}

		desc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
	}

	desc.Format = pTexture->texFormat;

	pTexture->textures.setNum(1);

	HRESULT res = pDevice->CreateTexture2D(&desc, NULL, (ID3D10Texture2D **) &pTexture->textures[0]);

	if (FAILED(res))
	{
		MsgError("Couldn't create depth target '%s' (%x)\n", pTexture->GetName(), res);
		return;
	}

	// render depth slices?
	if (pTexture->GetFlags() & TEXFLAG_RENDERSLICES)
	{
		if (pTexture->GetFlags() & TEXFLAG_SAMPLEDEPTH)
			MsgWarning("Warning: depth texture '%s' uses both TEXFLAG_RENDERSLICES and TEXFLAG_SAMPLEDEPTH flags, which could be unsupported\n", pTexture->GetName());

		for (UINT i = 0; i < desc.ArraySize; i++)
		{
			ID3D10DepthStencilView* pDSV = ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderDepthStencilView(pTexture->textures[0], pTexture->dsvFormat, i);

			pTexture->dsv.append(pDSV);
		}
	}
	else
	{
		// make single dsv
		ID3D10DepthStencilView* pDSV = ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderDepthStencilView(pTexture->textures[0], pTexture->dsvFormat);

		pTexture->dsv.append(pDSV);
	}

	if (pTexture->GetFlags() & TEXFLAG_SAMPLEDEPTH)
	{
		// sample slices?
		if (pTexture->GetFlags() & TEXFLAG_SAMPLESLICES)
		{
			for (UINT i = 0; i < desc.ArraySize; i++)
			{
				ID3D10ShaderResourceView* pSRV = ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderResourceView(pTexture->textures[0], pTexture->srvFormat, i);
				pTexture->srv.append(pSRV);
			}
		}
		else
		{
			ID3D10ShaderResourceView* pSRV = ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderResourceView(pTexture->textures[0], pTexture->srvFormat);
			pTexture->srv.append(pSRV);
		}
	}
}

void InternalCreateRenderTarget(CD3D10Texture* pTexture, ID3D10Device* pDevice)
{
	int nDepth = 1;
	int nMipCount = 1;
	int nMSAASamples = 1;
	int nArraySize = 1;

	bool bShouldCreateDepthRT = IsDepthFormat(pTexture->GetFormat());

	if(formats[pTexture->GetFormat()] == DXGI_FORMAT_UNKNOWN)
		MsgWarning("Unsupported format (%d) for '%s'\n", pTexture->GetFormat(), pTexture->GetName());

	// setup all formats
	pTexture->dsvFormat = formats[pTexture->GetFormat()];
	pTexture->srvFormat = formats[pTexture->GetFormat()];
	pTexture->rtvFormat = formats[pTexture->GetFormat()];
	pTexture->texFormat = formats[pTexture->GetFormat()];

	if( bShouldCreateDepthRT )
	{
		InternalCreateDepthTarget(pTexture, pDevice);
		return;
	}

	if (pTexture->GetFlags() & TEXFLAG_USE_SRGB)
	{
		// Change to the matching sRGB format
		switch (pTexture->texFormat)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM: pTexture->texFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;
			case DXGI_FORMAT_BC1_UNORM: pTexture->texFormat = DXGI_FORMAT_BC1_UNORM_SRGB; break;
			case DXGI_FORMAT_BC2_UNORM: pTexture->texFormat = DXGI_FORMAT_BC2_UNORM_SRGB; break;
			case DXGI_FORMAT_BC3_UNORM: pTexture->texFormat = DXGI_FORMAT_BC3_UNORM_SRGB; break;
		}
	}
	
	pTexture->textures.setNum(1);

	if (nDepth == 1)
	{
		D3D10_TEXTURE2D_DESC desc;
		desc.Width  = pTexture->GetWidth();
		desc.Height = pTexture->GetHeight();
		desc.Format = pTexture->texFormat;
		desc.MipLevels = nMipCount;
		desc.SampleDesc.Count = nMSAASamples;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_DEFAULT;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;

		if (pTexture->GetFlags() & TEXFLAG_CUBEMAP)
		{
			nArraySize = 6;
			desc.ArraySize = 6;
			desc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
		}
		else
		{
			desc.ArraySize = nArraySize;
			desc.MiscFlags = 0;
		}

		if (pTexture->GetFlags() & TEXFLAG_GENMIPMAPS)
			desc.MiscFlags |= D3D10_RESOURCE_MISC_GENERATE_MIPS;

		if (FAILED(pDevice->CreateTexture2D(&desc, NULL, (ID3D10Texture2D **) &pTexture->textures[0])))
		{
			MsgError("Failed to create 2D rendertarget '%s'\n", pTexture->GetName());
			return;
		}
	}
	else
	{
		D3D10_TEXTURE3D_DESC desc;
		desc.Width  = pTexture->GetWidth();
		desc.Height = pTexture->GetHeight();
		desc.Format = pTexture->texFormat;
		desc.MipLevels = nMipCount;
		desc.Depth  = nDepth;
		desc.Usage = D3D10_USAGE_DEFAULT;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		if (pTexture->GetFlags() & TEXFLAG_GENMIPMAPS)
			desc.MiscFlags |= D3D10_RESOURCE_MISC_GENERATE_MIPS;

		if (FAILED(pDevice->CreateTexture3D(&desc, NULL, (ID3D10Texture3D **) &pTexture->textures[0])))
		{
			MsgError("Failed to create volume rendertarget '%s'\n", pTexture->GetName());
			return;
		}
	}


	int nSliceCount = (nDepth == 1)? nArraySize : nDepth;
	
	// sample texture array?
	if (pTexture->GetFlags() & TEXFLAG_SAMPLESLICES)
	{
		for (int i = 0; i < nSliceCount; i++)
		{
			ID3D10ShaderResourceView* pSRV = ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderResourceView(pTexture->textures[0], pTexture->srvFormat, i);
			pTexture->srv.append( pSRV );
		}
	}
	else
	{
		pTexture->srv.append(((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderResourceView(pTexture->textures[0], pTexture->srvFormat));
	}

	// render texture array?
	if (pTexture->GetFlags() & TEXFLAG_RENDERSLICES)
	{
		for (int i = 0; i < nSliceCount; i++)
		{
			ID3D10RenderTargetView* pRTV = ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderRenderTargetView(pTexture->textures[0], pTexture->rtvFormat, i);
			pTexture->rtv.append( pRTV );
		}
	}
	else
		pTexture->rtv.append(((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderRenderTargetView(pTexture->textures[0], pTexture->rtvFormat));

}

// It will add new rendertarget
ITexture* ShaderAPID3DX10::CreateRenderTarget(int width, int height, ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType, ER_TextureAddressMode textureAddress, ER_CompareFunc comparison, int nFlags)
{
	CD3D10Texture *pTexture = new CD3D10Texture;

	pTexture->SetDimensions(width,height);

	pTexture->SetFormat(nRTFormat);
	pTexture->SetFlags(nFlags | TEXFLAG_RENDERTARGET);
	pTexture->SetName("_rt_001");

	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);
	texSamplerParams.compareFunc = comparison;
	pTexture->SetSamplerState(texSamplerParams);

	pTexture->m_pD3D10SamplerState = CreateSamplerState(texSamplerParams);

	CScopedMutex scoped(m_Mutex);

	InternalCreateRenderTarget( pTexture, m_pD3DDevice );

	if(!pTexture->textures[0])
	{
		DestroyRenderState( pTexture->m_pD3D10SamplerState );
		delete pTexture;
		return NULL;
	}

	m_TextureList.append(pTexture);

	Finish();

	return pTexture;
}

// It will add new rendertarget
ITexture* ShaderAPID3DX10::CreateNamedRenderTarget(const char* pszName,int width, int height,ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType, ER_TextureAddressMode textureAddress, ER_CompareFunc comparison, int nFlags)
{
	CD3D10Texture *pTexture = new CD3D10Texture;

	pTexture->SetDimensions(width,height);

	pTexture->SetFormat(nRTFormat);
	pTexture->SetFlags(nFlags | TEXFLAG_RENDERTARGET);
	pTexture->SetName(pszName);
	
	
	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);
	texSamplerParams.compareFunc = comparison;
	pTexture->SetSamplerState(texSamplerParams);

	pTexture->m_pD3D10SamplerState = CreateSamplerState(texSamplerParams);

	CScopedMutex scoped(m_Mutex);

	InternalCreateRenderTarget(pTexture, m_pD3DDevice);

	if(!pTexture->textures[0])
	{
		delete pTexture;
		return NULL;
	}

	m_TextureList.append(pTexture);

	return pTexture;
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

// saves rendertarget to texture, you can also save screenshots
void ShaderAPID3DX10::SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName)
{
	if(pTargetTexture && pTargetTexture->GetFlags() & TEXFLAG_RENDERTARGET)
	{
		CD3D10Texture* pTexture = (CD3D10Texture*)pTargetTexture;

		D3DX10SaveTextureToFile(pTexture->textures[0], D3DX10_IFF_DDS, pFileName);
	}
}

// Copy render target to texture
void ShaderAPID3DX10::CopyFramebufferToTexture(ITexture* pTargetTexture)
{
	//ASSERT(!"Programming error: Copy framebuffer to texture is not available in DX10! Just use _rt_backbuffer to do effects.\n");
}

// Changes render target (MRT)
void ShaderAPID3DX10::ChangeRenderTargets(ITexture** pRenderTargets, int nNumRTs, int* nCubemapFaces, ITexture* pDepthTarget, int nDepthSlice)
{
	CD3D10Texture* pDepth = (CD3D10Texture*)pDepthTarget;

	ID3D10RenderTargetView* rtv[16] = {0};
	ID3D10DepthStencilView* dsv = NULL;

	if(pDepth == m_pDepthBufferTexture || pDepth == NULL)
		dsv = m_pDepthBufferDSV;
	else
		dsv = pDepth->dsv[nDepthSlice];

	m_pCurrentDepthRenderTarget = pDepthTarget;
	m_nCurrentDepthSlice = nDepthSlice;

	CD3D10Texture* pBackBuffer = (CD3D10Texture*)m_pBackBufferTexture;

	for (uint8 i = 0; i < nNumRTs; i++)
	{
		CD3D10Texture* pRt = (CD3D10Texture*)pRenderTargets[i];

		int nSlice = -1;

		if (nCubemapFaces == NULL || nCubemapFaces[i] == -1)
		{
			if (pRt == m_pBackBufferTexture || !pRt)
			{
				rtv[i] = pBackBuffer->rtv[0];
			}
			else
			{
				rtv[i] = pRt->rtv[0];
			}
		}
		else
		{
			nSlice = nCubemapFaces[i];

			rtv[i] = pRt->rtv[nSlice];
		}

		m_pCurrentColorRenderTargets[i] = pRt;
		m_nCurrentCRTSlice[i] = nSlice;
	}

	for (uint8 i = nNumRTs; i < MAX_MRTS; i++)
	{
		m_pCurrentColorRenderTargets[i] = NULL;
		m_nCurrentCRTSlice[i] = 0;
	}

	m_pD3DDevice->OMSetRenderTargets(nNumRTs, rtv, dsv);

	D3D10_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;

	if (nNumRTs > 0)
	{
		CD3D10Texture* pRt = (CD3D10Texture*)pRenderTargets[0];

		if (pRt == m_pBackBufferTexture)
		{
			vp.Width  = m_pBackBufferTexture->GetWidth();
			vp.Height = m_pBackBufferTexture->GetHeight();
		}
		else
		{
			vp.Width  = pRt->GetWidth();
			vp.Height = pRt->GetHeight();
		}
	}
	else
	{
		vp.Width  = pDepthTarget->GetWidth();
		vp.Height = pDepthTarget->GetHeight();
	}

	m_pD3DDevice->RSSetViewports(1, &vp);
}

// Changes back to backbuffer
void ShaderAPID3DX10::ChangeRenderTargetToBackBuffer()
{
	/*
	memset(m_pSelectedTextures,0,sizeof(m_pSelectedTextures));
	memset(m_pSelectedVertexTextures,0,sizeof(m_pSelectedVertexTextures));
	ApplyTextures();
	*/

	CD3D10Texture* pBackBuffer = (CD3D10Texture*)m_pBackBufferTexture;

	m_pD3DDevice->OMSetRenderTargets(1, &pBackBuffer->rtv[0], m_pDepthBufferDSV);
	m_pCurrentColorRenderTargets[0] = m_pBackBufferTexture;

	for (int i = 1; i < MAX_MRTS; i++)
	{
		m_pCurrentColorRenderTargets[i] = NULL;
		m_nCurrentCRTSlice[i] = 0;
	}

	if(m_pCurrentDepthRenderTarget != m_pDepthBufferTexture)
		m_pCurrentDepthRenderTarget = m_pDepthBufferTexture;
}

// resizes render target
void ShaderAPID3DX10::ResizeRenderTarget(ITexture* pRT, int newWide, int newTall)
{
	/*
	if(pRT->GetWidth() == newWide && pRT->GetHeight() == newTall)
		return;

	CD3D9Texture* pRenderTarget = (CD3D9Texture*)(pRT);

	pRenderTarget->Release();

	pRenderTarget->SetDimensions(newWide, newTall);

	InternalCreateRenderTarget(m_pD3DDevice, pRenderTarget, pRenderTarget->GetFlags());
	*/
}

// fills the current rendertarget buffers
void ShaderAPID3DX10::GetCurrentRenderTargets(ITexture* pRenderTargets[MAX_MRTS], int *numRTs, ITexture** pDepthTarget, int cubeNumbers[MAX_MRTS])
{
	/*
	int nRts = 0;

	for (register int i = 0; i < m_nMRTs; i++)
	{
		nRts++;

		pRenderTargets[i] = m_pCurrentColorRenderTargets[i];

		if(cubeNumbers != NULL)
			cubeNumbers[i] = m_nCurrentCRTSlice[i];

		if(m_pCurrentColorRenderTargets[i] == NULL)
			break;
	}

	*pDepthTarget = m_pCurrentDepthRenderTarget;

	*numRTs = nRts;
	*/
}

// returns current size of backbuffer surface
void ShaderAPID3DX10::GetViewportDimensions(int &wide, int &tall)
{
	wide = m_pBackBufferTexture->GetWidth();
	tall = m_pBackBufferTexture->GetHeight();

	/*
	D3DVIEWPORT9 vp;
	m_pD3DDevice->GetViewport(&vp);

	wide = vp.Width;
	tall = vp.Height;
	*/
}

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

// Matrix mode
void ShaderAPID3DX10::SetMatrixMode(ER_MatrixMode nMatrixMode)
{
	m_nCurrentMatrixMode = nMatrixMode;
}

// Will save matrix
void ShaderAPID3DX10::PushMatrix()
{
	// TODO: implement!
}

// Will reset matrix
void ShaderAPID3DX10::PopMatrix()
{
	// TODO: implement!
}

// Load identity matrix
void ShaderAPID3DX10::LoadIdentityMatrix()
{
	m_matrices[m_nCurrentMatrixMode] = identity4();
}

// Load custom matrix
void ShaderAPID3DX10::LoadMatrix(const Matrix4x4 &matrix)
{
	m_matrices[m_nCurrentMatrixMode] = matrix;
}

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

// Set Depth range for next primitives
void ShaderAPID3DX10::SetDepthRange(float fZNear,float fZFar)
{
	// Setup the viewport
	D3D10_VIEWPORT viewport;

	uint nVPs = 1;
	m_pD3DDevice->RSGetViewports(&nVPs, &viewport);

	viewport.MinDepth = fZNear;
	viewport.MaxDepth = fZFar;

	m_pD3DDevice->RSSetViewports(1, &viewport);
}

// Changes the vertex format
void ShaderAPID3DX10::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	if (pVertexFormat != m_pCurrentVertexFormat)
	{
		CVertexFormatD3DX10* pFormat = (CVertexFormatD3DX10*)pVertexFormat;
		if (pFormat != NULL)
		{
			m_pD3DDevice->IASetInputLayout(pFormat->m_pVertexDecl);

			CVertexFormatD3DX10* pCurrentFormat = (CVertexFormatD3DX10*)m_pCurrentVertexFormat;
			if (pCurrentFormat != NULL)
			{
				for (register uint8 i = 0; i < MAX_VERTEXSTREAM; i++)
				{
					if (pFormat->m_nVertexSize[i] != pCurrentFormat->m_nVertexSize[i])
					{
						//MsgError("Programming error! the IShaderAPI::ChangeVertexFormat() must be called after Reset(), but not after Apply()\n");
						m_pCurrentVertexBuffers[i] = NULL;
					}
				}
			}
		}

		m_pCurrentVertexFormat = pVertexFormat;
	}
}

// Changes the vertex buffer
void ShaderAPID3DX10::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset)
{
	if (pVertexBuffer != m_pCurrentVertexBuffers[nStream] || offset != m_nCurrentOffsets[nStream])
	{
		CVertexBufferD3DX10* pVB = (CVertexBufferD3DX10*)pVertexBuffer;
		//CVertexFormatD3DX10* pCurrentFormat = (CVertexFormatD3DX10*)m_pCurrentVertexFormat;

		UINT strides[1];
		UINT offsets[1];

		if (pVB == NULL)
		{
			strides[0] = 0;
			offsets[0] = 0;

			ID3D10Buffer *null[] = { NULL };
			m_pD3DDevice->IASetVertexBuffers(nStream, 1, null, strides, offsets);
		}
		else
		{
			strides[0] = pVertexBuffer->GetStrideSize();//pCurrentFormat->m_nVertexSize[nStream];
			offsets[0] = (UINT)offset*strides[0];
			m_pD3DDevice->IASetVertexBuffers(nStream, 1, &pVB->m_pVertexBuffer, strides, offsets);
		}

		m_pCurrentVertexBuffers[nStream] = pVertexBuffer;
		m_nCurrentOffsets[nStream] = offset;
	}
}

// Changes the index buffer
void ShaderAPID3DX10::ChangeIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	if (pIndexBuffer != m_pCurrentIndexBuffer)
	{
		CIndexBufferD3DX10* pIB = (CIndexBufferD3DX10*)pIndexBuffer;

		if (pIB == NULL)
		{
			m_pD3DDevice->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
		}
		else
		{
			DXGI_FORMAT format = pIB->m_nIndexSize < 4? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			m_pD3DDevice->IASetIndexBuffer(pIB->m_pIndexBuffer, format, 0);
		}

		m_pCurrentIndexBuffer = pIndexBuffer;
	}
}

// Destroy vertex format
void ShaderAPID3DX10::DestroyVertexFormat(IVertexFormat* pFormat)
{
	CVertexFormatD3DX10* pVF = (CVertexFormatD3DX10*)pFormat;
	if(!pVF)
		return;

	CScopedMutex m(m_Mutex);

	// reset if in use
	if(m_pCurrentVertexFormat == pVF)
	{
		Reset(STATE_RESET_VF);
		Apply();
	}

	DevMsg(DEVMSG_SHADERAPI,"Destroying vertex format\n");

	if(pVF->m_pVertexDecl)
		pVF->m_pVertexDecl->Release();

	m_VFList.remove(pVF);
	delete pVF;
}

// Destroy vertex buffer
void ShaderAPID3DX10::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	CVertexBufferD3DX10* pVB = (CVertexBufferD3DX10*)pVertexBuffer;
	if(!pVB)
		return;

	CScopedMutex m(m_Mutex);

	// reset if in use
	Reset(STATE_RESET_VBO);
	Apply();

	DevMsg(DEVMSG_SHADERAPI,"Destroying vertex buffer\n");

	if(pVB->m_pVertexBuffer)
		pVB->m_pVertexBuffer->Release();

	m_VBList.remove(pVB);
	delete pVB;
}

// Destroy index buffer
void ShaderAPID3DX10::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	CIndexBufferD3DX10* pIB = (CIndexBufferD3DX10*)pIndexBuffer;
	if(!pIB)
		return;

	CScopedMutex m(m_Mutex);

	// reset if in use
	Reset(STATE_RESET_VBO);
	Apply();


	DevMsg(DEVMSG_SHADERAPI,"Destroying index buffer\n");

	if(pIB->m_pIndexBuffer)
		pIB->m_pIndexBuffer->Release();

	m_IBList.remove(pIB);
	delete pIB;
}

static CD3D10MeshBuilder s_MeshBuilder;

// Creates new mesh builder
IMeshBuilder* ShaderAPID3DX10::CreateMeshBuilder()
{
	return &s_MeshBuilder;//new D3D9MeshBuilder();
}

void ShaderAPID3DX10::DestroyMeshBuilder(IMeshBuilder* pBuilder)
{
	//delete pBuilder;
}

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

// search for existing shader program
IShaderProgram* ShaderAPID3DX10::FindShaderProgram(const char* pszName, const char* query)
{
	CScopedMutex scoped(m_Mutex);

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

// Creates shader class for needed ShaderAPI
IShaderProgram* ShaderAPID3DX10::CreateNewShaderProgram(const char* pszName, const char* query)
{
	IShaderProgram* pNewProgram = new CD3D10ShaderProgram;
	pNewProgram->SetName((_Es(pszName)+query).GetData());

	CScopedMutex scoped(m_Mutex);

	m_ShaderList.append(pNewProgram);

	return pNewProgram;
}

// Destroy all shader
void ShaderAPID3DX10::DestroyShaderProgram(IShaderProgram* pShaderProgram)
{
	CD3D10ShaderProgram* pShader = (CD3D10ShaderProgram*)pShaderProgram;

	if(!pShader)
		return;

	CScopedMutex scoped(m_Mutex);

	pShader->Ref_Drop(); // decrease references to this shader

	// remove it if reference is zero
	if(pShader->Ref_Count() <= 0)
	{
		// Cancel shader and destroy
		Reset(STATE_RESET_SHADER);
		Apply();

		m_ShaderList.remove(pShader);

		delete pShader;
	}
}

int SamplerComp(const void *s0, const void *s1)
{
	return strcmp(((Sampler_t *) s0)->name, ((Sampler_t *) s1)->name);
}

int ConstantComp(const void *s0, const void *s1)
{
	return strcmp(((DX10ShaderConstant_t *) s0)->name, ((DX10ShaderConstant_t *) s1)->name);
}

ConVar r_skipShaderCache("r_skipShaderCache", "0", "Shader debugging purposes", 0);

struct shaderCacheHdr_t
{
	int		ident;			// DXXS

	long	psChecksum;	// file crc32
	long	vsChecksum;	// file crc32
	long	gsChecksum;	// file crc32

	int		psSize;
	int		vsSize;
	int		gsSize;
};

#define SHADERCACHE_IDENT		MCHAR4('D','X','X','S')

// Load any shader from stream
bool ShaderAPID3DX10::CompileShadersFromStream(	IShaderProgram* pShaderOutput,
												const shaderProgramCompileInfo_t& info,
												const char* extra)
{
	CD3D10ShaderProgram* pShader = (CD3D10ShaderProgram*)pShaderOutput;

	if(!pShader)
		return false;

	// TODO: implement shader cache for D3D10
	g_fileSystem->MakeDir("ShaderCache_DX10", SP_MOD);

	ID3D10ShaderReflection *vsRefl = NULL;
	ID3D10ShaderReflection *gsRefl = NULL;
	ID3D10ShaderReflection *psRefl = NULL;

	CScopedMutex m(m_Mutex);

	EqString cache_file_name(varargs("ShaderCache_DX10/%s.scache", pShaderOutput->GetName()));

	IFile* pStream = NULL;

	bool needsCompile = true;

	if(!(info.disableCache || r_skipShaderCache.GetBool()))
	{
		pStream = g_fileSystem->Open(cache_file_name.GetData(), "rb", -1);

		if(pStream)
		{
			ubyte* pShaderMem = NULL;

			shaderCacheHdr_t scHdr;
			pStream->Read(&scHdr, 1, sizeof(shaderCacheHdr_t));

			if(	scHdr.ident == SHADERCACHE_IDENT &&
				scHdr.vsChecksum == info.vs.checksum &&
				scHdr.gsChecksum == info.gs.checksum &&
				scHdr.psChecksum == info.ps.checksum)
			{
				// read vertex shader
				if(scHdr.vsSize > 0)
				{
					pShaderMem = (ubyte*)malloc(scHdr.vsSize);
					pStream->Read(pShaderMem, 1, scHdr.vsSize);
		
					if (SUCCEEDED(m_pD3DDevice->CreateVertexShader(pShaderMem, scHdr.vsSize, &pShader->m_pVertexShader)))
					{
						D3D10GetInputSignatureBlob(pShaderMem, scHdr.vsSize, &pShader->m_pInputSignature);
						D3D10ReflectShader(pShaderMem, scHdr.vsSize, &vsRefl);
					}

					free(pShaderMem);
				}

				// read geometry shader
				if(scHdr.gsSize > 0)
				{
					pShaderMem = (ubyte*)malloc(scHdr.gsSize);
					pStream->Read(pShaderMem, 1, scHdr.gsSize);


					if (SUCCEEDED(m_pD3DDevice->CreateGeometryShader(pShaderMem, scHdr.gsSize, &pShader->m_pGeomShader)))
					{
						D3D10ReflectShader(pShaderMem, scHdr.gsSize, &gsRefl);
					}

					free(pShaderMem);
				}

				// read pixel shader
				if(scHdr.psSize > 0)
				{
					pShaderMem = (ubyte*)malloc(scHdr.psSize);
					pStream->Read(pShaderMem, 1, scHdr.psSize);

					if (SUCCEEDED(m_pD3DDevice->CreatePixelShader(pShaderMem, scHdr.psSize, &pShader->m_pPixelShader)))
					{
						D3D10ReflectShader(pShaderMem, scHdr.psSize, &psRefl);
					}

					free(pShaderMem);
				}

				needsCompile = false;
			}
			else
			{
				MsgWarning("Shader cache for '%s' broken and will be recompiled\n", pShaderOutput->GetName());
			}

			g_fileSystem->Close(pStream);
			pStream = NULL;
		}

	}

	if(needsCompile)
	{
		pStream = g_fileSystem->Open(cache_file_name.GetData(), "wb", SP_MOD);

		if(!pStream)
		{
			MsgError("ERROR: Cannot create shader cache for %s\n",pShaderOutput->GetName());

			pStream = NULL;
		}
	}
	else
	{
		goto create_constant_buffers;
	}

	shaderCacheHdr_t scHdr;
	scHdr.ident = SHADERCACHE_IDENT;
	scHdr.vsSize = 0;
	scHdr.gsSize = 0;
	scHdr.psSize = 0;
	
	if(pStream) // write empty header
		pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));

	ID3D10Blob *shaderBuf = NULL;
	ID3D10Blob *errorsBuf = NULL;

	UINT compileFlags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_STRICTNESS;// | D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;

	if (info.vs.text != NULL)
	{
		EqString shaderString;
		if (extra  != NULL)
			shaderString.Append(extra);

		// we're always compiling SM 4.0

		shaderString.Append(info.vs.text);

#ifdef COMPILE_D3D_10_1
		// Use D3DX functions so we can compile to SM4.1
		if (SUCCEEDED(D3DX10CompileFromMemory(shaderString.getData(), shaderString.length(), pShader->GetName(), NULL, NULL, "vs_main", "vs_4_1", compileFlags, 0, NULL, &shaderBuf, &errorsBuf, NULL)))
		{
#else
		if (SUCCEEDED(D3D10CompileShader(shaderString.GetData(), shaderString.Length(), pShader->GetName(), NULL, NULL, "vs_main", "vs_4_0", compileFlags, &shaderBuf, &errorsBuf)))
		{
#endif
			if (SUCCEEDED(m_pD3DDevice->CreateVertexShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &pShader->m_pVertexShader)))
			{
				D3D10GetInputSignatureBlob(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &pShader->m_pInputSignature);
				D3D10ReflectShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &vsRefl);

				scHdr.vsSize = shaderBuf->GetBufferSize();

				// write compiled shader
				if(pStream)
					pStream->Write(shaderBuf->GetBufferPointer(), 1, scHdr.vsSize);

#ifdef _DEBUG
				ID3D10Blob *disasm = NULL;

				if (SUCCEEDED(D3D10DisassembleShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), FALSE, NULL, &disasm)))
					MsgAccept("%s\n", (const char *) disasm->GetBufferPointer());

				SAFE_RELEASE(disasm);
#endif
			}
		}
		else 
		{
			MsgError("ERROR: Cannot compile vertex shader for %s\n",pShader->GetName());

			if(errorsBuf)
			{
				MsgError("%s\n",(const char *) errorsBuf->GetBufferPointer());
			}
			else
			{
				MsgError("Unknown error\n");
			}
		}

		SAFE_RELEASE(shaderBuf);
		SAFE_RELEASE(errorsBuf);
	}

	if (info.gs.text != NULL)
	{
		EqString shaderString;

		if (extra  != NULL)
			shaderString.Append(extra);

		// we always compiling SM 4.0

		shaderString.Append(info.gs.text);

#ifdef COMPILE_D3D_10_1
		// Use D3DX functions so we can compile to SM4.1
		if (SUCCEEDED(D3DX10CompileFromMemory(shaderString.getData(), shaderString.length(), pShader->GetName(), NULL, NULL, "gs_main", "gs_4_1", compileFlags, 0, NULL, &shaderBuf, &errorsBuf, NULL)))
		{
#else
		if (SUCCEEDED(D3D10CompileShader(shaderString.GetData(), shaderString.Length(), pShader->GetName(), NULL, NULL, "gs_main", "gs_4_0", compileFlags, &shaderBuf, &errorsBuf)))
		{
#endif
			if (SUCCEEDED(m_pD3DDevice->CreateGeometryShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &pShader->m_pGeomShader)))
			{
				D3D10ReflectShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &gsRefl);

				scHdr.gsSize = shaderBuf->GetBufferSize();

				// write compiled shader
				if(pStream)
					pStream->Write(shaderBuf->GetBufferPointer(), 1, scHdr.gsSize);
#ifdef _DEBUG
				ID3D10Blob *disasm = NULL;
				if (SUCCEEDED(D3D10DisassembleShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), FALSE, NULL, &disasm)))
					MsgAccept("%s\n", (const char *) disasm->GetBufferPointer());

				SAFE_RELEASE(disasm);
#endif
			}
		}
		else
		{
			MsgError("ERROR: Cannot compile geometry shader for %s\n",pShader->GetName());

			if(errorsBuf)
			{
				MsgError("%s\n",(const char *) errorsBuf->GetBufferPointer());
			}
			else
			{
				MsgError("Unknown error\n");
			}
		}

		SAFE_RELEASE(shaderBuf);
		SAFE_RELEASE(errorsBuf);
	}

	if (info.ps.text != NULL)
	{
		EqString shaderString;
		if (extra  != NULL)
			shaderString.Append(extra);

		// we always compiling SM 4.0

		shaderString.Append(info.ps.text);

#ifdef COMPILE_D3D_10_1
		// Use D3DX functions so we can compile to SM4.1
		if (SUCCEEDED(D3DX10CompileFromMemory(shaderString.getData(), shaderString.length(), pShader->GetName(), NULL, NULL, "ps_main", "ps_4_1", compileFlags, 0, NULL, &shaderBuf, &errorsBuf, NULL)))
		{
#else
		if (SUCCEEDED(D3D10CompileShader(shaderString.GetData(), shaderString.Length(), pShader->GetName(), NULL, NULL, "ps_main", "ps_4_0", compileFlags, &shaderBuf, &errorsBuf)))
		{
#endif
			if (SUCCEEDED(m_pD3DDevice->CreatePixelShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &pShader->m_pPixelShader)))
			{
				D3D10ReflectShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), &psRefl);

				scHdr.psSize = shaderBuf->GetBufferSize();

				// write compiled shader
				if(pStream)
					pStream->Write(shaderBuf->GetBufferPointer(), 1, scHdr.psSize);
#ifdef _DEBUG
				ID3D10Blob *disasm = NULL;

				if (SUCCEEDED(D3D10DisassembleShader(shaderBuf->GetBufferPointer(), shaderBuf->GetBufferSize(), FALSE, NULL, &disasm)))
					MsgAccept("%s\n", (const char *) disasm->GetBufferPointer());

				SAFE_RELEASE(disasm);
#endif
			}
		}
		else
		{
			MsgError("ERROR: Cannot compile pixel shader for %s\n",pShader->GetName());

			if(errorsBuf)
			{
				MsgError("%s\n",(const char *) errorsBuf->GetBufferPointer());
			}
			else
			{
				MsgError("Unknown error\n");
			}
		}

		SAFE_RELEASE(shaderBuf);
		SAFE_RELEASE(errorsBuf);
	}

	if(pStream)
	{
		scHdr.vsChecksum = info.vs.checksum;
		scHdr.gsChecksum = info.gs.checksum;
		scHdr.psChecksum = info.ps.checksum;

		pStream->Seek(0,VS_SEEK_SET);
		pStream->Write(&scHdr, 1, sizeof(shaderCacheHdr_t));

		g_fileSystem->Close(pStream);
	}

create_constant_buffers:

	D3D10_SHADER_DESC vsDesc, gsDesc, psDesc;

	if (vsRefl)
	{
		vsRefl->GetDesc(&vsDesc);

		if (vsDesc.ConstantBuffers)
		{
			pShader->m_nVSCBuffers = vsDesc.ConstantBuffers;
			pShader->m_pvsConstants = new ID3D10Buffer *[pShader->m_nVSCBuffers];
			pShader->m_pvsConstMem = new ubyte *[pShader->m_nVSCBuffers];
			pShader->m_pvsDirty = new bool[pShader->m_nVSCBuffers];
		}
	}
	if (gsRefl)
	{
		gsRefl->GetDesc(&gsDesc);

		if (gsDesc.ConstantBuffers)
		{
			pShader->m_nGSCBuffers = gsDesc.ConstantBuffers;
			pShader->m_pgsConstants = new ID3D10Buffer *[pShader->m_nGSCBuffers];
			pShader->m_pgsConstMem = new ubyte *[pShader->m_nGSCBuffers];
			pShader->m_pgsDirty = new bool[pShader->m_nGSCBuffers];
		}
	}
	if (psRefl)
	{
		psRefl->GetDesc(&psDesc);

		if (psDesc.ConstantBuffers)
		{
			pShader->m_nPSCBuffers = psDesc.ConstantBuffers;
			pShader->m_ppsConstants = new ID3D10Buffer *[pShader->m_nPSCBuffers];
			pShader->m_ppsConstMem = new ubyte *[pShader->m_nPSCBuffers];
			pShader->m_ppsDirty = new bool[pShader->m_nPSCBuffers];
		}
	}

	D3D10_SHADER_BUFFER_DESC sbDesc;

	D3D10_BUFFER_DESC cbDesc;
	cbDesc.Usage = D3D10_USAGE_DEFAULT;//D3D10_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;//D3D10_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;

	DkList <DX10ShaderConstant_t> constants;

	for (uint i = 0; i < pShader->m_nVSCBuffers; i++)
	{
		vsRefl->GetConstantBufferByIndex(i)->GetDesc(&sbDesc);

		cbDesc.ByteWidth = sbDesc.Size;
		m_pD3DDevice->CreateBuffer(&cbDesc, NULL, &pShader->m_pvsConstants[i]);

		pShader->m_pvsConstMem[i] = new ubyte[sbDesc.Size];
		for (uint k = 0; k < sbDesc.Variables; k++)
		{
			D3D10_SHADER_VARIABLE_DESC vDesc;
			vsRefl->GetConstantBufferByIndex(i)->GetVariableByIndex(k)->GetDesc(&vDesc);

			DX10ShaderConstant_t constant;
			size_t length = strlen(vDesc.Name);
			constant.name = new char[length + 1];
			strcpy(constant.name, vDesc.Name);
			constant.vsData = pShader->m_pvsConstMem[i] + vDesc.StartOffset;
			constant.gsData = NULL;
			constant.psData = NULL;
			constant.vsBuffer = i;
			constant.gsBuffer = -1;
			constant.psBuffer = -1;

			constant.constFlags = SCONST_VERTEX;

			constants.append(constant);
		}

		pShader->m_pvsDirty[i] = false;
	}

	uint maxConst = constants.numElem();
	for (uint i = 0; i < pShader->m_nGSCBuffers; i++)
	{
		gsRefl->GetConstantBufferByIndex(i)->GetDesc(&sbDesc);

		cbDesc.ByteWidth = sbDesc.Size;
		m_pD3DDevice->CreateBuffer(&cbDesc, NULL, &pShader->m_pgsConstants[i]);

		pShader->m_pgsConstMem[i] = new ubyte[sbDesc.Size];
		for (uint j = 0; j < sbDesc.Variables; j++)
		{
			D3D10_SHADER_VARIABLE_DESC vDesc;
			gsRefl->GetConstantBufferByIndex(i)->GetVariableByIndex(j)->GetDesc(&vDesc);

			int merge = -1;
			for (uint k = 0; k < maxConst; k++)
			{
				if (strcmp(constants[k].name, vDesc.Name) == 0)
				{
					merge = k;
					break;
				}
			}

			if (merge < 0)
			{
				DX10ShaderConstant_t constant;
				size_t length = strlen(vDesc.Name);
				constant.name = new char[length + 1];
				strcpy(constant.name, vDesc.Name);
				constant.vsData = NULL;
				constant.gsData = pShader->m_pgsConstMem[i] + vDesc.StartOffset;
				constant.psData = NULL;
				constant.vsBuffer = -1;
				constant.gsBuffer = i;
				constant.psBuffer = -1;

				constant.constFlags = SCONST_GEOMETRY;

				constants.append(constant);
			}
			else
			{
				constants[merge].gsData = pShader->m_pgsConstMem[i] + vDesc.StartOffset;
				constants[merge].gsBuffer = i;
				constants[merge].constFlags |= SCONST_GEOMETRY; // add shader setup flags
			}
		}

		pShader->m_pgsDirty[i] = false;
	}

	maxConst = constants.numElem();
	for (uint i = 0; i < pShader->m_nPSCBuffers; i++)
	{
		psRefl->GetConstantBufferByIndex(i)->GetDesc(&sbDesc);

		cbDesc.ByteWidth = sbDesc.Size;
		m_pD3DDevice->CreateBuffer(&cbDesc, NULL, &pShader->m_ppsConstants[i]);

		pShader->m_ppsConstMem[i] = new ubyte[sbDesc.Size];
		for (uint j = 0; j < sbDesc.Variables; j++)
		{
			D3D10_SHADER_VARIABLE_DESC vDesc;
			psRefl->GetConstantBufferByIndex(i)->GetVariableByIndex(j)->GetDesc(&vDesc);

			int merge = -1;
			for (uint k = 0; k < maxConst; k++)
			{
				if (strcmp(constants[k].name, vDesc.Name) == 0)
				{
					merge = k;
					break;
				}
			}

			if (merge < 0)
			{
				DX10ShaderConstant_t constant;
				size_t length = strlen(vDesc.Name);
				constant.name = new char[length + 1];
				strcpy(constant.name, vDesc.Name);
				constant.vsData = NULL;
				constant.gsData = NULL;
				constant.psData = pShader->m_ppsConstMem[i] + vDesc.StartOffset;
				constant.vsBuffer = -1;
				constant.gsBuffer = -1;
				constant.psBuffer = i;

				constant.constFlags = SCONST_PIXEL;

				constants.append(constant);
			}
			else
			{
				constants[merge].psData = pShader->m_ppsConstMem[i] + vDesc.StartOffset;
				constants[merge].psBuffer = i;
				constants[merge].constFlags |= SCONST_PIXEL; // add shader setup flags
			}
		}

		pShader->m_ppsDirty[i] = false;
	}

	pShader->m_numConstants = constants.numElem();
	pShader->m_pConstants = new DX10ShaderConstant_t[pShader->m_numConstants];

	memcpy(pShader->m_pConstants, constants.ptr(), pShader->m_numConstants * sizeof(DX10ShaderConstant_t));
	qsort(pShader->m_pConstants, pShader->m_numConstants, sizeof(DX10ShaderConstant_t), ConstantComp);

	uint nMaxVSRes = vsRefl? vsDesc.BoundResources : 0;
	uint nMaxGSRes = gsRefl? gsDesc.BoundResources : 0;
	uint nMaxPSRes = psRefl? psDesc.BoundResources : 0;

	uint maxResources = nMaxVSRes + nMaxGSRes + nMaxPSRes;

	if (maxResources > 0)
	{
		pShader->m_pTextures = (Sampler_t *) malloc(maxResources * sizeof(Sampler_t));
		pShader->m_pSamplers = (Sampler_t *) malloc(maxResources * sizeof(Sampler_t));
		pShader->m_numTextures = 0;
		pShader->m_numSamplers = 0;

		D3D10_SHADER_INPUT_BIND_DESC siDesc;
		for (uint i = 0; i < nMaxVSRes; i++)
		{
			vsRefl->GetResourceBindingDesc(i, &siDesc);

			if (siDesc.Type == D3D10_SIT_TEXTURE)
			{
				//size_t length = strlen(siDesc.Name);
				strcpy(pShader->m_pTextures[pShader->m_numTextures].name, siDesc.Name);
				pShader->m_pTextures[pShader->m_numTextures].vsIndex = siDesc.BindPoint;
				pShader->m_pTextures[pShader->m_numTextures].gsIndex = 0xFFFFFFFF;
				pShader->m_pTextures[pShader->m_numTextures].index = 0xFFFFFFFF;
				pShader->m_numTextures++;
			}
			else if (siDesc.Type == D3D10_SIT_SAMPLER)
			{
				//size_t length = strlen(siDesc.Name);
				strcpy(pShader->m_pSamplers[pShader->m_numSamplers].name, siDesc.Name);
				pShader->m_pSamplers[pShader->m_numSamplers].vsIndex = siDesc.BindPoint;
				pShader->m_pSamplers[pShader->m_numSamplers].gsIndex = 0xFFFFFFFF;
				pShader->m_pSamplers[pShader->m_numSamplers].index = 0xFFFFFFFF;
				pShader->m_numSamplers++;
			}
		}
		uint maxTexture = pShader->m_numTextures;
		uint maxSampler = pShader->m_numSamplers;
		for (uint i = 0; i < nMaxGSRes; i++)
		{
			gsRefl->GetResourceBindingDesc(i, &siDesc);

			if (siDesc.Type == D3D10_SIT_TEXTURE)
			{
				int merge = -1;
				for (uint j = 0; j < maxTexture; j++)
				{
					if (strcmp(pShader->m_pTextures[j].name, siDesc.Name) == 0)
					{
						merge = j;
						break;
					}
				}

				if (merge < 0)
				{
					//size_t length = strlen(siDesc.Name);
					strcpy(pShader->m_pTextures[pShader->m_numTextures].name, siDesc.Name);
					pShader->m_pTextures[pShader->m_numTextures].vsIndex = 0xFFFFFFFF;
					pShader->m_pTextures[pShader->m_numTextures].gsIndex = siDesc.BindPoint;
					pShader->m_pTextures[pShader->m_numTextures].index = 0xFFFFFFFF;
					pShader->m_numTextures++;
				}
				else
				{
					pShader->m_pTextures[merge].gsIndex = siDesc.BindPoint;
				}
			}
			else if (siDesc.Type == D3D10_SIT_SAMPLER)
			{
				int merge = -1;
				for (uint j = 0; j < maxSampler; j++)
				{
					if (strcmp(pShader->m_pSamplers[j].name, siDesc.Name) == 0)
					{
						merge = j;
						break;
					}
				}

				if (merge < 0)
				{
					//size_t length = strlen(siDesc.Name);
					strcpy(pShader->m_pSamplers[pShader->m_numSamplers].name, siDesc.Name);
					pShader->m_pSamplers[pShader->m_numSamplers].vsIndex = 0xFFFFFFFF;
					pShader->m_pSamplers[pShader->m_numSamplers].gsIndex = siDesc.BindPoint;
					pShader->m_pSamplers[pShader->m_numSamplers].index = 0xFFFFFFFF;
					pShader->m_numSamplers++;
				}
				else
				{
					pShader->m_pSamplers[merge].gsIndex = siDesc.BindPoint;
				}
			}
		}
		maxTexture = pShader->m_numTextures;
		maxSampler = pShader->m_numSamplers;
		for (uint i = 0; i < nMaxPSRes; i++)
		{
			psRefl->GetResourceBindingDesc(i, &siDesc);

			if (siDesc.Type == D3D10_SIT_TEXTURE)
			{
				int merge = -1;
				for (uint j = 0; j < maxTexture; j++)
				{
					if (strcmp(pShader->m_pTextures[j].name, siDesc.Name) == 0)
					{
						merge = j;
						break;
					}
				}

				if (merge < 0)
				{
					//size_t length = strlen(siDesc.Name);
					strcpy(pShader->m_pTextures[pShader->m_numTextures].name, siDesc.Name);
					pShader->m_pTextures[pShader->m_numTextures].vsIndex = 0xFFFFFFFF;
					pShader->m_pTextures[pShader->m_numTextures].gsIndex = 0xFFFFFFFF;
					pShader->m_pTextures[pShader->m_numTextures].index = siDesc.BindPoint;
					pShader->m_numTextures++;
				}
				else
				{
					pShader->m_pTextures[merge].index = siDesc.BindPoint;
				}
			}
			else if (siDesc.Type == D3D10_SIT_SAMPLER)
			{
				int merge = -1;
				for (uint j = 0; j < maxSampler; j++)
				{
					if (strcmp(pShader->m_pSamplers[j].name, siDesc.Name) == 0)
					{
						merge = j;
						break;
					}
				}

				if (merge < 0)
				{
					//size_t length = strlen(siDesc.Name);
					strcpy(pShader->m_pSamplers[pShader->m_numSamplers].name, siDesc.Name);
					pShader->m_pSamplers[pShader->m_numSamplers].vsIndex = 0xFFFFFFFF;
					pShader->m_pSamplers[pShader->m_numSamplers].gsIndex = 0xFFFFFFFF;
					pShader->m_pSamplers[pShader->m_numSamplers].index = siDesc.BindPoint;
					pShader->m_numSamplers++;
				}
				else
				{
					pShader->m_pSamplers[merge].index = siDesc.BindPoint;
				}
			}
		}

		pShader->m_pTextures = (Sampler_t *) realloc(pShader->m_pTextures, pShader->m_numTextures * sizeof(Sampler_t));
		pShader->m_pSamplers = (Sampler_t *) realloc(pShader->m_pSamplers, pShader->m_numSamplers * sizeof(Sampler_t));

		qsort(pShader->m_pTextures, pShader->m_numTextures, sizeof(Sampler_t), SamplerComp);
		qsort(pShader->m_pSamplers, pShader->m_numSamplers, sizeof(Sampler_t), SamplerComp);
	}

	if (vsRefl)
		vsRefl->Release();

	if (gsRefl)
		gsRefl->Release();

	if (psRefl)
		psRefl->Release();

	return true;
}

// Set current shader for rendering
void ShaderAPID3DX10::SetShader(IShaderProgram* pShader)
{
	m_pSelectedShader = pShader;
}

int	ShaderAPID3DX10::GetSamplerUnit(IShaderProgram* pProgram,const char* pszSamplerName)
{
	/*
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(pProgram);
	if(!pShader)
		return -1;

	Sampler_t *samplers = pShader->m_pSamplers;
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

	*/
	return -1;
}

// Set Texture for shader
void ShaderAPID3DX10::SetShaderTexture(const char* pszName, ITexture* pTexture)
{
	int unit = GetSamplerUnit(m_pSelectedShader, pszName);

	if (unit >= 0)
	{
		m_pSelectedTextures[unit] = pTexture;
	}
}

/*
void ShaderAPID3DX10::SetShaderConstantInt(const char *pszName, const int constant)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetInt(m_pD3DDevice, handle, constant);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetInt(m_pD3DDevice, handle, constant);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantFloat(const char *pszName, const float constant)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetFloat(m_pD3DDevice, handle, constant);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetFloat(m_pD3DDevice, handle, constant);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantVector2D(const char *pszName, const Vector2D &constant)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, 2);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, 2);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantVector3D(const char *pszName, const Vector3D &constant)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, 3);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, 3);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantVector4D(const char *pszName, const Vector4D &constant)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetVector(m_pD3DDevice, handle, (D3DXVECTOR4 *) &constant);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetVector(m_pD3DDevice, handle, (D3DXVECTOR4 *) &constant);
			}
		}
	}
}

// do quick setup for matrices
void ShaderAPID3DX10::SetShaderConstantMatrix4(const char *pszName, const Matrix4x4 &constant)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetMatrix(m_pD3DDevice, handle, (D3DXMATRIX *) &constant);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetMatrix(m_pD3DDevice, handle, (D3DXMATRIX *) &constant);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantArrayFloat(const char *pszName, const float *constant, int count)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, count);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, count);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantArrayVector2D(const char *pszName, const Vector2D *constant, int count)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, count * 2);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, count * 2);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantArrayVector3D(const char *pszName, const Vector3D *constant, int count)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, count * 3);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetFloatArray(m_pD3DDevice, handle, (FLOAT *) &constant, count * 3);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantArrayVector4D(const char *pszName, const Vector4D *constant, int count)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetVectorArray(m_pD3DDevice, handle, (D3DXVECTOR4 *) &constant, count);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetVectorArray(m_pD3DDevice, handle, (D3DXVECTOR4 *) &constant, count);
			}
		}
	}
}

void ShaderAPID3DX10::SetShaderConstantArrayMatrix4(const char *pszName, const Matrix4x4 *constant, int count)
{
	CD3D9ShaderProgram* pShader = dynamic_cast<CD3D9ShaderProgram*>(m_pSelectedShader);

	if(!pShader)
	{
		return;
	}

	D3DXHANDLE handle;
	if (pShader->m_pVertexShader != NULL)
	{
		if (pShader->m_pVSConstants != NULL)
		{
			if (handle = pShader->m_pVSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pVSConstants->SetMatrixArray(m_pD3DDevice, handle, (D3DXMATRIX *) &constant, count);
			}
		}
	}

	if (pShader->m_pPixelShader != NULL)
	{
		if (pShader->m_pPSConstants != NULL)
		{
			if (handle = pShader->m_pPSConstants->GetConstantByName(NULL, pszName))
			{
				pShader->m_pPSConstants->SetMatrixArray(m_pD3DDevice, handle, (D3DXMATRIX *) &constant, count);
			}
		}
	}
}
*/

// RAW Constant (Used for structure types, etc.)
int ShaderAPID3DX10::SetShaderConstantRaw(const char *pszName, const void *data, int nSize, int nConstID)
{
	if(data == NULL || nSize == NULL)
		return nConstID;

	CD3D10ShaderProgram* pProgram = (CD3D10ShaderProgram*)m_pSelectedShader;

	if(!pProgram)
		return -1;

	DX10ShaderConstant_t *constants = pProgram->m_pConstants;
	/*
	if(nConstID != -1)
	{
		DX10ShaderConstant_t *c = constants + nConstID;

		if (c->vsData)
		{
			if (memcmp(c->vsData, data, nSize))
			{
				memcpy(c->vsData, data, nSize);
				pProgram->m_pvsDirty[c->vsBuffer] = true;
			}
		}
		if (c->gsData)
		{
			if (memcmp(c->gsData, data, nSize))
			{
				memcpy(c->gsData, data, nSize);
				pProgram->m_pgsDirty[c->gsBuffer] = true;
			}
		}
		if (c->psData)
		{
			if (memcmp(c->psData, data, nSize))
			{
				memcpy(c->psData, data, nSize);
				pProgram->m_ppsDirty[c->psBuffer] = true;
			}
		}

		return nConstID;
	}
	*/

	int minConstant = 0;
	int maxConstant = pProgram->m_numConstants - 1;

	// Do a quick lookup in the sorted table with a binary search
	while (minConstant <= maxConstant)
	{
		int currConstant = (minConstant + maxConstant) >> 1;

		int res = strcmp(pszName, constants[currConstant].name);
		if (res == 0)
		{
			DX10ShaderConstant_t *c = constants + currConstant;

			if (c->vsData)
			{
				memcpy(c->vsData, data, nSize);
				pProgram->m_pvsDirty[c->vsBuffer] = true;
			}
			if (c->gsData)
			{
				memcpy(c->gsData, data, nSize);
				pProgram->m_pgsDirty[c->gsBuffer] = true;
			}
			if (c->psData)
			{
				memcpy(c->psData, data, nSize);
				pProgram->m_ppsDirty[c->psBuffer] = true;
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

//-----------------------------------------------------
// Advanced shader programming
//-----------------------------------------------------
/*
// Pixel Shader constants setup (by register number)
void ShaderAPID3DX10::SetPixelShaderConstantInt(int reg, const int constant)
{
	int val[4];

	val[0] = val[1] = val[2] = val[3] = constant;

//	m_pD3DDevice->SetPixelShaderConstantI(reg, val, 1);
}

void ShaderAPID3DX10::SetPixelShaderConstantFloat(int reg, const float constant)
{
	float val[4];

	val[0] = val[1] = val[2] = val[3] = constant;

	//m_pD3DDevice->SetPixelShaderConstantF(reg, val, 1);
}

void ShaderAPID3DX10::SetPixelShaderConstantVector2D(int reg, const Vector2D &constant)
{
	SetPixelShaderConstantRaw(reg, &constant);
}

void ShaderAPID3DX10::SetPixelShaderConstantVector3D(int reg, const Vector3D &constant)
{
	SetPixelShaderConstantRaw(reg, &constant);
}

void ShaderAPID3DX10::SetPixelShaderConstantVector4D(int reg, const Vector4D &constant)
{
	SetPixelShaderConstantRaw(reg, &constant);
}

void ShaderAPID3DX10::SetPixelShaderConstantMatrix4(int reg, const Matrix4x4 &constant)
{
	SetPixelShaderConstantRaw(reg, &constant, 4);
}

void ShaderAPID3DX10::SetPixelShaderConstantFloatArray(int reg, const float *constant, int count)
{
	SetPixelShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetPixelShaderConstantVector2DArray(int reg, const Vector2D *constant, int count)
{
	SetPixelShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetPixelShaderConstantVector3DArray(int reg, const Vector3D *constant, int count)
{
	SetPixelShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetPixelShaderConstantVector4DArray(int reg, const Vector4D *constant, int count)
{
	SetPixelShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetPixelShaderConstantMatrix4Array(int reg, const Matrix4x4 *constant, int count)
{
	SetPixelShaderConstantRaw(reg, constant, count * 4);
}

// Vertex Shader constants setup (by register number)
void ShaderAPID3DX10::SetVertexShaderConstantInt(int reg, const int constant)
{
	int val[4];

	val[0] = val[1] = val[2] = val[3] = constant;

//	m_pD3DDevice->SetVertexShaderConstantI(reg, val, 1);
}

void ShaderAPID3DX10::SetVertexShaderConstantFloat(int reg, const float constant)
{
	float val[4];

	val[0] = val[1] = val[2] = val[3] = constant;

	//m_pD3DDevice->SetVertexShaderConstantF(reg, val, 1);
}

void ShaderAPID3DX10::SetVertexShaderConstantVector2D(int reg, const Vector2D &constant)
{
	SetVertexShaderConstantRaw(reg, &constant);
}

void ShaderAPID3DX10::SetVertexShaderConstantVector3D(int reg, const Vector3D &constant)
{
	SetVertexShaderConstantRaw(reg, &constant);
}

void ShaderAPID3DX10::SetVertexShaderConstantVector4D(int reg, const Vector4D &constant)
{
	SetVertexShaderConstantRaw(reg, &constant);
}

void ShaderAPID3DX10::SetVertexShaderConstantMatrix4(int reg, const Matrix4x4 &constant)
{
	SetVertexShaderConstantRaw(reg, &constant, 4);
}

void ShaderAPID3DX10::SetVertexShaderConstantFloatArray(int reg, const float *constant, int count)
{
	SetVertexShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetVertexShaderConstantVector2DArray(int reg, const Vector2D *constant, int count)
{
	SetVertexShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetVertexShaderConstantVector3DArray(int reg, const Vector3D *constant, int count)
{
	SetVertexShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetVertexShaderConstantVector4DArray(int reg, const Vector4D *constant, int count)
{
	SetVertexShaderConstantRaw(reg, constant, count);
}

void ShaderAPID3DX10::SetVertexShaderConstantMatrix4Array(int reg, const Matrix4x4 *constant, int count)
{
	SetVertexShaderConstantRaw(reg, constant, count * 4);
}

// RAW Constant
void ShaderAPID3DX10::SetPixelShaderConstantRaw(int reg, const void *data, int nVectors)
{
	//m_pD3DDevice->SetPixelShaderConstantF(reg, (float*)data, nVectors);
}

void ShaderAPID3DX10::SetVertexShaderConstantRaw(int reg, const void *data, int nVectors)
{
	//m_pD3DDevice->SetVertexShaderConstantF(reg, (float*)data, nVectors);
}
*/
//-------------------------------------------------------------
// State manipulation 
//-------------------------------------------------------------

// creates blending state
IRenderState* ShaderAPID3DX10::CreateBlendingState( const BlendStateParam_t &blendDesc )
{
	CD3D10BlendingState* pState = NULL;

	for(int i = 0; i < m_BlendStates.numElem(); i++)
	{
		pState = (CD3D10BlendingState*)m_BlendStates[i];

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

	pState = new CD3D10BlendingState;
	pState->m_params = blendDesc;

	BOOL blendEnable = (blendDesc.srcFactor != BLENDFACTOR_ONE || blendDesc.dstFactor != BLENDFACTOR_ZERO);

	D3D10_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = (BOOL) false;//blendDesc.alphaTest;

	desc.BlendOp = blendingModes[blendDesc.blendFunc];
	desc.SrcBlend = blendingConsts[blendDesc.srcFactor];
	desc.DestBlend = blendingConsts[blendDesc.dstFactor];

	desc.BlendOpAlpha = blendingModes[blendDesc.blendFunc];
	desc.SrcBlendAlpha = blendingConstsAlpha[blendDesc.srcFactor];
	desc.DestBlendAlpha = blendingConstsAlpha[blendDesc.dstFactor];

	memset(&desc.BlendEnable, 0, sizeof(desc.BlendEnable));
	memset(&desc.RenderTargetWriteMask, 0, sizeof(desc.RenderTargetWriteMask));
	desc.BlendEnable[0] = blendEnable;

	for(int i = 0; i < 8; i++)
		desc.RenderTargetWriteMask[i] = blendDesc.mask;

	CScopedMutex scoped(m_Mutex);

	if (FAILED(m_pD3DDevice->CreateBlendState(&desc, &pState->m_blendState)))
	{
		MsgError("ERROR: Couldn't create blend state!\n");
		MsgError("blendfunc = %d!\n", blendDesc.blendFunc);
		MsgError("srcFactor = %d!\n", blendDesc.srcFactor);
		MsgError("dstFactor = %d!\n", blendDesc.dstFactor);
		MsgError("BlendEnable = %d!\n", blendDesc.blendEnable);
		MsgError("mask = %x!\n", blendDesc.mask);

		delete pState;
		return NULL;
	}

	m_BlendStates.append(pState);

	pState->AddReference();

	return pState;
}
	
// creates depth/stencil state
IRenderState* ShaderAPID3DX10::CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc )
{
	CD3D10DepthStencilState* pState = NULL;
	
	{
		CScopedMutex scoped(m_Mutex);

		for(int i = 0; i < m_DepthStates.numElem(); i++)
		{
			pState = (CD3D10DepthStencilState*)m_DepthStates[i];

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
	}
	
	pState = new CD3D10DepthStencilState;
	pState->m_params = depthDesc;

	D3D10_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = (BOOL) depthDesc.depthTest;
	desc.DepthWriteMask = depthDesc.depthWrite? D3D10_DEPTH_WRITE_MASK_ALL : D3D10_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = comparisonConst[depthDesc.depthFunc];
	desc.StencilEnable = (BOOL) depthDesc.doStencilTest;
	desc.StencilReadMask  = depthDesc.nStencilMask;
	desc.StencilWriteMask = depthDesc.nStencilWriteMask;

	desc.BackFace. StencilFunc = comparisonConst[depthDesc.nStencilFunc];
	desc.FrontFace.StencilFunc = comparisonConst[depthDesc.nStencilFunc];
	desc.BackFace. StencilDepthFailOp = stencilConst[depthDesc.nDepthFail];
	desc.FrontFace.StencilDepthFailOp = stencilConst[depthDesc.nDepthFail];
	desc.BackFace. StencilFailOp = stencilConst[depthDesc.nStencilFail];
	desc.FrontFace.StencilFailOp = stencilConst[depthDesc.nStencilFail];
	desc.BackFace. StencilPassOp = stencilConst[depthDesc.nStencilPass];
	desc.FrontFace.StencilPassOp = stencilConst[depthDesc.nStencilPass];

	CScopedMutex scoped(m_Mutex);

	if (FAILED(m_pD3DDevice->CreateDepthStencilState(&desc, &pState->m_dsState)))
	{
		MsgError("ERROR: Couldn't create depth state!\n");
		delete pState;
		return NULL;
	}

	m_DepthStates.append(pState);

	pState->AddReference();

	return pState;
}

// creates rasterizer state
IRenderState* ShaderAPID3DX10::CreateRasterizerState( const RasterizerStateParams_t &rasterDesc )
{
	CD3D10RasterizerState* pState = NULL;

	CScopedMutex scoped(m_Mutex);

	{
		for(int i = 0; i < m_RasterizerStates.numElem(); i++)
		{
			pState = (CD3D10RasterizerState*)m_RasterizerStates[i];

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
	}

	pState = new CD3D10RasterizerState;
	pState->m_params = rasterDesc;

	D3D10_RASTERIZER_DESC desc;
	desc.CullMode = cullConst[rasterDesc.cullMode];
	desc.FillMode = fillConst[rasterDesc.fillMode];
	desc.FrontCounterClockwise = FALSE;

	// TODO: implement depth bias
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;

	desc.AntialiasedLineEnable = FALSE;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = (BOOL) rasterDesc.multiSample;
	desc.ScissorEnable = (BOOL) rasterDesc.scissor;

	if (FAILED(m_pD3DDevice->CreateRasterizerState(&desc, &pState->m_rsState)))
	{
		MsgError("ERROR: Couldn't create rasterizer state!\n");
		delete pState;
		return NULL;
	}

	pState->AddReference();

	m_RasterizerStates.append(pState);

	return pState;
}

// creates rasterizer state
IRenderState* ShaderAPID3DX10::CreateSamplerState( const SamplerStateParam_t &samplerDesc )
{
	CD3D10SamplerState* pState = NULL;

	CScopedMutex scoped(m_Mutex);

	{
		for(int i = 0; i < m_SamplerStates.numElem(); i++)
		{
			pState = (CD3D10SamplerState*)m_SamplerStates[i];

			if(samplerDesc.aniso == pState->m_params.aniso &&
				samplerDesc.minFilter == pState->m_params.minFilter &&
				samplerDesc.magFilter == pState->m_params.magFilter &&
				samplerDesc.wrapS == pState->m_params.wrapS &&
				samplerDesc.wrapT == pState->m_params.wrapT &&
				samplerDesc.wrapR == pState->m_params.wrapR &&
				samplerDesc.lod == pState->m_params.lod &&
				samplerDesc.compareFunc == pState->m_params.compareFunc)
			{
				pState->AddReference();
				return pState;
			}
		}
	}

	pState = new CD3D10SamplerState;
	pState->m_params = samplerDesc;

	D3D10_SAMPLER_DESC desc;
	desc.Filter = d3dFilterType[samplerDesc.minFilter];

	if (samplerDesc.compareFunc != COMP_NEVER)
		desc.Filter = (D3D10_FILTER) (desc.Filter | D3D10_COMPARISON_FILTERING_BIT);

	desc.ComparisonFunc = comparisonConst[samplerDesc.compareFunc];
	desc.AddressU = d3dAddressMode[samplerDesc.wrapS];
	desc.AddressV = d3dAddressMode[samplerDesc.wrapT];
	desc.AddressW = d3dAddressMode[samplerDesc.wrapR];
	desc.MipLODBias = samplerDesc.lod;
	desc.MaxAnisotropy = HasAniso(samplerDesc.minFilter)? m_caps.maxTextureAnisotropicLevel : 1;
	desc.BorderColor[0] = 0;
	desc.BorderColor[1] = 0;
	desc.BorderColor[2] = 0;
	desc.BorderColor[3] = 0;
	desc.MinLOD = 0;
	desc.MaxLOD = HasAniso(samplerDesc.minFilter)? D3D10_FLOAT32_MAX : 0;

	if (FAILED(m_pD3DDevice->CreateSamplerState(&desc, &pState->m_samplerState)))
	{
		MsgError("ERROR: Couldn't create sampler state!\n");
		delete pState;
		return NULL;
	}

	pState->AddReference();

	m_SamplerStates.append(pState);

	return pState;
}

// completely destroys shader
void ShaderAPID3DX10::DestroyRenderState( IRenderState* pState, bool removeAllRefs )
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
			((CD3D10BlendingState*)pState)->m_blendState->Release();
			delete ((CD3D10BlendingState*)pState);
			m_BlendStates.remove(pState);
			break;
		case RENDERSTATE_RASTERIZER:
			((CD3D10RasterizerState*)pState)->m_rsState->Release();
			delete ((CD3D10RasterizerState*)pState);
			m_RasterizerStates.remove(pState);
			break;
		case RENDERSTATE_DEPTHSTENCIL:
			((CD3D10DepthStencilState*)pState)->m_dsState->Release();
			delete ((CD3D10DepthStencilState*)pState);
			m_DepthStates.remove(pState);
			break;
		case RENDERSTATE_SAMPLER:
			((CD3D10SamplerState*)pState)->m_samplerState->Release();
			delete ((CD3D10SamplerState*)pState);
			m_SamplerStates.remove(pState);
			break;
	}
}

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

IVertexFormat* ShaderAPID3DX10::CreateVertexFormat(VertexFormatDesc_s *formatDesc, int nAttribs)
{
	static const DXGI_FORMAT vformats[][4] = {
		DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8_UNORM,  DXGI_FORMAT_R8G8_UNORM,   DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R8G8B8A8_UNORM,
	};

	int index[8];
	memset(index, 0, sizeof(index));

	CVertexFormatD3DX10* pFormat = new CVertexFormatD3DX10();

	D3D10_INPUT_ELEMENT_DESC *pDesc = new D3D10_INPUT_ELEMENT_DESC[nAttribs + 1];

	static const char *semantics[] = {
		NULL,
		"COLOR",
		"POSITION",
		"TEXCOORD",
		"NORMAL",
		"TANGENT",
		"BINORMAL",
	};

	static const char *formatNames[] = {
		"float",
		"half",
		"uint", // not supported in shaders, so use int (tricky)
	};


	EqString vertexShaderString;
	vertexShaderString.Append("struct VsIn {\n");

	int nSemanticIndices[7];
	memset(nSemanticIndices,0,sizeof(nSemanticIndices));

	int numRealAttribs = 0;

	// Fill the vertex element array
	for (int i = 0; i < nAttribs; i++)
	{
		int stream = formatDesc[i].streamId;
		int size = formatDesc[i].elemCount;

		if(formatDesc[i].attribType != VERTEXATTRIB_UNUSED)
		{
			pDesc[numRealAttribs].InputSlot = stream;
			pDesc[numRealAttribs].AlignedByteOffset = pFormat->m_nVertexSize[stream];
			pDesc[numRealAttribs].SemanticName = semantics[formatDesc[i].attribType];
			pDesc[numRealAttribs].SemanticIndex = index[formatDesc[i].attribType]++;
			pDesc[numRealAttribs].Format = vformats[formatDesc[i].attribFormat][size - 1];
			pDesc[numRealAttribs].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
			pDesc[numRealAttribs].InstanceDataStepRate = 0;

			nSemanticIndices[formatDesc[i].attribType]++;
			numRealAttribs++;

			char str[512];
		
			if(size > 1)
			{
				if(nSemanticIndices[formatDesc[i].attribType] <= 1)
					sprintf(str, "%s%d %s_%d : %s;\n", formatNames[formatDesc[i].attribFormat], size, semantics[formatDesc[i].attribType], i, semantics[formatDesc[i].attribType]);
				else
					sprintf(str, "%s%d %s_%d : %s%d;\n", formatNames[formatDesc[i].attribFormat], size, semantics[formatDesc[i].attribType], i, semantics[formatDesc[i].attribType], nSemanticIndices[formatDesc[i].attribType]-1);
			}
			else
			{
				if(nSemanticIndices[formatDesc[i].attribType] <= 1)
					sprintf(str, "%s %s_%d : %s;\n", formatNames[formatDesc[i].attribFormat], semantics[formatDesc[i].attribType], i, semantics[formatDesc[i].attribType]);
				else
					sprintf(str, "%s %s_%d : %s%d;\n", formatNames[formatDesc[i].attribFormat], semantics[formatDesc[i].attribType], i, semantics[formatDesc[i].attribType], nSemanticIndices[formatDesc[i].attribType]-1);
			}

			vertexShaderString.Append(str);
		}

		pFormat->m_nVertexSize[stream] += size * s_attributeSize[formatDesc[i].attribFormat];
	}

	vertexShaderString.Append("};\n\n");
	vertexShaderString.Append("float4 main(VsIn In) : SV_Position{\n");
	vertexShaderString.Append("return float4(0,0,0,1);");
	vertexShaderString.Append("}");

	//Msg("%s\n", vertexShaderString.getData());

	ID3D10Blob* pGeneratedVertexShader	= NULL;
	ID3D10Blob*	errorsBuf				= NULL;
	ID3D10VertexShader*	pShader			= NULL;
	ID3D10Blob*	pInputSignature			= NULL;

	UINT compileFlags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_STRICTNESS;

	CScopedMutex scoped(m_Mutex);

	if (SUCCEEDED(D3D10CompileShader(vertexShaderString.GetData(), vertexShaderString.Length(), "VertexFormatInputSignaturePassShader", NULL, NULL, "main", "vs_4_0", compileFlags, &pGeneratedVertexShader, &errorsBuf)))
	{
		if (SUCCEEDED(m_pD3DDevice->CreateVertexShader(pGeneratedVertexShader->GetBufferPointer(), pGeneratedVertexShader->GetBufferSize(), &pShader)))
		{
			D3D10GetInputSignatureBlob(pGeneratedVertexShader->GetBufferPointer(), pGeneratedVertexShader->GetBufferSize(), &pInputSignature);
		}
	}
	
	HRESULT hr = m_pD3DDevice->CreateInputLayout(pDesc, numRealAttribs, pInputSignature->GetBufferPointer(), pInputSignature->GetBufferSize(), &pFormat->m_pVertexDecl);
	delete [] pDesc;

	SAFE_RELEASE(pGeneratedVertexShader)
	SAFE_RELEASE(errorsBuf)
	SAFE_RELEASE(pShader)
	SAFE_RELEASE(pInputSignature)

	if( FAILED(hr) )
	{
		delete pFormat;

		MsgError("Couldn't create vertex declaration!\n");
		ErrorMsg("Couldn't create vertex declaration");

		return NULL;
	}

	m_VFList.append(pFormat);

	return pFormat;
}

IVertexBuffer* ShaderAPID3DX10::CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData)
{
	ASSERT(m_pD3DDevice);

	CVertexBufferD3DX10* pBuffer = new CVertexBufferD3DX10();
	pBuffer->m_nSize = nNumVerts*strideSize;
	pBuffer->m_nUsage = d3dbufferusages[nBufAccess];
	pBuffer->m_nNumVertices = nNumVerts;
	pBuffer->m_nStrideSize = strideSize;

	D3D10_BUFFER_DESC desc;
	desc.Usage = d3dbufferusages[nBufAccess];
	desc.ByteWidth = pBuffer->m_nSize;
	desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = (nBufAccess == BUFFER_DYNAMIC) ? D3D10_CPU_ACCESS_WRITE : 0;
	desc.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA vbData;
	vbData.pSysMem = pData;
	vbData.SysMemPitch = 0;
	vbData.SysMemSlicePitch = 0;

	DevMsg(DEVMSG_SHADERAPI,"Creatting VBO with size %i KB\n", pBuffer->m_nSize / 1024);

	CScopedMutex scoped(m_Mutex);

	if (FAILED(m_pD3DDevice->CreateBuffer(&desc, pData ? (&vbData) : NULL, &pBuffer->m_pVertexBuffer)))
	{
		MsgError("Couldn't create vertex buffer!\n");
        ErrorMsg("Couldn't create vertex buffer");
		return NULL;
	}

	ASSERT(pBuffer->m_pVertexBuffer);

	m_VBList.append(pBuffer);

	return pBuffer;
}

IIndexBuffer* ShaderAPID3DX10::CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData)
{
	ASSERT(nIndexSize >= 2);
	ASSERT(nIndexSize <= 4);

	CIndexBufferD3DX10* pBuffer = new CIndexBufferD3DX10();

	pBuffer->m_nIndices = nIndices;
	pBuffer->m_nIndexSize = nIndexSize;
	pBuffer->m_nUsage = d3dbufferusages[nBufAccess];

	D3D10_BUFFER_DESC desc;
	desc.Usage = d3dbufferusages[nBufAccess];
	desc.ByteWidth = nIndices * nIndexSize;
	desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = (nBufAccess == BUFFER_DYNAMIC)? D3D10_CPU_ACCESS_WRITE : 0;
	desc.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA ibData;
	ibData.pSysMem = pData;
	ibData.SysMemPitch = 0;
	ibData.SysMemSlicePitch = 0;

	CScopedMutex scoped(m_Mutex);

	if (FAILED(m_pD3DDevice->CreateBuffer(&desc, pData ? (&ibData) : NULL, &pBuffer->m_pIndexBuffer)))
	{
		MsgError("Couldn't create index buffer!\n");
        ErrorMsg("Couldn't create index buffer");
		return NULL;
	}

	m_IBList.append(pBuffer);

	return pBuffer;
}

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

PRIMCOUNTER g_pDX10PrimCounterCallbacks[] = 
{
	PrimCount_TriangleList,
	PrimCount_TriangleFanStrip,
	PrimCount_TriangleFanStrip,
	PrimCount_None,
	PrimCount_ListList,
	PrimCount_ListStrip,
	PrimCount_None,
	PrimCount_Points,
	PrimCount_None,
};


// Indexed primitive drawer
void ShaderAPID3DX10::DrawIndexedPrimitives(ER_PrimitiveType nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
	int nTris = g_pDX10PrimCounterCallbacks[nType](nIndices);

	m_pD3DDevice->IASetPrimitiveTopology(d3dPrim[nType]);

	m_pD3DDevice->DrawIndexed(nIndices, nFirstIndex, nBaseVertex);

	m_nDrawIndexedPrimitiveCalls++;
	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

// Draw elements
void ShaderAPID3DX10::DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices)
{
	ASSERT(nVertices > 0);

	m_pD3DDevice->IASetPrimitiveTopology(d3dPrim[nType]);
	
	int nTris = g_pDX10PrimCounterCallbacks[nType](nVertices);

	/*
	int maxVerts = m_pCurrentVertexBuffers[0]->GetVertexCount();

	// get clamped
	if(nVertices <= 0)
		nVertices = maxVerts;
	else if(nVertices > maxVerts)
		nVertices = maxVerts;
	*/

	m_pD3DDevice->Draw(nVertices, nFirstVertex);

	m_nDrawCalls++;
	m_nTrianglesCount += nTris;
}

// mesh buffer FFP emulation
void ShaderAPID3DX10::DrawMeshBufferPrimitives(ER_PrimitiveType nType, int nVertices, int nIndices)
{
	if(m_pSelectedShader == NULL)
	{
		if(m_pCurrentTexturesPS[0] == NULL)
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

ID3D10ShaderResourceView* ShaderAPID3DX10::TexResource_CreateShaderResourceView(ID3D10Resource *resource, DXGI_FORMAT format, int firstSlice, int sliceCount)
{
	D3D10_RESOURCE_DIMENSION type;
	resource->GetType(&type);

#ifdef USE_D3D10_1
	D3D10_SHADER_RESOURCE_VIEW_DESC1 srvDesc;
	ID3D10ShaderResourceView1 *srv;
#else
	D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ID3D10ShaderResourceView *srv;
#endif

	switch (type){
		case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
			D3D10_TEXTURE1D_DESC desc1d;
			((ID3D10Texture1D *) resource)->GetDesc(&desc1d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc1d.Format;
			if (desc1d.ArraySize > 1)
			{
#ifdef USE_D3D10_1
				srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2DARRAY;
#else
				srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
#endif
				srvDesc.Texture1DArray.FirstArraySlice = 0;
				srvDesc.Texture1DArray.ArraySize = desc1d.ArraySize;
				srvDesc.Texture1DArray.MostDetailedMip = 0;
				srvDesc.Texture1DArray.MipLevels = desc1d.MipLevels;
			}
			else
			{
#ifdef USE_D3D10_1
				srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE1D;
#else
				srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE1D;
#endif
				srvDesc.Texture1D.MostDetailedMip = 0;
				srvDesc.Texture1D.MipLevels = desc1d.MipLevels;
			}
			break;
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) resource)->GetDesc(&desc2d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc2d.Format;

			if (desc2d.ArraySize > 1)
			{
				if (desc2d.MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE)
				{
#ifdef USE_D3D10_1
					if (desc2d.ArraySize == 6)
					{
						srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURECUBE;
						srvDesc.TextureCube.MostDetailedMip = 0;
						srvDesc.TextureCube.MipLevels = desc2d.MipLevels;
					}
					else
					{
						srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURECUBEARRAY;
						srvDesc.TextureCubeArray.MostDetailedMip = 0;
						srvDesc.TextureCubeArray.MipLevels = desc2d.MipLevels;
						srvDesc.TextureCubeArray.First2DArrayFace = 0;
						srvDesc.TextureCubeArray.NumCubes = desc2d.ArraySize / 6;
					}
#else
					srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.TextureCube.MostDetailedMip = 0;
					srvDesc.TextureCube.MipLevels = desc2d.MipLevels;
#endif
				}
				else
				{
#ifdef USE_D3D10_1
					srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2DARRAY;
#else
					srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
#endif
					if (firstSlice < 0)
					{
						srvDesc.Texture2DArray.FirstArraySlice = 0;
						srvDesc.Texture2DArray.ArraySize = desc2d.ArraySize;
					}
					else
					{
						srvDesc.Texture2DArray.FirstArraySlice = firstSlice;

						if (sliceCount < 0)
							srvDesc.Texture2DArray.ArraySize = 1;
						else
							srvDesc.Texture2DArray.ArraySize = sliceCount;
					}

					srvDesc.Texture2DArray.MostDetailedMip = 0;
					srvDesc.Texture2DArray.MipLevels = desc2d.MipLevels;
				}
			}
			else
			{
#ifdef USE_D3D10_1
				srvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_1_SRV_DIMENSION_TEXTURE2DMS : D3D10_1_SRV_DIMENSION_TEXTURE2D;
#else
				srvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_SRV_DIMENSION_TEXTURE2DMS : D3D10_SRV_DIMENSION_TEXTURE2D;
#endif
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = desc2d.MipLevels;
			}
			break;
		case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
			D3D10_TEXTURE3D_DESC desc3d;
			((ID3D10Texture3D *) resource)->GetDesc(&desc3d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc3d.Format;

#ifdef USE_D3D10_1
			srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE3D;
#else
			srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE3D;
#endif
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = desc3d.MipLevels;

			break;
		default:
			ErrorMsg("Unsupported type");
			return NULL;
	}

	HRESULT hr;

#ifdef COMPILE_D3D_10_1
	hr = m_pD3DDevice->CreateShaderResourceView1(resource, &srvDesc, &srv);
#else
	hr = m_pD3DDevice->CreateShaderResourceView(resource, &srvDesc, &srv);
#endif
	if (FAILED(hr))
	{
		MsgError("ERROR! CreateShaderResourceView failed (0x%p)\n", hr);
		return NULL;
	}

	return srv;

	/*
	D3D10_RESOURCE_DIMENSION type;
	resource->GetType(&type);

#ifdef COMPILE_D3D_10_1
	D3D10_SHADER_RESOURCE_VIEW_DESC1 srvDesc;
	ID3D10ShaderResourceView1 *srv = NULL;
#else
	D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ID3D10ShaderResourceView *srv = NULL;
#endif

	switch (type)
	{
		case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
			D3D10_TEXTURE1D_DESC desc1d;
			((ID3D10Texture1D *) resource)->GetDesc(&desc1d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc1d.Format;
			if (desc1d.ArraySize > 1)
			{
#ifdef COMPILE_D3D_10_1
				srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2DARRAY;
#else
				srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
#endif
				srvDesc.Texture1DArray.FirstArraySlice = 0;
				srvDesc.Texture1DArray.ArraySize = desc1d.ArraySize;
				srvDesc.Texture1DArray.MostDetailedMip = 0;
				srvDesc.Texture1DArray.MipLevels = desc1d.MipLevels;
			}
			else 
			{
#ifdef COMPILE_D3D_10_1
				srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE1D;
#else
				srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE1D;
#endif
				srvDesc.Texture1D.MostDetailedMip = 0;
				srvDesc.Texture1D.MipLevels = desc1d.MipLevels;
			}
			break;
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) resource)->GetDesc(&desc2d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc2d.Format;
			if (desc2d.ArraySize > 1)
			{
				if (desc2d.MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE)
				{
#ifdef COMPILE_D3D_10_1
					if (desc2d.ArraySize == 6)
					{
						srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURECUBE;
						srvDesc.TextureCube.MostDetailedMip = 0;
						srvDesc.TextureCube.MipLevels = desc2d.MipLevels;
					}
					else
					{
						srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURECUBEARRAY;
						srvDesc.TextureCubeArray.MostDetailedMip = 0;
						srvDesc.TextureCubeArray.MipLevels = desc2d.MipLevels;
						srvDesc.TextureCubeArray.First2DArrayFace = 0;
						srvDesc.TextureCubeArray.NumCubes = desc2d.ArraySize / 6;
					}
#else
					srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.TextureCube.MostDetailedMip = 0;
					srvDesc.TextureCube.MipLevels = desc2d.MipLevels;
#endif
				}
				else
				{
#ifdef COMPILE_D3D_10_1
					srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2DARRAY;
#else
					srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
#endif
					if (firstSlice < 0)
					{
						srvDesc.Texture2DArray.FirstArraySlice = 0;
						srvDesc.Texture2DArray.ArraySize = desc2d.ArraySize;
					}
					else
					{
						srvDesc.Texture2DArray.FirstArraySlice = firstSlice;

						if (sliceCount < 0)
							srvDesc.Texture2DArray.ArraySize = 1;
						else
							srvDesc.Texture2DArray.ArraySize = sliceCount;

					}
					srvDesc.Texture2DArray.MostDetailedMip = 0;
					srvDesc.Texture2DArray.MipLevels = desc2d.MipLevels;
				}
			}
			else
			{
#ifdef COMPILE_D3D_10_1
				srvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_1_SRV_DIMENSION_TEXTURE2DMS : D3D10_1_SRV_DIMENSION_TEXTURE2D;
#else
				srvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_SRV_DIMENSION_TEXTURE2DMS : D3D10_SRV_DIMENSION_TEXTURE2D;
#endif
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = desc2d.MipLevels;
			}
			break;
		case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
			D3D10_TEXTURE3D_DESC desc3d;
			((ID3D10Texture3D *) resource)->GetDesc(&desc3d);

			srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc3d.Format;

#ifdef USE_D3D10_1
			srvDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE3D;
#else
			srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE3D;
#endif
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = desc3d.MipLevels;

			break;
		default:
			MsgError("Unsupported texture type!\n");
			return NULL;
	}

	HRESULT hr;

#ifdef COMPILE_D3D_10_1
	hr = m_pD3DDevice->CreateShaderResourceView1(resource, &srvDesc, &srv);
#else
	hr = m_pD3DDevice->CreateShaderResourceView(resource, &srvDesc, &srv);
#endif
	if (FAILED(hr))
	{
		MsgError("ERROR! CreateShaderResourceView failed (0x%p)\n", hr);
		return NULL;
	}

	return srv;
	*/
}

ID3D10RenderTargetView*	ShaderAPID3DX10::TexResource_CreateShaderRenderTargetView(ID3D10Resource *resource, DXGI_FORMAT format, int firstSlice, int sliceCount)
{
	D3D10_RESOURCE_DIMENSION type;
	resource->GetType(&type);

	D3D10_RENDER_TARGET_VIEW_DESC rtvDesc;
	ID3D10RenderTargetView* rtv = NULL;

	switch (type)
	{
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:

			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) resource)->GetDesc(&desc2d);

			rtvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc2d.Format;

			if (desc2d.ArraySize > 1)
			{
				rtvDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DARRAY;

				if (firstSlice < 0)
				{
					rtvDesc.Texture2DArray.FirstArraySlice = 0;
					rtvDesc.Texture2DArray.ArraySize = desc2d.ArraySize;
				}
				else
				{
					rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;

					if (sliceCount < 0)
						rtvDesc.Texture2DArray.ArraySize = 1;
					else
						rtvDesc.Texture2DArray.ArraySize = sliceCount;

				}

				rtvDesc.Texture2DArray.MipSlice = 0;
			}
			else
			{
				rtvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_RTV_DIMENSION_TEXTURE2DMS : D3D10_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0;
			}
			break;
		case D3D10_RESOURCE_DIMENSION_TEXTURE3D:

			// Yes, you can render into volume texture!

			D3D10_TEXTURE3D_DESC desc3d;
			((ID3D10Texture3D *) resource)->GetDesc(&desc3d);

			rtvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc3d.Format;
			rtvDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE3D;
			if (firstSlice < 0)
			{
				rtvDesc.Texture3D.FirstWSlice = 0;
				rtvDesc.Texture3D.WSize = desc3d.Depth;
			}
			else
			{
				rtvDesc.Texture3D.FirstWSlice = firstSlice;

				if (sliceCount < 0)
					rtvDesc.Texture3D.WSize = 1;
				else
					rtvDesc.Texture3D.WSize = sliceCount;

			}
			rtvDesc.Texture3D.MipSlice = 0;
			break;
		default:
			MsgError("ERROR: Unsupported texture type!\n");
			return NULL;
	}

	HRESULT hr = m_pD3DDevice->CreateRenderTargetView(resource, &rtvDesc, &rtv);

	if (FAILED(hr))
	{
		MsgError("ERROR! CreateShaderResourceView failed (0x%p)\n", hr);
		return NULL;
	}

	return rtv;
}

ID3D10DepthStencilView* ShaderAPID3DX10::TexResource_CreateShaderDepthStencilView(ID3D10Resource *resource, DXGI_FORMAT format, int firstSlice, int sliceCount)
{
	D3D10_RESOURCE_DIMENSION type;
	resource->GetType(&type);

	D3D10_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ID3D10DepthStencilView* dsv = NULL;

	switch (type)
	{
		// Only Texture2D supported for depths
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:

			D3D10_TEXTURE2D_DESC desc2d;
			((ID3D10Texture2D *) resource)->GetDesc(&desc2d);

			dsvDesc.Format = (format != DXGI_FORMAT_UNKNOWN)? format : desc2d.Format;
			if (desc2d.ArraySize > 1)
			{
				dsvDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2DARRAY;

				if (firstSlice < 0)
				{
					dsvDesc.Texture2DArray.FirstArraySlice = 0;
					dsvDesc.Texture2DArray.ArraySize = desc2d.ArraySize;
				}
				else
				{
					dsvDesc.Texture2DArray.FirstArraySlice = firstSlice;

					if (sliceCount < 0)
						dsvDesc.Texture2DArray.ArraySize = 1;
					else
						dsvDesc.Texture2DArray.ArraySize = sliceCount;

				}

				dsvDesc.Texture2DArray.MipSlice = 0;
			}
			else
			{
				dsvDesc.ViewDimension = (desc2d.SampleDesc.Count > 1)? D3D10_DSV_DIMENSION_TEXTURE2DMS : D3D10_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;
			}
			break;
		default:
			MsgError("ERROR: Unsupported depth target type\n");
			return NULL;
	}

	HRESULT hr = m_pD3DDevice->CreateDepthStencilView(resource, &dsvDesc, &dsv);

	if (FAILED(hr))
	{
		MsgError("ERROR! CreateDepthStencilView failed (0x%p)\n", hr);
		return NULL;
	}

	return dsv;
}


void ShaderAPID3DX10::CreateTextureInternal(ITexture** pTex, const DkList<CImage*>& pImages, const SamplerStateParam_t& sampler,int nFlags)
{
	if(!pImages.numElem())
		return;

	CD3D10Texture* pTexture = NULL;

	// get or create
	if(*pTex)
		pTexture = (CD3D10Texture*)*pTex;
	else
		pTexture = new CD3D10Texture();

	int wide = 0, tall = 0;

	for(int i = 0; i < pImages.numElem(); i++)
	{
		ID3D10Resource* pD3DTex = CreateD3DTextureFromImage(pImages[i], wide, tall, nFlags);

		if(pD3DTex)
		{
			// add and make shader resource view for it
			pTexture->textures.append(pD3DTex);

			pTexture->texFormat = formats[pImages[i]->GetFormat()];
			pTexture->srvFormat = pTexture->texFormat;

			pTexture->srv.append( TexResource_CreateShaderResourceView(pD3DTex, pTexture->srvFormat) );
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

	pTexture->m_pD3D10SamplerState = CreateSamplerState(sampler);

	// Bind this sampler state to texture
	pTexture->SetSamplerState(sampler);
	pTexture->SetDimensions(wide, tall);
	pTexture->SetFormat(pImages[0]->GetFormat());
	pTexture->SetFlags(nFlags | TEXFLAG_MANAGED);
	pTexture->SetName( pImages[0]->GetName() );

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

ID3D10Resource* ShaderAPID3DX10::CreateD3DTextureFromImage(CImage* pSrc, int& wide, int& tall, int nFlags)
{
	if(!pSrc)
		return NULL;

	CScopedMutex m(m_Mutex);

	HOOK_TO_CVAR(r_loadmiplevel);

	int nQuality = r_loadmiplevel->GetInt();

	// force quality to best
	if(nFlags & TEXFLAG_NOQUALITYLOD)
		nQuality = 0;
	
	// wait before all textures will be processed by other threads
	Finish();

	bool bMipMaps = (pSrc->GetMipMapCount() > 1);

	// generate mipmaps if possible
	if(nQuality > 0 && !bMipMaps)
	{
		if(!pSrc->CreateMipMaps(nQuality))
			nQuality = 0;
	}

	ETextureFormat	nFormat = pSrc->GetFormat();

	if (nFormat == FORMAT_RGB8)
		pSrc->Convert(FORMAT_RGBA8);

	//if (nFormat == FORMAT_RGB8 || nFormat == FORMAT_RGBA8)
	//	pSrc->SwapChannels(0, 2);

	nFormat = pSrc->GetFormat();

	// set our referenced params
	wide = pSrc->GetWidth(nQuality);
	tall = pSrc->GetHeight(nQuality);

	uint nMipMaps = pSrc->GetMipMapCount()-nQuality;
	uint nSlices = pSrc->IsCube()? 6 : 1;
	uint arraySize = pSrc->GetArraySize();

	// the resource
	ID3D10Resource* pTexture = NULL;

	// create subresource
	static D3D10_SUBRESOURCE_DATA texData[1024];
	D3D10_SUBRESOURCE_DATA *dest = texData;

	for (uint n = 0; n < arraySize; n++)
	{
		for (uint k = 0; k < nSlices; k++)
		{
			for (uint i = 0; i < nMipMaps; i++)
			{
				int nMip = i+nQuality;

				uint pitch, slicePitch;

				if (IsCompressedFormat(nFormat))
				{
					pitch = ((pSrc->GetWidth(nMip) + 3) >> 2) * GetBytesPerBlock(nFormat);
					slicePitch = pitch * ((pSrc->GetHeight(nMip) + 3) >> 2);
				}
				else
				{
					pitch = pSrc->GetWidth(nMip) * GetBytesPerPixel(nFormat);
					slicePitch = pitch * pSrc->GetHeight(nMip);
				}

				dest->pSysMem = pSrc->GetPixels(nMip, n) + k * slicePitch;
				dest->SysMemPitch = pitch;
				dest->SysMemSlicePitch = slicePitch;
				dest++;
			}
		}
	}

	// The main part

	HRESULT hr;

	if (pSrc->Is1D())
	{
		D3D10_TEXTURE1D_DESC desc;
		desc.Width  = pSrc->GetWidth(nQuality);
		desc.Format = formats[nFormat];
		desc.MipLevels = nMipMaps;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.ArraySize = 1;
		desc.MiscFlags = 0;

		hr = m_pD3DDevice->CreateTexture1D(&desc, texData, (ID3D10Texture1D **) &pTexture);
	}
	else if (pSrc->Is2D() || pSrc->IsCube())
	{
		D3D10_TEXTURE2D_DESC desc;
		desc.Width  = pSrc->GetWidth(nQuality);
		desc.Height = pSrc->GetHeight(nQuality);
		desc.Format = formats[nFormat];
		desc.MipLevels = nMipMaps;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		if (pSrc->IsCube())
		{
			desc.ArraySize = 6 * arraySize;
			desc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
		}
		else
		{
			desc.ArraySize = 1;
			desc.MiscFlags = 0;
		}

		hr = m_pD3DDevice->CreateTexture2D(&desc, texData, (ID3D10Texture2D **) &pTexture);
	}
	else if (pSrc->Is3D())
	{
		D3D10_TEXTURE3D_DESC desc;
		desc.Width  = pSrc->GetWidth(nQuality);
		desc.Height = pSrc->GetHeight(nQuality);
		desc.Depth  = pSrc->GetDepth(nQuality);
		desc.Format = formats[nFormat];
		desc.MipLevels = nMipMaps;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		hr = m_pD3DDevice->CreateTexture3D(&desc, texData, (ID3D10Texture3D **) &pTexture);
	}
	else
	{
		hr = D3DERR_INVALIDCALL;
	}

	if(FAILED(hr))
	{
		MsgError("Couldn't create texture from '%s'\n", pSrc->GetName());

		return NULL;
	}

	return pTexture;
}

/*
void ShaderAPID3DX10::AddTextureInternal(ITexture** pTex, CImage *texImage, SamplerStateParam_t& sSamplingParams,int nFlags)
{
	HOOK_TO_CVAR(r_loadmiplevel);

	int quality = r_loadmiplevel->GetInt();

	if(nFlags & TEXFLAG_NOQUALITYLOD)
		quality = 0;

	if(!pTex)
		return;

	CD3D10Texture* pTexture = NULL;

	if(*pTex)
	{
		pTexture = (CD3D10Texture*)*pTex;
	}
	else
	{
		pTexture = new CD3D10Texture;
	}


	if(texImage->GetMipMapCount() <= 1)
		quality = 0;

	// create array of textures
	pTexture->textures.setNum(1);

	ETextureFormat format = texImage->GetFormat();

	if (format == FORMAT_RGB8)
		texImage->Convert(FORMAT_RGBA8);

	//if (format == FORMAT_RGB8 || format == FORMAT_RGBA8)
	//	texImage->SwapChannels(0, 2);

	format = texImage->GetFormat();

	uint nMipMaps = texImage->GetMipMapCount()-quality;
	uint nSlices = texImage->IsCube()? 6 : 1;
	uint arraySize = texImage->GetArraySize();

	// create subresource
	static D3D10_SUBRESOURCE_DATA texData[1024];
	D3D10_SUBRESOURCE_DATA *dest = texData;
	for (uint n = 0; n < arraySize; n++)
	{
		for (uint k = 0; k < nSlices; k++)
		{
			for (uint i = 0; i < nMipMaps; i++)
			{
				int nMip = i+quality;

				uint pitch, slicePitch;
				if (IsCompressedFormat(format))
				{
					pitch = ((texImage->GetWidth(nMip) + 3) >> 2) * GetBytesPerBlock(format);
					slicePitch = pitch * ((texImage->GetHeight(nMip) + 3) >> 2);
				}
				else
				{
					pitch = texImage->GetWidth(nMip) * GetBytesPerPixel(format);
					slicePitch = pitch * texImage->GetHeight(nMip);
				}

				dest->pSysMem = texImage->GetPixels(nMip, n) + k * slicePitch;
				dest->SysMemPitch = pitch;
				dest->SysMemSlicePitch = slicePitch;
				dest++;
			}
		}
	}

	pTexture->texFormat = formats[format];

	HRESULT hr;
	if (texImage->Is1D())
	{
		D3D10_TEXTURE1D_DESC desc;
		desc.Width  = texImage->GetWidth(quality);
		desc.Format = pTexture->texFormat;
		desc.MipLevels = nMipMaps;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.ArraySize = 1;
		desc.MiscFlags = 0;

		hr = m_pD3DDevice->CreateTexture1D(&desc, texData, (ID3D10Texture1D **) &pTexture->textures[0]);
	}
	else if (texImage->Is2D() || texImage->IsCube())
	{
		D3D10_TEXTURE2D_DESC desc;
		desc.Width  = texImage->GetWidth(quality);
		desc.Height = texImage->GetHeight(quality);
		desc.Format = pTexture->texFormat;
		desc.MipLevels = nMipMaps;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		if (texImage->IsCube())
		{
			desc.ArraySize = 6 * arraySize;
			desc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
		}
		else
		{
			desc.ArraySize = 1;
			desc.MiscFlags = 0;
		}

		hr = m_pD3DDevice->CreateTexture2D(&desc, texData, (ID3D10Texture2D **) &pTexture->textures[0]);
	}
	else if (texImage->Is3D())
	{
		D3D10_TEXTURE3D_DESC desc;
		desc.Width  = texImage->GetWidth(quality);
		desc.Height = texImage->GetHeight(quality);
		desc.Depth  = texImage->GetDepth(quality);
		desc.Format = pTexture->texFormat;
		desc.MipLevels = nMipMaps;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		hr = m_pD3DDevice->CreateTexture3D(&desc, texData, (ID3D10Texture3D **) &pTexture->textures[0]);
	}

	if(FAILED(hr))
	{
		MsgError("Couldn't create texture for %s\n", texImage->GetName());

		delete pTexture;
		*pTex = NULL;

		return;
	}

	CScopedMutex scoped(m_Mutex);

	pTexture->srvFormat = pTexture->texFormat;
	pTexture->srv.append(TexResource_CreateShaderResourceView(pTexture->textures[0], pTexture->srvFormat));

	pTexture->m_pD3D10SamplerState = CreateSamplerState(sSamplingParams);

	// Bind this sampler state to texture
	pTexture->SetSamplerState(sSamplingParams);
	pTexture->SetDimensions(texImage->GetWidth(quality),texImage->GetHeight(quality));
	pTexture->SetFormat(texImage->GetFormat());
	pTexture->SetFlags(nFlags | TEXFLAG_MANAGED);
	pTexture->SetName(texImage->GetName());

	if(! (*pTex) )
		m_TextureList.append(pTexture);

	*pTex = pTexture;
}
*/
const Sampler_t *GetSampler(const Sampler_t *samplers, const int count, const char *name)
{
	int minSampler = 0;
	int maxSampler = count - 1;

	// Do a quick lookup in the sorted table with a binary search
	while (minSampler <= maxSampler)
	{
		int currSampler = (minSampler + maxSampler) >> 1;
        int res = strcmp(name, samplers[currSampler].name);

		if (res == 0)
			return samplers + currSampler;
		else if (res > 0)
            minSampler = currSampler + 1;
		else
            maxSampler = currSampler - 1;
	}

	return NULL;
}

// sets texture
void ShaderAPID3DX10::SetTexture( ITexture* pTexture, const char* pszName, int index )
{
	CD3D10ShaderProgram* pShader = (CD3D10ShaderProgram*)m_pSelectedShader;

	if(!pShader || !pszName)
	{
		// NO SHADER? WE HAVE SURPIRSE FOR YOU!
		m_pSelectedTexturesVS[index] = pTexture;
		m_pSelectedTextureSlicesVS[index] = -1;

		m_pSelectedTexturesGS[index] = pTexture;
		m_pSelectedTextureSlicesGS[index] = -1;

		m_pSelectedTexturesPS[index] = pTexture;
		m_pSelectedTextureSlicesPS[index] = -1;

		return;
	}

	const Sampler_t *s = GetSampler(pShader->m_pTextures, pShader->m_numTextures, pszName);

	if(s)
	{
		if (s->vsIndex >= 0)
		{
			m_pSelectedTexturesVS[s->vsIndex] = pTexture;
			m_pSelectedTextureSlicesVS[s->vsIndex] = -1;
		}
		if (s->gsIndex >= 0)
		{
			m_pSelectedTexturesGS[s->gsIndex] = pTexture;
			m_pSelectedTextureSlicesGS[s->gsIndex] = -1;
		}
		if (s->index >= 0)
		{
			m_pSelectedTexturesPS[s->index] = pTexture;
			m_pSelectedTextureSlicesPS[s->index] = -1;
		}
	}
	else
	{
		m_pSelectedTexturesVS[index] = pTexture;
		m_pSelectedTextureSlicesVS[index] = -1;

		m_pSelectedTexturesGS[index] = pTexture;
		m_pSelectedTextureSlicesGS[index] = -1;

		m_pSelectedTexturesPS[index] = pTexture;
		m_pSelectedTextureSlicesPS[index] = -1;
	}
}

void ShaderAPID3DX10::SetViewport(int x, int y, int w, int h)
{
	// Setup the viewport
	D3D10_VIEWPORT viewport;

	//uint nVPs = 1;
	//m_pD3DDevice->RSGetViewports(&nVPs, &viewport);

	viewport.Width  = w;
	viewport.Height = h;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = x;
	viewport.TopLeftY = y;

	m_pD3DDevice->RSSetViewports(1, &viewport);
}

void ShaderAPID3DX10::GetViewport(int &x, int &y, int &w, int &h)
{
	// Setup the viewport
	D3D10_VIEWPORT vp;
	uint nVPs = 1;

	m_pD3DDevice->RSGetViewports(&nVPs, &vp);

	x = vp.TopLeftX;
	y = vp.TopLeftY;
	w = vp.Width;
	h = vp.Height;
}

// sets scissor rectangle
void ShaderAPID3DX10::SetScissorRectangle( const IRectangle &rect )
{
	D3D10_RECT scissorRect;
	scissorRect.left	= rect.vleftTop.x;
	scissorRect.top		= rect.vleftTop.y;
	scissorRect.right	= rect.vrightBottom.x;
	scissorRect.bottom	= rect.vrightBottom.y;

	m_pD3DDevice->RSSetScissorRects(1, &scissorRect);
}

bool ShaderAPID3DX10::IsDeviceActive()
{
	return !m_bDeviceIsLost;
}

bool ShaderAPID3DX10::CreateBackbufferDepth(int wide, int tall, DXGI_FORMAT depthFormat, int nMSAASamples)
{
	//----------------------------------
	// depth texture
	//----------------------------------

	if (depthFormat != DXGI_FORMAT_UNKNOWN)
	{
		// Create depth stencil texture
		D3D10_TEXTURE2D_DESC descDepth;
		descDepth.Width  = wide;
		descDepth.Height = tall;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = depthFormat;
		descDepth.SampleDesc.Count = nMSAASamples;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D10_USAGE_DEFAULT;
		descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;

		if (FAILED(m_pD3DDevice->CreateTexture2D(&descDepth, NULL, &m_pDepthBuffer)))
		{
			MsgError("Failed to create depth buffer texture!!!\n");
			return false;
		}

		// Create the depth stencil view
		D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
		descDSV.Format = descDepth.Format;
		descDSV.ViewDimension = nMSAASamples > 1? D3D10_DSV_DIMENSION_TEXTURE2DMS : D3D10_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		if (FAILED(m_pD3DDevice->CreateDepthStencilView(m_pDepthBuffer, &descDSV, &m_pDepthBufferDSV))) 
		{
			MsgError("Failed to create depth stencil view!!!\n");
			return false;
		}
	}

	if(!m_pDepthBufferTexture)
	{
		m_pDepthBufferTexture = new CD3D10Texture;
		m_pDepthBufferTexture->SetName("_rt_depthbuffer");
		m_pDepthBufferTexture->SetDimensions(wide, tall);
		m_pDepthBufferTexture->SetFlags(TEXFLAG_RENDERTARGET | TEXFLAG_FOREIGN | TEXFLAG_NOQUALITYLOD);
		m_pDepthBufferTexture->Ref_Grab();

		m_TextureList.append(m_pDepthBufferTexture);
	}

	CD3D10Texture* pDepthBuffer = (CD3D10Texture*)m_pDepthBufferTexture;

	pDepthBuffer->textures.append(m_pDepthBuffer);
	pDepthBuffer->dsv.append(m_pDepthBufferDSV);

	// depth buffer SRV's only available for few formats, yet
	//pDepthBuffer->srv.append(TexResource_CreateShaderResourceView(pDepthBuffer->textures[0]));

	m_pDepthBuffer->AddRef();
	m_pDepthBufferDSV->AddRef();

	m_pBackBufferTexture = g_pShaderAPI->FindTexture("_rt_backbuffer");

	if(!m_pBackBufferTexture)
		return false;

	SetViewport( 0, 0, wide, tall );

	return true;
}

void ShaderAPID3DX10::ReleaseBackbufferDepth()
{
	if (m_pD3DDevice)
	{
		m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);

		((CD3D10Texture*)m_pDepthBufferTexture)->Release();
		//((CD3D10Texture*)m_pBackBufferTexture)->Release();

		SAFE_RELEASE(m_pDepthBuffer);
		SAFE_RELEASE(m_pDepthBufferDSV);
	}
}