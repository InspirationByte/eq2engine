//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CShadowShader : public CBaseShader
{
public:
	CShadowShader()
	{
		m_pBaseTexture = NULL;

		SHADER_PASS(Unlit) = NULL;
		SHADER_FOGPASS(Unlit) = NULL;
	}

	void InitParams()
	{
		if(!m_bInitialized && !m_bIsError)
		{
			CBaseShader::InitParams();
		}
	}

	void InitTextures()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture, m_pBaseTexture);

		// set texture setup
		if(m_pBaseTexture)
			SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CShadowShader::SetupBaseTexture0);

		SetParameterFunctor(SHADERPARAM_COLOR, &CShadowShader::SetColorModulation);
	}

	bool InitShaders()
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

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_FOGSELECT(Unlit);
	}

	void SetupConstants()
	{
		if(IsError())
			return;

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

	const char* GetName()
	{
		return "Shadow";
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
	SHADER_DECLARE_FOGPASS(Unlit);
};

DEFINE_SHADER(Shadow, CShadowShader)