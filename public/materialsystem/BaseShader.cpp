//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

struct FilterTypeString_s
{
	const char*		pszFilterName;
	Filter_e		nFilterType;
};

const FilterTypeString_s pBaseTextureFilterTypes[] = {
	{ "nearest", TEXFILTER_NEAREST },
	{ "linear", TEXFILTER_LINEAR },
	{ "bilinear", TEXFILTER_BILINEAR },
	{ "trilinear", TEXFILTER_TRILINEAR },
	{ "aniso", TEXFILTER_TRILINEAR_ANISO },
};

Filter_e ResolveFilterType(const char* string)
{
	for(int i = 0; i < 5; i++)
	{
		if(!stricmp(string,pBaseTextureFilterTypes[i].pszFilterName))
			return pBaseTextureFilterTypes[i].nFilterType;
	}

	// by default
	return TEXFILTER_TRILINEAR_ANISO;
}

struct AddressingTypeString_s
{
	const char*		pszAddressName;
	AddressMode_e	nAddress;
};

const AddressingTypeString_s pBaseTextureAddressTypes[] = {
	{ "wrap", ADDRESSMODE_WRAP },
	{ "clamp", ADDRESSMODE_CLAMP },
};

AddressMode_e ResolveAddressType(const char* string)
{
	if(!stricmp(string,"wrap"))
		return ADDRESSMODE_WRAP;
	else
		return ADDRESSMODE_CLAMP;
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

	m_nFlags			= 0;

	m_pBaseTextureTransformVar = NULL;
	m_pBaseTextureScaleVar = NULL;
	m_pBaseTextureFrame = NULL;

	m_nAddressMode = ADDRESSMODE_WRAP;
	m_nTextureFilter = TEXFILTER_TRILINEAR_ANISO;

	for(int i = 0; i < SHADERPARAM_COUNT; i++)
		SetParameterFunctor(i, &CBaseShader::EmptyFunctor);

	SetParameterFunctor(SHADERPARAM_CUBEMAP, &CBaseShader::ParamSetup_Cubemap);
}

//--------------------------------------
// Init parameters
//--------------------------------------

