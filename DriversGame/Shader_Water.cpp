//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basis Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CShader_Water : public CBaseShader
{
private:
	ITexture*			m_pBumpTexture;
	ITexture*			m_pCubemap;

	IMatVar*			m_pWaterColor;
	IMatVar*			m_pBumpFrame;

	float				m_fSpecularScale;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

public:
	CShader_Water()
	{
		m_pBumpTexture	= NULL;

		m_fSpecularScale = 0.0f;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		m_nFlags = (MATERIAL_FLAG_WATER);
	}

	void InitTextures()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BumpMap, m_pBumpTexture);
		
		// load cubemap if available
		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BUMPMAP, &CShader_Water::SetupBumpTexture);
		SetParameterFunctor(SHADERPARAM_COLOR, &CShader_Water::SetColorModulation);
	}

	void InitParams()
	{
		if(!m_bInitialized && !m_bIsError)
		{
			CBaseShader::InitParams();

			m_pBumpFrame = m_pAssignedMaterial->GetMaterialVar("bumptextureframe", 0);
			m_pWaterColor = m_pAssignedMaterial->GetMaterialVar("Color", "[0.2 0.7 0.4]" );
		}
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Ambient))
			return true;


		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

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

		// compile without fog
		SHADER_FIND_OR_COMPILE(Ambient, "Water");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
		// compile with fog
		SHADER_FIND_OR_COMPILE(Ambient_fog, "Water");

		return true;
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_FOGSELECT( Ambient );
	}

	void SetupConstants()
	{
		if(IsError())
			return;

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

	const char* GetName()
	{
		return "DrvSynWater";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return m_pBumpTexture;
	}

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Ambient);
	}
};

DEFINE_SHADER(DrvSynWater, CShader_Water)