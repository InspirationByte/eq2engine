//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				For high level look for the material system (Engine)
//				Contains FFP functions and Shader (FFP overriding programs)
//
//				The renderer may do anti-wallhacking functions
//////////////////////////////////////////////////////////////////////////////////

#include "ShaderAPI_Base.h"
#include "CTexture.h"

#include "core/DebugInterface.h"
#include "core/IConsoleCommands.h"
#include "core/IFileSystem.h"

#include "imaging/PixWriter.h"
#include "imaging/ImageLoader.h"

#include "utils/strtools.h"
#include "utils/Tokenizer.h"
#include "utils/CRC32.h"
#include "utils/KeyValues.h"

static ConVar rs_echo_texture_loading("r_echo_texture_loading","0","Echo textrue loading");
static ConVar r_nomip("r_nomip", "0");

ShaderAPI_Base::ShaderAPI_Base()
{
	m_nViewportWidth			= 800;
	m_nViewportHeight			= 600;

	m_pCurrentShader			= NULL;
	m_pSelectedShader			= NULL;

	m_pCurrentBlendstate		= NULL;
	m_pCurrentDepthState		= NULL;
	m_pCurrentRasterizerState	= NULL;

	m_pSelectedBlendstate		= NULL;
	m_pSelectedDepthState		= NULL;
	m_pSelectedRasterizerState	= NULL;

	m_pErrorTexture				= NULL;

	// VF selectoin
	m_pSelectedVertexFormat		= NULL;
	m_pCurrentVertexFormat		= NULL;

	// Index buffer
	m_pSelectedIndexBuffer		= NULL;
	m_pCurrentIndexBuffer		= NULL;

	// Vertex buffer
	memset(m_pSelectedVertexBuffers, 0, sizeof(m_pSelectedVertexBuffers));
	memset(m_pCurrentVertexBuffers, 0, sizeof(m_pCurrentVertexBuffers));

	memset(m_pActiveVertexFormat, 0, sizeof(m_pActiveVertexFormat));

	memset(m_nCurrentOffsets, 0, sizeof(m_nCurrentOffsets));
	memset(m_nSelectedOffsets, 0, sizeof(m_nSelectedOffsets));

	// Index buffer
	m_pSelectedIndexBuffer		= NULL;
	m_pCurrentIndexBuffer		= NULL;

	memset(m_pSelectedTextures,0,sizeof(m_pSelectedTextures));
	memset(m_pCurrentTextures,0,sizeof(m_pCurrentTextures));

	memset(m_pSelectedVertexTextures,0,sizeof(m_pSelectedVertexTextures));
	memset(m_pCurrentVertexTextures,0,sizeof(m_pCurrentVertexTextures));

	memset(m_pCurrentColorRenderTargets,0,sizeof(m_pCurrentColorRenderTargets));
	memset(m_nCurrentCRTSlice,0,sizeof(m_nCurrentCRTSlice));

	m_pCurrentDepthRenderTarget = NULL;

	m_nDrawCalls				= 0;
	m_nTrianglesCount			= 0;
}

// Init + Shurdown
void ShaderAPI_Base::Init( shaderAPIParams_t &params )
{
	m_params = &params;

	m_nScreenFormat = params.screenFormat;

	// Do master reset for all things
	Reset();

	m_pErrorTexture = GenerateErrorTexture();
	m_pErrorTexture->Ref_Grab();

	ConVar* r_debug_showTexture = (ConVar*)g_consoleCommands->FindCvar("r_debug_showTexture");

	if(r_debug_showTexture)
		r_debug_showTexture->SetVariantsCallback(GetConsoleTextureList);
}

void ShaderAPI_Base::ThreadLock()
{
	m_Mutex.Lock();
}

void ShaderAPI_Base::ThreadUnlock()
{
	m_Mutex.Unlock();
}

ETextureFormat ShaderAPI_Base::GetScreenFormat()
{
	return m_nScreenFormat;
}

