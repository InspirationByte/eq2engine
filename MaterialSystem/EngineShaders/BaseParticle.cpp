//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: UnLit shader. Used for model
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "vars_generic.h"

//--------------------------------------
// BaseParticle
//--------------------------------------

//extern ConVar fog_enable;

class SH_BaseParticle : public CBaseShader
{
public:
	SH_BaseParticle();

	void InitParams();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	void Unload(bool bUnloadShaders = true, bool bUnloadTextures = true)
	{
		g_pShaderAPI->DestroyRenderState(m_additiveRasterState);
		m_additiveRasterState = NULL;

		CBaseShader::Unload(bUnloadShaders, bUnloadTextures);
	}

	// Return real shader name
	const char* GetName()
	{
		return "BaseParticle";
	}

	ITexture* GetBaseTexture(int id)		{return m_pBaseTexture;}
	ITexture* GetBumpTexture(int id)		{return NULL;}

	void SetupShader();

	// Sets constants
	void SetupConstants();

	void SetupBaseTexture0();

	void DepthRangeRasterSetup()
	{
		g_pShaderAPI->SetDepthRange(0.0f - 0.00008f, 1.0f - 0.00001f);
		CBaseShader::ParamSetup_RasterState();
	}

	void SetColorModulation()
	{
		//g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor()*ColorRGBA(color,1));
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetAdditiveColorModulation()
	{
		//g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", ColorRGBA(color,1));

		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void AdditiveDepthSetup();
	void AdditiveRasterSetup();
	void DefaultDepthSetup();

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Particle);
	}

private:
	SHADER_DECLARE_PASS(Particle);
	SHADER_DECLARE_FOGPASS(Particle);

	ITexture*			m_pBaseTexture;

	Vector3D			color;

	IMatVar*			m_pBaseTextureFrame;

	IRenderState*		m_additiveRasterState;
};

//--------------------------------------
// BaseUnlit
//--------------------------------------

SH_BaseParticle::SH_BaseParticle() : CBaseShader()
{
	SHADER_PASS(Particle) = NULL;
	SHADER_FOGPASS(Particle) = NULL;

	m_pBaseTexture			= NULL;
	m_additiveRasterState	= NULL;
	color = Vector3D(1);
}

//--------------------------------------
// Init parameters
//--------------------------------------

void SH_BaseParticle::InitParams()
{
	if(!IsInitialized() && !IsError())
	{
		IMatVar* mv_color			= GetAssignedMaterial()->FindMaterialVar("color");
		IMatVar* mv_depthSetup		= GetAssignedMaterial()->FindMaterialVar("depthRangeSetup");

		if(mv_color)
			color = mv_color->GetVector3();

		// Call base classinitialization
		CBaseShader::InitParams();

		if(mv_depthSetup && mv_depthSetup->GetInt() > 0)
		{
			SetParameterFunctor(SHADERPARAM_RASTERSETUP, &SH_BaseParticle::DepthRangeRasterSetup);
		}
		
		/*
		if(m_nFlags & MATERIAL_FLAG_ADDITIVE)
		{
			SetParameterFunctor(SHADERPARAM_COLOR, &SH_BaseParticle::SetAdditiveColorModulation);
			SetParameterFunctor(SHADERPARAM_DEPTHSETUP, &SH_BaseParticle::AdditiveDepthSetup);
			SetParameterFunctor(SHADERPARAM_RASTERSETUP, &SH_BaseParticle::AdditiveRasterSetup);
		}
		else
		{*/
			SetParameterFunctor(SHADERPARAM_COLOR, &SH_BaseParticle::SetColorModulation);
			//SetParameterFunctor(SHADERPARAM_DEPTHSETUP, &SH_BaseParticle::DefaultDepthSetup);
		//}
			
	}
}

void SH_BaseParticle::InitTextures()
{
	if(m_pBaseTexture == NULL)
	{
		// parse material variables
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &SH_BaseParticle::SetupBaseTexture0);
	}
}

void SH_BaseParticle::SetupBaseTexture0()
{
	ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

	g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
}

bool SH_BaseParticle::InitShaders()
{
	// just skip if we already have shader
	if(SHADER_PASS(Particle))
		return true;

	bool useAdvLighting = false;

	SHADER_PARAM_BOOL(AdvancedLighting, useAdvLighting);

	SHADERDEFINES_BEGIN

	SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ADDITIVE), "ADDITIVE")

	SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

	SHADER_DECLARE_SIMPLE_DEFINITION(useAdvLighting, "ADVANCED_LIGHTING");

	SHADER_FIND_OR_COMPILE(Particle, "BaseParticle")

	SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG")

	SHADER_FIND_OR_COMPILE(Particle_fog, "BaseParticle")

	return true;
}

