//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ShaderAPI.h"
#include "WGPUBackend.h"
#include "WGPUTexture.h"
#include "WGPUBuffer.h"
#include "ShaderInfo.h"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;

#define WGPU_INSTANCE_SPIN { g_renderWorker.SignalWork(); Platform_Sleep(0); }

class CWGPURenderAPI : public ShaderAPI_Base
{
	friend class CWGPURenderLib;
public:
	static CWGPURenderAPI Instance;

	CWGPURenderAPI() {}
	~CWGPURenderAPI() {}

	// Init + Shurdown
	void						Shutdown();

//-------------------------------------------------------------
// Renderer information
	void						PrintAPIInfo() const;
	bool						IsDeviceActive() const { return !m_deviceLost; }
	bool						IsDeviceValidationActive() const { return m_isValidationActive; }

	EShaderAPIType				GetShaderAPIClass()	const { return SHADERAPI_WEBGPU; }
	const char*					GetRendererName() const ;

//-------------------------------------------------------------
// MT Synchronization

	// Synchronization
	void						Flush();

//-------------------------------------------------------------
// Shaders
	int							LoadShaderPackage(const char* filename);
	int							ReloadShaderPackage(int id);
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

	WGPUDevice					GetWGPUDevice() const { return m_rhiDevice; }
	WGPUQueue					GetWGPUQueue() const { return m_rhiQueue; };

protected:

	WGPUShaderModule			CreateShaderSPIRV(const uint32* code, uint32 size, const char* dbgName = nullptr) const;
	WGPUShaderModule			CreateShaderWGSL(const char* szText, const char* dbgName = nullptr) const;

	WGPUShaderModule			GetOrLoadShaderModule(const ShaderInfo& shaderInfo, int shaderModuleIdx, const char* dbgName = nullptr) const;

	Map<int, ShaderInfo>		m_shaderCache{ PP_SL };
	WGPUInstance				m_rhiInstance{ nullptr };
	WGPUDevice					m_rhiDevice{ nullptr };
	WGPUQueue					m_rhiQueue{ nullptr };
	WGPUBackendType				m_rhiBackendType;

	mutable uint				m_pipelineIdCounter{ 0 };

	bool						m_deviceLost{ false };
	bool						m_isValidationActive{ false };
};
