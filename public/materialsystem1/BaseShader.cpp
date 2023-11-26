//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
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

IGPUBindGroupPtr CBaseShader::GetEmptyBindGroup(EBindGroupId bindGroupId, IShaderAPI* renderAPI) const
{
	// create empty bind group
	if(!m_emptyBindGroup[bindGroupId])
	{
		BindGroupDesc emptyBindGroupDesc;
		m_emptyBindGroup[bindGroupId] = renderAPI->CreateBindGroup(GetPipelineLayout(renderAPI), bindGroupId, emptyBindGroupDesc);
	}
	return m_emptyBindGroup[bindGroupId];
}

IGPUPipelineLayoutPtr CBaseShader::GetPipelineLayout(IShaderAPI* renderAPI) const
{
	if (!m_pipelineLayout)
	{
		PipelineLayoutDesc pipelineLayoutDesc;
		FillPipelineLayoutDesc(pipelineLayoutDesc);
		m_pipelineLayout = renderAPI->CreatePipelineLayout(pipelineLayoutDesc);
	}
	return m_pipelineLayout;
}

void CBaseShader::FillPipelineLayoutDesc(PipelineLayoutDesc& renderPipelineLayoutDesc) const
{
	// MatSystem shader defines three bind groups, which differ by the lifetime:
	// 
	//	BINDGROUP_CONSTANT
	//		- Bind group data is never going to be changed during the life time of the material
	//	BINDGROUP_RENDERPASS
	//		- Bind group persists across single render pass
	//	BINDGROUP_TRANSIENT
	//		- Bind group is unique for each draw call

	FillBindGroupLayout_Constant(renderPipelineLayoutDesc.bindGroups.append());
	FillBindGroupLayout_RenderPass(renderPipelineLayoutDesc.bindGroups.append());
	FillBindGroupLayout_Transient(renderPipelineLayoutDesc.bindGroups.append());
}

uint CBaseShader::GetRenderPipelineId(const IGPURenderPassRecorder* renderPass, int vertexLayoutNameHash, int vertexLayoutUsedBufferBits, EPrimTopology primitiveTopology) const
{
	uint hash = vertexLayoutNameHash | (vertexLayoutUsedBufferBits << 24);
	hash *= 31;
	hash += static_cast<uint>(primitiveTopology);
	for (int i = 0; i < MAX_RENDERTARGETS; ++i)
	{
		const ETextureFormat format = renderPass->GetRenderTargetFormat(i);
		if (format == FORMAT_NONE)
			break;
		hash *= 31;
		hash += static_cast<uint>(i);
		hash *= 31;
		hash += static_cast<uint>(format);
	}
	const ETextureFormat depthTargetFormat = renderPass->GetDepthTargetFormat();
	hash *= 31;
	hash += static_cast<uint>(depthTargetFormat);
	return hash;
}

