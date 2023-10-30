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

struct FilterTypeString_s
{
	const char*				name;
	ETexFilterMode	type;
};

struct AddressingTypeString_s
{
	const char*				name;
	ETexAddressMode	mode;
};

static const FilterTypeString_s s_textureFilterTypes[] = {
	{ "nearest",	TEXFILTER_NEAREST },
	{ "linear",		TEXFILTER_LINEAR },
	{ "bilinear",	TEXFILTER_BILINEAR },
	{ "trilinear",	TEXFILTER_TRILINEAR },
	{ "aniso",		TEXFILTER_TRILINEAR_ANISO },
};

static ETexFilterMode ResolveFilterType(const char* string)
{
	for(int i = 0; i < elementsOf(s_textureFilterTypes); i++)
	{
		if(!stricmp(string, s_textureFilterTypes[i].name))
			return s_textureFilterTypes[i].type;
	}

	// by default
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

//--------------------------------------
// Constructor
//--------------------------------------

CBaseShader::CBaseShader()
{
	m_bIsError			= false;
	m_bInitialized		= false;

	m_depthwrite		= true;
	m_depthtest			= true;

	m_fogenabled		= true;
	m_msaaEnabled		= true;

	m_polyOffset		= false;

	for(int i = 0; i < SHADERPARAM_COUNT; i++)
		SetParameterFunctor(i, &CBaseShader::EmptyFunctor);

	SetParameterFunctor(SHADERPARAM_CUBEMAP, &CBaseShader::ParamSetup_Cubemap);
	SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Solid);
	SetParameterFunctor(SHADERPARAM_TRANSFORM, &CBaseShader::ParamSetup_Transform);
	SetParameterFunctor(SHADERPARAM_RASTERSETUP, &CBaseShader::ParamSetup_RasterState);
	SetParameterFunctor(SHADERPARAM_DEPTHSETUP, &CBaseShader::ParamSetup_DepthSetup);
	SetParameterFunctor(SHADERPARAM_FOG, &CBaseShader::ParamSetup_Fog);
	SetParameterFunctor(SHADERPARAM_BONETRANSFORMS, &CBaseShader::ParamSetup_BoneTransforms);
}

//--------------------------------------
// Init parameters
//--------------------------------------

void CBaseShader::Init(IShaderAPI* renderAPI, IMaterial* assignee)
{
	m_material = assignee; 
	InitParams(renderAPI);
}

void CBaseShader::InitParams(IShaderAPI* renderAPI)
{
	MatStringProxy addressMode	= GetAssignedMaterial()->FindMaterialVar("Address");
	MatStringProxy texFilter = GetAssignedMaterial()->FindMaterialVar("Filtering");

	SHADER_PARAM_BOOL(ztest, m_depthtest, true)
	SHADER_PARAM_BOOL(zwrite, m_depthwrite, true)

	SHADER_PARAM_BOOL_NEG(NoFog, m_fogenabled, false)
	SHADER_PARAM_BOOL_NEG(NoMSAA, m_msaaEnabled, false)

	IMaterial* assignedMaterial = GetAssignedMaterial();

	// required
	m_baseTextureTransformVar	= assignedMaterial->GetMaterialVar("BaseTextureTransform", "[0 0]");
	m_baseTextureScaleVar		= assignedMaterial->GetMaterialVar("BaseTextureScale", "[1 1]");
	m_baseTextureFrame			= assignedMaterial->GetMaterialVar("BaseTextureFrame", "0");

	// resolve address type and filtering mode
	if( addressMode.IsValid()) m_texAddressMode = ResolveAddressType(addressMode.Get());
	if( texFilter.IsValid()) m_texFilter = ResolveFilterType(texFilter.Get());

	// setup render & shadowing parameters
	SHADER_PARAM_FLAG(NoDraw,			m_flags, MATERIAL_FLAG_INVISIBLE, false)
	SHADER_PARAM_FLAG(ReceiveShadows,	m_flags, MATERIAL_FLAG_RECEIVESHADOWS, true)
	SHADER_PARAM_FLAG(CastShadows,		m_flags, MATERIAL_FLAG_CASTSHADOWS, true)
	SHADER_PARAM_FLAG(Wireframe,		m_flags, MATERIAL_FLAG_WIREFRAME, false)

	SHADER_PARAM_FLAG(Decal,			m_flags, MATERIAL_FLAG_DECAL, false)
	SHADER_PARAM_FLAG(NoCull,			m_flags, MATERIAL_FLAG_NOCULL, false)

	SHADER_PARAM_FLAG(AlphaTest,		m_flags, MATERIAL_FLAG_ALPHATESTED, false)

	SHADER_PARAM_ENUM(Translucent,		m_blendMode, SHADER_BLEND_TRANSLUCENT)
	SHADER_PARAM_ENUM(Additive,			m_blendMode, SHADER_BLEND_ADDITIVE)
	SHADER_PARAM_ENUM(Modulate,			m_blendMode, SHADER_BLEND_MODULATE)

	bool additiveLight = false;
	SHADER_PARAM_BOOL(AdditiveLight, additiveLight, false)

	if(m_blendMode == SHADER_BLEND_TRANSLUCENT)
	{
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Translucent);
		m_depthwrite = false;
	}
	else if(m_blendMode == SHADER_BLEND_ADDITIVE)
	{
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Additive);
		m_depthwrite = false;
	}
	else if(m_blendMode == SHADER_BLEND_MODULATE)
	{
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Modulate);
		m_depthwrite = false;
	}
	else if (additiveLight)
	{
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_AdditiveLight);
		m_depthwrite = false;
		m_blendMode = SHADER_BLEND_ADDITIVE;
	}

	if (m_blendMode != SHADER_BLEND_NONE)
		m_flags |= MATERIAL_FLAG_TRANSPARENT;

	if(m_flags & MATERIAL_FLAG_NOCULL)
		SetParameterFunctor(SHADERPARAM_RASTERSETUP, &CBaseShader::ParamSetup_RasterState_NoCull);

	if(m_flags & MATERIAL_FLAG_DECAL)
		m_polyOffset = true;
}