void CBaseShader::InitParams()
{
	// The basic setup of the shader parameters
	if(	!m_bInitialized && !m_bIsError )
	{
		IMatVar *RecShadowed			= m_pAssignedMaterial->GetMaterialVar("ReceiveShadows", "1");
		IMatVar *CastShadows			= m_pAssignedMaterial->GetMaterialVar("CastShadows", "1");
		IMatVar *NoCull					= m_pAssignedMaterial->GetMaterialVar("NoCull", "0");

		IMatVar *pAddressMode			= m_pAssignedMaterial->FindMaterialVar("Address");
		IMatVar *pFilterType			= m_pAssignedMaterial->FindMaterialVar("Filtering");

		IMatVar *pAlphaTest				= m_pAssignedMaterial->GetMaterialVar("AlphaTest", "0");
		IMatVar *pTranslucent			= m_pAssignedMaterial->GetMaterialVar("Translucent", "0");
		IMatVar *pAdditive				= m_pAssignedMaterial->GetMaterialVar("Additive", "0");
		IMatVar *pModulate				= m_pAssignedMaterial->GetMaterialVar("Modulate", "0");

		IMatVar *pNoFog					= m_pAssignedMaterial->GetMaterialVar("nofog", "0");

		IMatVar* pDepthTest				= m_pAssignedMaterial->GetMaterialVar("ztest", "1");
		IMatVar* pDepthWrite			= m_pAssignedMaterial->GetMaterialVar("zwrite", "1");

		IMatVar* pNoMSAA			= m_pAssignedMaterial->GetMaterialVar("NoMSAA", "0");

		m_depthtest = (pDepthTest->GetInt() > 0);
		m_depthwrite = (pDepthWrite->GetInt() > 0);

		m_fogenabled = !(pNoFog->GetInt() > 0);

		m_msaaEnabled = !(pNoMSAA->GetInt() > 0);

		// required
		m_pBaseTextureTransformVar	= GetAssignedMaterial()->GetMaterialVar("BaseTextureTransform", "[0 0]");
		m_pBaseTextureScaleVar		= GetAssignedMaterial()->GetMaterialVar("BaseTextureScale", "[1 1]");
		m_pBaseTextureFrame			= GetAssignedMaterial()->GetMaterialVar("BaseTextureFrame", "0");

		// resolve address type and filter type
		if( pAddressMode )
			m_nAddressMode = ResolveAddressType(pAddressMode->GetString());

		if( pFilterType )
			m_nTextureFilter = ResolveFilterType(pFilterType->GetString());

		// setup shadowing parameters
		m_nFlags |= (RecShadowed->GetInt() > 0) ? MATERIAL_FLAG_RECEIVESHADOWS : 0;
		m_nFlags |= (CastShadows->GetInt() > 0) ? MATERIAL_FLAG_CASTSHADOWS : 0;

		// setup base rendering params
		m_nFlags |= (NoCull->GetInt() > 0) ? MATERIAL_FLAG_NOCULL : 0;

		// transparency
		m_nFlags |= (pAlphaTest->GetInt() > 0) ? MATERIAL_FLAG_ALPHATESTED : 0;

		m_nFlags |= (pTranslucent->GetInt() > 0) ? MATERIAL_FLAG_TRANSPARENT : 0;
		m_nFlags |= (pAdditive->GetInt() > 0) ? MATERIAL_FLAG_ADDITIVE : 0;
		m_nFlags |= (pModulate->GetInt() > 0) ? MATERIAL_FLAG_MODULATE : 0;

		if (materials->GetConfiguration().enableSpecular)
		{
			EqString cubemapStr;
			SHADER_PARAM_STRING(Cubemap, cubemapStr);

			// detect ENV_CUBEMAP
			if (cubemapStr.c_str()[0] == '$')
			{
				char* specialString = (char*)cubemapStr.c_str() + 1;
				if (!stricmp(specialString, "env_cubemap"))
					m_nFlags |= MATERIAL_FLAG_USE_ENVCUBEMAP;
			}
		}

		// first set solid alpha mode
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Solid);

		// setup functors for transparency
		//if(m_nFlags & MATERIAL_FLAG_ALPHATESTED)
		//	SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Alphatest);

		if(m_nFlags & MATERIAL_FLAG_TRANSPARENT)
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Translucent);

		if(m_nFlags & MATERIAL_FLAG_ADDITIVE)
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Additive);

		if(m_nFlags & MATERIAL_FLAG_MODULATE)
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Modulate);

		// setup functors
		SetParameterFunctor(SHADERPARAM_TRANSFORM, &CBaseShader::ParamSetup_Transform);
		SetParameterFunctor(SHADERPARAM_ANIMFRAME, &CBaseShader::ParamSetup_TextureFrames);
		SetParameterFunctor(SHADERPARAM_RASTERSETUP, &CBaseShader::ParamSetup_RasterState);
		SetParameterFunctor(SHADERPARAM_DEPTHSETUP, &CBaseShader::ParamSetup_DepthSetup);
		SetParameterFunctor(SHADERPARAM_FOG, &CBaseShader::ParamSetup_Fog);

		if(!materials->GetConfiguration().enableShadows)
		{
			m_nFlags &= ~MATERIAL_FLAG_RECEIVESHADOWS;
			m_nFlags &= ~MATERIAL_FLAG_CASTSHADOWS;
		}

		// Load textures
		InitTextures();

		if(materials->GetConfiguration().ffp_mode)
		{
			m_bInitialized = true;
			return;
		}

		// And then init shaders
		if( InitShaders() )
			m_bInitialized = true;
		else
			m_bIsError = true;
	}
}


// Unload shaders, textures
void CBaseShader::Unload(bool bUnloadShaders, bool bUnloadTextures)
{
	if(bUnloadShaders)
	{
		for(int i = 0; i < m_UsedPrograms.numElem(); i++)
		{
			g_pShaderAPI->DestroyShaderProgram(*m_UsedPrograms[i]);
			*m_UsedPrograms[i] = NULL;
		}
		m_UsedPrograms.clear();

		for(int i = 0; i < m_UsedRenderStates.numElem(); i++)
		{
			g_pShaderAPI->DestroyRenderState(*m_UsedRenderStates[i]);
			*m_UsedRenderStates[i] = NULL;
		}

		m_UsedRenderStates.clear();
	}

	if(bUnloadTextures)
	{
		for(int i = 0; i < m_UsedTextures.numElem(); i++)
		{
			g_pShaderAPI->FreeTexture(*m_UsedTextures[i]);
 			*m_UsedTextures[i] = g_pShaderAPI->GetErrorTexture();
		}

		m_UsedTextures.clear();
	}

	m_bInitialized = false;
	m_bIsError = false;
}