void CBaseShader::FillRenderPipelineDesc(const IGPURenderPassRecorder* renderPass, const MeshInstanceFormatRef& meshInstFormat, int vertexLayoutUsedBufferBits, EPrimTopology primitiveTopology, RenderPipelineDesc& renderPipelineDesc) const
{
	Builder<RenderPipelineDesc>(renderPipelineDesc)
		.ShaderName(GetName())
		.PrimitiveState(Builder<PrimitiveDesc>()
			.Topology(primitiveTopology)
			.Cull((m_flags & MATERIAL_FLAG_NO_CULL) ? CULL_NONE : CULL_BACK) // TODO: variant
			.StripIndex(primitiveTopology == PRIM_TRIANGLE_STRIP ? STRIPINDEX_UINT16 : STRIPINDEX_NONE) // TODO: variant
			.End())
		.End();

	if (meshInstFormat.layout.numElem())
	{
		renderPipelineDesc.shaderVertexLayoutId = meshInstFormat.nameHash;

		Builder<VertexPipelineDesc> vertexPipelineBuilder(renderPipelineDesc.vertex);
		ArrayCRef<VertexLayoutDesc> vertexLayouts = meshInstFormat.layout;
		for (int i = 0; i < vertexLayouts.numElem(); ++i)
		{
			const VertexLayoutDesc& layoutDesc = vertexLayouts[i];
			if (vertexLayoutUsedBufferBits & (1 << i))
				vertexPipelineBuilder.VertexLayout(layoutDesc);
		}
	}

	// setup render & shadowing parameters
	const ETextureFormat depthTargetFormat = renderPass->GetDepthTargetFormat();
	if (depthTargetFormat != FORMAT_NONE)
	{
		if (m_flags & MATERIAL_FLAG_DECAL)
			g_matSystem->GetPolyOffsetSettings(renderPipelineDesc.depthStencil.depthBias, renderPipelineDesc.depthStencil.depthBiasSlopeScale);

		Builder<DepthStencilStateParams>(renderPipelineDesc.depthStencil)
			.DepthTestOn(!(m_flags & MATERIAL_FLAG_NO_Z_TEST))
			.DepthWriteOn((m_flags & MATERIAL_FLAG_NO_Z_WRITE) == 0)
			.DepthFormat(depthTargetFormat);
	}

	if (!(m_flags & MATERIAL_FLAG_ONLY_Z))
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

		Builder<FragmentPipelineDesc> pipelineBuilder(renderPipelineDesc.fragment);
		for (int i = 0; i < MAX_RENDERTARGETS; ++i)
		{
			const ETextureFormat format = renderPass->GetRenderTargetFormat(i);
			if (format == FORMAT_NONE)
				break;
			pipelineBuilder.ColorTarget("CT", format, colorBlend, alphaBlend);
		}
		pipelineBuilder.End();
	}
}

IGPURenderPipelinePtr CBaseShader::GetRenderPipeline(IShaderAPI* renderAPI, const IGPURenderPassRecorder* renderPass, const MeshInstanceFormatRef& meshInstFormat, int vertexLayoutUsedBufferBits, EPrimTopology primitiveTopology, const void* userData) const
{
	// TODO: int vertexLayoutNameHash, int vertexLayoutUsedBufferBits

	const uint pipelineId = GetRenderPipelineId(renderPass, meshInstFormat.nameHash, vertexLayoutUsedBufferBits, primitiveTopology);
	auto it = m_renderPipelines.find(pipelineId);
	if (it.atEnd())
	{
		RenderPipelineDesc renderPipelineDesc;
		FillRenderPipelineDesc(renderPass, meshInstFormat, vertexLayoutUsedBufferBits, primitiveTopology, renderPipelineDesc);
		BuildPipelineShaderQuery(meshInstFormat, vertexLayoutUsedBufferBits, renderPipelineDesc.shaderQuery);
		it = m_renderPipelines.insert(pipelineId, renderAPI->CreateRenderPipeline(GetPipelineLayout(renderAPI), renderPipelineDesc));
	}
	return *it;
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

void CBaseShader::InitShader(IShaderAPI* renderAPI)
{
	m_isInit = true;
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

MatTextureProxy CBaseShader::FindTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar)
{
	MatStringProxy mv = FindMaterialVar(paramName);
	if(mv.IsValid()) 
	{
		AddManagedTexture(mv, renderAPI->FindTexture(mv.Get()));
	}
	else if(errorTextureIfNoVar)
		AddManagedTexture(mv, g_matSystem->GetErrorCheckerboardTexture());

	return mv;
}

MatTextureProxy CBaseShader::LoadTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar)
{
	MatStringProxy mv = FindMaterialVar(paramName);

	if(mv.IsValid()) 
	{
		if(mv.Get().Length())
			AddManagedTexture(MatTextureProxy(mv), g_texLoader->LoadTextureFromFileSync(mv.Get(), SamplerStateParams(m_texFilter, m_texAddressMode)));
	}
	else if(errorTextureIfNoVar)
		AddManagedTexture(MatTextureProxy(mv), g_matSystem->GetErrorCheckerboardTexture());

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
