//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Empty ShaderAPI for some applications using matsystem
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ShaderAPI_Base.h"
#include "EmptyTexture.h"
#include "imaging/ImageLoader.h"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;
extern CEqMutex	g_sapi_ShaderMutex;
extern CEqMutex	g_sapi_VBMutex;
extern CEqMutex	g_sapi_IBMutex;
extern CEqMutex	g_sapi_Mutex;

class CEmptyVertexFormat : public IVertexFormat
{
public:
	CEmptyVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> desc)
	{
		m_name = name;
		m_nameHash = StringToHash(name);

		m_vertexDesc.setNum(desc.numElem());
		for(int i = 0; i < desc.numElem(); i++)
			m_vertexDesc[i] = desc[i];
	}

	const char* GetName() const {return m_name.ToCString();}
	int			GetNameHash() const { return m_nameHash; }

	int	GetVertexSize(int stream) const
	{
		return 0;
	}

	ArrayCRef<VertexLayoutDesc> GetFormatDesc() const
	{
		return m_vertexDesc;
	}

protected:
	EqString				m_name;
	int						m_nameHash;
	Array<VertexLayoutDesc>	m_vertexDesc{ PP_SL };
};

class ShaderAPIEmpty : public ShaderAPI_Base
{
	friend class CEmptyRenderLib;
	friend class CEmptyTexture;
public:

	ShaderAPIEmpty() {}
	~ShaderAPIEmpty() {}

	// Init + Shurdown
	void			Init(const ShaderAPIParams &params) 
	{
		memset(&m_caps, 0, sizeof(m_caps));
		ShaderAPI_Base::Init(params);
	}
	//void			Shutdown() {}

	void			PrintAPIInfo() const {}
	bool			IsDeviceActive() const {return true;}
	EShaderAPIType	GetShaderAPIClass() {return SHADERAPI_EMPTY;}
	const char*		GetRendererName() const {return "Empty";}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// Synchronization
	void			Flush() {}
	void			Finish() {}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// It will add new rendertarget
	ITexturePtr		CreateRenderTarget(const TextureDesc& targetDesc)
	{
		CRefPtr<CEmptyTexture> pTexture = CRefPtr_new(CEmptyTexture);
		pTexture->SetName(targetDesc.name);

		CScopedMutex scoped(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(pTexture);
		m_TextureList.insert(pTexture->m_nameHash, pTexture);

		pTexture->SetDimensions(targetDesc.size.width, targetDesc.size.height, targetDesc.size.arraySize);
		pTexture->SetFormat(targetDesc.format);

		return ITexturePtr(pTexture);
	}

	void						ResizeRenderTarget(ITexture* renderTarget, const TextureExtent& newSize, int mipmapCount = 1, int sampleCount = 1) {}

//-------------------------------------------------------------
// Pipeline management
	void						LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines) const {}

	IGPUPipelineLayoutPtr		CreatePipelineLayout(const PipelineLayoutDesc& layoutDesc) const { return nullptr; }
	IGPUBindGroupPtr			CreateBindGroup(const IGPUPipelineLayout* pipelineLayout, const BindGroupDesc& bindGroupDesc) const { return nullptr; }
	IGPUBindGroupPtr			CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const { return nullptr; }
	IGPUBindGroupPtr			CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const { return nullptr; }
	IGPURenderPipelinePtr		CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout = nullptr) const { return nullptr; }

	IGPUComputePipelinePtr		CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) const { return nullptr; }

//-------------------------------------------------------------
// Buffer management

	IGPUBufferPtr				CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name = nullptr) const { return nullptr; }

	IVertexFormat*				CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc)
	{
		IVertexFormat* pVF = PPNew CEmptyVertexFormat(name, formatDesc);
		m_VFList.append(pVF);
		return pVF;
	}

	void						DestroyVertexFormat(IVertexFormat* pFormat)
	{
		if (m_VFList.fastRemove(pFormat))
			delete pFormat;
	}

//-------------------------------------------------------------
// Render pass management

	IGPUCommandRecorderPtr		CreateCommandRecorder(const char* name = nullptr, void* userData = nullptr) const { return nullptr; }
	IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const { return nullptr; }
	IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData = nullptr) const { return nullptr; }

//-------------------------------------------------------------
// Command buffer management

	void						SubmitCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const {}

protected:

	ITexturePtr		CreateTextureResource(const char* pszName)
	{
		CRefPtr<CEmptyTexture> texture = CRefPtr_new(CEmptyTexture);
		texture->SetName(pszName);

		m_TextureList.insert(texture->m_nameHash, texture);
		return ITexturePtr(texture);
	}
};