void ShaderAPI_Base::Shutdown()
{
	ConVar* r_debug_showTexture = (ConVar*)g_consoleCommands->FindCvar("r_debug_showTexture");

	if(r_debug_showTexture)
		r_debug_showTexture->SetVariantsCallback(nullptr);

	Reset();

	FreeTexture(m_pErrorTexture);
	m_pErrorTexture = nullptr;

	for(int i = 0; i < m_TextureList.numElem();i++)
	{
		FreeTexture(m_TextureList[i]);
		i--;
	}
	m_TextureList.clear();

	for(int i = 0; i < m_ShaderList.numElem();i++)
	{
		DestroyShaderProgram(m_ShaderList[i]);
		i--;
	}
	m_ShaderList.clear();

	for(int i = 0; i < m_VFList.numElem();i++)
	{
		DestroyVertexFormat(m_VFList[i]);
		i--;
	}
	m_VFList.clear();

	for(int i = 0; i < m_VBList.numElem();i++)
	{
		DestroyVertexBuffer(m_VBList[i]);
		i--;
	}
	m_VBList.clear();

	for(int i = 0; i < m_IBList.numElem();i++)
	{
		DestroyIndexBuffer(m_IBList[i]);
		i--;
	}
	m_IBList.clear();

	for(int i = 0; i < m_SamplerStates.numElem();i++)
	{
		DestroyRenderState(m_SamplerStates[i]);
		i--;
	}
	m_SamplerStates.clear();

	for(int i = 0; i < m_BlendStates.numElem();i++)
	{
		DestroyRenderState(m_BlendStates[i]);
		i--;
	}
	m_BlendStates.clear();

	for(int i = 0; i < m_DepthStates.numElem();i++)
	{
		DestroyRenderState(m_DepthStates[i]);
		i--;
	}
	m_DepthStates.clear();

	for(int i = 0; i < m_RasterizerStates.numElem();i++)
	{
		DestroyRenderState(m_RasterizerStates[i]);
		i--;
	}
	m_RasterizerStates.clear();

	for(int i = 0; i < m_OcclusionQueryList.numElem(); i++)
	{
		DestroyOcclusionQuery(m_OcclusionQueryList[i]);
		i--;
	}
	m_OcclusionQueryList.clear();
}

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

// Draw call counter
int ShaderAPI_Base::GetDrawCallsCount() const
{
	return m_nDrawCalls;
}

// Draw call counter
int ShaderAPI_Base::GetDrawIndexedPrimitiveCallsCount() const
{
	return m_nDrawIndexedPrimitiveCalls;
}

// triangles per scene drawing
int ShaderAPI_Base::GetTrianglesCount() const
{
	return m_nTrianglesCount;
}

// Resetting the counters
void ShaderAPI_Base::ResetCounters()
{
	m_nDrawCalls					= 0;
	m_nTrianglesCount				= 0;
	m_nDrawIndexedPrimitiveCalls	= 0;
}

void ShaderAPI_Base::Reset(int nResetTypeFlags)
{
	if (nResetTypeFlags & STATE_RESET_SHADER)
		m_pSelectedShader = NULL;

	if (nResetTypeFlags & STATE_RESET_BS)
		m_pSelectedBlendstate = NULL;

	if (nResetTypeFlags & STATE_RESET_DS)
		m_pSelectedDepthState = NULL;

	if (nResetTypeFlags & STATE_RESET_RS)
		m_pSelectedRasterizerState = NULL;

	if (nResetTypeFlags & STATE_RESET_VF)
		m_pSelectedVertexFormat = NULL;

	if (nResetTypeFlags & STATE_RESET_IB)
		m_pSelectedIndexBuffer = NULL;

	if (nResetTypeFlags & STATE_RESET_VB)
	{
		// i think is faster
		memset(m_pSelectedVertexBuffers,0,sizeof(m_pSelectedVertexBuffers));
		memset(m_nSelectedOffsets,0,sizeof(m_nSelectedOffsets));
	}

	// i think is faster
	if (nResetTypeFlags & STATE_RESET_TEX)
	{
		memset(m_pSelectedTextures,0,sizeof(m_pSelectedTextures));
		memset(m_pSelectedVertexTextures,0,sizeof(m_pSelectedVertexTextures));
	}
}

void ShaderAPI_Base::Apply()
{
	// Apply shaders
	ApplyShaderProgram();

	// Apply shader constants
	ApplyConstants();

	// Apply the textures
	ApplyTextures();

	// Apply the sampling
	ApplySamplerState();

	// Apply the index,vertex buffers
	ApplyBuffers();

	// Apply the depth state
	ApplyDepthState();

	// Apply the blending
	ApplyBlendState();

	// Apply the rasterizer state
	ApplyRasterizerState();
}

void ShaderAPI_Base::ApplyBuffers()
{
	// First change the vertex format
	ChangeVertexFormat( m_pSelectedVertexFormat );

	for (int i = 0; i < MAX_VERTEXSTREAM; i++)
		ChangeVertexBuffer(m_pSelectedVertexBuffers[i], i,m_nSelectedOffsets[i]);

	ChangeIndexBuffer( m_pSelectedIndexBuffer );
}

// default error texture pointer
ITexture* ShaderAPI_Base::GetErrorTexture()
{
	return m_pErrorTexture;
}



