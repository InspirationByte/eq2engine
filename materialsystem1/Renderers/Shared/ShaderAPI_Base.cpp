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

#include "core/IEqParallelJobs.h"

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

void ShaderAPI_Base::GetImagesForTextureName(DkList<EqString>& textureNames, const char* pszFileName, int nFlags)
{
	EqString texturePath;

	if (!(nFlags & TEXFLAG_REALFILEPATH))
		texturePath = EqString(m_params->texturePath) + pszFileName;
	else
		texturePath = pszFileName;

	texturePath.Path_FixSlashes();

	// build valid texture paths
	EqString texturePathExt = texturePath + EqString(TEXTURE_DEFAULT_EXTENSION);
	EqString textureAnimPathExt = texturePath + EqString(TEXTURE_ANIMATED_EXTENSION);

	texturePathExt.Path_FixSlashes();
	textureAnimPathExt.Path_FixSlashes();

	// has pattern for animated texture?
	int animCountStart = texturePath.Find("[");
	int animCountEnd = -1;

	if (animCountStart != -1 &&
		(animCountEnd = texturePath.Find("]", false, animCountStart)) != -1)
	{
		// trying to load animated texture
		EqString textureWildcard = texturePath.Left(animCountStart);
		EqString textureFrameCount = texturePath.Mid(animCountStart + 1, (animCountEnd - animCountStart) - 1);
		int numFrames = atoi(textureFrameCount.ToCString());

		if (rs_echo_texture_loading.GetBool())
			Msg("Loading animated %d animated textures (%s)\n", numFrames, textureWildcard.ToCString());

		for (int i = 0; i < numFrames; i++)
		{
			EqString textureNameFrame = EqString::Format(textureWildcard.ToCString(), i);
			textureNames.append(textureNameFrame);
		}
	}
	else
	{
		// try loading older Animated Texture Index file
		EqString textureAnimPathExt = texturePath + EqString(TEXTURE_ANIMATED_EXTENSION);
		textureAnimPathExt.Path_FixSlashes();

		char* animScriptBuffer = g_fileSystem->GetFileBuffer(textureAnimPathExt.GetData());
		if (animScriptBuffer)
		{
			DkList<EqString> frameFilenames;
			xstrsplit(animScriptBuffer, "\n", frameFilenames);
			for (int i = 0; i < frameFilenames.numElem(); i++)
			{
				// delete carriage return character if any
				EqString animFrameFilename = frameFilenames[i].TrimChar('\r', true, true);

				if (!(nFlags & TEXFLAG_REALFILEPATH))
					animFrameFilename = EqString(m_params->texturePath) + animFrameFilename;
				else
					animFrameFilename = animFrameFilename;

				textureNames.append(animFrameFilename);
			}

			PPFree(animScriptBuffer);
		}
		else
		{
			textureNames.append(texturePath);
		}
	}
}

