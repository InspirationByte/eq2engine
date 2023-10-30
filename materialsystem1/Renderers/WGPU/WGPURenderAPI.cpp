//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "imaging/ImageLoader.h"
#include "WGPURenderAPI.h"

#define ASSERT_DEPRECATED() // ASSERT_FAIL("Deprecated API %s", __func__)

CWGPURenderAPI CWGPURenderAPI::Instance;

void CWGPURenderAPI::Init(const ShaderAPIParams& params)
{
	ShaderAPI_Base::Init(params);
}
//void CWGPURenderAPI::Shutdown() {}

void CWGPURenderAPI::PrintAPIInfo() const
{
}

bool CWGPURenderAPI::IsDeviceActive() const
{
	return true;
}

IVertexFormat* CWGPURenderAPI::CreateVertexFormat(const char* name, ArrayCRef<VertexFormatDesc> formatDesc)
{
	IVertexFormat* pVF = PPNew CWGPUVertexFormat(name, formatDesc.ptr(), formatDesc.numElem());
	m_VFList.append(pVF);
	return pVF;
}

IVertexBuffer* CWGPURenderAPI::CreateVertexBuffer(const BufferInfo& bufferInfo)
{
	CWGPUVertexBuffer* buffer = PPNew CWGPUVertexBuffer(bufferInfo);
	m_VBList.append(buffer);
	return buffer;
}

IIndexBuffer* CWGPURenderAPI::CreateIndexBuffer(const BufferInfo& bufferInfo)
{
	CWGPUIndexBuffer* buffer = PPNew CWGPUIndexBuffer(bufferInfo);
	m_IBList.append(buffer);
	return buffer;
}

// Destroy vertex format
void CWGPURenderAPI::DestroyVertexFormat(IVertexFormat* pFormat)
{
	if (m_VFList.fastRemove(pFormat))
		delete pFormat;
}

// Destroy vertex buffer
void CWGPURenderAPI::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	if (m_VBList.fastRemove(pVertexBuffer))
		delete pVertexBuffer;
}

// Destroy index buffer
void CWGPURenderAPI::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	if (m_IBList.fastRemove(pIndexBuffer))
		delete pIndexBuffer;
}

//-------------------------------------------------------------
// Shaders and it's operations

IShaderProgramPtr CWGPURenderAPI::CreateNewShaderProgram(const char* pszName, const char* query)
{
	return nullptr;
}

void CWGPURenderAPI::FreeShaderProgram(IShaderProgram* pShaderProgram)
{
}

bool CWGPURenderAPI::CompileShadersFromStream(IShaderProgramPtr pShaderOutput, const ShaderProgCompileInfo& info, const char* extra)
{
	return true; 
}

//-------------------------------------------------------------
// Occlusion query

// creates occlusion query object
IOcclusionQuery* CWGPURenderAPI::CreateOcclusionQuery()
{
	return nullptr;
}

// removal of occlusion query object
void CWGPURenderAPI::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
}

//-------------------------------------------------------------
// Textures

ITexturePtr CWGPURenderAPI::CreateTextureResource(const char* pszName)
{
	CRefPtr<CWGPUTexture> texture = CRefPtr_new(CWGPUTexture);
	texture->SetName(pszName);

	m_TextureList.insert(texture->m_nameHash, texture);
	return ITexturePtr(texture);
}

// It will add new rendertarget
ITexturePtr	CWGPURenderAPI::CreateRenderTarget(const char* pszName,int width, int height, ETextureFormat nRTFormat, ETexFilterMode textureFilterType, ETexAddressMode textureAddress, ECompareFunc comparison, int nFlags)
{
	CRefPtr<CWGPUTexture> pTexture = CRefPtr_new(CWGPUTexture);
	pTexture->SetName(pszName);

	CScopedMutex scoped(g_sapi_TextureMutex);
	CHECK_TEXTURE_ALREADY_ADDED(pTexture);
	m_TextureList.insert(pTexture->m_nameHash, pTexture);

	pTexture->SetDimensions(width, height);
	pTexture->SetFormat(nRTFormat);

	return ITexturePtr(pTexture);
}

//-------------------------------------------------------------
// Pipeline management

struct FragmentPipelineDesc
{
	struct ColorTargetDesc
	{
		BlendStateParams	blendParams;
		ETextureFormat		format;
	};
	using ColorTargetDescList = FixedArray<ColorTargetDesc, MAX_RENDERTARGETS>;