void ShaderAPI_Base::GetConsoleTextureList(const ConCommandBase* base, DkList<EqString>& list, const char* query)
{
	ShaderAPI_Base* baseApi = ((ShaderAPI_Base*)g_pShaderAPI);

	CScopedMutex m(baseApi->m_Mutex);

	DkList<ITexture*>& texList = ((ShaderAPI_Base*)g_pShaderAPI)->m_TextureList;

	const int LIST_LIMIT = 50;

	for ( int i = 0; i < texList.numElem(); i++ )
	{
		if(list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		if(*query == 0 || xstristr(texList[i]->GetName(), query))
			list.append(texList[i]->GetName());
	}
}

void ShaderAPI_Base::SetViewport(int x, int y, int w, int h)
{
	m_nViewportWidth = w;
	m_nViewportHeight = h;
}

void ShaderAPI_Base::GetViewport(int &x, int &y, int &w, int &h)
{
	w = m_nViewportWidth;
	h = m_nViewportHeight;
}

// Find texture
ITexture* ShaderAPI_Base::FindTexture(const char* pszName)
{
	EqString searchStr(pszName);
	searchStr.Path_FixSlashes();

	int nameHash = StringToHash(pszName, true);

	CScopedMutex m(m_Mutex);
	for(int i = 0; i < m_TextureList.numElem();i++)
	{
		if(((CTexture*)m_TextureList[i])->m_nameHash == nameHash)
			return m_TextureList[i];
	}

	return NULL;
}


SamplerStateParam_t ShaderAPI_Base::MakeSamplerState(ER_TextureFilterMode textureFilterType,ER_TextureAddressMode addressS, ER_TextureAddressMode addressT, ER_TextureAddressMode addressR)
{
	HOOK_TO_CVAR(r_anisotropic);

	/*
	for(int i = 0; i < m_SamplerStates.numElem();i++)
	{
		if( m_SamplerStates[i]->wrapS == addressS &&
			m_SamplerStates[i]->wrapT == addressT &&
			m_SamplerStates[i]->wrapR == addressR &&
			m_SamplerStates[i]->minFilter == textureFilterType)
			return m_SamplerStates[i];
	}
	*/

	// If nothing found, create new one
	SamplerStateParam_t newParam;

	// Setup filtering mode
	newParam.minFilter = textureFilterType;
	newParam.magFilter = (textureFilterType == TEXFILTER_NEAREST)? TEXFILTER_NEAREST : TEXFILTER_LINEAR;

	// Setup clamping
	newParam.wrapS = addressS;
	newParam.wrapT = addressT;
	newParam.wrapR = addressR;
	newParam.compareFunc = COMP_LESS;

	newParam.lod = 0.0f;

	newParam.aniso = (int)clamp(m_caps.maxTextureAnisotropicLevel,0, r_anisotropic->GetInt());

	return newParam;
}

// Find Rasterizer State with adding new one (if not exist)
RasterizerStateParams_t ShaderAPI_Base::MakeRasterizerState(ER_CullMode nCullMode, ER_FillMode nFillMode, bool bMultiSample, bool bScissor)
{
	/*
	for(int i = 0; i < m_RasterizerStates.numElem();i++)
	{
		if( m_RasterizerStates[i]->cullMode == nCullMode &&
			m_RasterizerStates[i]->fillMode == nFillMode &&
			m_RasterizerStates[i]->multiSample == bMultiSample &&
			m_RasterizerStates[i]->scissor == bScissor)
			return m_RasterizerStates[i];
	}
	*/

	// If nothing found, create new one
	RasterizerStateParams_t newParam;
	newParam.cullMode = nCullMode;
	newParam.fillMode = nFillMode;
	newParam.multiSample = bMultiSample;
	newParam.scissor = bScissor;
	newParam.depthBias = 1.0f;
	newParam.slopeDepthBias = 0.0f;

	return newParam;
}

// Find Depth State with adding new one (if not exist)
DepthStencilStateParams_t ShaderAPI_Base::MakeDepthState(bool bDoDepthTest, bool bDoDepthWrite, ER_CompareFunc depthCompFunc)
{
	/*
	for(int i = 0; i < m_DepthStates.numElem();i++)
	{
		if( m_DepthStates[i]->depthFunc == depthCompFunc &&
			m_DepthStates[i]->depthTest == bDoDepthTest &&
			m_DepthStates[i]->depthWrite == bDoDepthWrite)
			return m_DepthStates[i];
	}
	*/

	// If nothing found, create new one
	DepthStencilStateParams_t newParam;
	newParam.depthFunc = depthCompFunc;
	newParam.depthTest = bDoDepthTest;
	newParam.depthWrite = bDoDepthWrite;

	return newParam;
}


//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

// Load texture from file (DDS or TGA only)
ITexture* ShaderAPI_Base::LoadTexture( const char* pszFileName, ER_TextureFilterMode textureFilterType, ER_TextureAddressMode textureAddress/* = TEXADDRESS_WRAP*/, int nFlags/* = 0*/ )
{
	// first search for existing texture
	ITexture* pFoundTexture = FindTexture(pszFileName);

	EqString texturePath;

	if(!(nFlags & TEXFLAG_REALFILEPATH))
		texturePath = EqString(m_params->texturePath) + pszFileName;
	else
		texturePath = pszFileName;

	texturePath.Path_FixSlashes();

	// build valid texture paths
	EqString texturePathExt = texturePath + EqString(TEXTURE_DEFAULT_EXTENSION);
	EqString textureAnimPathExt = texturePath + EqString(TEXTURE_ANIMATED_EXTENSION);

	texturePathExt.Path_FixSlashes();
	textureAnimPathExt.Path_FixSlashes();

	if(!pFoundTexture)
		pFoundTexture = FindTexture(texturePathExt.GetData());

	if(!pFoundTexture)
		pFoundTexture = FindTexture(textureAnimPathExt.GetData());

	// found?
	if(pFoundTexture != NULL)
		return pFoundTexture;

	// Don't load textures starting with special symbols
	if(pszFileName[0] == '$')
		return NULL;

	// create sampler state
	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType,textureAddress,textureAddress,textureAddress);

	// try reading animation buffer
	char* animScriptBuffer = g_fileSystem->GetFileBuffer(textureAnimPathExt.GetData());

	DkList<CImage*> pImages;

	if(animScriptBuffer)
	{
		if(rs_echo_texture_loading.GetBool())
			Msg("Loading animated textures from %s\n", textureAnimPathExt.GetData());

		DkList<EqString> cmds;
		xstrsplit(animScriptBuffer, "\n", cmds);

		bool success = true;

		EqString texturePathA;
		EqString texturePathExtA;

		// generate file names
		// and load image files for further uploading to GPU
		for(int i = 0; i < cmds.numElem(); i++)
		{
			cmds[i] = cmds[i].Left(cmds[i].Length()-1);

			if(!(nFlags & TEXFLAG_REALFILEPATH))
				texturePathA = EqString(m_params->texturePath) + cmds[i];
			else
				texturePathA = cmds[i];

			texturePathExtA = texturePathA + EqString(TEXTURE_DEFAULT_EXTENSION);

			texturePathExtA.Path_FixSlashes();

			CImage* pImg = new CImage();

			success = pImg->LoadDDS(texturePathExtA.GetData(), 0);

			if(success)
			{
				if(rs_echo_texture_loading.GetBool())
					MsgInfo("Animated Texture loaded: '%s'\n", cmds[i].GetData());
			}

			pImg->SetName( textureAnimPathExt.GetData() );

			if(!success)
			{
				delete pImg;

				MsgError("Can't load texture '%s' for animations\n", cmds[i].GetData());
				break;
			}

			// add image
			pImages.append(pImg);
		}

		// failt
		if(!success)
		{
			for(int i = 0; i < pImages.numElem();i++)
				delete pImages[i];

			PPFree(animScriptBuffer);

			return m_pErrorTexture;
		}

		PPFree(animScriptBuffer);
	}
	else
	{
		CImage *pImage = new CImage();

		bool stateLoad = pImage->LoadDDS(texturePathExt.GetData(),0);

		if(!stateLoad)
		{
			texturePathExt = texturePath + EqString(TEXTURE_SECONDARY_EXTENSION);
			texturePathExt.Path_FixSlashes();
			stateLoad = pImage->LoadTGA(texturePathExt.GetData());
			pImage->SetName((texturePath + EqString(TEXTURE_DEFAULT_EXTENSION)).GetData());
		}

		if(stateLoad)
		{
			pImages.append(pImage);

			if(rs_echo_texture_loading.GetBool())
				MsgInfo("Texture loaded: %s\n",pszFileName);
		}
		else
		{
			MsgError("Can't open texture \"%s\"\n",pszFileName);
			delete pImage;
		}
	}

	// Now create the texture
	pFoundTexture = CreateTexture(pImages, texSamplerParams, nFlags);

	// free images
	for(int i = 0;i < pImages.numElem();i++)
		delete pImages[i];

	// Generate the error
	if(!pFoundTexture)
		pFoundTexture = m_pErrorTexture;

	return pFoundTexture;
}