// Load texture from file (DDS or TGA only)
ITexture* ShaderAPI_Base::LoadTexture( const char* pszFileName, 
	ER_TextureFilterMode textureFilterType, ER_TextureAddressMode textureAddress/* = TEXADDRESS_WRAP*/, 
	int nFlags/* = 0*/ )
{
	// first search for existing texture
	ITexture* pFoundTexture = FindTexture(pszFileName);

	if (pFoundTexture != NULL)
		return pFoundTexture;

	// Don't load textures starting with special symbols
	if (pszFileName[0] == '$')
		return nullptr;

	DkList<EqString> textureNames;
	GetImagesForTextureName(textureNames, pszFileName, nFlags);

	DkList<CImage*> pImages;

	// load frames
	for (int i = 0; i < textureNames.numElem(); i++)
	{
		EqString texturePathExt = textureNames[i] + EqString(TEXTURE_DEFAULT_EXTENSION);

		CImage* img = new CImage();

		bool stateLoad = img->LoadDDS(texturePathExt.GetData(), 0);

		if (!stateLoad)
		{
			texturePathExt = textureNames[i] + EqString(TEXTURE_SECONDARY_EXTENSION);

			stateLoad = img->LoadTGA(texturePathExt.GetData());
			img->SetName((textureNames[i] + EqString(TEXTURE_DEFAULT_EXTENSION)).GetData());
		}

		if (stateLoad)
		{
			pImages.append(img);

			if (rs_echo_texture_loading.GetBool())
				MsgInfo("Texture loaded: %s\n", texturePathExt.ToCString());
		}
		else
		{
			MsgError("Can't open texture \"%s\"\n", texturePathExt.ToCString());
			delete img;
		}
	}

	// create sampler state
	SamplerStateParam_t texSamplerParams = MakeSamplerState(textureFilterType, textureAddress, textureAddress, textureAddress);

	// Now create the texture
	pFoundTexture = CreateTexture(pImages, texSamplerParams, nFlags);

	// free images
	for(int i = 0;i < pImages.numElem();i++)
		delete pImages[i];

	// Generate the error
	if(!pFoundTexture && !(nFlags & TEXFLAG_NULL_ON_ERROR))
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
	DkList<EqString> textureNames;
	GetImagesForTextureName(textureNames, pTexture->GetName(), pTexture->GetFlags());

	DkList<CImage*> pImages;

	// load frames
	for (int i = 0; i < textureNames.numElem(); i++)
	{
		EqString texturePathExt = textureNames[i] + EqString(TEXTURE_DEFAULT_EXTENSION);

		CImage* img = new CImage();

		bool stateLoad = img->LoadDDS(texturePathExt.GetData(), 0);

		if (!stateLoad)
		{
			texturePathExt = textureNames[i] + EqString(TEXTURE_SECONDARY_EXTENSION);

			stateLoad = img->LoadTGA(texturePathExt.GetData());
			img->SetName((textureNames[i] + EqString(TEXTURE_DEFAULT_EXTENSION)).GetData());
		}

		if (stateLoad)
		{
			pImages.append(img);

			if (rs_echo_texture_loading.GetBool())
				MsgInfo("Texture loaded: %s\n", texturePathExt.ToCString());
		}
		else
		{
			MsgError("Can't open texture \"%s\"\n", texturePathExt.ToCString());
			delete img;
		}
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

// returns vertex format
IVertexFormat* ShaderAPI_Base::FindVertexFormat(const char* name) const
{
	for (int i = 0; i < m_VFList.numElem(); i++)
	{
		if(!strcmp(name, m_VFList[i]->GetName()))
			return m_VFList[i];
	}
	return nullptr;
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
void LoadShaderFiles(char** buffer, const char* pszFileName, const char* rootPath, shaderProgramText_t& textData, bool bStart, bool bIgnoreIfFailed)
{
	EqString shaderFilePath;
	CombinePath(shaderFilePath, 2, rootPath, pszFileName);

	// try loading file
	*buffer = g_fileSystem->GetFileBuffer(shaderFilePath.ToCString());

	if (!(*buffer))
	{
		if(!bIgnoreIfFailed)
			MsgError("LoadShaderFiles: cannot open file '%s'!\n", shaderFilePath.ToCString());

		return;
	}

	// add include filename
	textData.includes.append(shaderFilePath);

	const bool isOpenGL = g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_OPENGL;

	const int thisIncludeNumber = textData.includes.numElem();

	// set main source filename
	EqString newSrc;
	if (isOpenGL)
		newSrc = EqString::Format("#line 0 %d\r\n", thisIncludeNumber);
	else
		newSrc = "#line 0 \"" + _Es(pszFileName) + "\"\r\n";

	bool afterSkipLine = false;
	int nLine = 0;

	Tokenizer tok, lineParser;
	tok.setString(*buffer);

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

				char* psBuffer = nullptr;
				LoadShaderFiles(&psBuffer, inc_text, rootPath, textData, false, false);

				if(psBuffer)
				{
					newSrc = newSrc + _Es("\r\n") + psBuffer + _Es("\r\n");
					PPFree( psBuffer );
				}

				skipLine = true;

				break;
			}
		}

		// don't parse more than 10 lines
		if (nLine > 10)
		{
			newSrc = newSrc + str;
			continue;
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
				newSrc = newSrc + EqString::Format("#line %d %d\r\n", nLine, thisIncludeNumber);
			else
				newSrc = newSrc + EqString::Format("#line %d \"", nLine) + _Es(pszFileName) + "\"\r\n";

			afterSkipLine = false;
		}

		newSrc = newSrc + str;
	}

	*buffer = (char*)PPReAlloc(*buffer, newSrc.Length()+1);
	strcpy(*buffer, newSrc.GetData());
}

struct shaderCompileJob_t
{
	eqParallelJob_t base;
	shaderProgramCompileInfo_t info;
	IShaderAPI* thisShaderAPI;
	IShaderProgram* program;
	EqString filePrefix;
	EqString extra;
};

