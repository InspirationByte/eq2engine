//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

		SHADER_PASS(Unlit) = nullptr;
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

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_COLOR);

		g_pShaderAPI->SetShaderConstantVector4D("FontParams", m_fontParamsVar.GetVector4());
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	ITexture*	GetBaseTexture(int stage) const {return nullptr;}
	ITexture*	GetBumpTexture(int stage) const {return nullptr;}

	SHADER_DECLARE_PASS(Unlit);

	MatVarProxy	m_fontParamsVar;
END_SHADER_CLASS