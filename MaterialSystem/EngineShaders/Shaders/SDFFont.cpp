//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Signed Distance Field font
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(SDFFont)
	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = NULL;
	}

	SHADER_INIT_TEXTURES()
	{

	}

	SHADER_INIT_RHI()
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

		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	void SetupConstants()
	{
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

	ITexture*	GetBaseTexture(int stage) {return NULL;}
	ITexture*	GetBumpTexture(int stage) {return NULL;}


	SHADER_DECLARE_PASS(Unlit);


END_SHADER_CLASS