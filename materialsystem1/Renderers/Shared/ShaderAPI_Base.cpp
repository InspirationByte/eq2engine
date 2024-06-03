//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
#include "CTexture.h"

using namespace Threading;

CEqMutex g_sapi_TextureMutex;
CEqMutex g_sapi_ShaderMutex;
CEqMutex g_sapi_VBMutex;
CEqMutex g_sapi_IBMutex;
CEqMutex g_sapi_Mutex;

DECLARE_CMD(r_info, "Prints renderer info", 0)
{
	g_renderAPI->PrintAPIInfo();
}

ShaderAPI_Base::ShaderAPI_Base()
{
}

// Init + Shurdown
void ShaderAPI_Base::Init( const ShaderAPIParams &params )
{
	m_params = params;	
	ConVar* r_debugShowTexture = (ConVar*)g_consoleCommands->FindCvar("r_debugShowTexture");

	if(r_debugShowTexture)
		r_debugShowTexture->SetVariantsCallback(GetConsoleTextureList);
}

void ShaderAPI_Base::Shutdown()
{
	ConVar* r_debugShowTexture = (ConVar*)g_consoleCommands->FindCvar("r_debugShowTexture");

	if(r_debugShowTexture)
		r_debugShowTexture->SetVariantsCallback(nullptr);

	for(auto it = m_TextureList.begin(); !it.atEnd(); ++it)
	{
		it.value()->Ref_Drop();
	}
	m_TextureList.clear(true);

	for(int i = 0; i < m_VFList.numElem();i++)
	{
		DestroyVertexFormat(m_VFList[i]);
		i--;
	}
	m_VFList.clear(true);
}


void ShaderAPI_Base::SubmitCommandBuffer(const IGPUCommandBuffer* cmdBuffer) const
{
	IGPUCommandBufferPtr ptr(const_cast<IGPUCommandBuffer*>(cmdBuffer));
	SubmitCommandBuffers(ArrayCRef(&ptr, 1));
}

Future<bool> ShaderAPI_Base::SubmitCommandBufferAwaitable(const IGPUCommandBuffer* cmdBuffer) const
{
	IGPUCommandBufferPtr ptr(const_cast<IGPUCommandBuffer*>(cmdBuffer));
	return SubmitCommandBuffersAwaitable(ArrayCRef(&ptr, 1));
}

void ShaderAPI_Base::GetConsoleTextureList(const ConCommandBase* base, Array<EqString>& list, const char* query)
{
	const int LIST_LIMIT = 50;

	ShaderAPI_Base* baseApi = ((ShaderAPI_Base*)g_renderAPI);

	CScopedMutex m(g_sapi_TextureMutex);
	Map<int, ITexture*>& texList = ((ShaderAPI_Base*)g_renderAPI)->m_TextureList;

	for (auto it = texList.begin(); !it.atEnd(); ++it)
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

// Find texture
ITexturePtr ShaderAPI_Base::FindTexture(const char* pszName)
{
	EqString searchStr(pszName);
	fnmPathFixSeparators(searchStr);

	const int nameHash = StringToHash(searchStr, true);

	{
		CScopedMutex m(g_sapi_TextureMutex);
		auto it = m_TextureList.find(nameHash);
		if (!it.atEnd())
		{
			return CRefPtr(*it);
		}
	}

	return nullptr;
}

ITexture* ShaderAPI_Base::FindTexturePtr(int nameHash) const
{
	auto it = m_TextureList.find(nameHash);
	if (!it.atEnd())
		return *it;
	return nullptr;
}

// Searches for existing texture or creates new one. Use this for resource loading
ITexturePtr ShaderAPI_Base::FindOrCreateTexture(const char* pszName, bool& justCreated)
{
	EqString searchStr(pszName);
	fnmPathFixSeparators(searchStr);

	const int nameHash = StringToHash(searchStr, true);

	CScopedMutex m(g_sapi_TextureMutex);
	auto it = m_TextureList.find(nameHash);
	if (!it.atEnd())
		return CRefPtr(*it);

	if (*pszName == '$')
	{
		ASSERT_FAIL("Texture %s should be pre-created\n", pszName);
		return nullptr;
	}

	justCreated = true;
	return CreateTextureResource(pszName);
}

// Unload the texture and free the memory
void ShaderAPI_Base::FreeTexture(ITexture* pTexture)
{
	if (pTexture == nullptr)
		return;

	ASSERT_MSG(pTexture->Ref_Count() == 0, "Material %s refcount = %d", pTexture->GetName(), pTexture->Ref_Count());
	DevMsg(DEVMSG_RENDER, "Unloading texture %s\n", pTexture->GetName());
	{
		CScopedMutex scoped(g_sapi_TextureMutex);
		m_TextureList.remove(((CTexture*)pTexture)->GetNameHash());
	}
}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

ITexturePtr ShaderAPI_Base::CreateTexture(const ArrayCRef<CImagePtr>& images, const SamplerStateParams& sampler, int nFlags)
{
	if(!images.numElem())
		return nullptr;

	// create texture
	ITexturePtr texture = nullptr;
	{
		CScopedMutex m(g_sapi_TextureMutex);
		texture = CreateTextureResource(images[0]->GetName());
	}
	
	texture->Init(images, sampler, nFlags);

	// the created texture is automatically added to list
	return texture;
}

// creates procedural (lockable) texture
ITexturePtr ShaderAPI_Base::CreateProceduralTexture(const char* pszName,
													ETextureFormat format,
													int width, int height,
													int arraySize,
													const SamplerStateParams& sampler,
													int flags,
													int dataSize, const ubyte* data)
{
	CImagePtr genTex = CRefPtr_new(CImage);
	genTex->SetName(pszName);

	const int depth = (flags & TEXFLAG_CUBEMAP) ? 0 : 1;

	// make texture
	ubyte* newData = genTex->Create(format, width, height, depth, 1, arraySize);
	if(newData)
	{
		const int texDataSize = width * height * arraySize * GetBytesPerPixel(format);
		memset(newData, 0, texDataSize);

		if(data && dataSize)
		{
			ASSERT(dataSize <= texDataSize);
			memcpy(newData, data, dataSize);
		}
	}
	else
	{
		MsgError("ERROR -  Cannot create procedural texture '%s', probably bad format\n", pszName);
		return nullptr;	// don't generate error
	}

	FixedArray<CImagePtr, 1> imgs;
	imgs.append(genTex);

	return CreateTexture(imgs, sampler, flags);
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

IVertexFormat* ShaderAPI_Base::FindVertexFormatById(int nameHash) const
{
	for (int i = 0; i < m_VFList.numElem(); i++)
	{
		if (m_VFList[i]->GetNameHash() == nameHash)
			return m_VFList[i];
	}
	return nullptr;
}

int	ShaderAPI_Base::GetDrawCallsCount() const
{
	return m_statDrawCount;
}

int	ShaderAPI_Base::GetDrawIndexedPrimitiveCallsCount() const
{
	return m_statDrawIndexedCount;
}

int	ShaderAPI_Base::GetTrianglesCount() const
{
	return m_statPrimitiveCount;
}

void ShaderAPI_Base::ResetCounters()
{
	m_statDrawIndexedCount = 0;
	m_statDrawCount = 0;
	m_statPrimitiveCount = 0;
}