//--------------------------------------
// Set Constants
//--------------------------------------

void SH_BaseParticle::SetupShader()
{
	if(IsError())
		return;

	FogInfo_t fg;
	materials->GetFogInfo(fg);

	SHADER_BIND_PASS_FOGSELECT(Particle);
}

void SH_BaseParticle::SetupConstants()
{
	// If we has shader
	if(!IsError())
	{
		materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO);

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}
	else
	{
		// Set error texture
		g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
	}
}

void SH_BaseParticle::AdditiveDepthSetup()
{
	materials->SetDepthStates(m_depthtest, m_depthwrite, COMP_LEQUAL);


	// simpler than setting up depth bias, but needs more tweaking
	//g_pShaderAPI->SetDepthRange(0.0f - 0.06f, 1.0f - 0.000001f);
}

void SH_BaseParticle::AdditiveRasterSetup()
{
	if(!m_additiveRasterState)
	{
		RasterizerStateParams_t raster;
		raster.cullMode = materials->GetCurrentCullMode();
		raster.useDepthBias = true;

#pragma fixme("Use depth biasing instead of SetDepthRange. However, I can't find proper values for them")

		//raster.depthBias = 0.0f;
		//raster.slopeDepthBias = -100.0f;

		if(m_nFlags & MATERIAL_FLAG_NOCULL)
			raster.cullMode = CULL_NONE;

		m_additiveRasterState = g_pShaderAPI->CreateRasterizerState(raster);
	}

	g_pShaderAPI->SetRasterizerState(m_additiveRasterState);

#pragma fixme("SetDepthRange for this particles doesn't work properly on nvidia")

	//g_pShaderAPI->SetDepthRange(0.0f/* - 0.06f*/, 1.0f - 0.000001f);
}

void SH_BaseParticle::DefaultDepthSetup()
{
	materials->SetDepthStates(m_depthtest, m_depthwrite, COMP_LEQUAL);
	g_pShaderAPI->SetDepthRange(0.0f, 1.0f);
}

