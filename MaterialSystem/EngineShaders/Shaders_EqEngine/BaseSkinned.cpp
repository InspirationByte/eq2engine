//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Basic skinned shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "../vars_generic.h"

BEGIN_SHADER_CLASS(BaseSkinned)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture	= NULL;
		m_pBumpTexture	= NULL;
		m_pSpecIllum	= NULL;
		m_pCubemap		= NULL;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;
		SHADER_PASS(Ambient_noskin) = NULL;
		SHADER_FOGPASS(Ambient_noskin) = NULL;

		SHADER_PASS(AmbientInst) = NULL;
		SHADER_FOGPASS(AmbientInst) = NULL;
		SHADER_PASS(AmbientInst_noskin) = NULL;
		SHADER_FOGPASS(AmbientInst_noskin) = NULL;

		m_nFlags		|= MATERIAL_FLAG_SKINNED;

		m_bSpecularIllum = false;
		m_bParallax = false;
		m_fParallaxScale = 1.0f;
		m_fSpecularScale = 0.0f;
		m_bCubeMapLighting = false;
		m_fCubemapLightingScale = 1.0f;
	}

	SHADER_INIT_TEXTURES()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		if(materials->GetConfiguration().enableBumpmapping)
			SHADER_PARAM_TEXTURE_NOERROR(Bumpmap, m_pBumpTexture);

		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Specular, m_pSpecIllum);

		// load cubemap if available
		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);

		if(m_pBumpTexture)
			SetParameterFunctor(SHADERPARAM_BUMPMAP, &ThisShaderClass::SetupBumpmap);

		if(m_pSpecIllum)
			SetParameterFunctor(SHADERPARAM_SPECULARILLUM, &ThisShaderClass::SetupSpecular);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Ambient))
			return true;

		bool bDeferredShading = (materials->GetConfiguration().lighting_model == MATERIAL_LIGHT_DEFERRED);

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		// illumination from specular alpha
		SHADER_PARAM_BOOL(SpecularIllum, m_bSpecularIllum, false);

		SHADER_PARAM_BOOL(CubeMapLighting, m_bCubeMapLighting, false);

		SHADER_PARAM_FLOAT(CubeMapLightingScale, m_fCubemapLightingScale, 1.0f);

		// parallax as the bump map alpha
		SHADER_PARAM_BOOL(Parallax, m_bParallax, false);

		// parallax scale
		SHADER_PARAM_FLOAT(ParallaxScale, m_fParallaxScale, 1.0f);

		if(m_pSpecIllum)
			m_fSpecularScale = 1.0f;

		// parallax scale
		SHADER_PARAM_FLOAT(SpecularScale, m_fSpecularScale, m_fSpecularScale);

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define bump mapping
		SHADER_BEGIN_DEFINITION((m_pBumpTexture != NULL), "BUMPMAP")

			// define parallax
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bParallax, "PARALLAX")

		SHADER_END_DEFINITION

		// define cubemap parameter.
		SHADER_BEGIN_DEFINITION((m_pCubemap != NULL), "CUBEMAP")
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bCubeMapLighting, "CUBEMAP_LIGHTING")
		SHADER_END_DEFINITION

		// define specular usage
		SHADER_BEGIN_DEFINITION((m_pSpecIllum != NULL), "SPECULAR")
			// define self illumination
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bSpecularIllum, "SELFILLUMINATION")
		SHADER_END_DEFINITION

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(materials->GetConfiguration().editormode, "EDITOR");

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		if(bDeferredShading)
		{
			// disable FFP alphatesting, let use the shader for better perfomance
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &ThisShaderClass::ParamSetup_AlphaModel_Solid);

			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BaseSkinned_Deferred");
			SHADER_FIND_OR_COMPILE(Ambient_noskin, "BaseSkinned_Deferred_NoSkin");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BaseSkinned_Deferred");
			SHADER_FIND_OR_COMPILE(Ambient_noskin_fog, "BaseSkinned_Deferred_NoSkin");
		}
		else
		{
			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BaseSkinned_Ambient");
			SHADER_FIND_OR_COMPILE(Ambient_noskin, "BaseSkinned_Ambient_NoSkin");
			SHADER_FIND_OR_COMPILE(AmbientInst, "BaseSkinned_Inst_Ambient");
			SHADER_FIND_OR_COMPILE(AmbientInst_noskin, "BaseSkinned_Inst_Ambient_NoSkin");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BaseSkinned_Ambient");
			SHADER_FIND_OR_COMPILE(Ambient_noskin_fog, "BaseSkinned_Ambient_NoSkin");
			SHADER_FIND_OR_COMPILE(AmbientInst_fog, "BaseSkinned_Inst_Ambient");
			SHADER_FIND_OR_COMPILE(AmbientInst_noskin_fog, "BaseSkinned_Inst_Ambient_NoSkin");
		}

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if(materials->IsSkinningEnabled())
		{
			if(materials->IsInstancingEnabled())
				SHADER_BIND_PASS_FOGSELECT(AmbientInst)
			else
				SHADER_BIND_PASS_FOGSELECT(Ambient)
		}
		else
		{
			if(materials->IsInstancingEnabled())
				SHADER_BIND_PASS_FOGSELECT(AmbientInst_noskin)
			else
				SHADER_BIND_PASS_FOGSELECT(Ambient_noskin)
		}
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);
		SetupDefaultParameter(SHADERPARAM_BUMPMAP);
		SetupDefaultParameter(SHADERPARAM_SPECULARILLUM);
		SetupDefaultParameter(SHADERPARAM_CUBEMAP);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);

		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);

		g_pShaderAPI->SetShaderConstantFloat("PARALLAX_SCALE", m_fParallaxScale);
		g_pShaderAPI->SetShaderConstantFloat("CUBEMAP_LIGHTING_SCALE", m_fCubemapLightingScale);
		g_pShaderAPI->SetShaderConstantFloat("SPECULAR_SCALE", m_fSpecularScale);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetConfiguration().wireframeMode ? ColorRGBA(0,1,1,1) : materials->GetAmbientColor());
	}

	void ParamSetup_Cubemap()
	{
		g_pShaderAPI->SetTexture(m_pCubemap, "CubemapSampler", 12);
	}

	void SetupBaseTexture()
	{
		ITexture* pSetupTexture = (materials->GetConfiguration().wireframeMode || r_showlightmaps->GetInt() == 1) ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTexture", 0);
	}

	void SetupBumpmap()
	{
		g_pShaderAPI->SetTexture(m_pBumpTexture, "BumpTexture", 4);
	}

	void SetupSpecular()
	{
		g_pShaderAPI->SetTexture(m_pSpecIllum, "SpecularTexture", 8);
	}

	ITexture*	GetBaseTexture(int stage)	{ return m_pBaseTexture; }
	ITexture*	GetBumpTexture(int stage)	{ return m_pBumpTexture; }

	ITexture*			m_pBaseTexture;
	ITexture*			m_pBumpTexture;
	ITexture*			m_pSpecIllum;
	ITexture*			m_pCubemap;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	SHADER_DECLARE_PASS(Ambient_noskin);
	SHADER_DECLARE_FOGPASS(Ambient_noskin);

	SHADER_DECLARE_PASS(AmbientInst);
	SHADER_DECLARE_FOGPASS(AmbientInst);

	SHADER_DECLARE_PASS(AmbientInst_noskin);
	SHADER_DECLARE_FOGPASS(AmbientInst_noskin);

	bool				m_bSpecularIllum;
	bool				m_bParallax;
	float				m_fParallaxScale;
	float				m_fSpecularScale;
	bool				m_bCubeMapLighting;
	float				m_fCubemapLightingScale;

END_SHADER_CLASS