	ColorTargetDescList		targetDesc;
	EqString				shaderEntryPoint;
	bool					enabled{ false };
};

struct VertexPipelineDesc
{
	using VertexFormatDescList = Array<VertexFormatDesc>;
	VertexFormatDescList	vertexFormat{ PP_SL };
	EqString				shaderEntryPoint;
};

struct PrimitiveDesc
{
	ECullMode				cullMode{ CULL_NONE };
	EPrimTopology			topology{ PRIM_TRIANGLES };
	EStripIndexFormat		stripIndex{ STRIP_INDEX_NONE };
};

struct RenderPipelineDesc
{
	VertexPipelineDesc		vertex;
	DepthStencilStateParams	depthStencil;
	FragmentPipelineDesc	fragment;
	PrimitiveDesc			primitive;
};

void* CWGPURenderAPI::CreateRenderPipeline()
{
	// Pipeline layout and bind group layout
	// are objects of CWGPUPipeline
	// There are 3 distinctive bind groups or buffers as our MatSystem design defines:
	//		- Material Constant Properties (static buffer)
	//		- Material Proxy Properties (buffers of these group updated every frame)
	//		- Scene Properties (camera, transform, fog, clip planes)
	WGPUPipelineLayout pipelineLayout = nullptr;
	FixedArray<WGPUBindGroupLayout, 16> bindGroupLayoutList;
	{
		{
			FixedArray<WGPUBindGroupLayoutEntry, 16> layoutEntry;
			{
				WGPUBindGroupLayoutEntry bglEntry = {};
				bglEntry.binding = 0;
				bglEntry.visibility = WGPUShaderStage_Vertex;
				{
					bglEntry.buffer.type = WGPUBufferBindingType_Uniform;
					bglEntry.buffer.minBindingSize = 0;
					bglEntry.buffer.hasDynamicOffset = false; // set in wgpuRenderPassEncoderSetBindGroup
				}
				//{
				//	bglEntry.sampler.type = WGPUSamplerBindingType_Filtering;
				//}
				//{
				//	bglEntry.texture.sampleType = WGPUTextureSampleType_Uint;
				//	bglEntry.texture.viewDimension = WGPUTextureViewDimension_2D;
				//	bglEntry.texture.multisampled = false;
				//}
				//{
				//	bglEntry.storageTexture.access = WGPUStorageTextureAccess_ReadWrite;
				//	bglEntry.storageTexture.format = WGPUTextureFormat_RGBA8Unorm;
				//	bglEntry.storageTexture.viewDimension = WGPUTextureViewDimension_2D;
				//}
				layoutEntry.append(bglEntry);
			}

			WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
			bindGroupLayoutDesc.entryCount = layoutEntry.numElem();
			bindGroupLayoutDesc.entries = layoutEntry.ptr();

			WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_rhiDevice, &bindGroupLayoutDesc);
			bindGroupLayoutList.append(bindGroupLayout);
		}

		{
			WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
			pipelineLayoutDesc.bindGroupLayoutCount = bindGroupLayoutList.numElem();
			pipelineLayoutDesc.bindGroupLayouts = bindGroupLayoutList.ptr();
			pipelineLayout = wgpuDeviceCreatePipelineLayout(m_rhiDevice, &pipelineLayoutDesc);
		}
	}

	// Each instance of MatSystemShader has own bind groups
	FixedArray<WGPUBindGroup, 16> bindGroupList;
	for(WGPUBindGroupLayout layout : bindGroupLayoutList)
	{
		// when creating pipeline itself
		FixedArray<WGPUBindGroupEntry, 16> bindGroupEntryDescList;
		{
			WGPUBindGroupEntry bindGroupEntryDesc = {};
			bindGroupEntryDesc.binding = 0;
			//bindGroupEntryDesc.buffer;
			//bindGroupEntryDesc.sampler;
			//bindGroupEntryDesc.textureView;
			bindGroupEntryDesc.size = 16384;
			bindGroupEntryDesc.offset = 0;
			bindGroupEntryDescList.append(bindGroupEntryDesc);
		}

		WGPUBindGroupDescriptor bindGroupDesc = {};
		bindGroupDesc.layout = layout;
		bindGroupDesc.entryCount = bindGroupEntryDescList.numElem();
		bindGroupDesc.entries = bindGroupEntryDescList.ptr();

		WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &bindGroupDesc);
		bindGroupList.append(bindGroup);
	}

	// After bindGroupList successfully created, they can be bound to the pipeline
	//WGPURenderPassEncoder renderPassEnc = nullptr;
	//for (int bindGroup = 0; bindGroup < bindGroupList.numElem(); ++bindGroup)
	//	wgpuRenderPassEncoderSetBindGroup(renderPassEnc, bindGroup, bindGroupList[bindGroup], 0, nullptr);

	// 1. The pipeline almost fully constructed by the MatSystemShader
	//    except the primitive state, which is:
	//    - topology
	//    - cull mode
	//    - strip index format for primitive restart
	// 
	//    The rest is happily encoded by the render pass
	//    - scissor
	//    - viewport
	//
	// 2. MatSystemShader must know which vertex format (layout) it does support
	//    so for each vertex (DynMeshVertex, EGFVertex, LevelVertex)
	//    the pipeline is generated.
	//
	// 3. When building DrawCall for command buffer, MatSystem must decide
	//    which pipeline to use from shader. 
	//    For example, RenderDrawCmd settings that selecting pipeline:
	//    - vertexFormat
	//    - primitiveTopology
	//
	WGPURenderPipelineDescriptor renderPipelineDesc = {};
	renderPipelineDesc.layout = pipelineLayout;

	FixedArray<WGPUVertexAttribute, 128> vertexAttribList;
	FixedArray<WGPUVertexBufferLayout, 16> vertexBufferLayoutList;
	{
		const int firstVertexAttrib = vertexAttribList.numElem();
		{
			WGPUVertexAttribute vertAttr = {};
			vertAttr.format = WGPUVertexFormat_Float32x2;
			vertAttr.offset = 0;
			vertAttr.shaderLocation = 0;
			vertexAttribList.append(vertAttr);
		}

		{
			WGPUVertexAttribute vertAttr = {};
			vertAttr.format = WGPUVertexFormat_Float32x3;
			vertAttr.offset = 2 * sizeof(float);
			vertAttr.shaderLocation = 1;
			vertexAttribList.append(vertAttr);
		}

		// TODO: size table for WGPUVertexFormat enum

		WGPUVertexBufferLayout vertexBufferLayout = {};
		vertexBufferLayout.arrayStride = 5 * sizeof(float);
		vertexBufferLayout.attributeCount = vertexAttribList.numElem() - firstVertexAttrib;
		vertexBufferLayout.attributes = &vertexAttribList[firstVertexAttrib];
		vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
		vertexBufferLayoutList.append(vertexBufferLayout);
	}

	WGPUShaderModule vertMod = nullptr;//createShader(triangle_vert_wgsl);
	WGPUShaderModule fragMod = nullptr;//createShader(triangle_frag_wgsl);

	// Setup vertex pipeline
	// Required
	renderPipelineDesc.vertex.module = vertMod;
	renderPipelineDesc.vertex.entryPoint = "main";
	renderPipelineDesc.vertex.bufferCount = vertexBufferLayoutList.numElem();
	renderPipelineDesc.vertex.buffers = vertexBufferLayoutList.ptr();

	Array<WGPUConstantEntry> vertexPipelineConstants(PP_SL);
	renderPipelineDesc.vertex.constants = vertexPipelineConstants.ptr();
	renderPipelineDesc.vertex.constantCount = vertexPipelineConstants.numElem();

	// Setup fragment pipeline
	FixedArray<WGPUColorTargetState, 16> colorTargets;
	FixedArray<WGPUBlendState, 16> colorTargetBlends;
	{
		{
			WGPUBlendState blend = {};
			blend.color.operation = WGPUBlendOperation_Add;
			blend.color.srcFactor = WGPUBlendFactor_One;
			blend.color.dstFactor = WGPUBlendFactor_One;

			blend.alpha.operation = WGPUBlendOperation_Add;
			blend.alpha.srcFactor = WGPUBlendFactor_One;
			blend.alpha.dstFactor = WGPUBlendFactor_One;
			colorTargetBlends.append(blend);
		}

		WGPUColorTargetState colorTarget = {};
		colorTarget.format = WGPUTextureFormat_Undefined; //webgpu::getSwapChainFormat(device);
		colorTarget.blend = &colorTargetBlends.back();
		colorTarget.writeMask = WGPUColorWriteMask_All;
		colorTargets.append(colorTarget);
	}
	
	// Depth state
	// Optional when depth read = false
	WGPUDepthStencilState depthStencil = {};
	depthStencil.format = WGPUTextureFormat_Depth24Plus; // backbuffer depth format for example
	depthStencil.depthWriteEnabled = false;
	depthStencil.depthCompare = WGPUCompareFunction_LessEqual;
	depthStencil.stencilReadMask = 0xFFFFFFFF;
	depthStencil.stencilWriteMask = 0xFFFFFFFF;
	depthStencil.depthBias = 0;
	depthStencil.depthBiasSlopeScale = 0;
	depthStencil.depthBiasClamp = 0;
	depthStencil.stencilBack.compare = WGPUCompareFunction_Always;
	depthStencil.stencilBack.failOp = WGPUStencilOperation_Keep;
	depthStencil.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
	depthStencil.stencilBack.passOp = WGPUStencilOperation_Keep;
	depthStencil.stencilFront.compare = WGPUCompareFunction_Always;
	depthStencil.stencilFront.failOp = WGPUStencilOperation_Keep;
	depthStencil.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
	depthStencil.stencilFront.passOp = WGPUStencilOperation_Keep;
	renderPipelineDesc.depthStencil = &depthStencil;

	// Fragment state
	// When opted out, requires depthStencil state
	WGPUFragmentState fragmentState = {};
	fragmentState.module = fragMod;
	fragmentState.entryPoint = "main";
	fragmentState.targetCount = colorTargets.numElem();
	fragmentState.targets = colorTargets.ptr();

	Array<WGPUConstantEntry> fragmentPipelineConstants(PP_SL);
	fragmentState.constants = fragmentPipelineConstants.ptr();
	fragmentState.constantCount = fragmentPipelineConstants.numElem();

	renderPipelineDesc.fragment = &fragmentState;

	// Multisampling
	renderPipelineDesc.multisample.count = 1;
	renderPipelineDesc.multisample.mask = 0xFFFFFFFF;
	renderPipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Primitive toplogy
	renderPipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
	renderPipelineDesc.primitive.cullMode = WGPUCullMode_Back;
	renderPipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	renderPipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
	WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(m_rhiDevice, &renderPipelineDesc);

	return nullptr;
}