void CBaseShader::SetupDefaultParameter(ShaderDefaultParams_e paramtype)
{
	// call it from this
	(this->*m_param_functors[paramtype]) ();
}

void CBaseShader::ParamSetup_AlphaModel_Solid()
{
	// setup default alphatesting from shaderapi
	materials->SetBlendingStates( BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDFUNC_ADD );
}

void CBaseShader::ParamSetup_AlphaModel_Alphatest()
{
	// setup default alphatesting from shaderapi
	//materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD, COLORMASK_ALL, true);
}

void CBaseShader::ParamSetup_AlphaModel_Translucent()
{
	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD);
	m_depthwrite = false;
}

void CBaseShader::ParamSetup_AlphaModel_Additive()
{
	materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

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
	CullMode_e cull_mode = materials->GetCurrentCullMode();

	if(m_nFlags & MATERIAL_FLAG_NOCULL)
		cull_mode = CULL_NONE;

	// force no culling
	if(materials->GetConfiguration().wireframeMode && materials->GetConfiguration().editormode)
		cull_mode = CULL_NONE;

	materials->SetRasterizerStates(cull_mode, (FillMode_e)materials->GetConfiguration().wireframeMode, m_msaaEnabled);
}

void CBaseShader::ParamSetup_Transform()
{
	Matrix4x4 wvp_matrix = identity4();
	materials->GetWorldViewProjection(wvp_matrix);

	Matrix4x4 worldtransform = identity4();
	materials->GetMatrix(MATRIXMODE_WORLD, worldtransform);

	g_pShaderAPI->SetShaderConstantMatrix4("WVP", wvp_matrix);
	g_pShaderAPI->SetShaderConstantMatrix4("World", worldtransform);

	// setup texture transform
	SetupVertexShaderTextureTransform(m_pBaseTextureTransformVar, m_pBaseTextureScaleVar, "BaseTextureTransform");
}

void CBaseShader::ParamSetup_TextureFrames()
{
	// TODO: texture frame parameters
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
	float fogScale = 1.0 / (fog.fogfar - fog.fognear);

	Vector4D VectorFOGParams(fog.fognear,fog.fogfar, fogScale, 1.0f);

	g_pShaderAPI->SetShaderConstantVector3D("ViewPos", fog.viewPos);

	g_pShaderAPI->SetShaderConstantVector4D("FogParams", VectorFOGParams);
	g_pShaderAPI->SetShaderConstantVector3D("FogColor", fog.fogColor);
}

// get texture transformation from vars
Vector4D CBaseShader::GetTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar)
{
	if(pTransformVar && pScaleVar)
		return Vector4D(pScaleVar->GetVector2(), pTransformVar->GetVector2());

	return Vector4D(1,1,0,0);
}

// sends texture transformation to shader
void CBaseShader::SetupVertexShaderTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar, const char* pszConstName)
{
	Vector4D trans = GetTextureTransform(pTransformVar, pScaleVar);

	g_pShaderAPI->SetShaderConstantVector4D(pszConstName, trans);
}

IMaterial* CBaseShader::GetAssignedMaterial()
{
	return m_pAssignedMaterial;
}

bool CBaseShader::IsError()
{
	return m_bIsError;
}

bool CBaseShader::IsInitialized()
{
	return m_bInitialized;
}

// get flags
int CBaseShader::GetFlags()
{
	return m_nFlags;
}

void CBaseShader::AddShaderToAutoremover(IShaderProgram** pShader)
{
	if(*pShader)
		m_UsedPrograms.append(pShader);
}

void CBaseShader::AddTextureToAutoremover(ITexture** pShader)
{
	if(*pShader)
		m_UsedTextures.append(pShader);
}