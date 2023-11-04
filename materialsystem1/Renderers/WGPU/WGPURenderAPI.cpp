//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "imaging/ImageLoader.h"
#include "WGPURenderAPI.h"
#include "WGPURenderDefs.h"

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

IVertexFormat* CWGPURenderAPI::CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc)
{
	IVertexFormat* pVF = PPNew CWGPUVertexFormat(name, formatDesc);
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

#pragma optimize("", off)

static void PipelineLayoutDescBuilder()
{
	// FIXME: names?
	RenderPipelineLayoutDesc pipelineLayoutDesc = Builder<RenderPipelineLayoutDesc>()
		.Group(
			Builder<BindGroupLayoutDesc>()
			.Texture("baseTexture", 0, SHADER_VISIBLE_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.Buffer("materialProps", 1, SHADER_VISIBLE_FRAGMENT, BUFFERBIND_UNIFORM)
			.End()
		)
		.End();

	sizeof(RenderPipelineLayoutDesc);

	Msg("%s dun\n", __func__);
}

static void PipelineDescBuilder()
{
	RenderPipelineDesc pipelineDesc = Builder<RenderPipelineDesc>()
		.DepthState(
			Builder<DepthStencilStateParams>()
			.DepthFormat(FORMAT_R16F)
			.DepthTestOn()
			.DepthWriteOn()
			.End()
		)
		.VertexState(
			Builder<VertexPipelineDesc>()
			.ShaderEntry("main")
			.VertexLayout(
				Builder<VertexLayoutDesc>()
				.Attribute(VERTEXATTRIB_POSITION, "position", 0, 0, ATTRIBUTEFORMAT_FLOAT, 3)
				.Attribute(VERTEXATTRIB_TEXCOORD, "texCoord", 1, sizeof(Vector3D), ATTRIBUTEFORMAT_HALF, 2)
				.Stride(sizeof(Vector3D))
				.End()
			)
			.End()
		)
		.FragmentState(
			Builder<FragmentPipelineDesc>()
			.ColorTarget("Color", FORMAT_RGBA8)
			.End()
		)
		.PrimitiveState(
			Builder<PrimitiveDesc>()
			.Cull(CULL_BACK)
			.Topology(PRIM_TRIANGLES)
			.End()
		)
		.End();

	sizeof(RenderPipelineDesc);

	Msg("%s dun\n", __func__);
}

static void BindGroupBuilder()
{
	BindGroupDesc bindGroup = Builder<BindGroupDesc>()
		.Buffer(1, nullptr, 0, 16)
		.Texture(2, nullptr)
		.End();

	Msg("%s dun\n", __func__);
}

DECLARE_CMD(test_wgpu, nullptr, 0)
{
	PipelineLayoutDescBuilder();
	PipelineDescBuilder();
	BindGroupBuilder();
}

class CWGPUPipelineLayout : public IGPUPipelineLayout
{
public:
	~CWGPUPipelineLayout()
	{
		wgpuPipelineLayoutRelease(m_rhiPipelineLayout);
		for(WGPUBindGroupLayout layout: m_rhiBindGroupLayout)
			wgpuBindGroupLayoutRelease(layout);
	}

	// TODO: name
	Array<WGPUBindGroupLayout>	m_rhiBindGroupLayout{ PP_SL };
	WGPUPipelineLayout			m_rhiPipelineLayout{ nullptr };

};

class CWGPURenderPipeline : public IGPURenderPipeline
{
public:
	~CWGPURenderPipeline()
	{
		wgpuRenderPipelineRelease(m_rhiRenderPipeline);
	}
	// TODO: name
	WGPURenderPipeline	m_rhiRenderPipeline{ nullptr };

};

class CWGPUBindGroup : public IGPUBindGroup
{
public:
	~CWGPUBindGroup()
	{
		wgpuBindGroupRelease(m_rhiBindGroup);
	}

	// TODO: name
	WGPUBindGroup	m_rhiBindGroup{ nullptr };

};

IGPUBufferPtr CWGPURenderAPI::CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name)
{
	CRefPtr<CWGPUBuffer> buffer = CRefPtr_new(CWGPUBuffer);
	int wgpuUsageFlags = 0;
	
	if (bufferUsageFlags & BUFFERUSAGE_UNIFORM) wgpuUsageFlags |= WGPUBufferUsage_Uniform;
	if (bufferUsageFlags & BUFFERUSAGE_VERTEX)	wgpuUsageFlags |= WGPUBufferUsage_Vertex;
	if (bufferUsageFlags & BUFFERUSAGE_INDEX)	wgpuUsageFlags |= WGPUBufferUsage_Index;
	if (bufferUsageFlags & BUFFERUSAGE_INDIRECT)wgpuUsageFlags |= WGPUBufferUsage_Indirect;
	if (bufferUsageFlags & BUFFERUSAGE_STORAGE)	wgpuUsageFlags |= WGPUBufferUsage_Storage;

	buffer->Init(bufferInfo, wgpuUsageFlags , name);
	return IGPUBufferPtr(buffer);
}

IGPUPipelineLayoutPtr CWGPURenderAPI::CreatePipelineLayout(const RenderPipelineLayoutDesc& layoutDesc)
{
	// Pipeline layout and bind group layout
	// are also objects of IGPURenderPipeline
	// There are 3 distinctive bind groups or buffers as our MatSystem design defines:
	//		- Material Constant Properties (static buffer)
	//		- Material Proxy Properties (buffers of these group updated every frame)
	//		- Scene Properties (camera, transform, fog, clip planes)
	Array<WGPUBindGroupLayout> rhiBindGroupLayout(PP_SL);
	Array<WGPUBindGroupLayoutEntry> rhiBindGroupLayoutEntry(PP_SL);
	for(const BindGroupLayoutDesc& bindGroupDesc : layoutDesc.bindGroups)
	{
		rhiBindGroupLayoutEntry.clear();

		for(const BindGroupLayoutDesc::Entry& entry : bindGroupDesc.entries)
		{
			WGPUBindGroupLayoutEntry bglEntry = {};
			bglEntry.binding = entry.binding;

			if (entry.visibility & SHADER_VISIBLE_VERTEX)	bglEntry.visibility |= WGPUShaderStage_Vertex;
			if (entry.visibility & SHADER_VISIBLE_FRAGMENT) bglEntry.visibility |= WGPUShaderStage_Fragment;
			if (entry.visibility & SHADER_VISIBLE_COMPUTE)	bglEntry.visibility |= WGPUShaderStage_Compute;

			switch (entry.type)
			{
				case BINDENTRY_BUFFER:
					bglEntry.buffer.hasDynamicOffset = entry.buffer.hasDynamicOffset;
					bglEntry.buffer.type = g_wgpuBufferBindingType[entry.buffer.bindType];
					break;
				case BINDENTRY_SAMPLER:
					bglEntry.sampler.type = g_wgpuSamplerBindingType[entry.sampler.bindType];
					break;
				case BINDENTRY_TEXTURE:
					bglEntry.texture.sampleType = g_wgpuTexSampleType[entry.texture.sampleType];
					bglEntry.texture.viewDimension = g_wgpuTexViewDimensions[entry.texture.dimension];
					bglEntry.texture.multisampled = entry.texture.multisampled;
					break;
				case BINDENTRY_STORAGETEXTURE:
					bglEntry.storageTexture.access = g_wgpuStorageTexAccess[entry.storageTexture.access];
					bglEntry.storageTexture.viewDimension = g_wgpuTexViewDimensions[entry.storageTexture.dimension];
					bglEntry.storageTexture.format = g_wgpuTexFormats[entry.storageTexture.format];
					break;
			}
			rhiBindGroupLayoutEntry.append(bglEntry);
		}

		WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
		bindGroupLayoutDesc.entryCount = rhiBindGroupLayoutEntry.numElem();
		bindGroupLayoutDesc.entries = rhiBindGroupLayoutEntry.ptr();

		WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_rhiDevice, &bindGroupLayoutDesc);
		if (!bindGroupLayout)
			return nullptr;

		rhiBindGroupLayout.append(bindGroupLayout);
	}

	WGPUPipelineLayoutDescriptor rhiPipelineLayoutDesc = {};
	rhiPipelineLayoutDesc.label = layoutDesc.name.Length() ? layoutDesc.name.ToCString() : nullptr;
	rhiPipelineLayoutDesc.bindGroupLayoutCount = rhiBindGroupLayout.numElem();
	rhiPipelineLayoutDesc.bindGroupLayouts = rhiBindGroupLayout.ptr();

	WGPUPipelineLayout rhiPipelineLayout = wgpuDeviceCreatePipelineLayout(m_rhiDevice, &rhiPipelineLayoutDesc);
	if (!rhiPipelineLayout)
		return nullptr;

	CRefPtr<CWGPUPipelineLayout> pipelineLayout = CRefPtr_new(CWGPUPipelineLayout);
	pipelineLayout->m_rhiBindGroupLayout = std::move(rhiBindGroupLayout);
	pipelineLayout->m_rhiPipelineLayout = rhiPipelineLayout;
	return IGPUPipelineLayoutPtr(pipelineLayout);
}