//-------------------------------------------------------------
// Render states management

IRenderState* CWGPURenderAPI::CreateBlendingState( const BlendStateParams &blendDesc )
{
	ASSERT_DEPRECATED();
	return nullptr;
}

IRenderState* CWGPURenderAPI::CreateDepthStencilState( const DepthStencilStateParams &depthDesc )
{
	ASSERT_DEPRECATED();
	return nullptr;
}

IRenderState* CWGPURenderAPI::CreateRasterizerState( const RasterizerStateParams &rasterDesc )
{
	ASSERT_DEPRECATED();
	return nullptr;
}

void CWGPURenderAPI::DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Render target operations

void CWGPURenderAPI::Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const MColor& fillColor, float fDepth, int nStencil)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::CopyFramebufferToTexture(const ITexturePtr& pTargetTexture)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect, IAARectangle* destRect)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets, ArrayCRef<int> rtSlice, const ITexturePtr& depthTarget, int depthSlice)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeRenderTargetToBackBuffer()
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ResizeRenderTarget(const ITexturePtr& pRT, int newWide, int newTall)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Various setup functions for drawing

void CWGPURenderAPI::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::ChangeIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// State management

void  CWGPURenderAPI::SetScissorRectangle(const IAARectangle& rect)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::SetDepthRange(float fZNear, float fZFar)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Renderer state managemet

void CWGPURenderAPI::SetShader(IShaderProgramPtr pShader)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::SetTexture(int nameHash, const ITexturePtr& texture)
{
	ASSERT_DEPRECATED();
}

void CWGPURenderAPI::SetShaderConstantRaw(int nameHash, const void* data, int nSize)
{
	ASSERT_DEPRECATED();
}

//-------------------------------------------------------------
// Primitive drawing

void CWGPURenderAPI::DrawIndexedPrimitives(EPrimTopology nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
}

void CWGPURenderAPI::DrawNonIndexedPrimitives(EPrimTopology nType, int nFirstVertex, int nVertices)
{
}