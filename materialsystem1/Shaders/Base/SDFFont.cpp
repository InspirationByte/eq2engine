//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Signed Distance Field font
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(SDFFont)
	SHADER_INIT_PARAMS()
	{
		m_fontParamsVar = GetAssignedMaterial()->GetMaterialVar("FontParams", "[0.94 0.95, 0, 1]");
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);

		SHADER_PASS(Unlit) = nullptr;
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture)
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

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_COLOR);

		renderAPI->SetShaderConstant(StringToHashConst("FontParams"), m_fontParamsVar.Get());
	}

	void SetupBaseTexture(IShaderAPI* renderAPI)
	{
		renderAPI->SetTexture(StringToHashConst("BaseTextureSampler"), m_baseTexture.Get());
	}

	void SetColorModulation(IShaderAPI* renderAPI)
	{
		ColorRGBA setColor = g_matSystem->GetAmbientColor();
		renderAPI->SetShaderConstant(StringToHashConst("AmbientColor"), setColor);
	}

	SHADER_DECLARE_PASS(Unlit);

	MatTextureProxy	m_baseTexture;
	MatVec4Proxy	m_fontParamsVar;
END_SHADER_CLASS