IGPUBindGroupPtr CWGPURenderAPI::CreateBindGroup(const IGPUPipelineLayoutPtr layoutDesc, int layoutBindGroupIdx, const BindGroupDesc& bindGroupDesc)
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
	}

	// Bind group is a collection of GPU resources (buffers, samplers, textures)
	// layed out for the pipelines so you can set resources in one shot.
	Array<WGPUBindGroupEntry> rhiBindGroupEntryList(PP_SL);

	// release samplers after bind groups has been made
	// other things like textures and buffer are released by user
	// and still can be used by this bind groups
	defer{
		for (WGPUBindGroupEntry& entry : rhiBindGroupEntryList)
		{
			if (entry.sampler)
				wgpuSamplerRelease(entry.sampler);
		}
	};

	for(const BindGroupDesc::Entry& bindGroupEntry : bindGroupDesc.entries)
	{
		WGPUBindGroupEntry rhiBindGroupEntryDesc = {};
		rhiBindGroupEntryDesc.binding = bindGroupEntry.binding;
		switch (bindGroupEntry.type)
		{
			case BINDENTRY_BUFFER:
			{
				CWGPUBuffer* buffer = static_cast<CWGPUBuffer*>(bindGroupEntry.buffer);
				if (buffer)
					rhiBindGroupEntryDesc.buffer = buffer->GetWGPUBuffer();
				else
					ASSERT_FAIL("NULL buffer for bindGroup %d binding %d", layoutBindGroupIdx, bindGroupEntry.binding);

				rhiBindGroupEntryDesc.size = bindGroupEntry.bufferSize;
				rhiBindGroupEntryDesc.offset = bindGroupEntry.bufferOffset;
				break;
			}
			case BINDENTRY_SAMPLER:
			{
				const SamplerStateParams& samplerParams = bindGroupEntry.sampler;
				WGPUSamplerDescriptor rhiSamplerDesc = {};
				rhiSamplerDesc.addressModeU = g_wgpuAddressMode[samplerParams.addressU];
				rhiSamplerDesc.addressModeV = g_wgpuAddressMode[samplerParams.addressV];
				rhiSamplerDesc.addressModeW = g_wgpuAddressMode[samplerParams.addressW];
				rhiSamplerDesc.compare = g_wgpuCompareFunc[samplerParams.compareFunc];
				rhiSamplerDesc.minFilter = g_wgpuFilterMode[samplerParams.minFilter];
				rhiSamplerDesc.magFilter = g_wgpuFilterMode[samplerParams.magFilter];
				rhiSamplerDesc.mipmapFilter = g_wgpuMipmapFilterMode[samplerParams.mipmapFilter];
				rhiSamplerDesc.maxAnisotropy = samplerParams.maxAnisotropy;
				rhiBindGroupEntryDesc.sampler = wgpuDeviceCreateSampler(m_rhiDevice, &rhiSamplerDesc);
				break;
			}
			case BINDENTRY_STORAGETEXTURE:
			case BINDENTRY_TEXTURE:
				CWGPUTexture* texture = static_cast<CWGPUTexture*>(bindGroupEntry.texture);
				if(texture)
					rhiBindGroupEntryDesc.textureView = texture->m_rhiViews[texture->GetAnimationFrame()];
				else
					ASSERT_FAIL("NULL texture for bindGroup %d binding %d", layoutBindGroupIdx, bindGroupEntry.binding);
				break;
		}

		rhiBindGroupEntryList.append(rhiBindGroupEntryDesc);
	}

	const CWGPUPipelineLayout* pipelineLayoutDesc = static_cast<CWGPUPipelineLayout*>(layoutDesc.Ptr());

	WGPUBindGroupDescriptor rhiBindGroupDesc = {};
	rhiBindGroupDesc.label = bindGroupDesc.name.Length() ? bindGroupDesc.name.ToCString() : nullptr;
	rhiBindGroupDesc.layout = pipelineLayoutDesc->m_rhiBindGroupLayout[layoutBindGroupIdx];
	rhiBindGroupDesc.entryCount = rhiBindGroupEntryList.numElem();
	rhiBindGroupDesc.entries = rhiBindGroupEntryList.ptr();

	WGPUBindGroup rhiBindGroup = wgpuDeviceCreateBindGroup(m_rhiDevice, &rhiBindGroupDesc);
	if (!rhiBindGroup)
		return nullptr;
	
	CRefPtr<CWGPUBindGroup> bindGroup = CRefPtr_new(CWGPUBindGroup);
	bindGroup->m_rhiBindGroup = rhiBindGroup;

	return IGPUBindGroupPtr(bindGroup);
}