ITexture* ShaderAPI_Base::CreateTexture(const DkList<CImage*>& pImages, const SamplerStateParam_t& sampler, int nFlags)
{
	if(!pImages.numElem())
		return NULL;

	
	if(r_nomip.GetBool())
	{
		for(int i = 0; i < pImages.numElem(); i++)
			pImages[i]->RemoveMipMaps(0,1);
	}

	// create texture
	ITexture* pTexture = NULL;
	CreateTextureInternal(&pTexture, pImages, sampler, nFlags);

	// the created texture is automatically added to list

	return pTexture;
}

// creates procedural (lockable) texture
ITexture* ShaderAPI_Base::CreateProceduralTexture(const char* pszName,
													ETextureFormat nFormat,
													int width, int height,
													int depth,
													int arraySize,
													ER_TextureFilterMode texFilter,
													ER_TextureAddressMode textureAddress,
													int nFlags,
													int nDataSize, const unsigned char* pData)
{
	CImage genTex;
	genTex.SetName(pszName);
	// make texture
	ubyte* newData = genTex.Create(nFormat, width, height, depth, 1, arraySize);

	if(newData)
	{
		int nTexDataSize = width*height*depth*arraySize*GetBytesPerPixel(nFormat);

		memset(newData, 0, nTexDataSize);

		// copy data if available
		if(pData && nDataSize)
		{
			ASSERT(nDataSize <= nTexDataSize);

			memcpy(newData, pData, nDataSize);
		}
	}
	else
	{
		MsgError("ERROR -  Cannot create procedural texture '%s', probably bad format\n", pszName);
		return NULL;	// don't generate error
	}

	DkList<CImage*> imgs;
	imgs.append(&genTex);

	SamplerStateParam_t sampler = g_pShaderAPI->MakeSamplerState(texFilter,textureAddress,textureAddress,textureAddress);
	return g_pShaderAPI->CreateTexture(imgs, sampler, nFlags);
}

