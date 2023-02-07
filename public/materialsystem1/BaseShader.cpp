//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
	ER_TextureFilterMode	type;
};

struct AddressingTypeString_s
{
	const char*				name;
	ER_TextureAddressMode	mode;
};

static const FilterTypeString_s s_textureFilterTypes[] = {
	{ "nearest",	TEXFILTER_NEAREST },
	{ "linear",		TEXFILTER_LINEAR },
	{ "bilinear",	TEXFILTER_BILINEAR },
	{ "trilinear",	TEXFILTER_TRILINEAR },
	{ "aniso",		TEXFILTER_TRILINEAR_ANISO },
};

static ER_TextureFilterMode ResolveFilterType(const char* string)
{
	for(int i = 0; i < elementsOf(s_textureFilterTypes); i++)
	{
		if(!stricmp(string, s_textureFilterTypes[i].name))
			return s_textureFilterTypes[i].type;
	}

	// by default
	return TEXFILTER_TRILINEAR_ANISO;
}

static ER_TextureAddressMode ResolveAddressType(const char* string)
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
}

//--------------------------------------
// Init parameters
//--------------------------------------

void CBaseShader::Init(IMaterial* assignee)
{
	m_material = assignee; 
	InitParams();
}

void CBaseShader::InitParams()
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

void CBaseShader::InitShader()
{
	// And then init shaders
	if( _ShaderInitRHI() )
		m_bInitialized = true;
	else
		m_bIsError = true;
}

// Unload shaders, textures
void CBaseShader::Unload()
{
	for(int i = 0; i < m_UsedPrograms.numElem(); ++i)
	{
		IShaderProgram* program = *m_UsedPrograms[i];
		*m_UsedPrograms[i] = nullptr;

		g_pShaderAPI->DestroyShaderProgram(program);
	}
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


void CBaseShader::SetupParameter(uint mask, ShaderDefaultParams_e type)
{
	// call it from this
	if(mask & (1 << (uint)type))
		(this->*m_param_functors[type]) ();
}

MatVarProxyUnk CBaseShader::FindMaterialVar(const char* paramName) const
{
	MatStringProxy mv;

	// editor parameters are separate prefixed ones
	// this allows to have values of parameters that are represented in the game while alter rendering in editor.
	if (materials->GetConfiguration().editormode)
		mv = GetAssignedMaterial()->FindMaterialVar(EqString::Format("editor.%s", paramName));

	if (!mv.IsValid())
		mv = GetAssignedMaterial()->FindMaterialVar(paramName);

	// check if we want to use material system global var under that name
	// NOTE: they have to be valid always in order to link proxies
	if (mv.IsValid() && *mv.Get() == '$')
		mv = materials->GetGlobalMaterialVarByName(((const char*)mv.Get())+1);

	return mv;
}

MatTextureProxy CBaseShader::FindTextureByVar(const char* paramName, bool errorTextureIfNoVar)
{
	MatStringProxy mv = FindMaterialVar(paramName);
	if(mv.IsValid()) 
	{
		AddManagedTexture(MatTextureProxy(mv), g_pShaderAPI->FindTexture(mv.Get()));
	}
	else if(errorTextureIfNoVar)
		AddManagedTexture(MatTextureProxy(mv), g_pShaderAPI->GetErrorTexture());

	return mv;
}

MatTextureProxy CBaseShader::LoadTextureByVar(const char* paramName, bool errorTextureIfNoVar)
{
	MatStringProxy mv = FindMaterialVar(paramName);

	if(mv.IsValid()) 
	{
		SamplerStateParam_t samplerParams;
		SamplerStateParams_Make(samplerParams, g_pShaderAPI->GetCaps(), (ER_TextureFilterMode)m_texFilter, (ER_TextureAddressMode)m_texAddressMode, (ER_TextureAddressMode)m_texAddressMode, (ER_TextureAddressMode)m_texAddressMode);

		if(mv.Get().Length())
			AddManagedTexture(MatTextureProxy(mv), g_texLoader->LoadTextureFromFileSync(mv.Get(), samplerParams));
	}
	else if(errorTextureIfNoVar)
		AddManagedTexture(MatTextureProxy(mv), g_pShaderAPI->GetErrorTexture());

	return mv;
}

void CBaseShader::ParamSetup_AlphaModel_Solid()
{
	// setup default alphatesting from shaderapi
	materials->SetBlendingStates( BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDFUNC_ADD );
}

void CBaseShader::ParamSetup_AlphaModel_Translucent()
{
	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD);
}

