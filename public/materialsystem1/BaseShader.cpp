//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"
#include "utils/TextureAtlas.h"

#include "IMaterialSystem.h"
#include "BaseShader.h"
#include "materialsystem1/ITextureLoader.h"

struct FilterTypeString
{
	const char*		name;
	ETexFilterMode	type;
};

static const FilterTypeString s_textureFilterTypes[] = {
	{ "nearest",	TEXFILTER_NEAREST },
	{ "linear",		TEXFILTER_LINEAR },
	{ "bilinear",	TEXFILTER_BILINEAR },
	{ "trilinear",	TEXFILTER_TRILINEAR },
	{ "aniso",		TEXFILTER_TRILINEAR_ANISO },
};

static ETexFilterMode ResolveFilterType(const char* string)
{
	for(const FilterTypeString& ft: s_textureFilterTypes)
	{
		if(!stricmp(string, ft.name))
			return ft.type;
	}
	return TEXFILTER_TRILINEAR_ANISO;
}

static ETexAddressMode ResolveAddressType(const char* string)
{
	if(!stricmp(string,"wrap"))
		return TEXADDRESS_WRAP;
	else if (!stricmp(string, "mirror"))
		return TEXADDRESS_MIRROR;

	return TEXADDRESS_CLAMP;
}

CBaseShader::CBaseShader()
{
}

void CBaseShader::Init(IShaderAPI* renderAPI, IMaterial* material)
{
	m_material = material;

	// required
	m_baseTextureTransformVar = m_material->GetMaterialVar("BaseTextureTransform", "[0 0]");
	m_baseTextureScaleVar = m_material->GetMaterialVar("BaseTextureScale", "[1 1]");
	m_baseTextureFrame = m_material->GetMaterialVar("BaseTextureFrame", "0");

	// resolve address type and filtering mode
	MatStringProxy addressMode = m_material->FindMaterialVar("Address");
	if(addressMode.IsValid()) m_texAddressMode = ResolveAddressType(addressMode.Get());

	MatStringProxy texFilter = m_material->FindMaterialVar("Filtering");
	if(texFilter.IsValid()) m_texFilter = ResolveFilterType(texFilter.Get());

	int materialFlags = 0;
	SHADER_PARAM_FLAG(NoDraw, materialFlags, MATERIAL_FLAG_INVISIBLE, false)
	SHADER_PARAM_FLAG(ReceiveShadows, materialFlags, MATERIAL_FLAG_RECEIVESHADOWS, true)
	SHADER_PARAM_FLAG(CastShadows, materialFlags, MATERIAL_FLAG_CASTSHADOWS, true)
	SHADER_PARAM_FLAG(Wireframe, materialFlags, MATERIAL_FLAG_WIREFRAME, false)
	SHADER_PARAM_FLAG(Decal, materialFlags, MATERIAL_FLAG_DECAL, false)
	SHADER_PARAM_FLAG(NoCull, materialFlags, MATERIAL_FLAG_NO_CULL, false)
	SHADER_PARAM_FLAG(AlphaTest, materialFlags, MATERIAL_FLAG_ALPHATESTED, false)
	SHADER_PARAM_FLAG(ZOnly, materialFlags, MATERIAL_FLAG_ONLY_Z, false)
	SHADER_PARAM_FLAG_NEG(ZTest, materialFlags, MATERIAL_FLAG_NO_Z_TEST, false)
	SHADER_PARAM_FLAG_NEG(ZWrite, materialFlags, MATERIAL_FLAG_NO_Z_WRITE, false)

	EShaderBlendMode blendMode = SHADER_BLEND_NONE;
	SHADER_PARAM_ENUM(Translucent, blendMode, EShaderBlendMode, SHADER_BLEND_TRANSLUCENT)
	SHADER_PARAM_ENUM(Additive, blendMode, EShaderBlendMode, SHADER_BLEND_ADDITIVE)
	SHADER_PARAM_ENUM(Modulate, blendMode, EShaderBlendMode, SHADER_BLEND_MODULATE)

	if (blendMode != SHADER_BLEND_NONE)
		materialFlags |= MATERIAL_FLAG_TRANSPARENT | MATERIAL_FLAG_NO_Z_WRITE;

	m_flags = materialFlags;
	m_blendMode = blendMode;
}

