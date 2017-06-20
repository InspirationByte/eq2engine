//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CHDRBloomFilter : public CBaseShader
{
public:
	CHDRBloomFilter()
	{
		m_pBaseTexture = NULL;

		SHADER_PASS(Unlit) = NULL;
	}

	void InitTextures()
	{
		// parse material variables
		SHADER_PARAM_RENDERTARGET_FIND(BaseTexture, m_pBaseTexture);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CHDRBloomFilter::SetupBaseTexture0);
		//SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CHDRBloomFilter::ParamSetup_AlphaModel_Additive);
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "HDR_Bloom");

		m_depthtest = false;
		m_depthwrite = false;

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Unlit);

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());
	}

	void SetupBaseTexture0()
	{
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

		//g_pShaderAPI->CopyFramebufferToTexture( m_pBaseTexture );

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	const char* GetName()
	{
		return "HDRBloom";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return m_pBaseTexture;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Unlit);
	}
private:

	ITexture*			m_pBaseTexture;

	SHADER_DECLARE_PASS(Unlit);
};

DEFINE_SHADER(HDRBloom, CHDRBloomFilter)