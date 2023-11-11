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

	// DEPRECATED Init function table
	for (int i = 0; i < SHADERPARAM_COUNT; i++)
		SetParameterFunctor(i, &CBaseShader::ParamSetup_Empty);

	if (blendMode == SHADER_BLEND_TRANSLUCENT)
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Translucent);
	else if (blendMode == SHADER_BLEND_ADDITIVE)
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Additive);
	else if (blendMode == SHADER_BLEND_MODULATE)
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Modulate);
	else
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Solid);

	if (materialFlags & MATERIAL_FLAG_NO_CULL)
		SetParameterFunctor(SHADERPARAM_RASTERSETUP, &CBaseShader::ParamSetup_RasterState_NoCull);
	else
		SetParameterFunctor(SHADERPARAM_RASTERSETUP, &CBaseShader::ParamSetup_RasterState);

	SetParameterFunctor(SHADERPARAM_TRANSFORM, &CBaseShader::ParamSetup_Transform);
	SetParameterFunctor(SHADERPARAM_DEPTHSETUP, &CBaseShader::ParamSetup_DepthSetup);
	SetParameterFunctor(SHADERPARAM_FOG, &CBaseShader::ParamSetup_Fog);
	SetParameterFunctor(SHADERPARAM_BONETRANSFORMS, &CBaseShader::ParamSetup_BoneTransforms);
}

void CBaseShader::FillPipelineLayoutDesc(PipelineLayoutDesc& renderPipelineLayoutDesc) const
{
	Builder<PipelineLayoutDesc> builder(renderPipelineLayoutDesc);

	// matsystem bindgroup is always first
	builder.Group(
		Builder<BindGroupLayoutDesc>()
		.Buffer("camera", 0, SHADERKIND_FRAGMENT | SHADERKIND_VERTEX, BUFFERBIND_UNIFORM)
		.End()
	);
	FillShaderBindGroupLayout(renderPipelineLayoutDesc.bindGroups.append());
}

void CBaseShader::FillRenderPipelineDesc(RenderPipelineDesc& renderPipelineDesc) const
{
	// setup render & shadowing parameters
	if (!(m_flags & MATERIAL_FLAG_NO_Z_TEST))
	{
		if (m_flags & MATERIAL_FLAG_DECAL)
			g_matSystem->GetPolyOffsetSettings(renderPipelineDesc.depthStencil.depthBias, renderPipelineDesc.depthStencil.depthBiasSlopeScale);

		Builder<DepthStencilStateParams>(renderPipelineDesc.depthStencil)
			.DepthTestOn()
			.DepthWriteOn((m_flags & MATERIAL_FLAG_NO_Z_WRITE) == 0)
			.DepthFormat(g_matSystem->GetBackBufferDepthFormat());
	}

	if (!(m_flags & MATERIAL_FLAG_ONLY_Z))
	{
		BlendStateParams colorBlend;
		BlendStateParams alphaBlend;
		switch (m_blendMode)
		{
		case SHADER_BLEND_TRANSLUCENT:
			colorBlend = BlendStateAlpha;
			alphaBlend = BlendStateAlpha;
			break;
		case SHADER_BLEND_ADDITIVE:
			colorBlend = BlendStateAdditive;
			alphaBlend = BlendStateAdditive;
			break;
		case SHADER_BLEND_MODULATE:
			colorBlend = BlendStateModulate;
			alphaBlend = BlendStateModulate;
			break;
		}

		// TODO: number of rendertargets and their formats
		// and their types should come from render pass defined by shader!!!
		// Builder<FragmentPipelineDesc>(renderPipelineDesc.fragment)
		//	.ColorTarget("Albedo", FORMAT_RGBA8, colorBlend, alphaBlend);
	}
}

void CBaseShader::InitShader(IShaderAPI* renderAPI)
{
	// And then init shaders
	if( InitRenderPassPipeline(renderAPI) )
		m_isInit = true;
	else
		m_error = true;
}

// Unload shaders, textures
void CBaseShader::Unload()
{
	for(int i = 0; i < m_usedPrograms.numElem(); ++i)
		*m_usedPrograms[i] = nullptr;
	m_usedPrograms.clear(true);

	for (int i = 0; i < m_usedTextures.numElem(); ++i)
	{
		MatTextureProxy& texProxy = m_usedTextures[i];
		texProxy.Set(nullptr);
	}
	m_usedTextures.clear(true);

	m_isInit = false;
	m_error = false;
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
		SamplerStateParams samplerParams((ETexFilterMode)m_texFilter, (ETexAddressMode)m_texAddressMode);

		if(mv.Get().Length())
			AddManagedTexture(MatTextureProxy(mv), g_texLoader->LoadTextureFromFileSync(mv.Get(), samplerParams));
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

void CBaseShader::AddManagedShader(IShaderProgramPtr* pShader)
{
	if (!*pShader)
		return;
	m_usedPrograms.append(pShader);
}

void CBaseShader::AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex)
{
	if (!tex)
		return;

	var.Set(tex);
	m_usedTextures.append(var);
}

