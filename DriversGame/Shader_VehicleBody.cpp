//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basis Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "ConVar.h"

ConVar r_carPerPixelLights("r_carPerPixelLights", "0", nullptr, CV_ARCHIVE);

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

		bool bHasAnyTranslucency = (m_nFlags & MATERIAL_FLAG_TRANSPARENT) || (m_nFlags & MATERIAL_FLAG_ADDITIVE) || (m_nFlags & MATERIAL_FLAG_MODULATE);

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		bool bBaseTextureSpecularAlpha;
		SHADER_PARAM_BOOL(BaseTextureSpecularAlpha, bBaseTextureSpecularAlpha, false);

		bool bLicensePlate;
		SHADER_PARAM_BOOL(LicensePlate, bLicensePlate, false);

		// parallax scale
		SHADER_PARAM_FLOAT(SpecularScale, m_fSpecularScale, 0.0f);

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define modulate parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_MODULATE), "MODULATE");

		SHADER_DECLARE_SIMPLE_DEFINITION(r_carPerPixelLights.GetBool(), "PIXEL_LIGHTING");

		bool useCubemap = (m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP) || (m_pCubemap != NULL);

		// define cubemap parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(useCubemap, "USE_CUBEMAP");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pColorMap != NULL), "USE_COLORMAP");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pDamageTexture != NULL), "USE_DAMAGE_TEXTURE");
		SHADER_DECLARE_SIMPLE_DEFINITION(bLicensePlate, "USE_LPLATE");
		

		SHADER_BEGIN_DEFINITION(bBaseTextureSpecularAlpha, "USE_BASETEXTUREALPHA_SPECULAR")
			SHADER_DECLARE_SIMPLE_DEFINITION(true, "USE_SPECULAR");
		SHADER_END_DEFINITION;

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Ambient, "Vehicle");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
		// compile with fog
		SHADER_FIND_OR_COMPILE(Ambient_fog, "Vehicle");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_FOGSELECT( Ambient );
	}

	SHADER_SETUP_CONSTANTS()
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

		if(m_pColorMap)
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