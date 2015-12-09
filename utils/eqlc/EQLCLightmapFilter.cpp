//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap filter
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CEQLCLightmapFilter : public CBaseShader
{
public:
	CEQLCLightmapFilter()
	{
		SHADER_PASS(Filter) = NULL;
	}

	void InitTextures()
	{

	}

	bool InitShaders()
	{
		if(SHADER_PASS(Filter))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Filter, "EQLC/LightmapFilter");

		return true;
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Filter);
	}

	void SetupConstants()
	{
		if(IsError())
			return;

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());
	}

	const char* GetName()
	{
		return "EQLCLightmapFilter";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}
private:

	SHADER_DECLARE_PASS(Filter);
};

DEFINE_SHADER(EQLCLightmapFilter, CEQLCLightmapFilter)

class CEQLCLightmapWireCombine : public CBaseShader
{
public:
	CEQLCLightmapWireCombine()
	{
		SHADER_PASS(Filter) = NULL;
	}

	void InitTextures()
	{
		// combiner is additive
		//SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Additive);
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Filter))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Filter, "EQLC/LightmapWireCombine");

		return true;
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Filter);
	}

	void SetupConstants()
	{
		if(IsError())
			return;

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());
	}

	const char* GetName()
	{
		return "EQLCLightmapWireCombine";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}
private:

	SHADER_DECLARE_PASS(Filter);
};

DEFINE_SHADER(EQLCLightmapWireCombine, CEQLCLightmapWireCombine)