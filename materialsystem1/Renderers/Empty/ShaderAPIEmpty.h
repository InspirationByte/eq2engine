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
		return m_vertexDesc[stream].stride;
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

class CEmptyBindGroup : public IGPUBindGroup {};
class CEmptyIPipelineLayout : public IGPUPipelineLayout {};
class CEmptyCommandBuffer : public IGPUCommandBuffer {};
class CEmptyRenderPipeline : public IGPURenderPipeline {};
class CEmptyComputePipeline : public IGPUComputePipeline {};

class CEmptyBuffer : public IGPUBuffer
{
public:
	CEmptyBuffer(int size, int usageFlags) : m_size(size), m_usageFlags(usageFlags) {}

	void		Update(const void* data, int64 size, int64 offset) {}
	LockFuture	Lock(int lockOfs, int sizeToLock, int flags) { return LockFuture::Failure(-1, "Unsupported"); }
	void		Unlock() {}
	int			GetSize() const { return m_size; }

	int			GetUsageFlags() const { return m_usageFlags; }

	int			m_size{ 0 };
	int			m_usageFlags{ 0 };
};

class CEmptyRenderPassRecorder : public IGPURenderPassRecorder
{
public:
	IVector2D					GetRenderTargetDimensions() const { return IVector2D(800, 600); }
	ArrayCRef<ETextureFormat>	GetRenderTargetFormats() const { return ArrayCRef(m_targets, MAX_RENDERTARGETS); }
	ETextureFormat			GetDepthTargetFormat() const {return m_depthFormat;}

	bool					IsDepthReadOnly() const { return true; }
	bool					IsStencilReadOnly() const  { return true; }

	void					AddBundle(IGPURenderBundleRecorder* bundle) {}

	void					SetPipeline(IGPURenderPipeline* pipeline)  { m_curPipeline.Assign(pipeline); }
	IGPURenderPipelinePtr	GetPipeline() const { return m_curPipeline; }
	void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets = nullptr) {}

	void					SetVertexBuffer(int slot, IGPUBuffer* vertexBuffer, int64 offset = 0, int64 size = -1)  {}
	void					SetIndexBuffer(IGPUBuffer* indexBuffer, EIndexFormat indexFormat, int64 offset = 0, int64 size = -1) {}

	void					SetViewport(const AARectangle& rectangle, float minDepth, float maxDepth) {}
	void					SetScissorRectangle(const IAARectangle& rectangle)  {}

	void					Draw(int vertexCount, int firstVertex, int instanceCount, int firstInstance = 0) {}
	void					DrawIndexed(int indexCount, int firstIndex, int instanceCount, int baseVertex = 0, int firstInstance = 0) {}
	void					DrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset)  {}
	void					DrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset) {}

	void*					GetUserData() const { return m_userData; }
	void					Complete() {}
	
	IGPUCommandBufferPtr	End() { return IGPUCommandBufferPtr(CRefPtr_new(CEmptyCommandBuffer)); }

	ETextureFormat			m_targets[MAX_RENDERTARGETS] { FORMAT_RGBA8, FORMAT_NONE };
	ETextureFormat			m_depthFormat{ FORMAT_D24 };

	IGPURenderPipelinePtr	m_curPipeline;
	void*					m_userData{ nullptr };
};

class CEmptyComputePassRecorder : public IGPUComputePassRecorder
{
public:
	void					SetPipeline(IGPUComputePipeline* pipeline) { m_curPipeline.Assign(pipeline); }
	IGPUComputePipelinePtr	GetPipeline() const { return m_curPipeline; }

	void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets = nullptr) {}

	void					DispatchWorkgroups(int32 workgroupCountX, int32 workgroupCountY = 1, int32 workgroupCountZ = 1) {}
	void					DispatchWorkgroupsIndirect(IGPUBuffer* indirectBuffer, int64 indirectOffset){}

	void*					GetUserData() const { return m_userData; }
	void					Complete() {}
	IGPUCommandBufferPtr	End()  { return IGPUCommandBufferPtr(CRefPtr_new(CEmptyCommandBuffer)); }

	IGPUComputePipelinePtr	m_curPipeline;
	void*					m_userData{ nullptr };
};

class CEmptyCommandRecorder : public IGPUCommandRecorder
{
public:
	IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const
	{
		return IGPURenderPassRecorderPtr(CRefPtr_new(CEmptyRenderPassRecorder));
	}

	IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData = nullptr) const
	{
		return IGPUComputePassRecorderPtr(CRefPtr_new(CEmptyComputePassRecorder));
	}

	void						WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const {}
	void						CopyBufferToBuffer(IGPUBuffer* source, int64 sourceOffset, IGPUBuffer* destination, int64 destinationOffset, int64 size) const {}
	void						ClearBuffer(IGPUBuffer* buffer, int64 offset, int64 size) const {}

	void						CopyTextureToTexture(const TextureCopyInfo& source, const TextureCopyInfo& destination, const TextureExtent& copySize) const  {}
	void						CopyTextureToBuffer(const TextureCopyInfo& source, const IGPUBuffer* destination, const TextureExtent& copySize) const {}

	void*						GetUserData() const { return m_userData; }
	IGPUCommandBufferPtr		End() { return IGPUCommandBufferPtr(CRefPtr_new(CEmptyCommandBuffer)); }

	void*						m_userData{ nullptr };
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

		pTexture->SetFlags(targetDesc.flags);
		pTexture->SetDimensions(targetDesc.size.width, targetDesc.size.height, targetDesc.size.arraySize);
		pTexture->SetMipCount(targetDesc.mipmapCount);
		pTexture->SetSampleCount(targetDesc.sampleCount);
		pTexture->SetFormat(targetDesc.format);

		return ITexturePtr(pTexture);
	}

	void						ResizeRenderTarget(ITexture* renderTarget, const TextureExtent& newSize, int mipmapCount = 1, int sampleCount = 1)
	{
		if(!renderTarget)
			return;

		CEmptyTexture* texture = static_cast<CEmptyTexture*>(renderTarget);
		texture->SetDimensions(newSize.width, newSize.height, newSize.arraySize);
		texture->SetMipCount(mipmapCount);
		texture->SetSampleCount(sampleCount);
	}

//-------------------------------------------------------------
// Pipeline management
	void						LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines) const {}

	IGPUPipelineLayoutPtr		CreatePipelineLayout(const PipelineLayoutDesc& layoutDesc) const { return IGPUPipelineLayoutPtr(CRefPtr_new(CEmptyIPipelineLayout)); }
	IGPUBindGroupPtr			CreateBindGroup(const IGPUPipelineLayout* pipelineLayout, const BindGroupDesc& bindGroupDesc) const { return IGPUBindGroupPtr(CRefPtr_new(CEmptyBindGroup)); }
	IGPUBindGroupPtr			CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const { return IGPUBindGroupPtr(CRefPtr_new(CEmptyBindGroup)); }
	IGPUBindGroupPtr			CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const { return IGPUBindGroupPtr(CRefPtr_new(CEmptyBindGroup)); }
	IGPURenderPipelinePtr		CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout = nullptr) const { return IGPURenderPipelinePtr(CRefPtr_new(CEmptyRenderPipeline)); }

	IGPUComputePipelinePtr		CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) const { return IGPUComputePipelinePtr(CRefPtr_new(CEmptyComputePipeline)); }

//-------------------------------------------------------------
// Buffer management

	IGPUBufferPtr				CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name = nullptr) const
	{
		CRefPtr<CEmptyBuffer> buffer = CRefPtr_new(CEmptyBuffer, bufferInfo.elementSize * bufferInfo.elementCapacity, bufferUsageFlags);
		return IGPUBufferPtr(buffer); 
	}

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

	IGPUCommandRecorderPtr		CreateCommandRecorder(const char* name = nullptr, void* userData = nullptr) const { return IGPUCommandRecorderPtr(CRefPtr_new(CEmptyCommandRecorder)); }
	IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const { return IGPURenderPassRecorderPtr(CRefPtr_new(CEmptyRenderPassRecorder)); }
	IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData = nullptr) const { return IGPUComputePassRecorderPtr(CRefPtr_new(CEmptyComputePassRecorder)); }

//-------------------------------------------------------------
// Command buffer management

	void						SubmitCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const {}
	Future<bool>				SubmitCommandBuffersAwaitable(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const { return Future<bool>::Succeed(true); }

protected:

	ITexturePtr		CreateTextureResource(const char* pszName)
	{
		CRefPtr<CEmptyTexture> texture = CRefPtr_new(CEmptyTexture);
		texture->SetName(pszName);

		m_TextureList.insert(texture->m_nameHash, texture);
		return ITexturePtr(texture);
	}
};