// DECLARE SHADER!
DEFINE_SHADER(BaseParticle,SH_BaseParticle)

	/*
//--------------------------------------
// BaseParticle DS
//--------------------------------------


class SH_BaseParticleDS : public CBaseShader
{
public:
	SH_BaseParticleDS();

	// Sets parameters and compiles shader
	void InitParams();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	// Return real shader name
	const char* GetName()
	{
		return "BaseParticleDS";
	}

	ITexture* GetBaseTexture()		{return m_nBaseTexture;}
	ITexture* GetBumpTexture()		{return NULL;}
	bool IsHasAlphaTexture()		{return bHasAlpha;}

	void Unload(bool bUnloadShaders = true, bool bUnloadTextures = true)
	{
		if(bUnloadShaders)
		{
			SHADER_PASS_UNLOAD(Particle);
			SHADER_FOGPASS_UNLOAD(Particle);
		}

		if(bUnloadTextures)
		{
			g_pShaderAPI->FreeTexture(m_nBaseTexture);
			m_nBaseTexture = NULL;
		}
	}

	// Sets constants
	void SetConstants();
private:
	SHADER_DECLARE_PASS(Particle);
	SHADER_DECLARE_FOGPASS(Particle);

	ITexture*			m_nBaseTexture;
	ITexture*			m_pRTDepth_Trans;

	bool				bHasAlpha;
	bool				m_bAdditive;
	bool				m_bAlphaTest;
	bool				m_bNotSoft;

	Vector3D			color;

	IMatVar*			m_pBaseTextureFrame;
};

//--------------------------------------
// BaseUnlit
//--------------------------------------

SH_BaseParticleDS::SH_BaseParticleDS() : CBaseShader()
{
	SHADER_PASS(Particle)			= NULL;
	SHADER_FOGPASS(Particle)		= NULL;
	m_nBaseTexture		= NULL;
	m_pRTDepth_Trans	= NULL;
	bHasAlpha			= false;
	m_bAlphaTest		= false;
	m_bAdditive			= false;
	m_bNotSoft			= false;
	color = Vector3D(1);
}

//--------------------------------------
// Init parameters
//--------------------------------------

void SH_BaseParticleDS::InitParams()
{
	if(!GetAssignedMaterial()->IsError() && !IsInitialized() && !IsError())
	{
		IMatVar *m_HasAlpha			= GetAssignedMaterial()->FindMaterialVar("translucent");
		IMatVar *m_AlphaTest		= GetAssignedMaterial()->FindMaterialVar("alphatest");
		IMatVar *mvAdditive			= GetAssignedMaterial()->FindMaterialVar("additive");
		IMatVar *mv_color			= GetAssignedMaterial()->FindMaterialVar("color");
		IMatVar *mv_notsoft			= GetAssignedMaterial()->FindMaterialVar("notsoft");

		if(mv_notsoft)
			m_bNotSoft = mv_notsoft->GetInt() > 0;

		if(mvAdditive)
		{
			m_bAdditive = mvAdditive->GetInt() > 0;
		}

		if(m_AlphaTest)
		{
			m_bAlphaTest = m_AlphaTest->GetInt() > 0;
		}

		if(m_HasAlpha)
		{
			bHasAlpha = m_HasAlpha->GetInt() > 0;
		}

		if(bHasAlpha)
			m_nFlags |= MATERIAL_FLAG_TRANSPARENT;

		if(mv_color)
		{
			color = mv_color->GetVector3();
		}

		// Call base classinitialization
		CBaseShader::InitParams();
	}
}

void SH_BaseParticleDS::InitTextures()
{
	if(m_nBaseTexture == NULL)
	{
		// required
		m_pBaseTextureFrame			= GetAssignedMaterial()->FindMaterialVar("BaseTextureFrame");

		if(!m_pBaseTextureFrame)
		{
			m_pBaseTextureFrame = GetAssignedMaterial()->CreateMaterialVar("BaseTextureFrame");
			m_pBaseTextureFrame->SetInt(0);
		}

		// parse material variables
		IMatVar *m_BaseTextureName = GetAssignedMaterial()->FindMaterialVar("BaseTexture");

		m_pRTDepth_Trans = g_pShaderAPI->FindTexture("_gbuffer_depth");

		if(!m_pRTDepth_Trans)
			m_pRTDepth_Trans = g_pShaderAPI->GetErrorTexture();

		if(m_BaseTextureName)
		{
			m_nBaseTexture = g_pShaderAPI->LoadTexture(m_BaseTextureName->GetString(),m_nTextureFilter,ADDRESSMODE_CLAMP);
		}
		else
		{
			MsgError("ERROR! Cannot find variable 'BaseTexture' for material %s\n",GetAssignedMaterial()->GetName());
			// don't use IShaderAPI::GetErrorTexture() for replacement, just generate.
			m_nBaseTexture = g_pShaderAPI->GenerateErrorTexture();
		}
	}
}

bool SH_BaseParticleDS::InitShaders()
{
	// just skip if we already have shader
	if(SHADER_PASS(Particle))
		return true;

	SHADERDEFINES_BEGIN

	SHADER_DECLARE_SIMPLE_DEFINITION(m_bAlphaTest, "ALPHATEST")
	SHADER_DECLARE_SIMPLE_DEFINITION(m_bNotSoft, "NOT_SOFT")

	SHADER_FIND_OR_COMPILE(Particle, "BaseParticleDS")

	SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG")

	SHADER_FIND_OR_COMPILE(Particle_fog, "BaseParticleDS")

	return true;
}

//--------------------------------------
// Set Constants
//--------------------------------------

void SH_BaseParticleDS::SetConstants()
{
	// If we has shader
	if(!IsError())
	{

		bool depthTestEnable = true;
		bool depthWriteEnable = true;


		g_pShaderAPI->SetBlendingStateFromParams(NULL);

		if(bHasAlpha)
		{
			depthWriteEnable = false;
			g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_SRC_ALPHA,BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
		}

		if(m_bAdditive)
		{
			depthWriteEnable = false;
			g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);
		}

		g_pShaderAPI->ChangeRasterStateEx(m_bNoCull ? CULL_NONE : materials->GetCurrentCullMode(),FILL_SOLID);

		if(m_pBaseTextureFrame->GetInt() >= m_nBaseTexture->GetAnimatedTextureFrames())
			m_pBaseTextureFrame->SetInt(0);

		m_nBaseTexture->SetAnimatedTextureFrame(m_pBaseTextureFrame->GetInt());

		// Setup shader
		SHADER_BIND_PASS_FOGSELECT(Particle);

		// setup base texture
		g_pShaderAPI->SetTexture(m_nBaseTexture, 0);

		if(!m_bNotSoft)
		{
			g_pShaderAPI->SetTexture(m_pRTDepth_Trans, 1);
			//depthTestEnable = false;
		}

		g_pShaderAPI->ChangeDepthStateEx(depthTestEnable,depthWriteEnable);
	}
	else
	{
		// Set error texture
		g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
	}
}

// DECLARE SHADER!
DEFINE_SHADER(BaseParticleDS,SH_BaseParticleDS)
*/
/*
class SH_Decal : public CBaseShader
{
public:
	SH_Decal();

	// Sets parameters and compiles shader
	void InitParams();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	// Return real shader name
	const char* GetName()
	{
		return "Decal";
	}

	ITexture* GetBaseTexture()		{return m_nBaseTexture;}
	ITexture* GetBumpTexture()		{return NULL;}
	bool IsHasAlphaTexture()		{return bHasAlpha;}

	void Unload(bool bUnloadShaders = true, bool bUnloadTextures = true)
	{
		if(bUnloadShaders)
		{
			g_pShaderAPI->DestroyShaderProgram(m_nShader);
			m_nShader = NULL;
		}

		if(bUnloadTextures)
		{
			g_pShaderAPI->FreeTexture(m_nBaseTexture);
			m_nBaseTexture = NULL;
		}
	}

	// Sets constants
	void SetConstants();
private:
	IShaderProgram*		m_nShader;
	ITexture*			m_nBaseTexture;
	ITexture*			m_nBumpTexture;

	bool				bHasAlpha;
	bool				m_bAdditive;
	bool				m_bAlphaTest;
	bool				m_bModulation;
	bool				m_bParallax;
	float				m_fParallaxScale;

	Vector3D			color;

	IMatVar*			m_pBaseTextureFrame;
};

//--------------------------------------
// BaseUnlit
//--------------------------------------

SH_Decal::SH_Decal() : CBaseShader()
{
	m_nShader			= NULL;
	m_nBaseTexture		= NULL;
	m_nBumpTexture		= NULL;
	bHasAlpha			= false;
	m_bAlphaTest		= false;
	m_bAdditive			= false;
	m_bModulation		= false;

	m_bParallax			= false;
	m_fParallaxScale	= 1.0f;

	color = Vector3D(1);
}

//--------------------------------------
// Init parameters
//--------------------------------------

void SH_Decal::InitParams()
{
	if(!GetAssignedMaterial()->IsError() && !IsInitialized() && !IsError())
	{
		IMatVar *m_HasAlpha			= GetAssignedMaterial()->FindMaterialVar("translucent");
		IMatVar *m_AlphaTest		= GetAssignedMaterial()->FindMaterialVar("alphatest");
		IMatVar *mvAdditive			= GetAssignedMaterial()->FindMaterialVar("additive");
		IMatVar *mv_color			= GetAssignedMaterial()->FindMaterialVar("color");
		IMatVar *mv_modulate		= GetAssignedMaterial()->FindMaterialVar("modulate");
		IMatVar *mv_parallax		= GetAssignedMaterial()->FindMaterialVar("parallax");
		IMatVar *mv_parallaxscale	= GetAssignedMaterial()->FindMaterialVar("parallaxscale");

		if(mv_parallax)
			m_bParallax = mv_parallax->GetInt() > 0;

		if(mv_parallaxscale)
			m_fParallaxScale = mv_parallaxscale->GetFloat();

		if(mvAdditive)
		{
			m_bAdditive = mvAdditive->GetInt() > 0;
		}

		if(m_AlphaTest)
		{
			m_bAlphaTest = m_AlphaTest->GetInt() > 0;
		}

		if(m_HasAlpha)
		{
			bHasAlpha = m_HasAlpha->GetInt() > 0;
		}

		if(bHasAlpha)
			m_nFlags |= MATERIAL_FLAG_TRANSPARENT;

		if(mv_color)
		{
			color = mv_color->GetVector3();
		}

		if(mv_modulate)
		{
			m_bModulation = mv_modulate->GetInt() > 0;
		}

		// Call base classinitialization
		CBaseShader::InitParams();
	}
}

void SH_Decal::InitTextures()
{
	if(m_nBaseTexture == NULL)
	{
		// required
		m_pBaseTextureFrame			= GetAssignedMaterial()->FindMaterialVar("BaseTextureFrame");

		if(!m_pBaseTextureFrame)
		{
			m_pBaseTextureFrame = GetAssignedMaterial()->CreateMaterialVar("BaseTextureFrame");
			m_pBaseTextureFrame->SetInt(0);
		}

		// parse material variables
		IMatVar *m_BaseTextureName = GetAssignedMaterial()->FindMaterialVar("BaseTexture");

		if(m_BaseTextureName)
		{
			m_nBaseTexture = g_pShaderAPI->LoadTexture(m_BaseTextureName->GetString(),m_nTextureFilter,ADDRESSMODE_CLAMP);
		}
		else
		{
			MsgError("ERROR! Cannot find variable 'BaseTexture' for material %s\n",GetAssignedMaterial()->GetName());
			// don't use IShaderAPI::GetErrorTexture() for replacement, just generate.
			m_nBaseTexture = g_pShaderAPI->GenerateErrorTexture();
		}

		// parse material variables
		IMatVar *m_BumpTextureName = GetAssignedMaterial()->FindMaterialVar("bumpmap");
		if(m_BumpTextureName && r_bumpmapping->GetBool())
		{
			m_nBumpTexture = g_pShaderAPI->LoadTexture(m_BumpTextureName->GetString(),m_nTextureFilter,ADDRESSMODE_CLAMP);
		}
	}
}

bool SH_Decal::InitShaders()
{
	// just skip if we already have shader
	if(m_nShader)
		return true;

	DkStr defines;

	if(m_bAlphaTest)
	{
		defines.append("#define ALPHATEST\n");
	}

	if(m_bModulation)
	{
		defines.append("#define MODULATE\n");
	}

	if(m_nBumpTexture)
	{
		defines.append("#define BUMPMAP\n");
	}

	if(m_bParallax)
	{
		defines.append("#define USE_OFFSETMAP\n");
		defines.append(varargs("#define OFFSETMAP_SCALE %f\n", m_fParallaxScale));
	}

	// Download shader from disk (shaders\*.shader)
	m_nShader = g_pShaderAPI->CreateNewShaderProgram("Decal");

	if(materials->GetLightingModel() == MATERIAL_LIGHT_DEFERRED)
		return g_pShaderAPI->LoadShadersFromFile(m_nShader,"DecalDS",defines.getData());
	else
		return g_pShaderAPI->LoadShadersFromFile(m_nShader,"Decal",defines.getData());
}

//--------------------------------------
// Set Constants
//--------------------------------------

ConVar r_decals_depthbias("decal_depthbias", "-0.00001", "Decal depth bias", CV_ARCHIVE);
ConVar r_decals_slopedepthbias("decal_slopedepthbias", "0", "Decal depth bias", CV_ARCHIVE);

void SH_Decal::SetConstants()
{
	// If we has shader
	if(!IsError())
	{

		bool depthTestEnable = true;
		bool depthWriteEnable = true;


		g_pShaderAPI->SetBlendingStateFromParams(NULL);

		if(bHasAlpha)
		{
			depthWriteEnable = false;
			g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_SRC_ALPHA,BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
		}

		if(m_bAdditive)
		{
			depthWriteEnable = false;
			g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);
		}

		if(m_bModulation)
		{
			g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_DST_COLOR,BLENDFACTOR_SRC_COLOR,BLENDFUNC_ADD);
			depthWriteEnable = false;
		}

		g_pShaderAPI->ChangeRasterStateEx(m_bNoCull ? CULL_NONE : materials->GetCurrentCullMode(),FILL_SOLID);

		if(m_pBaseTextureFrame->GetInt() >= m_nBaseTexture->GetAnimatedTextureFrames())
			m_pBaseTextureFrame->SetInt(0);

		m_nBaseTexture->SetAnimatedTextureFrame(m_pBaseTextureFrame->GetInt());

		g_pShaderAPI->SetVertexShaderConstantVector3D(5, scenerenderer->GetView()->GetOrigin());

		// Setup shader
		g_pShaderAPI->SetShader(m_nShader);

		// setup base texture
		g_pShaderAPI->SetTexture(m_nBaseTexture, 0);

		g_pShaderAPI->SetTexture(m_nBumpTexture, 1);

		g_pShaderAPI->ChangeDepthStateEx(depthTestEnable,depthWriteEnable, DEPTHCOMP_LEQUAL, r_decals_depthbias.GetFloat(), r_decals_slopedepthbias.GetFloat());
	}
	else
	{
		// Set error texture
		g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
	}
}

// DECLARE SHADER!
DEFINE_SHADER(Decal,SH_Decal)
*/