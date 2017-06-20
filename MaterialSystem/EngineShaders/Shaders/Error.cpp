//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Error Shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(Error)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = NULL;
		m_pBaseTexture = NULL;
	}

	SHADER_INIT_TEXTURES()
	{
		if(m_pBaseTexture)
			return;

		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture)

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CErrorShader::SetupBaseTexture0);
		SetParameterFunctor(SHADERPARAM_COLOR, &CErrorShader::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "BaseUnlit");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
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

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	ITexture*	GetBaseTexture(int stage) {return m_pBaseTexture;}
	ITexture*	GetBumpTexture(int stage) {return NULL;}

	ITexture*			m_pBaseTexture;

	SHADER_DECLARE_PASS(Unlit);

END_SHADER_CLASS