bool ShaderAPI_Base::RestoreTextureInternal(ITexture* pTexture)
{
	char* animScriptBuffer = g_fileSystem->GetFileBuffer(pTexture->GetName());

	DkList<CImage*> pImages;

	if(animScriptBuffer)
	{
		if(rs_echo_texture_loading.GetBool())
			Msg("Loading animated textures from %s\n", pTexture->GetName());

		DkList<EqString> cmds;
		xstrsplit(animScriptBuffer, "\n", cmds);

		bool success = true;

		EqString texturePathA;
		EqString texturePathExtA;

		for(int i = 0; i < cmds.numElem(); i++)
		{
			cmds[i] = cmds[i].Left(cmds[i].Length()-1);

			texturePathA = EqString(m_params->texturePath) + cmds[i];
			texturePathExtA = texturePathA + EqString(TEXTURE_DEFAULT_EXTENSION);

			CImage* pImage = new CImage();

			success = pImages[i]->LoadDDS(texturePathExtA.GetData(), 0);

			if(success)
			{
				if(rs_echo_texture_loading.GetBool())
					MsgInfo("Animated Texture loaded: %s\n", cmds[i].GetData());
			}

			pImages[i]->SetName(pTexture->GetName());

			if(!success)
			{
				delete pImage;

				MsgError("Can't load texture %s for animations\n", cmds[i].GetData());
				break;
			}

			pImages.append(pImage);
		}

		// failt
		if(!success)
		{
			for(int i = 0; i < pImages.numElem();i++)
				delete pImages[i];

			PPFree(animScriptBuffer);

			return false;
		}

		PPFree(animScriptBuffer);
	}

	CImage* pImage = new CImage();

	bool stateLoad = pImage->LoadImage(pTexture->GetName(),0);

	if(stateLoad)
	{
		if(rs_echo_texture_loading.GetBool())
			MsgInfo("Texture loaded: %s\n", pTexture->GetName());

		pImages.append(pImage);
	}
	else
	{
		MsgError("Can't open texture \"%s\"\n", pTexture->GetName());
		delete pImage;
	}

	CreateTextureInternal(&pTexture, pImages, pTexture->GetSamplerState(),pTexture->GetFlags());

	for(int i = 0;i < pImages.numElem();i++)
		delete pImages[i];

	// Generate the error
	if(!pTexture)
		return false;

	return true;
}

