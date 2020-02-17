//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Error Shader
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem/BaseShader.h"

BEGIN_SHADER_CLASS(EditorFlatColor)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Ambient) = NULL;
	}

	SHADER_INIT_TEXTURES()
	{
		// set texture setup
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if (SHADER_PASS(Ambient))
			return true;

		SHADERDEFINES_BEGIN;

		SHADER_FIND_OR_COMPILE(Ambient, "EditorFlatColor");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Ambient);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);

		//m_depthwrite = true;

		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);

	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());

		if(materials->GetAmbientColor().w < 1.0f)
		{
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &ThisShaderClass::ParamSetup_AlphaModel_Translucent);
		}
		else
		{
			m_depthwrite = true;
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &ThisShaderClass::ParamSetup_AlphaModel_Solid);
		}
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

	SHADER_DECLARE_PASS(Ambient);

END_SHADER_CLASS