IGPUBindGroupPtr CBaseShader::CreateBindGroup(BindGroupDesc& bindGroupDesc, EBindGroupId bindGroupId, IShaderAPI* renderAPI, const PipelineInfo& pipelineInfo) const
{
	static const char* s_bindGroupNames[] = {
		"Constant",
		"RenderPass",
		"Transient",
	};

	bindGroupDesc.groupIdx = bindGroupId;
	if(pipelineInfo.layout)
	{
		bindGroupDesc.name = EqString::Format("%s-%s-ShaderLayout", GetName(), s_bindGroupNames[bindGroupId]);
		return renderAPI->CreateBindGroup(pipelineInfo.layout, bindGroupDesc);
	}

	bindGroupDesc.name = EqString::Format("%s-%s-PipelineLayout", GetName(), s_bindGroupNames[bindGroupId]);
	return renderAPI->CreateBindGroup(pipelineInfo.pipeline, bindGroupDesc);
}

IGPUBindGroupPtr CBaseShader::GetEmptyBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const PipelineInfo& pipelineInfo) const
{
	if (!pipelineInfo.layout)
		return nullptr;

	// create empty bind group
	if (!pipelineInfo.emptyBindGroup[bindGroupId])
	{
		BindGroupDesc emptyBindGroupDesc;
		emptyBindGroupDesc.groupIdx = bindGroupId;
		pipelineInfo.emptyBindGroup[bindGroupId] = renderAPI->CreateBindGroup(pipelineInfo.layout, emptyBindGroupDesc);
	}
	return pipelineInfo.emptyBindGroup[bindGroupId];
}

uint CBaseShader::GetRenderPipelineId(ArrayCRef<ETextureFormat> colorTargetFormat, ETextureFormat depthTargetFormat, const MeshInstanceFormatRef& meshInstFormat, EPrimTopology primitiveTopology) const
{
	uint hash = meshInstFormat.formatId | (meshInstFormat.usedLayoutBits << 24);
	hash *= 31;
	hash += m_shaderQueryId;
	hash *= 31;
	hash += static_cast<uint>(primitiveTopology);
	hash *= 31;
	hash += m_flags;
	hash *= 31;
	hash += m_blendMode;
	for (int i = 0; i < colorTargetFormat.numElem(); ++i)
	{
		const ETextureFormat format = colorTargetFormat[i];
		if (format == FORMAT_NONE)
			break;
		hash *= 31;
		hash += static_cast<uint>(i);
		hash *= 31;
		hash += static_cast<uint>(format);
	}
	hash *= 31;
	hash += static_cast<uint>(depthTargetFormat);
	return hash;
}