// Error texture generator
ITexture* ShaderAPI_Base::GenerateErrorTexture(int nFlags/* = 0*/)
{
	if(nFlags & TEXFLAG_NULL_ON_ERROR)
		return NULL;

	SamplerStateParam_t texSamplerParams = MakeSamplerState(TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_WRAP,TEXADDRESS_WRAP,TEXADDRESS_WRAP);

	Vector4D color;
	Vector4D color2;

	color.x = color.y = color.z = 0; color.w = 128;
	color2.x = 255;
	color2.y = 64;
	color2.z = 255;
	color2.w = 0;

	int m_nCheckerSize = 4;

	CImage image;
	ubyte *dest = image.Create(FORMAT_RGBA8,32,32,1,1);

	image.SetName("error");

	PixelWriter pixelWriter;
	pixelWriter.SetPixelMemory(FORMAT_RGBA8,dest,0);

	int nWidth = image.GetWidth();
	int nHeight = image.GetHeight();

	for (int y = 0; y < nHeight; ++y)
	{
		pixelWriter.Seek( 0, y );
		for (int x = 0; x < nWidth; ++x)
		{
			if ((x & m_nCheckerSize) ^ (y & m_nCheckerSize))
			{
				pixelWriter.WritePixel( color.x, color.y, color.z, color.w );
			}
			else
			{
				pixelWriter.WritePixel( color2.x, color2.y, color2.z, color2.w );
			}
		}
	}

	//if(nFlags & TEXTURE_FLAG_NORMALMAP)
	//	pImage->toNormalMap(FORMAT_RGBA8);

	image.CreateMipMaps();

	DkList<CImage*> images;
	images.append(&image);

	ITexture* pOutTexture = CreateTexture(images, texSamplerParams, nFlags);

	return pOutTexture;
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

// Changes render target (single RT)
void ShaderAPI_Base::ChangeRenderTarget(ITexture* pRenderTarget, int nCubemapFace, ITexture* pDepthTarget, int nDepthSlice )
{
	ChangeRenderTargets(&pRenderTarget, pRenderTarget ? 1 : 0, &nCubemapFace, pDepthTarget, nDepthSlice);
}

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

// Sets the reserved blending state (from pre-defined preset)
void ShaderAPI_Base::SetBlendingState( IRenderState* pBlending )
{
	if(pBlending)
	{
		if(pBlending->GetType() == RENDERSTATE_BLENDING)
			m_pSelectedBlendstate = pBlending;
		else
		{
			MsgError("RenderState setup invalid: not a RENDERSTATE_BLENDING, got %d\n", pBlending->GetType());
			ASSERT(!"RenderState setup invalid");
		}
	}
	else
	{
		m_pSelectedBlendstate = NULL;
	}
}

// Set depth and stencil state
void ShaderAPI_Base::SetDepthStencilState( IRenderState *pDepthStencilState )
{
	if(pDepthStencilState)
	{
		if(pDepthStencilState->GetType() == RENDERSTATE_DEPTHSTENCIL)
			m_pSelectedDepthState = pDepthStencilState;
		else
		{
			MsgError("RenderState setup invalid: not a RENDERSTATE_DEPTHSTENCIL, got %d\n", pDepthStencilState->GetType());
			ASSERT(!"RenderState setup invalid");
		}
	}
	else
	{
		m_pSelectedDepthState = NULL;
	}
}

// sets reserved rasterizer mode
void ShaderAPI_Base::SetRasterizerState( IRenderState* pState )
{
	if(pState)
	{
		if(pState->GetType() == RENDERSTATE_RASTERIZER)
		{
			m_pSelectedRasterizerState = pState;
		}
		else
		{
			MsgError("RenderState setup invalid: not a RENDERSTATE_RASTERIZER, got %d\n", pState->GetType());
			ASSERT(!"RenderState setup invalid");
		}
	}
	else
	{
		m_pSelectedRasterizerState = NULL;
	}
}

// Set Texture for Fixed-Function Pipeline
void ShaderAPI_Base::SetTextureOnIndex(ITexture* pTexture,int level /* = 0*/)
{
	// Setup for FFP
	if(m_caps.maxTextureUnits <= 1 && level > 0)
		return; // If multitexturing is not supported

	if(level < 0)
	{
		int vLevel = level+(MAX_VERTEXTEXTURES+1);
		m_pSelectedVertexTextures[vLevel] = pTexture;
	}
	else
		m_pSelectedTextures[level] = pTexture;
}

// returns the currently set textre at level
ITexture* ShaderAPI_Base::GetTextureAt( int level ) const
{
	// Setup for FFP
	if(m_caps.maxTextureUnits <= 1 && level > 0)
		return NULL; // If multitexturing is not supported

	if(level < 0)
	{
		int vLevel = level+(MAX_VERTEXTEXTURES+1);
		return m_pSelectedVertexTextures[vLevel];
	}

	return m_pSelectedTextures[level];
}

// VBO

// Sets the vertex format
void ShaderAPI_Base::SetVertexFormat(IVertexFormat* pVertexFormat)
{
	m_pSelectedVertexFormat = pVertexFormat;
}

// Sets the vertex buffer
void ShaderAPI_Base::SetVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset)
{
	m_pSelectedVertexBuffers[nStream] = pVertexBuffer;
	m_nSelectedOffsets[nStream] = offset;
}

// Sets the vertex buffer
void ShaderAPI_Base::SetVertexBuffer(int nStream, const void* base)
{
	m_pSelectedVertexBuffers[nStream] = NULL;
	m_nSelectedOffsets[nStream] = (intptr) base;
}

// Changes the index buffer
void ShaderAPI_Base::SetIndexBuffer(IIndexBuffer *pIndexBuffer)
{
	m_pSelectedIndexBuffer = pIndexBuffer;
}

bool isShaderInc(const char ch)
{
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '/' || ch == '\\' || ch == '.');
}

