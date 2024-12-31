//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>

#include "ShaderAPI_Base.h"
#include "NVRHITexture.h"
#include "NVRHIBuffer.h"
#include "NVRHIVertexFormat.h"
#include "NVRHIShader.h"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;
extern CEqMutex	g_sapi_ShaderMutex;
extern CEqMutex	g_sapi_VBMutex;
extern CEqMutex	g_sapi_IBMutex;
extern CEqMutex	g_sapi_Mutex;

class CNVRHIRenderAPI : public ShaderAPI_Base
{
	friend class CWGPURenderLib;
public:
	static CNVRHIRenderAPI Instance;

	CNVRHIRenderAPI() {}
	~CNVRHIRenderAPI() {}

	// Init + Shurdown
	void						Init(const ShaderAPIParams& params);
	void						Shutdown();

//-------------------------------------------------------------
// Renderer information
	void						PrintAPIInfo() const;
	bool						IsDeviceActive() const;

	// shader API class type for shader developers.
	EShaderAPIType				GetShaderAPIClass()		{ return SHADERAPI_WEBGPU; }

	// Renderer string (ex: OpenGL, D3D9)
	const char*					GetRendererName() const { return "WebGPU"; }

//-------------------------------------------------------------
// MT Synchronization

	// Synchronization
	void						Flush();

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
	IVertexFormat*				CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc);
	void						DestroyVertexFormat(IVertexFormat* pFormat);

//-------------------------------------------------------------
// Private access

	nvrhi::DeviceHandle			GetNVRHIDevice() const { return m_rhiDevice; }

protected:

	nvrhi::ShaderHandle			CreateShaderSPIRV(const uint32* code, uint32 size, const char* name = nullptr) const;
	nvrhi::ShaderHandle			CreateShaderWGSL(const char* szText, const char* name = nullptr) const;

	nvrhi::ShaderHandle			GetOrLoadShaderModule(const ShaderInfoNVRHIImpl& shaderInfo, int shaderModuleIdx) const;
	int							LoadShaderPackage(const char* filename);

	Map<int, ShaderInfoNVRHIImpl>	m_shaderCache{ PP_SL };
	nvrhi::DeviceHandle			m_rhiDevice{ nullptr };
	bool						m_deviceLost{ false };
};
