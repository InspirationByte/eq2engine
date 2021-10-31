//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Depth combiner shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(DepthCombiner)

	SHADER_INIT_PARAMS()
	{
		memset(m_pTextures, 0, sizeof(m_pTextures));
		SHADER_PASS(Unlit) = NULL;

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_RENDERTARGET_FIND(Texture1, m_pTextures[0]);
		SHADER_PARAM_RENDERTARGET_FIND(Texture2, m_pTextures[1]);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// TODO: mode (greater/great or equal etc)
		// SHADER_DECLARE_SIMPLE_DEFINITION(modeVar, "MODE_GREATER_OR_EQUAL");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "DepthCombiner");

		m_depthtest = false;
		m_depthwrite = false;

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
	}


	void SetupBaseTexture0()
	{
		g_pShaderAPI->SetTexture(m_pTextures[0], "Texture1", 0);
		g_pShaderAPI->SetTexture(m_pTextures[1], "Texture2", 1);
	}

	ITexture*	GetBaseTexture(int stage) {return m_pTextures[stage & 1];}
	ITexture*	GetBumpTexture(int stage) {return NULL;}

	ITexture*			m_pTextures[2];

	SHADER_DECLARE_PASS(Unlit);

END_SHADER_CLASS