bool isShaderIncDef(const char ch)
{
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '$' || ch == '/' || ch == '\\' || ch == '.');
}

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------
void ProcessShaderFileIncludes(char** buffer, const char* pszFileName, shaderProgramText_t& textData, bool bStart = false)
{
	if (!(*buffer))
		return;

	int nLine = 0;

	Tokenizer tok;
	tok.setString(*buffer);

	Tokenizer lineParser;

	// set main source filename
	EqString newSrc;

	bool isOpenGL = g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_OPENGL;

	if (isOpenGL)
		newSrc = EqString::Format("\r\n#line 1 %d\r\n", textData.includes.numElem());		// I hate, hate and hate GLSL for not supporting source file names, only file numbers.
	else
		newSrc = "\r\n#line 1 \"" + _Es(pszFileName) + "\"\r\n";

	bool afterSkipLine = false;

	char* str;
	while((str = tok.nextLine()) != NULL)
	{
		lineParser.setString(str);

		bool skipLine = false;

		char* str2;
		while((str2 = lineParser.next(isShaderIncDef)) != NULL)
		{
			nLine++;

			if(!strcmp("//", str2))
			{
				str2 = lineParser.next(isShaderIncDef);

				if(!str2 || strcmp("$INCLUDE", str2))
					break;

				char* inc_text = lineParser.next(isShaderInc);

				EqString inc_filename(EqString(SHADERS_DEFAULT_PATH) + g_pShaderAPI->GetRendererName() + "/" + EqString(inc_text));

				// add include filename
				textData.includes.append( inc_filename );

				char* psBuffer = g_fileSystem->GetFileBuffer(inc_filename.GetData());

				ProcessShaderFileIncludes(&psBuffer, inc_filename.GetData(), textData);

				if(psBuffer)
				{
					newSrc = newSrc + _Es("\r\n") + psBuffer + _Es("\r\n");
					PPFree( psBuffer );
				}
				else
					MsgError("'%s': cannot open file '%s' for include!\n", pszFileName, inc_filename.GetData());

				skipLine = true;

				break;
			}
		}

		if(skipLine)
		{
			afterSkipLine = true;
			continue;
		}

		// restore line counter
		if (afterSkipLine)
		{
			if (isOpenGL)
				newSrc = newSrc + EqString::Format("#line %d %d\r\n", nLine, textData.includes.numElem());
			else
				newSrc = newSrc + EqString::Format("#line %d \"", nLine) + _Es(pszFileName) + "\"\r\n";

			afterSkipLine = false;
		}

		newSrc = newSrc + str;
	}

	*buffer = (char*)PPReAlloc(*buffer, newSrc.Length()+1);
	strcpy(*buffer, newSrc.GetData());
}

// Loads and compiles shaders from files
bool ShaderAPI_Base::LoadShadersFromFile(IShaderProgram* pShaderOutput, const char* pszFilePrefix, const char *extra)
{
	if(pShaderOutput == NULL)
		return false;

	bool bResult = false;

	EqString fileNameVS = (EqString::Format(SHADERS_DEFAULT_PATH "%s/%s.vs", GetRendererName(), pszFilePrefix));
	EqString fileNamePS = (EqString::Format(SHADERS_DEFAULT_PATH "%s/%s.ps", GetRendererName(), pszFilePrefix));
	EqString fileNameGS = (EqString::Format(SHADERS_DEFAULT_PATH "%s/%s.gs", GetRendererName(), pszFilePrefix));

	bool vsRequiried = true;	// vertex shader is always required
	bool psRequiried = false;
	bool gsRequiried = false;

	shaderProgramCompileInfo_t info;

	// Load KeyValues
	KeyValues pKv;

	if( pKv.LoadFromFile((EqString(SHADERS_DEFAULT_PATH) + pszFilePrefix + ".txt").GetData()) )
	{
		kvkeybase_t* sec = pKv.GetRootSection();

		kvkeybase_t* pixelProgramName = sec->FindKeyBase("PixelShaderProgram");
		kvkeybase_t* vertexProgramName = sec->FindKeyBase("VertexShaderProgram");
		kvkeybase_t* geometryProgramName = sec->FindKeyBase("GeometryShaderProgram");

		info.disableCache = KV_GetValueBool(sec->FindKeyBase("DisableCache"));

		if(pixelProgramName)
			psRequiried = true;

		if(geometryProgramName)
			gsRequiried = true;

		const char* vertexProgNameStr = KV_GetValueString(vertexProgramName, 0, pszFilePrefix);
		const char* pixelProgNameStr = KV_GetValueString(pixelProgramName, 0, pszFilePrefix);
		const char* geomProgNameStr = KV_GetValueString(geometryProgramName, 0, pszFilePrefix);

		fileNameVS = EqString::Format(SHADERS_DEFAULT_PATH "%s/%s.vs", GetRendererName(), vertexProgNameStr);
		fileNamePS = EqString::Format(SHADERS_DEFAULT_PATH "%s/%s.ps", GetRendererName(), pixelProgNameStr);
		fileNameGS = EqString::Format(SHADERS_DEFAULT_PATH "%s/%s.gs", GetRendererName(), geomProgNameStr);

		// API section
		// find corresponding API
		for(int i = 0; i < sec->keys.numElem(); i++)
		{
			kvkeybase_t* apiKey = sec->keys[i];

			if(!stricmp(apiKey->GetName(), "api"))
			{
				for(int j = 0; j < apiKey->values.numElem(); j++)
				{
					if(!stricmp(KV_GetValueString(apiKey, j), GetRendererName()))
					{
						info.apiPrefs = apiKey;
						break;
					}
				}

				if(info.apiPrefs)
					break;
			}
		}
	}

	// add first files as includes (index = 0)
	info.vs.includes.append(fileNameVS);
	info.ps.includes.append(fileNamePS);
	info.gs.includes.append(fileNameGS);

	// load them
	info.vs.text = g_fileSystem->GetFileBuffer(fileNameVS.GetData());
	info.ps.text = g_fileSystem->GetFileBuffer(fileNamePS.GetData());
	info.gs.text = g_fileSystem->GetFileBuffer(fileNameGS.GetData());

	if (!info.ps.text && psRequiried)
	{
		MsgError("Can't open pixel shader file '%s'!\n", fileNamePS.GetData());
		PPFree(info.vs.text);
		PPFree(info.ps.text);
		PPFree(info.gs.text);
		return false;
	}

	if(!info.vs.text && vsRequiried)
	{
		MsgError("Can't open vertex shader file '%s'!\n",fileNameVS.GetData());
		PPFree(info.vs.text);
		PPFree(info.ps.text);
		PPFree(info.gs.text);
		return false;
	}

	if (!info.gs.text && gsRequiried)
	{
		MsgError("Can't open geometry shader file '%s'!\n", fileNameVS.GetData());
		PPFree(info.vs.text);
		PPFree(info.ps.text);
		PPFree(info.gs.text);
		return false;
	}

	ProcessShaderFileIncludes(&info.vs.text, fileNameVS.GetData(), info.vs, true);
	ProcessShaderFileIncludes(&info.ps.text, fileNamePS.GetData(), info.ps, true);
	ProcessShaderFileIncludes(&info.gs.text, fileNameGS.GetData(), info.gs, true);

	// checksum please
	if(info.vs.text)
		info.vs.checksum = CRC32_BlockChecksum(info.vs.text, strlen(info.vs.text));

	if(info.ps.text)
		info.ps.checksum = CRC32_BlockChecksum(info.ps.text, strlen(info.ps.text));

	if(info.gs.text)
		info.gs.checksum = CRC32_BlockChecksum(info.gs.text, strlen(info.gs.text));

	// compile the shaders
	bResult = CompileShadersFromStream( pShaderOutput, info, extra );

	PPFree(info.vs.text);
	PPFree(info.ps.text);
	PPFree(info.gs.text);

	return bResult;
}

