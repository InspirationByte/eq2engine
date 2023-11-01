//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Depth combiner shader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(DepthCombiner)

	bool IsSupportVertexFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE_FIND(Texture1, m_textures[0]);
		SHADER_PARAM_TEXTURE_FIND(Texture2, m_textures[1]);
	}

	SHADER_INIT_RENDERPASS_PIPELINE()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// TODO: mode (greater/great or equal etc)
		// SHADER_DECLARE_SIMPLE_DEFINITION(modeVar, "MODE_GREATER_OR_EQUAL");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "DepthCombiner");
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

	void SetupBaseTexture0(IShaderAPI* renderAPI)
	{
		renderAPI->SetTexture(StringToHashConst("Texture1"), m_textures[0].Get());
		renderAPI->SetTexture(StringToHashConst("Texture2"), m_textures[1].Get());
	}

	const ITexturePtr& GetBaseTexture(int stage)  const {return m_textures[stage & 1].Get();}

	MatTextureProxy	m_textures[2];

	SHADER_DECLARE_PASS(Unlit);

END_SHADER_CLASS