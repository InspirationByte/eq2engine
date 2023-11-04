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

	Msg("PipelineLayoutDescBuilder dun\n");
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

	Msg("PipelineDescBuilder dun\n");
}

DECLARE_CMD(test_wgpu, nullptr, 0)
{
	PipelineLayoutDescBuilder();
	PipelineDescBuilder();
}

void* CWGPURenderAPI::CreateRenderPipeline(const RenderPipelineLayoutDesc& layoutDesc, const RenderPipelineDesc& pipelineDesc)
{
	// Pipeline layout and bind group layout
	// are objects of CWGPUPipeline
	// There are 3 distinctive bind groups or buffers as our MatSystem design defines:
	//		- Material Constant Properties (static buffer)
	//		- Material Proxy Properties (buffers of these group updated every frame)
	//		- Scene Properties (camera, transform, fog, clip planes)
	WGPUPipelineLayout pipelineLayout = nullptr;
	FixedArray<WGPUBindGroupLayout, 4> bindGroupLayoutList;
	for(const BindGroupLayoutDesc& bindGroupDesc : layoutDesc.bindGroups)
	{
		Array<WGPUBindGroupLayoutEntry> layoutEntry(PP_SL);

		for(const BindGroupLayoutDesc::Entry& entry : bindGroupDesc.entries)
		{
			WGPUBindGroupLayoutEntry bglEntry = {};
			bglEntry.binding = entry.binding;
			bglEntry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
			switch (entry.type)
			{
			case BindGroupLayoutDesc::ENTRY_BUFFER:
				bglEntry.buffer.hasDynamicOffset = entry.buffer.hasDynamicOffset;
				bglEntry.buffer.type = g_wgpuBufferBindingType[entry.buffer.bindType];
				break;
			case BindGroupLayoutDesc::ENTRY_SAMPLER:
				bglEntry.sampler.type = g_wgpuSamplerBindingType[entry.sampler.bindType];
				break;
			case BindGroupLayoutDesc::ENTRY_TEXTURE:
				bglEntry.texture.sampleType = g_wgpuTexSampleType[entry.texture.sampleType];
				bglEntry.texture.viewDimension = g_wgpuTexViewDimensions[entry.texture.dimension];
				bglEntry.texture.multisampled = entry.texture.multisampled;
				break;
			case BindGroupLayoutDesc::ENTRY_STORAGETEXTURE:
				bglEntry.storageTexture.access = g_wgpuStorageTexAccess[entry.storageTexture.access];
				bglEntry.storageTexture.viewDimension = g_wgpuTexViewDimensions[entry.storageTexture.dimension];
				bglEntry.storageTexture.format = g_wgpuTexFormats[entry.storageTexture.format];
				break;
			}
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
	
	// Those are runtime bind groups that have resources bound to them
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

	// pipeline-overridable constants
	Array<WGPUConstantEntry> vertexPipelineConstants(PP_SL);
	Array<WGPUConstantEntry> fragmentPipelineConstants(PP_SL);

	WGPURenderPipelineDescriptor renderPipelineDesc = {};
	renderPipelineDesc.layout = pipelineLayout;

	// Setup vertex pipeline
	// Required
	Array<WGPUVertexAttribute> vertexAttribList(PP_SL);
	Array<WGPUVertexBufferLayout> vertexBufferLayoutList(PP_SL);
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No vertex shader entrypoint set");

		for(const VertexLayoutDesc& vertexLayout : pipelineDesc.vertex.vertexLayout)
		{
			const int firstVertexAttrib = vertexAttribList.numElem();
			for(const VertexLayoutDesc::AttribDesc& attrib : vertexLayout.attributes)
			{
				WGPUVertexAttribute vertAttr = {};
				vertAttr.format = g_wgpuVertexFormats[attrib.format][attrib.count];
				vertAttr.offset = attrib.offset;
				vertAttr.shaderLocation = attrib.location;
				vertexAttribList.append(vertAttr);
			}

			WGPUVertexBufferLayout vertexBufferLayout = {};
			vertexBufferLayout.arrayStride = vertexLayout.stride;
			vertexBufferLayout.attributeCount = vertexAttribList.numElem() - firstVertexAttrib;
			vertexBufferLayout.attributes = &vertexAttribList[firstVertexAttrib];
			vertexBufferLayout.stepMode = g_wgpuVertexStepMode[vertexLayout.stepMode];
			vertexBufferLayoutList.append(vertexBufferLayout);
		}

		WGPUShaderModule vertMod = nullptr; // TODO: retrieve from cache
		renderPipelineDesc.vertex.module = vertMod;
		renderPipelineDesc.vertex.entryPoint = pipelineDesc.vertex.shaderEntryPoint;
		renderPipelineDesc.vertex.bufferCount = vertexBufferLayoutList.numElem();
		renderPipelineDesc.vertex.buffers = vertexBufferLayoutList.ptr();
		renderPipelineDesc.vertex.constants = vertexPipelineConstants.ptr();
		renderPipelineDesc.vertex.constantCount = vertexPipelineConstants.numElem();
	}
	
	// Depth state
	// Optional when depth read = false
	WGPUDepthStencilState depthStencil = {};
	if (pipelineDesc.depthStencil.depthTest)
	{
		ASSERT_MSG(pipelineDesc.depthStencil.format != FORMAT_NONE, "Must set valid depthStencil texture format");
		
		depthStencil.format = g_wgpuTexFormats[pipelineDesc.depthStencil.format];
		depthStencil.depthWriteEnabled = pipelineDesc.depthStencil.depthWrite;
		depthStencil.depthCompare = g_wgpuCompareFunc[pipelineDesc.depthStencil.depthFunc];
		depthStencil.stencilReadMask = pipelineDesc.depthStencil.stencilMask;
		depthStencil.stencilWriteMask = pipelineDesc.depthStencil.stencilWriteMask;
		depthStencil.depthBias = pipelineDesc.depthStencil.depthBias;
		depthStencil.depthBiasSlopeScale = pipelineDesc.depthStencil.depthBiasSlopeScale;
		depthStencil.depthBiasClamp = 0; // TODO

		// back
		depthStencil.stencilBack.compare = g_wgpuCompareFunc[pipelineDesc.depthStencil.stencilBack.compareFunc];
		depthStencil.stencilBack.failOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.failOp];
		depthStencil.stencilBack.depthFailOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.depthFailOp];
		depthStencil.stencilBack.passOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilBack.passOp];

		// front
		depthStencil.stencilFront.compare = g_wgpuCompareFunc[pipelineDesc.depthStencil.stencilFront.compareFunc];
		depthStencil.stencilFront.failOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.failOp];
		depthStencil.stencilFront.depthFailOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.depthFailOp];
		depthStencil.stencilFront.passOp = g_wgpuStencilOp[pipelineDesc.depthStencil.stencilFront.passOp];
		renderPipelineDesc.depthStencil = &depthStencil;
	}

	// Setup fragment pipeline
	// Fragment state
	// When opted out, requires depthStencil state
	WGPUFragmentState fragmentState = {};
	FixedArray<WGPUColorTargetState, MAX_RENDERTARGETS> colorTargets;
	FixedArray<WGPUBlendState, MAX_RENDERTARGETS> colorTargetBlends;
	if(pipelineDesc.fragment.targets.numElem())
	{
		ASSERT_MSG(pipelineDesc.vertex.shaderEntryPoint.Length(), "No fragment shader entrypoint set");

		// TODO: convert this
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
				colorTargetBlends.append(blend);
			}

			WGPUColorTargetState colorTarget = {};
			colorTarget.format = g_wgpuTexFormats[target.format];
			colorTarget.blend = &colorTargetBlends.back();
			colorTarget.writeMask = target.writeMask;
			colorTargets.append(colorTarget);
		}

		WGPUShaderModule fragMod = nullptr;
		fragmentState.module = fragMod; // TODO: fetch from cache of fragment modules?
		fragmentState.entryPoint = pipelineDesc.fragment.shaderEntryPoint;
		fragmentState.targetCount = colorTargets.numElem();
		fragmentState.targets = colorTargets.ptr();
		fragmentState.constants = fragmentPipelineConstants.ptr();
		fragmentState.constantCount = fragmentPipelineConstants.numElem();

		renderPipelineDesc.fragment = &fragmentState;
	}

	// Multisampling
	renderPipelineDesc.multisample.count = pipelineDesc.multiSample.count;
	renderPipelineDesc.multisample.mask = pipelineDesc.multiSample.mask;
	renderPipelineDesc.multisample.alphaToCoverageEnabled = pipelineDesc.multiSample.alphaToCoverage;

	// Primitive toplogy
	renderPipelineDesc.primitive.frontFace = WGPUFrontFace_CCW; // for now always, TODO
	renderPipelineDesc.primitive.cullMode = g_wgpuCullMode[pipelineDesc.primitive.cullMode];
	renderPipelineDesc.primitive.topology = g_wgpuPrimTopology[pipelineDesc.primitive.topology];
	renderPipelineDesc.primitive.stripIndexFormat = g_wgpuStripIndexFormat[pipelineDesc.primitive.stripIndex];
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