IGPURenderPipelinePtr CWGPURenderAPI::CreateRenderPipeline(const IGPUPipelineLayoutPtr layoutDesc, const RenderPipelineDesc& pipelineDesc)
{
	if (!layoutDesc)
	{
		ASSERT_FAIL("layoutDesc is null");
		return nullptr;
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

	// pipeline-overridable constants
	Array<WGPUConstantEntry> rhiVertexPipelineConstants(PP_SL);
	Array<WGPUConstantEntry> rhiFragmentPipelineConstants(PP_SL);

	WGPURenderPipelineDescriptor rhiRenderPipelineDesc = {};
	rhiRenderPipelineDesc.layout = static_cast<CWGPUPipelineLayout*>(layoutDesc.Ptr())->m_rhiPipelineLayout;

	// Setup vertex pipeline
	// Required
	Array<WGPUVertexAttribute> rhiVertexAttribList(PP_SL);
	Array<WGPUVertexBufferLayout> rhiVertexBufferLayoutList(PP_SL);
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No vertex shader entrypoint set");

		for(const VertexLayoutDesc& vertexLayout : pipelineDesc.vertex.vertexLayout)
		{
			const int firstVertexAttrib = rhiVertexAttribList.numElem();
			for(const VertexLayoutDesc::AttribDesc& attrib : vertexLayout.attributes)
			{
				WGPUVertexAttribute vertAttr = {};
				vertAttr.format = g_wgpuVertexFormats[attrib.format][attrib.count];
				vertAttr.offset = attrib.offset;
				vertAttr.shaderLocation = attrib.location;
				rhiVertexAttribList.append(vertAttr);
			}

			WGPUVertexBufferLayout rhiVertexBufferLayout = {};
			rhiVertexBufferLayout.arrayStride = vertexLayout.stride;
			rhiVertexBufferLayout.attributeCount = rhiVertexAttribList.numElem() - firstVertexAttrib;
			rhiVertexBufferLayout.attributes = &rhiVertexAttribList[firstVertexAttrib];
			rhiVertexBufferLayout.stepMode = g_wgpuVertexStepMode[vertexLayout.stepMode];
			rhiVertexBufferLayoutList.append(rhiVertexBufferLayout);
		}

		WGPUShaderModule vertMod = nullptr; // TODO: retrieve from cache
		rhiRenderPipelineDesc.vertex.module = vertMod;
		rhiRenderPipelineDesc.vertex.entryPoint = pipelineDesc.vertex.shaderEntryPoint;
		rhiRenderPipelineDesc.vertex.bufferCount = rhiVertexBufferLayoutList.numElem();
		rhiRenderPipelineDesc.vertex.buffers = rhiVertexBufferLayoutList.ptr();
		rhiRenderPipelineDesc.vertex.constants = rhiVertexPipelineConstants.ptr();
		rhiRenderPipelineDesc.vertex.constantCount = rhiVertexPipelineConstants.numElem();
	}
	
	// Depth state
	// Optional when depth read = false
	WGPUDepthStencilState rhiDepthStencil = {};
	if (pipelineDesc.depthStencil.depthTest)
	{
		ASSERT_MSG(pipelineDesc.depthStencil.format != FORMAT_NONE, "Must set valid depthStencil texture format");
		
		rhiDepthStencil.format = g_wgpuTexFormats[pipelineDesc.depthStencil.format];
		rhiDepthStencil.depthWriteEnabled = pipelineDesc.depthStencil.depthWrite;
		rhiDepthStencil.depthCompare = g_wgpuCompareFunc[pipelineDesc.depthStencil.depthFunc];
		rhiDepthStencil.stencilReadMask = pipelineDesc.depthStencil.stencilMask;
		rhiDepthStencil.stencilWriteMask = pipelineDesc.depthStencil.stencilWriteMask;
		rhiDepthStencil.depthBias = pipelineDesc.depthStencil.depthBias;
		rhiDepthStencil.depthBiasSlopeScale = pipelineDesc.depthStencil.depthBiasSlopeScale;
		rhiDepthStencil.depthBiasClamp = 0; // TODO

		// back
		rhiDepthStencil.stencilBack.compare = g_wgpuCompareFunc[pipelineDesc.depthStencil.stencilBack.compareFunc];
		rhiDepthStencil.stencilBack.failOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.failOp];
		rhiDepthStencil.stencilBack.depthFailOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.depthFailOp];
		rhiDepthStencil.stencilBack.passOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.passOp];

		// front
		rhiDepthStencil.stencilFront.compare = g_wgpuCompareFunc[pipelineDesc.depthStencil.stencilFront.compareFunc];
		rhiDepthStencil.stencilFront.failOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.failOp];
		rhiDepthStencil.stencilFront.depthFailOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.depthFailOp];
		rhiDepthStencil.stencilFront.passOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.passOp];
		rhiRenderPipelineDesc.depthStencil = &rhiDepthStencil;
	}

	// Setup fragment pipeline
	// Fragment state
	// When opted out, requires rhiDepthStencil state
	WGPUFragmentState rhiFragmentState = {};
	FixedArray<WGPUColorTargetState, MAX_RENDERTARGETS> rhiColorTargets;
	FixedArray<WGPUBlendState, MAX_RENDERTARGETS> rhiColorTargetBlends;
	if(pipelineDesc.fragment.targets.numElem())
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No fragment shader entrypoint set");

		for(const FragmentPipelineDesc::ColorTargetDesc& target : pipelineDesc.fragment.targets)
		{
			{
				WGPUBlendState blend = {};
				blend.color.operation = g_wgpuBlendOp[target.colorBlend.blendFunc];
				blend.color.srcFactor = g_wgpuBlendFactor[target.colorBlend.srcFactor];
				blend.color.dstFactor = g_wgpuBlendFactor[target.colorBlend.dstFactor];

				blend.alpha.operation = g_wgpuBlendOp[target.alphaBlend.blendFunc];
				blend.alpha.srcFactor = g_wgpuBlendFactor[target.alphaBlend.srcFactor];
				blend.alpha.dstFactor = g_wgpuBlendFactor[target.alphaBlend.dstFactor];
				rhiColorTargetBlends.append(blend);
			}

			WGPUColorTargetState rhiColorTarget = {};
			rhiColorTarget.format = g_wgpuTexFormats[target.format];
			rhiColorTarget.blend = &rhiColorTargetBlends.back();
			rhiColorTarget.writeMask = target.writeMask;
			rhiColorTargets.append(rhiColorTarget);
		}

		WGPUShaderModule fragMod = nullptr;
		rhiFragmentState.module = fragMod; // TODO: fetch from cache of fragment modules?
		rhiFragmentState.entryPoint = pipelineDesc.fragment.shaderEntryPoint;
		rhiFragmentState.targetCount = rhiColorTargets.numElem();
		rhiFragmentState.targets = rhiColorTargets.ptr();
		rhiFragmentState.constants = rhiFragmentPipelineConstants.ptr();
		rhiFragmentState.constantCount = rhiFragmentPipelineConstants.numElem();

		rhiRenderPipelineDesc.fragment = &rhiFragmentState;
	}

	// Multisampling
	rhiRenderPipelineDesc.multisample.count = pipelineDesc.multiSample.count;
	rhiRenderPipelineDesc.multisample.mask = pipelineDesc.multiSample.mask;
	rhiRenderPipelineDesc.multisample.alphaToCoverageEnabled = pipelineDesc.multiSample.alphaToCoverage;

	// Primitive toplogy
	rhiRenderPipelineDesc.primitive.frontFace = WGPUFrontFace_CCW; // for now always, TODO
	rhiRenderPipelineDesc.primitive.cullMode = g_wgpuCullMode[pipelineDesc.primitive.cullMode];
	rhiRenderPipelineDesc.primitive.topology = g_wgpuPrimTopology[pipelineDesc.primitive.topology];
	rhiRenderPipelineDesc.primitive.stripIndexFormat = g_wgpuStripIndexFormat[pipelineDesc.primitive.stripIndex];
	rhiRenderPipelineDesc.label = pipelineDesc.name.Length() ? pipelineDesc.name.ToCString() : nullptr;

	WGPURenderPipeline rhiRenderPipeline = wgpuDeviceCreateRenderPipeline(m_rhiDevice, &rhiRenderPipelineDesc);

	if (!rhiRenderPipeline)
		return nullptr;

	CRefPtr<CWGPURenderPipeline> renderPipeline = CRefPtr_new(CWGPURenderPipeline);
	renderPipeline->m_rhiRenderPipeline = rhiRenderPipeline;

	return IGPURenderPipelinePtr(renderPipeline);
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