// Loads and compiles shaders from files
bool ShaderAPI_Base::LoadShadersFromFile(IShaderProgram* pShaderOutput, const char* pszFilePrefix, const char *extra)
{
	if(pShaderOutput == NULL)
		return false;

	bool bResult = true;

	shaderCompileJob_t* job = new shaderCompileJob_t();
	job->filePrefix = pszFilePrefix;
	job->extra = extra;
	job->program = pShaderOutput;
	job->thisShaderAPI = this;
	job->base.arguments = job;
	job->base.typeId = JOB_TYPE_RENDERER;

	job->base.func = [](void* data, int i) {
		shaderCompileJob_t* jobData = (shaderCompileJob_t*)data;

		jobData->program->Ref_Grab();

		EqString fileNameFX = EqString::Format("%s.fx", jobData->filePrefix.ToCString());

		bool vsRequiried = true;	// vertex shader is always required
		bool psRequiried = false;
		bool gsRequiried = false;

		shaderProgramCompileInfo_t& info = jobData->info;

		// Load KeyValues
		KeyValues pKv;

		EqString shaderDescFilename = (EqString(SHADERS_DEFAULT_PATH) + jobData->filePrefix + ".txt");

		if( pKv.LoadFromFile(shaderDescFilename) )
		{
			kvkeybase_t* sec = pKv.GetRootSection();

			info.disableCache = KV_GetValueBool(sec->FindKeyBase("DisableCache"));

			kvkeybase_t* programName = sec->FindKeyBase("ShaderProgram");
			if (programName)
			{
				const char* programNameStr = KV_GetValueString(programName, 0, jobData->filePrefix);

				for (int i = 1; i < programName->values.numElem(); i++)
				{
					const char* usageValue = KV_GetValueString(programName, i);

					if (!stricmp(usageValue, "vertex"))
						vsRequiried = true;
					else if (!stricmp(usageValue, "pixel") || !stricmp(usageValue, "fragment"))
						psRequiried = true;
					else if (!stricmp(usageValue, "geometry"))
						gsRequiried = true;
				}

				fileNameFX = EqString::Format("%s.fx", programNameStr);
			}

			// API section
			// find corresponding API
			for(int i = 0; i < sec->keys.numElem(); i++)
			{
				kvkeybase_t* apiKey = sec->keys[i];

				if(!stricmp(apiKey->GetName(), "api"))
				{
					for(int j = 0; j < apiKey->values.numElem(); j++)
					{
						if(!stricmp(KV_GetValueString(apiKey, j), jobData->thisShaderAPI->GetRendererName()))
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

		EqString shaderRootPath = EqString::Format(SHADERS_DEFAULT_PATH "%s", jobData->thisShaderAPI->GetRendererName());

		LoadShaderFiles(&info.data.text, fileNameFX.ToCString(), shaderRootPath.ToCString(), info.data, true, !vsRequiried);

		if (!info.data.text && psRequiried)
			return;

		if(info.data.text)
			info.data.checksum = CRC32_BlockChecksum(info.data.text, strlen(info.data.text));

		// compile the shaders
		jobData->thisShaderAPI->CompileShadersFromStream(jobData->program, jobData->info, jobData->extra);

	};

	job->base.onComplete = [](eqParallelJob_t* job)
	{
		shaderCompileJob_t* jobData = (shaderCompileJob_t*)job;

		// compile the shaders
		//jobData->thisShaderAPI->CompileShadersFromStream(jobData->program, jobData->info, jobData->extra);

		jobData->program->Ref_Drop();

		PPFree(jobData->info.data.text);

		// required to delete job manually here
		// becuase eqDecalPolygonJob_t would have destructors
		delete jobData;
	};

	g_parallelJobs->AddJob((eqParallelJob_t*)job);
	g_parallelJobs->Submit();

	return bResult;
}


// search for existing shader program
IShaderProgram* ShaderAPI_Base::FindShaderProgram(const char* pszName, const char* query)
{
	CScopedMutex m(m_Mutex);

	EqString shaderName(pszName);
	if(query)
		shaderName.Append(query);

	const int nameHash = StringToHash(shaderName.ToCString());

	for (int i = 0; i < m_ShaderList.numElem(); i++)
	{
		if(m_ShaderList[i]->GetNameHash() == nameHash)
			return m_ShaderList[i];
	}

	return NULL;
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
