//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(Shadow)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture = NULL;
		SHADER_PASS(Unlit) = NULL;
		SHADER_FOGPASS(Unlit) = NULL;
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture, m_pBaseTexture);

		// set texture setup
		if(m_pBaseTexture)
			SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "Shadow");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");

		// compile with fog
		SHADER_FIND_OR_COMPILE(Unlit_fog, "Shadow");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_FOGSELECT(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetupBaseTexture0()
	{
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	ITexture* GetBaseTexture(int stage)
	{
		return m_pBaseTexture;
	}

	ITexture* GetBumpTexture(int stage)
	{
		return NULL;
	}

private:
	ITexture*			m_pBaseTexture;

	SHADER_DECLARE_PASS(Unlit);
	SHADER_DECLARE_FOGPASS(Unlit);

END_SHADER_CLASS