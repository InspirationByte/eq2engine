//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				For high level look for the material system (Engine)
//				Contains FFP functions and Shader (FFP overriding programs)
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "imaging/textureformats.h"

#include "ShaderAPI_defs.h"

#include "IGPUBuffer.h"
#include "IGPUCommandRecorder.h"
#include "IVertexFormat.h"
#include "ITexture.h"

#undef far
#undef near

struct KVSection;
class CImage;

// shader api initializer
struct ShaderAPIParams
{
	RenderWindowInfo	windowInfo;
	ETextureFormat		screenFormat{ FORMAT_RGB8 };		// screen back buffer format

	int					screenRefreshRateHZ{ 60 };			// refresh rate in HZ
	int					multiSamplingMode{ 0 };				// multisampling
	int					depthBits{ 24 };					// bit depth for depth/stencil
};

//---------------------------------------------------------------

//---------------------------------
// Pipeline layout. Used for creating bind groups and pipelines
class IGPUPipelineLayout : public RefCountedObject<IGPUPipelineLayout> {};
using IGPUPipelineLayoutPtr = CRefPtr<IGPUPipelineLayout>;

//---------------------------------
// Render pipeline. Used for rendering things
class IGPURenderPipeline : public RefCountedObject<IGPURenderPipeline> {};
using IGPURenderPipelinePtr = CRefPtr<IGPURenderPipeline>;

//---------------------------------
// Compute pipeline
class IGPUComputePipeline : public RefCountedObject<IGPUComputePipeline> {};
using IGPUComputePipelinePtr = CRefPtr<IGPUComputePipeline>;

//---------------------------------
// Bind group. References used resources needed to render (textures, uniform buffers etc)
// not used for Vertex and Index buffers.
class IGPUBindGroup : public RefCountedObject<IGPUBindGroup> {};
using IGPUBindGroupPtr = CRefPtr<IGPUBindGroup>;

//---------------------------------
// ShaderAPI interface
//
class IShaderAPI
{
public:
	virtual ~IShaderAPI() {}

	// initializes shader api.
	// Don't use this, this already called by materials->Init()
	virtual void				Init( const ShaderAPIParams &params ) = 0;

	// shutdowns shader api. Don't use this, this already called by materials->Shutdown()
	virtual void				Shutdown() = 0;

	// returns the parameters
	virtual const ShaderAPIParams&	GetParams() const = 0;

//-------------------------------------------------------------
// Renderer capabilities and information
//-------------------------------------------------------------

	virtual const ShaderAPICapabilities&	GetCaps() const = 0;

	virtual EShaderAPIType		GetShaderAPIClass() const = 0;
	virtual const char*			GetRendererName() const = 0;
	virtual void				PrintAPIInfo() const = 0;

//-------------------------------------------------------------
// Device statistics
//-------------------------------------------------------------

	virtual bool				IsDeviceActive() const = 0;

	virtual int					GetDrawCallsCount() const = 0;
	virtual int					GetDrawIndexedPrimitiveCallsCount() const = 0;
	virtual int					GetTrianglesCount() const = 0;
	virtual void				ResetCounters() = 0;

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	virtual void				Flush() = 0;
	virtual void				Finish() = 0;

//-------------------------------------------------------------
// Buffer management

	virtual IGPUBufferPtr				CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name = nullptr) const = 0;

	// DEPRECATED
	virtual IVertexFormat*				CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> vertexLayout) = 0;
	virtual void						DestroyVertexFormat(IVertexFormat* pFormat) = 0;
	virtual IVertexFormat*				FindVertexFormat(const char* name) const = 0;
	virtual IVertexFormat*				FindVertexFormatById(int nameHash) const = 0;

//-------------------------------------------------------------
// Pipeline management

	// loads shader modules (caches in RHI/Driver)
	virtual void						LoadShaderModules(const char* shaderName, ArrayCRef<EqString> defines) const = 0;

	virtual IGPUPipelineLayoutPtr		CreatePipelineLayout(const PipelineLayoutDesc& layoutDesc) const = 0;
	virtual IGPURenderPipelinePtr		CreateRenderPipeline(const RenderPipelineDesc& pipelineDesc, const IGPUPipelineLayout* pipelineLayout = nullptr) const = 0;
	virtual IGPUComputePipelinePtr		CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) const = 0;

	// constructs bind group using explicit user-defined pipeline layoyt
	virtual IGPUBindGroupPtr			CreateBindGroup(const IGPUPipelineLayout* pipelineLayout, const BindGroupDesc& bindGroupDesc) const = 0;
	
	// constructs bind group using render pipeline layout  defined by shader module
	virtual IGPUBindGroupPtr			CreateBindGroup(const IGPURenderPipeline* renderPipeline, const BindGroupDesc& bindGroupDesc) const = 0;
	
	// constructs bind group using compute pipeline layout defined by shader module
	virtual IGPUBindGroupPtr			CreateBindGroup(const IGPUComputePipeline* computePipeline, const BindGroupDesc& bindGroupDesc) const = 0;
//-------------------------------------------------------------
// Command management

	virtual IGPUCommandRecorderPtr		CreateCommandRecorder(const char* name = nullptr, void* userData = nullptr) const = 0;
	virtual IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const = 0;
	virtual IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData = nullptr) const = 0;

//-------------------------------------------------------------
// Command buffer management
	
	virtual void						SubmitCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) const = 0;
	virtual void						SubmitCommandBuffer(const IGPUCommandBuffer* cmdBuffer) const = 0;

//-------------------------------------------------------------
// Texture resource managenent
//-------------------------------------------------------------

	// texture LOD uploading frequency
	virtual void				SetProgressiveTextureFrequency(int frames) = 0;
	virtual int					GetProgressiveTextureFrequency() const = 0;

	virtual ITexturePtr			FindOrCreateTexture(const char* pszName, bool& justCreated) = 0;
	virtual ITexturePtr			FindTexture(const char* pszName) = 0;

	virtual void				FreeTexture(ITexture* pTexture) = 0;

	// creates static texture from image (array for animated textures)
	virtual	ITexturePtr			CreateTexture(const ArrayCRef<CRefPtr<CImage>>& images, const SamplerStateParams& sampler, int flags = 0) = 0;
	
	// creates texture that will be render target or storage target
	virtual ITexturePtr			CreateRenderTarget(const TextureDesc& targetDesc) = 0;
	
	// resizes render target with new size (NOTE: data is not preserved)
	virtual void				ResizeRenderTarget(ITexture* renderTarget, const TextureExtent& newSize, int mipmapCount = 1, int sampleCount = 1) = 0;

//-------------------------------------------------------------
// DEPRECATED Procedural texture creation
//-------------------------------------------------------------

	// creates lockable texture
	virtual ITexturePtr			CreateProceduralTexture(const char* pszName,
														ETextureFormat nFormat,
														int width, int height,
														int arraySize = 1,
														const SamplerStateParams& sampler = {},
														int flags = 0,
														int dataSize = 0, const ubyte* data = nullptr
														) = 0;
};


// it always external, declare new one in your app...
extern IShaderAPI* g_renderAPI;
