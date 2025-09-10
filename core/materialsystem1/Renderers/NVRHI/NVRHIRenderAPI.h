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
#include "NVRHIVertexFormat.h"
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
	void						Shutdown();
	bool						IsDeviceValidationActive() const { return m_isValidationActive; }

//-------------------------------------------------------------
// Renderer information
	void						PrintAPIInfo() const;
	bool						IsDeviceActive() const;

	// shader API class type for shader developers.
	EShaderAPIType				GetShaderAPIClass()		{ return SHADERAPI_D3D12; }

	// Renderer string (ex: OpenGL, D3D9)
	const char*					GetRendererName() const { return "D3D12"; }

//-------------------------------------------------------------
// MT Synchronization

	// Synchronization
	void						Flush();

//-------------------------------------------------------------
// Shaders
	int							LoadShaderPackage(const char* filename);
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

	IGPUPipelineLayoutPtr		CreatePipelineLayout(const PipelineLayoutDesc& layoutDesc) const;
	IGPURenderPipelinePtr		CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout = nullptr) const;
	IGPUComputePipelinePtr		CreateComputePipeline(const ComputePipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout = nullptr) const;

	IGPUBindGroupPtr			CreateBindGroup(const IGPUPipelineLayout* pipelineLayout, const BindGroupDesc& bindGroupDesc) const;
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

// DEPRECATED
	IVertexFormatPtr			CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc);
	void						DestroyVertexFormat(IVertexFormat* pFormat);

//-------------------------------------------------------------
// Private access

	nvrhi::DeviceHandle			GetNVRHIDevice() const { return m_rhiDevice; }

protected:
	nvrhi::BindingLayoutHandle	CreateBindingLayout(const BindGroupLayoutDesc& bindGroupDesc, int bindGroupIndex) const;
	IGPUBindGroupPtr			CreateBindGroupImpl(const NVRHIBindingLayoutList& rhiBindingLayouts, const BindGroupDesc& bindGroupDesc) const;

	const ShaderInfo::Module&	GetOrLoadShaderModule(const ShaderInfo& shaderInfo, int shaderModuleIdx) const;

	Map<int, ShaderInfo>		m_shaderCache{ PP_SL };
	nvrhi::DeviceHandle			m_rhiDevice{ nullptr };
	ENVRHIBackendType			m_backendType;
	bool						m_deviceLost{ false };
	bool						m_isValidationActive{ false };
};
