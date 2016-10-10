//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CShadowBuild : public CBaseShader
{
public:
	CShadowBuild()
	{
		SHADER_PASS(Unlit) = NULL;
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
		SHADER_FIND_OR_COMPILE(Unlit, "BaseUnlit");

		return true;
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Unlit);
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
	}

	void SetupBaseTexture0()
	{
		/*
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;
		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
		*/
	}

	const char* GetName()
	{
		return "ShadowBuild";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
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

	SHADER_DECLARE_PASS(Unlit);
};

DEFINE_SHADER(ShadowBuild, CShadowBuild)