void CBaseShader::FillRenderPipelineDesc(ArrayCRef<ETextureFormat> colorTargetFormat, ETextureFormat depthTargetFormat, const MeshInstanceFormatRef& meshInstFormat, EPrimTopology primitiveTopology, RenderPipelineDesc& renderPipelineDesc) const
{
	Builder<RenderPipelineDesc>(renderPipelineDesc)
		.ShaderName(GetName())
		.ShaderQuery(m_shaderQuery)
		.PrimitiveState(Builder<PrimitiveDesc>()
			.Topology(primitiveTopology)
			.Cull((m_flags & MATERIAL_FLAG_NO_CULL) ? CULL_NONE : CULL_BACK) // TODO: variant
			.StripIndex(primitiveTopology == PRIM_TRIANGLE_STRIP ? STRIPINDEX_UINT16 : STRIPINDEX_NONE) // TODO: variant
			.End())
		.End();

	if (meshInstFormat.layout.numElem())
	{
		renderPipelineDesc.shaderVertexLayoutId = meshInstFormat.formatId;

		Builder<VertexPipelineDesc> vertexPipelineBuilder(renderPipelineDesc.vertex);
		ArrayCRef<VertexLayoutDesc> vertexLayouts = meshInstFormat.layout;
		for (int i = 0; i < vertexLayouts.numElem(); ++i)
		{
			const VertexLayoutDesc& layoutDesc = vertexLayouts[i];

			if (layoutDesc.stepMode == VERTEX_STEPMODE_INSTANCE)
			{
				ASSERT_MSG(meshInstFormat.usedLayoutBits & (1 << i), "Instance buffer must be bound");
			}

			if (meshInstFormat.usedLayoutBits & (1 << i))
				vertexPipelineBuilder.VertexLayout(layoutDesc);
		}
	}

	// setup render & shadowing parameters
	if (depthTargetFormat != FORMAT_NONE)
	{
		if (m_flags & MATERIAL_FLAG_DECAL)
			g_matSystem->GetPolyOffsetSettings(renderPipelineDesc.depthStencil.depthBias, renderPipelineDesc.depthStencil.depthBiasSlopeScale);

		Builder<DepthStencilStateParams>(renderPipelineDesc.depthStencil)
			.DepthTestOn(!(m_flags & MATERIAL_FLAG_NO_Z_TEST))
			.DepthWriteOn((m_flags & MATERIAL_FLAG_NO_Z_WRITE) == 0)
			.DepthFormat(depthTargetFormat);
	}

	// TODO: 
	//renderPipelineDesc.multiSample.count = renderPass->GetSampleCount();

	const bool onlyZ = (m_flags & MATERIAL_FLAG_ONLY_Z);
	Builder<FragmentPipelineDesc> pipelineBuilder(renderPipelineDesc.fragment);
	if (onlyZ)
	{
		// leave empty entry point to disable fragment pipeline
		pipelineBuilder.ShaderEntry("");
	}
	else
	{
		BlendStateParams colorBlend;
		BlendStateParams alphaBlend;
		switch (m_blendMode)
		{
		case SHADER_BLEND_TRANSLUCENT:
			colorBlend = BlendStateTranslucent;
			alphaBlend = BlendStateTranslucentAlpha;
			break;
		case SHADER_BLEND_ADDITIVE:
			colorBlend = BlendStateAdditive;
			alphaBlend = BlendStateAdditive;
			break;
		case SHADER_BLEND_MODULATE:
			colorBlend = BlendStateModulate;
			alphaBlend = BlendStateModulate; // TODO: figure out
			break;
		}

		for (int i = 0; i < colorTargetFormat.numElem(); ++i)
		{
			const ETextureFormat format = colorTargetFormat[i];
			if (format == FORMAT_NONE)
				break;

			if(m_blendMode != SHADER_BLEND_NONE)
				pipelineBuilder.ColorTarget("CT", format, colorBlend, alphaBlend);
			else
				pipelineBuilder.ColorTarget("CT", format);
		}
	}
	pipelineBuilder.End();
}