void CBaseShader::ParamSetup_AlphaModel_Additive()
{
	materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

	//m_fogenabled = false;
	m_depthwrite = false;
}

void CBaseShader::ParamSetup_AlphaModel_AdditiveLight()
{
	materials->SetBlendingStates(BLENDFACTOR_DST_COLOR, BLENDFACTOR_SRC_COLOR, BLENDFUNC_ADD);

	//m_fogenabled = false;
	m_depthwrite = false;
}

// this mode is designed for control of fog
void CBaseShader::ParamSetup_AlphaModel_Additive_Fog()
{
	materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

	m_depthwrite = false;
}

void CBaseShader::ParamSetup_AlphaModel_Modulate()
{
	// setup default alphatesting from shaderapi
	materials->SetBlendingStates(BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR, BLENDFUNC_ADD);
	m_depthwrite = false;
}

void CBaseShader::ParamSetup_RasterState()
{
	const matsystem_render_config_t& config = materials->GetConfiguration();

	ER_CullMode cull_mode = materials->GetCurrentCullMode();
	if(config.wireframeMode && config.editormode)
		cull_mode = CULL_NONE;

	materials->SetRasterizerStates(cull_mode, (ER_FillMode)config.wireframeMode, m_msaaEnabled, false, m_polyOffset);
}

void CBaseShader::ParamSetup_RasterState_NoCull()
{
	const matsystem_render_config_t& config = materials->GetConfiguration();
	materials->SetRasterizerStates(CULL_NONE, (ER_FillMode)config.wireframeMode, m_msaaEnabled, false, m_polyOffset);
}

void CBaseShader::ParamSetup_Transform()
{
	Matrix4x4 wvp_matrix, world, view, proj;
	materials->GetWorldViewProjection(wvp_matrix);
	materials->GetMatrix(MATRIXMODE_WORLD, world);
	materials->GetMatrix(MATRIXMODE_VIEW, view);
	materials->GetMatrix(MATRIXMODE_PROJECTION, proj);

	g_pShaderAPI->SetShaderConstantMatrix4("WVP", wvp_matrix);
	g_pShaderAPI->SetShaderConstantMatrix4("World", world);
	g_pShaderAPI->SetShaderConstantMatrix4("View", view);
	g_pShaderAPI->SetShaderConstantMatrix4("Proj", proj);

	// setup texture transform
	SetupVertexShaderTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar, "BaseTextureTransform");
}

void CBaseShader::ParamSetup_DepthSetup()
{
	materials->SetDepthStates(m_depthtest, m_depthwrite);
}

void CBaseShader::ParamSetup_Fog()
{
	FogInfo_t fog;
	materials->GetFogInfo(fog);

	// setup shader fog
	const float fogScale = 1.0f / (fog.fogfar - fog.fognear);
	const Vector4D VectorFOGParams(fog.fognear,fog.fogfar, fogScale, 1.0f);

	g_pShaderAPI->SetShaderConstantVector3D("ViewPos", fog.viewPos);
	g_pShaderAPI->SetShaderConstantVector4D("FogParams", VectorFOGParams);
	g_pShaderAPI->SetShaderConstantVector3D("FogColor", fog.fogColor);
}

void CBaseShader::ParamSetup_Cubemap()
{
	g_pShaderAPI->SetTexture(m_cubemapTexture.Get(), "CubemapTexture", 12);
}

// get texture transformation from vars
Vector4D CBaseShader::GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const
{
	if(transformVar.IsValid() && scaleVar.IsValid())
		return Vector4D(scaleVar.Get(), transformVar.Get());

	return Vector4D(1,1,0,0);
}

// sends texture transformation to shader
void CBaseShader::SetupVertexShaderTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar, const char* pszConstName)
{
	Vector4D trans = GetTextureTransform(transformVar, scaleVar);

	g_pShaderAPI->SetShaderConstantVector4D(pszConstName, trans);
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

void CBaseShader::AddManagedShader(IShaderProgram** pShader)
{
	if(!*pShader)
		return;

	(*pShader)->Ref_Grab();
	m_UsedPrograms.append(pShader);
}

void CBaseShader::AddManagedTexture(MatTextureProxy& var, const ITexturePtr& tex)
{
	if(!tex)
		return;

	var.Set(tex);
	m_UsedTextures.append(var);
}