// DEPRECATED BELOW

void CBaseShader::SetupParameter(IShaderAPI* renderAPI, uint mask, EShaderParamSetup type)
{
	// call it from this
	if (mask & (1 << (uint)type))
		(this->*m_paramFunc[type]) (renderAPI);
}

void CBaseShader::ParamSetup_AlphaModel_Solid(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates( BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDFUNC_ADD );
}

void CBaseShader::ParamSetup_AlphaModel_Translucent(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD);
}

void CBaseShader::ParamSetup_AlphaModel_Additive(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);
}

void CBaseShader::ParamSetup_AlphaModel_Modulate(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates(BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR, BLENDFUNC_ADD);
}

void CBaseShader::ParamSetup_RasterState(IShaderAPI* renderAPI)
{
	const MaterialsRenderSettings& config = g_matSystem->GetConfiguration();

	ECullMode cull_mode = g_matSystem->GetCurrentCullMode();
	if(config.wireframeMode && config.editormode)
		cull_mode = CULL_NONE;

	g_matSystem->SetRasterizerStates(cull_mode, (EFillMode)(config.wireframeMode || (m_flags & MATERIAL_FLAG_WIREFRAME)));
}

void CBaseShader::ParamSetup_RasterState_NoCull(IShaderAPI* renderAPI)
{
	const MaterialsRenderSettings& config = g_matSystem->GetConfiguration();
	g_matSystem->SetRasterizerStates(CULL_NONE, (EFillMode)(config.wireframeMode || (m_flags & MATERIAL_FLAG_WIREFRAME)));
}

void CBaseShader::ParamSetup_Transform(IShaderAPI* renderAPI)
{
	Matrix4x4 wvp_matrix, world, view, proj;
	g_matSystem->GetWorldViewProjection(wvp_matrix);
	g_matSystem->GetMatrix(MATRIXMODE_WORLD, world);
	g_matSystem->GetMatrix(MATRIXMODE_VIEW, view);
	g_matSystem->GetMatrix(MATRIXMODE_PROJECTION, proj);

	// TODO: constant buffer CameraTransform
	renderAPI->SetShaderConstant(StringToHashConst("WVP"), wvp_matrix);
	renderAPI->SetShaderConstant(StringToHashConst("World"), world);
	renderAPI->SetShaderConstant(StringToHashConst("View"), view);
	renderAPI->SetShaderConstant(StringToHashConst("Proj"), proj);

	// setup texture transform
	const Vector4D texTransform = GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar);
	renderAPI->SetShaderConstant(StringToHashConst("BaseTextureTransform"), texTransform);
}

void CBaseShader::ParamSetup_DepthSetup(IShaderAPI* renderAPI)
{
	const int flags = m_flags;
	g_matSystem->SetDepthStates((flags & MATERIAL_FLAG_NO_Z_TEST) == 0, (flags & MATERIAL_FLAG_NO_Z_WRITE) == 0, (flags & MATERIAL_FLAG_DECAL) != 0);
}

void CBaseShader::ParamSetup_Fog(IShaderAPI* renderAPI)
{
	FogInfo fog;
	g_matSystem->GetFogInfo(fog);

	// setup shader fog
	const float fogScale = 1.0f / (fog.fogfar - fog.fognear);
	const Vector4D VectorFOGParams(fog.fognear,fog.fogfar, fogScale, 1.0f);

	// TODO: constant buffer FogInfo
	renderAPI->SetShaderConstant(StringToHashConst("ViewPos"), fog.viewPos);
	renderAPI->SetShaderConstant(StringToHashConst("FogParams"), VectorFOGParams);
	renderAPI->SetShaderConstant(StringToHashConst("FogColor"), fog.fogColor);
}

void CBaseShader::ParamSetup_BoneTransforms(IShaderAPI* renderAPI)
{
	if (!g_matSystem->IsSkinningEnabled())
		return;
	ArrayCRef<RenderBoneTransform> rendBones(nullptr);
	g_matSystem->GetSkinningBones(rendBones);
	renderAPI->SetShaderConstantArray(StringToHashConst("Bones"), rendBones);
}
