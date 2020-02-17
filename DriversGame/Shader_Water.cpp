//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Basis Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(DrvSynWater)
	SHADER_INIT_PARAMS()
	{
		m_pBumpTexture	= NULL;
		m_fSpecularScale = 0.0f;
		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		// disable shadows
		m_nFlags |= MATERIAL_FLAG_WATER;
		m_nFlags &= ~MATERIAL_FLAG_RECEIVESHADOWS;

		m_pBumpFrame = m_pAssignedMaterial->GetMaterialVar("bumptextureframe", 0);
		m_pWaterColor = m_pAssignedMaterial->GetMaterialVar("Color", "[0.2 0.7 0.4]" );
	}

	SHADER_INIT_TEXTURES()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BumpMap, m_pBumpTexture);
		
		// load cubemap if available
		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BUMPMAP, &ThisShaderClass::SetupBumpTexture);
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Ambient))
			return true;

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		// parallax scale
		SHADER_PARAM_FLOAT(SpecularScale, m_fSpecularScale, 0.0f);

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

		// compile without fog
		SHADER_FIND_OR_COMPILE(Ambient, "Water");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
		// compile with fog
		SHADER_FIND_OR_COMPILE(Ambient_fog, "Water");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_FOGSELECT( Ambient );
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter( SHADERPARAM_TRANSFORM );
		SetupDefaultParameter( SHADERPARAM_ANIMFRAME );
		SetupDefaultParameter( SHADERPARAM_BUMPMAP );
		SetupDefaultParameter( SHADERPARAM_CUBEMAP );

		SetupDefaultParameter( SHADERPARAM_ALPHASETUP );
		SetupDefaultParameter( SHADERPARAM_DEPTHSETUP );
		SetupDefaultParameter( SHADERPARAM_RASTERSETUP );

		SetupDefaultParameter( SHADERPARAM_COLOR );
		SetupDefaultParameter( SHADERPARAM_FOG );
		

		g_pShaderAPI->SetShaderConstantFloat("SPECULAR_SCALE", m_fSpecularScale);
	}

	void ParamSetup_TextureFrames()
	{
		if(m_pBumpTexture)
			m_pBumpTexture->SetAnimatedTextureFrame(m_pBumpFrame->GetInt());
	}

	void ParamSetup_Cubemap()
	{
		if(m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP)
			g_pShaderAPI->SetTexture( materials->GetEnvironmentMapTexture() , "CubemapSampler", 12 );
		else
			g_pShaderAPI->SetTexture(m_pCubemap, "CubemapTexture", 12);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);

		ColorRGBA waterColor = ColorRGBA(m_pWaterColor->GetVector3(), 1.0f);
		g_pShaderAPI->SetShaderConstantVector4D("WaterColor", waterColor);
	}

	void SetupBumpTexture()
	{
		g_pShaderAPI->SetTexture(m_pBumpTexture, "BumpTextureSampler", 0);
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return m_pBumpTexture;
	}

	ITexture*			m_pBumpTexture;
	ITexture*			m_pCubemap;

	IMatVar*			m_pWaterColor;
	IMatVar*			m_pBumpFrame;

	float				m_fSpecularScale;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);
END_SHADER_CLASS