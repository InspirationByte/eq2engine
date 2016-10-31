//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basis Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(DrvSynVehicle)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture	= NULL;
		m_pCubemap		= NULL;
		m_pDamageTexture = NULL;
		m_pColorMap = NULL;

		m_fSpecularScale = 0.0f;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		m_nFlags |= MATERIAL_FLAG_SKINNED;
	}

	SHADER_INIT_TEXTURES()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);
		SHADER_PARAM_TEXTURE_NOERROR(DamageTexture, m_pDamageTexture);
		SHADER_PARAM_TEXTURE_NOERROR(ColorMap, m_pColorMap);
		
		// load cubemap if available
		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Ambient))
			return true;

		bool bDeferredShading = (materials->GetLightingModel() == MATERIAL_LIGHT_DEFERRED);
		bool bHasAnyTranslucency = (m_nFlags & MATERIAL_FLAG_TRANSPARENT) || (m_nFlags & MATERIAL_FLAG_ADDITIVE) || (m_nFlags & MATERIAL_FLAG_MODULATE);

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		bool bBaseTextureSpecularAlpha;
		SHADER_PARAM_BOOL(BaseTextureSpecularAlpha, bBaseTextureSpecularAlpha);

		// parallax scale
		SHADER_PARAM_FLOAT(SpecularScale, m_fSpecularScale);

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define modulate parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_MODULATE), "MODULATE");

		//SHADER_DECLARE_SIMPLE_DEFINITION(true, "PIXEL_LIGHTING");

		bool useCubemap = (m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP) || (m_pCubemap != NULL);

		// define cubemap parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(useCubemap, "USE_CUBEMAP");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pColorMap != NULL), "USE_COLORMAP");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pDamageTexture != NULL), "USE_DAMAGE_TEXTURE");

		SHADER_BEGIN_DEFINITION(bBaseTextureSpecularAlpha, "USE_BASETEXTUREALPHA_SPECULAR")
			SHADER_DECLARE_SIMPLE_DEFINITION(true, "USE_SPECULAR");
		SHADER_END_DEFINITION;

		
		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		if(bDeferredShading && !bHasAnyTranslucency)
		{
			// disable FFP alphatesting, let use the shader for better perfomance
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Solid);

			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "VehicleBody_Deferred");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "VehicleBody_Deferred");
		}
		else
		{
			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "VehicleBody");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "VehicleBody");
		}

		return true;
	}

	void SetupShader()
	{
		SHADER_BIND_PASS_FOGSELECT( Ambient );
	}

	void SetupConstants()
	{
		SetupDefaultParameter( SHADERPARAM_TRANSFORM );

		SetupDefaultParameter( SHADERPARAM_BASETEXTURE );
		SetupDefaultParameter( SHADERPARAM_BUMPMAP );
		SetupDefaultParameter( SHADERPARAM_SPECULARILLUM );
		SetupDefaultParameter( SHADERPARAM_CUBEMAP );

		SetupDefaultParameter( SHADERPARAM_ALPHASETUP );
		SetupDefaultParameter( SHADERPARAM_DEPTHSETUP );
		SetupDefaultParameter( SHADERPARAM_RASTERSETUP );

		SetupDefaultParameter( SHADERPARAM_COLOR );
		SetupDefaultParameter( SHADERPARAM_FOG );

		g_pShaderAPI->SetShaderConstantFloat("SPECULAR_SCALE", m_fSpecularScale);
	}

	void ParamSetup_Cubemap()
	{
		if(m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP)
			g_pShaderAPI->SetTexture( materials->GetEnvironmentMapTexture() , "CubemapTexture", 12 );
		else
			g_pShaderAPI->SetTexture(m_pCubemap, "CubemapTexture", 12);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetupBaseTexture()
	{
		ITexture* pSetupTexture = (materials->GetConfiguration().wireframeMode) ? materials->GetWhiteTexture() : m_pBaseTexture;
		ITexture* pSetupDamTexture = (materials->GetConfiguration().wireframeMode) ? materials->GetWhiteTexture() : m_pDamageTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
		g_pShaderAPI->SetTexture(pSetupDamTexture, "DamageTextureSampler", 1);

		g_pShaderAPI->SetTexture(m_pColorMap, "ColormapSampler", 2);
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return m_pBaseTexture;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

private:

	ITexture*			m_pBaseTexture;
	ITexture*			m_pDamageTexture;
	ITexture*			m_pCubemap;
	ITexture*			m_pColorMap;

	float				m_fSpecularScale;
	bool				m_bBaseTextureSpecularAlpha;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

END_SHADER_CLASS