void CBaseShader::InitShader(IShaderAPI* renderAPI)
{
	// And then init shaders
	if( _ShaderInitRHI(renderAPI) )
		m_bInitialized = true;
	else
		m_bIsError = true;
}

// Unload shaders, textures
void CBaseShader::Unload()
{
	for(int i = 0; i < m_UsedPrograms.numElem(); ++i)
		*m_UsedPrograms[i] = nullptr;
	m_UsedPrograms.clear(true);

	for (int i = 0; i < m_UsedTextures.numElem(); ++i)
	{
		MatTextureProxy& texProxy = m_UsedTextures[i];
		texProxy.Set(nullptr);
	}
	m_UsedTextures.clear(true);

	m_bInitialized = false;
	m_bIsError = false;
}


void CBaseShader::SetupParameter(IShaderAPI* renderAPI, uint mask, EShaderParamSetup type)
{
	// call it from this
	if(mask & (1 << (uint)type))
		(this->*m_param_functors[type]) (renderAPI);
}

MatVarProxyUnk CBaseShader::FindMaterialVar(const char* paramName, bool allowGlobals) const
{
	MatStringProxy mv;

	// editor parameters are separate prefixed ones
	// this allows to have values of parameters that are represented in the game while alter rendering in editor.
	if (g_matSystem->GetConfiguration().editormode)
		mv = GetAssignedMaterial()->FindMaterialVar(EqString::Format("editor.%s", paramName));

	if (!mv.IsValid())
		mv = GetAssignedMaterial()->FindMaterialVar(paramName);

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

void CBaseShader::ParamSetup_AlphaModel_Solid(IShaderAPI* renderAPI)
{
	// setup default alphatesting from shaderapi
	g_matSystem->SetBlendingStates( BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDFUNC_ADD );
}

void CBaseShader::ParamSetup_AlphaModel_Translucent(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD);
}

void CBaseShader::ParamSetup_AlphaModel_Additive(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

	//m_fogenabled = false;
	m_depthwrite = false;
}

void CBaseShader::ParamSetup_AlphaModel_AdditiveLight(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates(BLENDFACTOR_DST_COLOR, BLENDFACTOR_SRC_COLOR, BLENDFUNC_ADD);

	//m_fogenabled = false;
	m_depthwrite = false;
}

// this mode is designed for control of fog
void CBaseShader::ParamSetup_AlphaModel_Additive_Fog(IShaderAPI* renderAPI)
{
	g_matSystem->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

	m_depthwrite = false;
}

void CBaseShader::ParamSetup_AlphaModel_Modulate(IShaderAPI* renderAPI)
{
	// setup default alphatesting from shaderapi
	g_matSystem->SetBlendingStates(BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR, BLENDFUNC_ADD);
	m_depthwrite = false;
}

void CBaseShader::ParamSetup_RasterState(IShaderAPI* renderAPI)
{
	const MaterialsRenderSettings& config = g_matSystem->GetConfiguration();

	ECullMode cull_mode = g_matSystem->GetCurrentCullMode();
	if(config.wireframeMode && config.editormode)
		cull_mode = CULL_NONE;

	g_matSystem->SetRasterizerStates(cull_mode, (EFillMode)(config.wireframeMode || (m_flags & MATERIAL_FLAG_WIREFRAME)), m_msaaEnabled, false);
}

void CBaseShader::ParamSetup_RasterState_NoCull(IShaderAPI* renderAPI)
{
	const MaterialsRenderSettings& config = g_matSystem->GetConfiguration();
	g_matSystem->SetRasterizerStates(CULL_NONE, (EFillMode)(config.wireframeMode || (m_flags & MATERIAL_FLAG_WIREFRAME)), m_msaaEnabled, false);
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
	g_matSystem->SetDepthStates(m_depthtest, m_depthwrite, m_polyOffset);
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

void CBaseShader::ParamSetup_Cubemap(IShaderAPI* renderAPI)
{
	renderAPI->SetTexture(StringToHashConst("CubemapTexture"), m_cubemapTexture.Get());
}

void CBaseShader::ParamSetup_BoneTransforms(IShaderAPI* renderAPI)
{
	if (!g_matSystem->IsSkinningEnabled())
		return;
	ArrayCRef<RenderBoneTransform> rendBones(nullptr);
	g_matSystem->GetSkinningBones(rendBones);
	renderAPI->SetShaderConstantArray(StringToHashConst("Bones"), rendBones);
}

// get texture transformation from vars
Vector4D CBaseShader::GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const
{
	if(transformVar.IsValid() && scaleVar.IsValid())
		return Vector4D(scaleVar.Get(), transformVar.Get());

	return Vector4D(1,1,0,0);
}

IMaterial* CBaseShader::GetAssignedMaterial() const
{
	return m_material;
}

bool CBaseShader::IsError() const
{
	return m_bIsError;
}

bool CBaseShader::IsInitialized() const
{
	return m_bInitialized;
}

// get flags
int CBaseShader::GetFlags() const
{
	return m_flags;
}

void CBaseShader::AddManagedShader(IShaderProgramPtr* pShader)
{
	if(!*pShader)
		return;
	m_UsedPrograms.append(pShader);
}

void CBaseShader::AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex)
{
	if(!tex)
		return;

	var.Set(tex);
	m_UsedTextures.append(var);
}