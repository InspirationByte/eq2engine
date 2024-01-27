//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>

#include "ShaderAPI_Base.h"
#include "WGPUTexture.h"
#include "WGPUBuffer.h"
#include "WGPUVertexFormat.h"
#include "WGPUShader.h"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;
extern CEqMutex	g_sapi_ShaderMutex;
extern CEqMutex	g_sapi_VBMutex;
extern CEqMutex	g_sapi_IBMutex;
extern CEqMutex	g_sapi_Mutex;

class CWGPURenderAPI : public ShaderAPI_Base
{
	friend class CWGPURenderLib;
public:
	static CWGPURenderAPI Instance;

	CWGPURenderAPI() {}
	~CWGPURenderAPI() {}

	// Init + Shurdown
	void						Init(const ShaderAPIParams& params);
	//void						Shutdown() {}

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
	void						Flush() {}
	void						Finish() {}

//-------------------------------------------------------------
// Textures

	ITexturePtr					CreateTextureResource(const char* pszName);
	ITexturePtr					CreateRenderTarget(const TextureDesc& targetDesc);
	void						ResizeRenderTarget(ITexture* renderTarget, const TextureExtent& newSize, int mipmapCount = 1, int sampleCount = 1);

//-------------------------------------------------------------
// Pipeline management

	void						LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines) const;

	IGPUPipelineLayoutPtr		CreatePipelineLayout(const PipelineLayoutDesc& layoutDesc) const;
	IGPURenderPipelinePtr		CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout = nullptr) const;
	IGPUComputePipelinePtr		CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) const;

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

// DEPRECATED
	IVertexFormat*				CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc);
	void						DestroyVertexFormat(IVertexFormat* pFormat);

//-------------------------------------------------------------
// Private access

	WGPUDevice					GetWGPUDevice() const { return m_rhiDevice; }
	WGPUQueue					GetWGPUQueue() const { return m_rhiQueue; };

protected:

	WGPUShaderModule			CreateShaderSPIRV(const uint32* code, uint32 size, const char* name = nullptr) const;

	WGPUShaderModule			GetOrLoadShaderModule(const ShaderInfoWGPUImpl& shaderInfo, int shaderModuleIdx) const;
	int							LoadShaderPackage(const char* filename);

	Map<int, ShaderInfoWGPUImpl>	m_shaderCache{ PP_SL };
	WGPUDevice					m_rhiDevice{ nullptr };
	WGPUQueue					m_rhiQueue{ nullptr };
	bool						m_deviceLost{ false };
};
