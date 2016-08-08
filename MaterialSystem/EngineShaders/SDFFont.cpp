//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Signed Distance Field font
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CSDFFontShader : public CBaseShader
{
public:
	CSDFFontShader()
	{
		SHADER_PASS(Unlit) = NULL;
	}

	void InitParams()
	{
		if(!m_pAssignedMaterial->IsError() && !m_bInitialized && !m_bIsError)
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

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "SDFFont");

		m_depthtest = false;
		m_depthwrite = false;

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

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
	}

	void SetColorModulation()
	{

	}

	void SetupBaseTexture0()
	{

	}

	const char* GetName()
	{
		return "SDFFont";
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

DEFINE_SHADER(SDFFont, CSDFFontShader)