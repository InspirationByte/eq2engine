//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>

#include "ShaderAPI.h"
#include "NVRHITexture.h"
#include "NVRHIBuffer.h"
#include "NVRHIStates.h"
#include "ShaderInfo.h"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;

enum ENVRHIBackendType : int;

class CNVRHIRenderAPI : public ShaderAPI_Base
{
	friend class CNVRHIRenderLibDXGIBase;
	friend class CNVRHIRenderLibD3D11;
	friend class CNVRHIRenderLibD3D12;
	friend class CNVRHIRenderLibVK;

public:
	static CNVRHIRenderAPI Instance;

	// Init + Shurdown
	void						Init(const ShaderAPIParams& params);
	void						Shutdown();
	bool						IsDeviceValidationActive() const;

//-------------------------------------------------------------
// Renderer information
	void						PrintAPIInfo() const;
	bool						IsDeviceActive() const;

	EShaderAPIType				GetShaderAPIClass() const { return SHADERAPI_NVRHI; }
	const char*					GetRendererName() const;
	ENVRHIBackendType			GetBackendType() const { return m_rhiBackendType; }

//-------------------------------------------------------------
// MT Synchronization

	// Synchronization
	void						Flush();

//-------------------------------------------------------------
// Shaders
	int							LoadShaderPackage(const char* filename);
	void						ReloadShaderPackage(int id);
	void						FreeShaderPackage(int id);
	void						ClearShaderPackages();

//-------------------------------------------------------------
// Textures

	ITexturePtr					CreateTextureResource(const char* pszName);
	ITexturePtr					CreateRenderTarget(const TextureDesc& targetDesc);
	void						ResizeRenderTarget(ITexture* renderTarget, const TextureExtent& newSize, int mipmapCount = 1, int sampleCount = 1);

//-------------------------------------------------------------
// Pipeline management

	void						LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines, const char* entryPointName) const;

	IGPUBindingLayoutPtr		CreateBindingLayout(const BindingLayoutDesc& layoutDesc) const;
	IGPURenderPipelinePtr		CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUBindingLayout* pipelineLayout = nullptr) const;
	IGPUComputePipelinePtr		CreateComputePipeline(const ComputePipelineDesc& pipelineDesc, const IGPUBindingLayout* pipelineLayout = nullptr) const;

	IGPUBindGroupPtr			CreateSharedBindGroup(const IGPUBindingLayout* pipelineLayout, const BindGroupDesc& bindGroupDesc) const;
	IGPUBindGroupPtr			CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const;
	IGPUBindGroupPtr			CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const;


//-------------------------------------------------------------
// Buffer management

	IGPUBufferPtr				CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name = nullptr) const;

//-------------------------------------------------------------
// Command management

	IGPUCommandRecorderPtr		CreateCommandRecorder(const char* name = nullptr, void* userData = nullptr) const;
	IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const;
	IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData = nullptr) const;

//-------------------------------------------------------------
// Command buffer management

	void						SubmitCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const;
	Future<bool>				SubmitCommandBuffersAwaitable(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const;

//-------------------------------------------------------------
// Private access

	nvrhi::DeviceHandle			GetNVRHIDevice() const { return m_rhiDevice; }
	nvrhi::SamplerHandle		GetRHISampler(const SamplerStateParams& samplerStateParams);

	int							AcquireRHITransientTextureHeap();
	void						ReleaseRHITransientTextureHeap(int heapIdx);

	int							AcquireRHITransientBufferHeap();
	void						ReleaseRHITransientBufferHeap(int heapIdx);

	nvrhi::HeapHandle			GetRHITextureHeap(int heapIdx) const { return m_rhiTransientTextureHeaps[heapIdx]; }
	nvrhi::HeapHandle			GetRHIBufferHeap(int heapIdx) const { return m_rhiTransientBufferHeaps[heapIdx]; }

	nvrhi::CommandListHandle	AcquireRHICommandList(int& cmdListIdx) const;
	void						ReleaseCommandList(int cmdListIdx);

protected:

	//nvrhi::BindingLayoutHandle	CreateBindingLayout(const BindGroupLayoutDesc& bindGroupDesc, int bindGroupIndex) const;
	IGPUBindGroupPtr			CreateBindGroupImpl(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, NVRHIBindingLayoutsCRef rhiBindingLayouts) const;

	const ShaderInfo::Module&	GetOrLoadShaderModule(const ShaderInfo& shaderInfo, int shaderModuleIdx, const char* dbgName = nullptr) const;

	Array<nvrhi::HeapHandle>	m_rhiTransientTextureHeaps{ PP_SL };
	Array<int>					m_rhiFreeTransientTextureHeaps{ PP_SL };

	Array<nvrhi::HeapHandle>	m_rhiTransientBufferHeaps{ PP_SL };
	Array<int>					m_rhiFreeTransientBufferHeaps{ PP_SL };

	mutable Array<nvrhi::CommandListHandle>	m_rhiCommandLists{ PP_SL };
	mutable Array<int>						m_rhiFreeCommandLists{ PP_SL };

	Map<int, nvrhi::SamplerHandle>	m_rhiSamplers{ PP_SL };

	Map<int, ShaderInfo>		m_shaderCache{ PP_SL };
	nvrhi::DeviceHandle			m_rhiDevice{ nullptr };
	ENVRHIBackendType			m_rhiBackendType;

	bool						m_deviceLost{ false };
};