// Shader constants setup
int ShaderAPI_Base::SetShaderConstantInt(const char *pszName, const int constant, int const_id)
{
	return SetShaderConstantRaw(pszName, &constant, sizeof(constant), const_id);
}

int ShaderAPI_Base::SetShaderConstantFloat(const char *pszName, const float constant, int const_id)
{
	return SetShaderConstantRaw(pszName, &constant, sizeof(constant), const_id);
}

int ShaderAPI_Base::SetShaderConstantVector2D(const char *pszName, const Vector2D &constant, int const_id)
{
	return SetShaderConstantRaw(pszName, &constant, sizeof(constant), const_id);
}

int ShaderAPI_Base::SetShaderConstantVector3D(const char *pszName, const Vector3D &constant, int const_id)
{
	return SetShaderConstantRaw(pszName, &constant, sizeof(constant), const_id);
}

int ShaderAPI_Base::SetShaderConstantVector4D(const char *pszName, const Vector4D &constant, int const_id)
{
	return SetShaderConstantRaw(pszName, &constant, sizeof(constant), const_id);
}

int ShaderAPI_Base::SetShaderConstantMatrix4(const char *pszName, const Matrix4x4 &constant, int const_id)
{
	return SetShaderConstantRaw(pszName, &constant, sizeof(constant), const_id);
}

int ShaderAPI_Base::SetShaderConstantArrayFloat(const char *pszName, const float *constant, int count, int const_id)
{
	return SetShaderConstantRaw(pszName, constant, count * sizeof(float), const_id);
}

int ShaderAPI_Base::SetShaderConstantArrayVector2D(const char *pszName, const Vector2D *constant, int count, int const_id)
{
	return SetShaderConstantRaw(pszName, constant, count * sizeof(Vector2D), const_id);
}

int ShaderAPI_Base::SetShaderConstantArrayVector3D(const char *pszName, const Vector3D *constant, int count, int const_id)
{
	return SetShaderConstantRaw(pszName, constant, count * sizeof(Vector3D), const_id);
}

int ShaderAPI_Base::SetShaderConstantArrayVector4D(const char *pszName, const Vector4D *constant, int count, int const_id)
{
	return SetShaderConstantRaw(pszName, constant, count * sizeof(Vector4D), const_id);
}

int ShaderAPI_Base::SetShaderConstantArrayMatrix4(const char *pszName, const Matrix4x4 *constant, int count, int const_id)
{
	return SetShaderConstantRaw(pszName, constant, count * sizeof(Matrix4x4), const_id);
}
