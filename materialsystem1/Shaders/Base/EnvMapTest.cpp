//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Environment map test
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

//--------------------------------------
// Basic cubemap skybox shader
//--------------------------------------

BEGIN_SHADER_CLASS(EnvMapTest)
	bool IsSupportVertexFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("EGFVertex");
	}

	SHADER_INIT_PARAMS()
	{
	}

	// Initialize textures
	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_cubemapTexture);
	}

	SHADER_INIT_RENDERPASS_PIPELINE()
	{
		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "EnvMapTest");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	// Sets constants
	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_BONETRANSFORMS);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);

		renderAPI->SetTexture(StringToHashConst("Base"), m_cubemapTexture.Get());
	}

	MatTextureProxy m_cubemapTexture;

	SHADER_DECLARE_PASS(Unlit);

END_SHADER_CLASS