const CBaseShader::PipelineInfo& CBaseShader::EnsureRenderPipeline(IShaderAPI* renderAPI, ArrayCRef<ETextureFormat> colorTargetFormat, ETextureFormat depthTargetFormat, const MeshInstanceFormatRef& meshInstFormat, EPrimTopology primTopology, bool onInit)
{
	HOOK_TO_CVAR(r_showPipelineCacheMisses);

	const uint pipelineId = GetRenderPipelineId(colorTargetFormat, depthTargetFormat, meshInstFormat, primTopology);

	auto it = m_renderPipelines.find(pipelineId);
	if (it.atEnd())
	{
		it = m_renderPipelines.insert(pipelineId);
		PipelineInfo& newPipelineInfo = *it;
		newPipelineInfo.vertexLayoutId = meshInstFormat.formatId;

		// Create pipeline layout
		{
			// MatSystem shader by default defines three bind groups, which differ by the lifetime:
			// 
			//	BINDGROUP_CONSTANT
			//		- Bind group data is never going to be changed during the life time of the material
			//	BINDGROUP_RENDERPASS
			//		- Bind group persists across single render pass
			//	BINDGROUP_TRANSIENT
			//		- Bind group is unique for each draw call
			//
			// you're not obligated to use all of them

			PipelineLayoutDesc pipelineLayoutDesc;
			pipelineLayoutDesc.name = GetName();

			FillBindGroupLayout_Constant(meshInstFormat, pipelineLayoutDesc.bindGroups.append());
			FillBindGroupLayout_RenderPass(meshInstFormat, pipelineLayoutDesc.bindGroups.append());
			FillBindGroupLayout_Transient(meshInstFormat, pipelineLayoutDesc.bindGroups.append());

			bool userDefinedPipelineLayout = false;
			for (const BindGroupLayoutDesc& layout : pipelineLayoutDesc.bindGroups)
			{
				if (layout.entries.numElem() > 0)
				{
					userDefinedPipelineLayout = true;
					break;
				}
			}

			if (userDefinedPipelineLayout)
				newPipelineInfo.layout = renderAPI->CreatePipelineLayout(pipelineLayoutDesc);
		}

		MatSysShaderPipelineCache& shaderPipelineCache = g_matSystem->GetRenderPipelineCache(GetNameHash());
		auto cacheIt = shaderPipelineCache.pipelines.find(pipelineId);
		if (cacheIt.atEnd())
		{
			RenderPipelineDesc renderPipelineDesc;
			FillRenderPipelineDesc(colorTargetFormat, depthTargetFormat, meshInstFormat, primTopology, renderPipelineDesc);

			IGPURenderPipelinePtr renderPipeline = renderAPI->CreateRenderPipeline(renderPipelineDesc, newPipelineInfo.layout);
			if (!renderPipeline)
			{
				ASSERT_FAIL("Shader %s is unable to create pipeline", GetName());
			}

			cacheIt = shaderPipelineCache.pipelines.insert(pipelineId, renderPipeline);

#ifndef _RETAIL

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

			if (renderPipeline && r_showPipelineCacheMisses->GetBool())
			{
				if (onInit)
				{
					MsgInfo("Pre-create pipeline %s-%s_" BYTE_TO_BINARY_PATTERN "-PT%d-DF%d-CF%d\n", GetName(), meshInstFormat.name, BYTE_TO_BINARY(meshInstFormat.usedLayoutBits), primTopology, depthTargetFormat, colorTargetFormat[0]);
				}
				else
				{
					MsgWarning("Render pipeline %s-%s_" BYTE_TO_BINARY_PATTERN "-PT%d-DF%d-CF%d was not pre-created\n", GetName(), meshInstFormat.name, BYTE_TO_BINARY(meshInstFormat.usedLayoutBits), primTopology, depthTargetFormat, colorTargetFormat[0]);
				}
			}
		}
#endif // !_RETAIL

		newPipelineInfo.pipeline = *cacheIt;
	}

	return *it;
}

bool CBaseShader::SetupRenderPass(IShaderAPI* renderAPI, const MeshInstanceFormatRef& meshInstFormat, EPrimTopology primTopology, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext)
{
	const PipelineInfo& pipelineInfo = EnsureRenderPipeline(renderAPI, passContext.recorder->GetRenderTargetFormats(), passContext.recorder->GetDepthTargetFormat(), meshInstFormat, primTopology, false);

	if (!pipelineInfo.pipeline)
		return false;

	passContext.recorder->SetPipeline(pipelineInfo.pipeline);
	passContext.recorder->SetBindGroup(BINDGROUP_CONSTANT, GetBindGroup(renderAPI, BINDGROUP_CONSTANT, pipelineInfo, uniformBuffers, passContext));
	passContext.recorder->SetBindGroup(BINDGROUP_RENDERPASS, GetBindGroup(renderAPI, BINDGROUP_RENDERPASS, pipelineInfo, uniformBuffers, passContext));
	passContext.recorder->SetBindGroup(BINDGROUP_TRANSIENT, GetBindGroup(renderAPI, BINDGROUP_TRANSIENT, pipelineInfo, uniformBuffers, passContext));

	return true;
}

