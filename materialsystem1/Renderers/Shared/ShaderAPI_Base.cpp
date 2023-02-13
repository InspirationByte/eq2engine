//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				For high level look for the material system (Engine)
//				Contains FFP functions and Shader (FFP overriding programs)
//
//				The renderer may do anti-wallhacking functions
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/IFileSystem.h"
#include "core/IEqParallelJobs.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"
#include "utils/Tokenizer.h"
#include "utils/KeyValues.h"
#include "ShaderAPI_Base.h"

#ifdef EQRHI_GL
#define STB_INCLUDE_LINE_GLSL
#endif

#define STB_INCLUDE_IMPLEMENTATION
#include "dependency/stb_include.h"

#include "CTexture.h"

using namespace Threading;

CEqMutex g_sapi_TextureMutex;
CEqMutex g_sapi_ShaderMutex;
CEqMutex g_sapi_VBMutex;
CEqMutex g_sapi_IBMutex;
CEqMutex g_sapi_Mutex;

DECLARE_CMD(r_info, "Prints renderer info", 0)
{
	g_pShaderAPI->PrintAPIInfo();
}

ShaderAPI_Base::ShaderAPI_Base()
{
	m_nViewportWidth			= 800;
	m_nViewportHeight			= 600;

	m_pCurrentShader			= nullptr;
	m_pSelectedShader			= nullptr;

	m_pCurrentBlendstate		= nullptr;
	m_pCurrentDepthState		= nullptr;
	m_pCurrentRasterizerState	= nullptr;

	m_pSelectedBlendstate		= nullptr;
	m_pSelectedDepthState		= nullptr;
	m_pSelectedRasterizerState	= nullptr;

	m_pErrorTexture				= nullptr;

	// VF selectoin
	m_pSelectedVertexFormat		= nullptr;
	m_pCurrentVertexFormat		= nullptr;

	// Index buffer
	m_pSelectedIndexBuffer		= nullptr;
	m_pCurrentIndexBuffer		= nullptr;

	// Vertex buffer
	memset(m_pSelectedVertexBuffers, 0, sizeof(m_pSelectedVertexBuffers));
	memset(m_pCurrentVertexBuffers, 0, sizeof(m_pCurrentVertexBuffers));

	memset(m_pActiveVertexFormat, 0, sizeof(m_pActiveVertexFormat));

	memset(m_nCurrentOffsets, 0, sizeof(m_nCurrentOffsets));
	memset(m_nSelectedOffsets, 0, sizeof(m_nSelectedOffsets));

	// Index buffer
	m_pSelectedIndexBuffer		= nullptr;
	m_pCurrentIndexBuffer		= nullptr;

	m_pCurrentDepthRenderTarget = nullptr;

	m_nDrawCalls				= 0;
	m_nTrianglesCount			= 0;
}

// Init + Shurdown
void ShaderAPI_Base::Init( const shaderAPIParams_t &params )
{
	m_params = params;

	// Do master reset for all things
	Reset();
	
	DevMsg(DEVMSG_SHADERAPI, "[DEBUG] Generate error texture...\n");

	m_pErrorTexture = CreateTextureResource("error");
	m_pErrorTexture->GenerateErrorTexture();

	ConVar* r_debug_showTexture = (ConVar*)g_consoleCommands->FindCvar("r_debug_showTexture");

	if(r_debug_showTexture)
		r_debug_showTexture->SetVariantsCallback(GetConsoleTextureList);
}

