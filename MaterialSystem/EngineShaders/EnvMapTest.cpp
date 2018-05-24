//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Environment map test
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

//--------------------------------------
// Basic cubemap skybox shader
//--------------------------------------

BEGIN_SHADER_CLASS(EnvMapTest)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = NULL;
	}

	// Initialize textures
	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

	}

	SHADER_INIT_RHI()
	{
		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "EnvMapTest");

		return true;
	}

	ITexture* GetBaseTexture(int stage) {return NULL;}
	ITexture* GetBumpTexture(int stage) {return NULL;}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	// Sets constants
	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);

		// setup base texture
		if (m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP)
			g_pShaderAPI->SetTexture(materials->GetEnvironmentMapTexture(), "Base", 0);
		else
			g_pShaderAPI->SetTexture(m_pCubemap, "Base", 0);
	}

	ITexture* m_pCubemap;

	SHADER_DECLARE_PASS(Unlit);

END_SHADER_CLASS