void CBaseShader::FillBindGroupLayout_Constant_Samplers(BindGroupLayoutDesc& bindGroupLayout) const
{
	Builder<BindGroupLayoutDesc>(bindGroupLayout)
		.Sampler("MaterialSampler", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
		.Sampler("LinearMirrorWrapSampler", 1, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
		.Sampler("LinearClampSampler", 2, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
		.Sampler("NearestSampler", 3, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING);
}

void CBaseShader::FillBindGroup_Constant_Samplers(BindGroupDesc& bindGroupDesc) const
{
	Builder<BindGroupDesc>(bindGroupDesc)
		.Sampler(0, SamplerStateParams(m_texFilter, m_texAddressMode))
		.Sampler(1, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_MIRROR))
		.Sampler(2, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
		.Sampler(3, SamplerStateParams(TEXFILTER_NEAREST, TEXADDRESS_CLAMP));
}

IGPUBufferPtr CBaseShader::CreateAtlasBuffer(IShaderAPI* renderAPI) const
{
	struct {
		int entryCount{ 0 };
		int _pad[3];
		Vector4D entries[64]{ 0 };
	} atlasBufferData;

	// add default atlas entry
	atlasBufferData.entries[0] = Vector4D(1.0f, 1.0f, 0.0f, 0.0f);

	const CTextureAtlas* atlas = m_material->GetAtlas();
	if (atlas)
	{
		for (int i = 0; i < atlas->GetEntryCount(); ++i)
		{
			const AtlasEntry& entry = *atlas->GetEntry(i);
			atlasBufferData.entries[1 + atlasBufferData.entryCount++] = Vector4D(entry.rect.GetSize(), entry.rect.leftTop);
		}
	}
	return renderAPI->CreateBuffer(BufferInfo(reinterpret_cast<ubyte*>(&atlasBufferData), sizeof(int) * 4 + sizeof(Vector4D) * (1 + atlasBufferData.entryCount)), BUFFERUSAGE_STORAGE, "atlasRects");
}

ArrayCRef<int> CBaseShader::GetSupportedVertexLayoutIds() const
{
	static const int defaultVertexLayoutId = 0;
	return ArrayCRef(&defaultVertexLayoutId, 1);
}

void CBaseShader::InitShader(IShaderAPI* renderAPI)
{
	if (m_isInit)
		return;

	m_isInit = true;

	BuildPipelineShaderQuery(m_shaderQuery);
	
	EqString queryStr;
	for (const EqString& define : m_shaderQuery)
	{
		if (queryStr.Length())
			queryStr.Append("|");
		queryStr.Append(define);
	}
	m_shaderQueryId = StringToHash(queryStr, true);

	// cache shader modules
	renderAPI->LoadShaderModules(GetName(), m_shaderQuery);

	// TODO: more RT format variants
	ETextureFormat rtFormat = FORMAT_RGBA8;
	ETextureFormat depthFormat = g_matSystem->GetDefaultDepthBuffer()->GetFormat();

	// cache pipeline state objects
	ArrayCRef<int> supportedLayoutIds = GetSupportedVertexLayoutIds();
	for (const int layoutId : supportedLayoutIds)
	{
		const IVertexFormat* vertFormat = renderAPI->FindVertexFormatById(layoutId);
		if (!vertFormat)
			continue;

		MeshInstanceFormatRef instFormat;
		instFormat.name = vertFormat->GetName();
		instFormat.formatId = vertFormat->GetNameHash();
		instFormat.layout = vertFormat->GetFormatDesc();

		uint layoutMask = 0;
		for (int i = 0; i < instFormat.layout.numElem(); ++i)
			layoutMask |= (1 << i);

		instFormat.usedLayoutBits &= layoutMask;

		// TODO: different variants of instFormat.usedLayoutBits
		EnsureRenderPipeline(renderAPI, ArrayCRef(&rtFormat, 1), depthFormat, instFormat, PRIM_TRIANGLES, true);
		EnsureRenderPipeline(renderAPI, ArrayCRef(&rtFormat, 1), depthFormat, instFormat, PRIM_TRIANGLE_STRIP, true);
	}
}

// Unload shaders, textures
void CBaseShader::Unload()
{
	m_renderPipelines.clear(true);
	for (int i = 0; i < m_usedTextures.numElem(); ++i)
	{
		MatTextureProxy& texProxy = m_usedTextures[i];
		texProxy.Set(nullptr);
	}
	m_usedTextures.clear(true);
}

MatVarProxyUnk CBaseShader::FindMaterialVar(const char* paramName, bool allowGlobals) const
{
	MatStringProxy mv;

	// editor parameters are separate prefixed ones
	// this allows to have values of parameters that are represented in the game while alter rendering in editor.
	if (g_matSystem->GetConfiguration().editormode)
		mv = m_material->FindMaterialVar(EqString::Format("editor.%s", paramName));

	if (!mv.IsValid())
		mv = m_material->FindMaterialVar(paramName);

	// check if we want to use material system global var under that name
	// NOTE: they have to be valid always in order to link proxies
	if (allowGlobals && mv.IsValid() && *mv.Get() == '$')
		mv = g_matSystem->GetGlobalMaterialVarByName(((const char*)mv.Get())+1);

	return mv;
}

MatTextureProxy CBaseShader::FindTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar, int texFlags)
{
	MatStringProxy mv = FindMaterialVar(paramName);
	if(mv.IsValid()) 
	{
		ITexturePtr texture = renderAPI->FindTexture(mv.Get());
		if (texture)
		{
			ASSERT_MSG((texture->GetFlags() & texFlags) == texFlags, "MatVar '%s' texture '%s' doesn't match required flags", paramName, texture->GetName());
		}
		AddManagedTexture(mv, texture);
	}
	else if(errorTextureIfNoVar)
		AddManagedTexture(mv, g_matSystem->GetErrorCheckerboardTexture((texFlags & TEXFLAG_CUBEMAP) ? TEXDIMENSION_CUBE : TEXDIMENSION_2D));

	return mv;
}

MatTextureProxy CBaseShader::LoadTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar, int texFlags)
{
	MatStringProxy mv = FindMaterialVar(paramName);

	if(mv.IsValid()) 
	{
		if(mv.Get().Length())
		{
			ITexturePtr texture = g_texLoader->LoadTextureFromFileSync(mv.Get(), SamplerStateParams(m_texFilter, m_texAddressMode), texFlags, EqString::Format("Material %s var '%s'", m_material->GetName(), paramName));
			if (texture)
			{
				ASSERT_MSG((texture->GetFlags() & texFlags) == texFlags, "MatVar '%s' texture '%s' doesn't match required flags", paramName, texture->GetName());
			}
			AddManagedTexture(MatTextureProxy(mv), texture);
		}
	}
	else if(errorTextureIfNoVar)
		AddManagedTexture(MatTextureProxy(mv), g_matSystem->GetErrorCheckerboardTexture((texFlags & TEXFLAG_CUBEMAP) ? TEXDIMENSION_CUBE : TEXDIMENSION_2D));

	return mv;
}

// get texture transformation from vars
Vector4D CBaseShader::GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const
{
	if (transformVar.IsValid() && scaleVar.IsValid())
		return Vector4D(scaleVar.Get(), transformVar.Get());

	return Vector4D(1, 1, 0, 0);
}

void CBaseShader::AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex)
{
	if (!tex)
		return;

	var.Set(tex);
	m_usedTextures.append(var);
}