void ShaderAPI_Base::Shutdown()
{
	ConVar* r_debug_showTexture = (ConVar*)g_consoleCommands->FindCvar("r_debug_showTexture");

	if(r_debug_showTexture)
		r_debug_showTexture->SetVariantsCallback(nullptr);

	ChangeRenderTargetToBackBuffer();

	Reset();
	Apply();

	m_pErrorTexture = nullptr;

	for(auto it = m_TextureList.begin(); it != m_TextureList.end(); ++it)
	{
		FreeTexture(*it);
	}
	m_TextureList.clear();

	for (auto it = m_ShaderList.begin(); it != m_ShaderList.end(); ++it)
	{
		DestroyShaderProgram(*it);
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
#ifndef _RETAIL
	m_nDrawCalls					= 0;
	m_nTrianglesCount				= 0;
	m_nDrawIndexedPrimitiveCalls	= 0;
#endif
}

void ShaderAPI_Base::Reset(int nResetTypeFlags)
{
	if (nResetTypeFlags & STATE_RESET_SHADER)
		m_pSelectedShader = nullptr;

	if (nResetTypeFlags & STATE_RESET_BS)
		m_pSelectedBlendstate = nullptr;

	if (nResetTypeFlags & STATE_RESET_DS)
		m_pSelectedDepthState = nullptr;

	if (nResetTypeFlags & STATE_RESET_RS)
		m_pSelectedRasterizerState = nullptr;

	if (nResetTypeFlags & STATE_RESET_VF)
		m_pSelectedVertexFormat = nullptr;

	if (nResetTypeFlags & STATE_RESET_IB)
		m_pSelectedIndexBuffer = nullptr;

	if (nResetTypeFlags & STATE_RESET_VB)
	{
		// i think is faster
		memset(m_pSelectedVertexBuffers,0,sizeof(m_pSelectedVertexBuffers));
		memset(m_nSelectedOffsets,0,sizeof(m_nSelectedOffsets));
	}

	// i think is faster
	if (nResetTypeFlags & STATE_RESET_TEX)
	{
		for (int i = 0; i < MAX_TEXTUREUNIT; ++i)
			m_pSelectedTextures[i] = nullptr;

		for (int i = 0; i < MAX_VERTEXTEXTURES; ++i)
			m_pSelectedVertexTextures[i] = nullptr;
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
const ITexturePtr& ShaderAPI_Base::GetErrorTexture() const
{
	return m_pErrorTexture;
}

void ShaderAPI_Base::GetConsoleTextureList(const ConCommandBase* base, Array<EqString>& list, const char* query)
{
	const int LIST_LIMIT = 50;

	ShaderAPI_Base* baseApi = ((ShaderAPI_Base*)g_pShaderAPI);

	CScopedMutex m(g_sapi_TextureMutex);
	Map<int, ITexture*>& texList = ((ShaderAPI_Base*)g_pShaderAPI)->m_TextureList;

	for (auto it = texList.begin(); it != texList.end(); ++it)
	{
		ITexture* texture = *it;

		if(list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		if(*query == 0 || xstristr(texture->GetName(), query))
			list.append(texture->GetName());
	}
}

// texture uploading frequency
void ShaderAPI_Base::SetProgressiveTextureFrequency(int frames)
{
	m_progressiveTextureFrequency = frames;
}

int	ShaderAPI_Base::GetProgressiveTextureFrequency() const
{
	return m_progressiveTextureFrequency;
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
ITexturePtr ShaderAPI_Base::FindTexture(const char* pszName)
{
	EqString searchStr(pszName);
	searchStr.Path_FixSlashes();

	const int nameHash = StringToHash(searchStr.ToCString(), true);

	{
		CScopedMutex m(g_sapi_TextureMutex);
		auto it = m_TextureList.find(nameHash);
		if (it != m_TextureList.end())
		{
			return CRefPtr(*it);
		}
	}

	return nullptr;
}

// Searches for existing texture or creates new one. Use this for resource loading
ITexturePtr ShaderAPI_Base::FindOrCreateTexture(const char* pszName, bool& justCreated)
{
	EqString searchStr(pszName);
	searchStr.Path_FixSlashes();

	const int nameHash = StringToHash(searchStr.ToCString(), true);

	CScopedMutex m(g_sapi_TextureMutex);
	auto it = m_TextureList.find(nameHash);
	if (it != m_TextureList.end())
		return CRefPtr(*it);

	if (*pszName == '$')
		return nullptr;

	justCreated = true;
	return CreateTextureResource(pszName);
}

// Unload the texture and free the memory
void ShaderAPI_Base::FreeTexture(ITexture* pTexture)
{
	if (pTexture == nullptr)
		return;

	ASSERT_MSG(pTexture->Ref_Count() == 0, "Material %s refcount = %d", pTexture->GetName(), pTexture->Ref_Count());
	DevMsg(DEVMSG_SHADERAPI, "Unloading texture %s\n", pTexture->GetName());
	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		m_TextureList.remove(((CTexture*)pTexture)->GetNameHash());
	}
}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

ITexturePtr ShaderAPI_Base::CreateTexture(const ArrayCRef<CRefPtr<CImage>>& pImages, const SamplerStateParam_t& sampler, int nFlags)
{
	if(!pImages.numElem())
		return nullptr;

	// create texture
	ITexturePtr texture = nullptr;
	{
		CScopedMutex m(g_sapi_TextureMutex);
		texture = CreateTextureResource(pImages[0]->GetName());
	}

	for (int i = 0; i < pImages.numElem(); ++i)
	{
		if (GetShaderAPIClass() == SHADERAPI_DIRECT3D9)
		{
			if (pImages[i]->GetFormat() == FORMAT_RGB8 || pImages[i]->GetFormat() == FORMAT_RGBA8)
				pImages[i]->SwapChannels(0, 2); // convert to BGR

			// Convert if needed and upload datas
			if (pImages[i]->GetFormat() == FORMAT_RGB8) // as the D3DFMT_X8R8G8B8 used
				pImages[i]->Convert(FORMAT_RGBA8);
		}
	}
	
	texture->Init(sampler, pImages, nFlags);

	// the created texture is automatically added to list
	return texture;
}

// creates procedural (lockable) texture
ITexturePtr ShaderAPI_Base::CreateProceduralTexture(const char* pszName,
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
	genTex.Ref_Grab();	// by grabbing ref we make sure it won't be deleted

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
		return nullptr;	// don't generate error
	}

	FixedArray<CRefPtr<CImage>, 1> imgs;
	imgs.append(CRefPtr(&genTex));

	SamplerStateParam_t sampler;
	SamplerStateParams_Make(sampler, m_caps, texFilter, textureAddress, textureAddress, textureAddress);
	return g_pShaderAPI->CreateTexture(imgs, sampler, nFlags);
}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

// Changes render target (single RT)
void ShaderAPI_Base::ChangeRenderTarget(const ITexturePtr& renderTarget, int rtSlice, const ITexturePtr& depthTarget, int depthSlice)
{
	ChangeRenderTargets(ArrayCRef(&renderTarget, renderTarget ? 1 : 0), ArrayCRef(&rtSlice, 1), depthTarget, depthSlice);
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
		m_pSelectedBlendstate = nullptr;
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
		m_pSelectedDepthState = nullptr;
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
		m_pSelectedRasterizerState = nullptr;
	}
}

// Set Texture for Fixed-Function Pipeline
void ShaderAPI_Base::SetTextureAtIndex(const ITexturePtr& texture, int level)
{
	if(level > 0 && m_caps.maxTextureUnits <= 1)
		return;

	// exclusive for D3D api
	if(level & 0x8000)
	{
		level &= ~0x8000;
		if (level > m_caps.maxVertexTextureUnits)
			return;
		m_pSelectedVertexTextures[level] = texture;
		return;
	}

	if(level < m_caps.maxTextureUnits)
		m_pSelectedTextures[level] = texture;
}

// returns the currently set textre at level
const ITexturePtr& ShaderAPI_Base::GetTextureAt( int level ) const
{
	if (level > 0 && m_caps.maxTextureUnits <= 1)
		return ITexturePtr::Null();

	// exclusive for D3D api
	if (level & 0x8000)
	{
		level &= ~0x8000;
		if (level > m_caps.maxVertexTextureUnits)
			return ITexturePtr::Null();
		return m_pSelectedVertexTextures[level];
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
	m_pSelectedVertexBuffers[nStream] = nullptr;
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
	if(pShaderOutput == nullptr)
		return false;

	PROF_EVENT("ShaderAPI Load-Build Shaders");

	shaderProgramCompileInfo_t info;
	EqString fileNameFX = EqString::Format("%s.fx", pszFilePrefix);

	bool vsRequiried = true;	// vertex shader is always required
	bool psRequiried = false;
	bool gsRequiried = false;

	// Load KeyValues
	KeyValues pKv;

	EqString shaderDescFilename = (EqString(SHADERS_DEFAULT_PATH) + pszFilePrefix + ".txt");

	if( pKv.LoadFromFile(shaderDescFilename) )
	{
		KVSection* sec = pKv.GetRootSection();

		info.disableCache = KV_GetValueBool(sec->FindSection("DisableCache"));

		KVSection* programName = sec->FindSection("ShaderProgram");
		if (programName)
		{
			const char* programNameStr = KV_GetValueString(programName, 0, pszFilePrefix);

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
			KVSection* apiKey = sec->keys[i];

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

	EqString shaderRootPath = EqString::Format(SHADERS_DEFAULT_PATH "%s", "Uber"); //EqString::Format(SHADERS_DEFAULT_PATH "%s", GetRendererName());


	EqString firstFileName;
	CombinePath(firstFileName, 2, shaderRootPath.ToCString(), fileNameFX.ToCString());

	auto loadShaderFile = [](char* filename, size_t *plen, void* userData) -> char*
	{
		shaderProgramCompileInfo_t& compileInfo = *static_cast<shaderProgramCompileInfo_t*>(userData);
		compileInfo.data.includes.append(filename);

		IFile* file = g_fileSystem->Open(filename, "rb");

		if (!file)
			return nullptr;

		const long length = file->GetSize();
		char* buffer = (char*)malloc(length + 1);

		file->Read(buffer, 1, length);
		buffer[length] = 0;

		g_fileSystem->Close(file);

		if (plen)
			*plen = length;
		return buffer;
	};

	char errorStr[256];
	info.data.text = stb_include_file(const_cast<char*>(firstFileName.ToCString()), nullptr, const_cast<char*>(shaderRootPath.ToCString()), loadShaderFile, &info, errorStr);

	if (!info.data.text && vsRequiried)
	{
		MsgError("LoadShadersFromFile %s\n", errorStr);
		return false;
	}

	if(info.data.text)
		info.data.checksum = CRC32_BlockChecksum(info.data.text, strlen(info.data.text));

	EqString boilerplateFile;
	CombinePath(boilerplateFile, 2, SHADERS_DEFAULT_PATH, EqString::Format("BoilerPlate_%s.h", GetRendererName()).ToCString());
	info.data.boilerplate = g_fileSystem->GetFileBuffer(boilerplateFile);

	if (!info.data.boilerplate)
	{
		MsgError("Cannot open '%s', expect shader compilation errors\n", boilerplateFile.ToCString());
	}

	// compile the shaders
	const bool result = CompileShadersFromStream(pShaderOutput, info, extra);

	free(info.data.text);
	PPFree(info.data.boilerplate);

	return result;
}


// search for existing shader program
IShaderProgram* ShaderAPI_Base::FindShaderProgram(const char* pszName, const char* query)
{
	EqString shaderName(pszName);
	if (query)
		shaderName.Append(query);

	const int nameHash = StringToHash(shaderName.ToCString());

	{
		CScopedMutex m(g_sapi_ShaderMutex);
		auto it = m_ShaderList.find(nameHash);
		if (it != m_ShaderList.end())
		{
			return *it;
		}
	}

	return nullptr;
}

// Shader constants setup
void ShaderAPI_Base::SetShaderConstantInt(int nameHash, const int constant)
{
	SetShaderConstantRaw(nameHash, &constant, sizeof(constant));
}

void ShaderAPI_Base::SetShaderConstantFloat(int nameHash, const float constant)
{
	SetShaderConstantRaw(nameHash, &constant, sizeof(constant));
}

void ShaderAPI_Base::SetShaderConstantVector2D(int nameHash, const Vector2D &constant)
{
	SetShaderConstantRaw(nameHash, &constant, sizeof(constant));
}

void ShaderAPI_Base::SetShaderConstantVector3D(int nameHash, const Vector3D &constant)
{
	SetShaderConstantRaw(nameHash, &constant, sizeof(constant));
}

void ShaderAPI_Base::SetShaderConstantVector4D(int nameHash, const Vector4D &constant)
{
	SetShaderConstantRaw(nameHash, &constant, sizeof(constant));
}

void ShaderAPI_Base::SetShaderConstantMatrix4(int nameHash, const Matrix4x4 &constant)
{
	SetShaderConstantRaw(nameHash, &constant, sizeof(constant));
}

void ShaderAPI_Base::SetShaderConstantArrayFloat(int nameHash, const float *constant, int count)
{
	SetShaderConstantRaw(nameHash, constant, count * sizeof(float));
}

void ShaderAPI_Base::SetShaderConstantArrayVector2D(int nameHash, const Vector2D *constant, int count)
{
	SetShaderConstantRaw(nameHash, constant, count * sizeof(Vector2D));
}

void ShaderAPI_Base::SetShaderConstantArrayVector3D(int nameHash, const Vector3D *constant, int count)
{
	SetShaderConstantRaw(nameHash, constant, count * sizeof(Vector3D));
}

void ShaderAPI_Base::SetShaderConstantArrayVector4D(int nameHash, const Vector4D *constant, int count)
{
	SetShaderConstantRaw(nameHash, constant, count * sizeof(Vector4D));
}

void ShaderAPI_Base::SetShaderConstantArrayMatrix4(int nameHash, const Matrix4x4 *constant, int count)
{
	SetShaderConstantRaw(nameHash, constant, count * sizeof(Matrix4x4));
}
