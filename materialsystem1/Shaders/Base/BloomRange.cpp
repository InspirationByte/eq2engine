//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Bloom range compression shader
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem1/BaseShader.h"

BEGIN_SHADER_CLASS(BloomRange)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = NULL;

		m_rangeProps = GetAssignedMaterial()->GetMaterialVar("RangeProps", "[0.6 40 100 100]");
		m_bloomSource = GetAssignedMaterial()->GetMaterialVar("BloomSource", "");

		m_depthtest = false;
		m_depthwrite = false;
	}

	SHADER_INIT_TEXTURES()
	{
		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTextures);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "BloomRange");

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

		g_pShaderAPI->SetShaderConstantVector4D("RangeProps", m_rangeProps->GetVector4());
	}

	void SetupBaseTextures()
	{
		g_pShaderAPI->SetTexture(m_bloomSource->GetTexture(), "BaseTexture", 0);
	}

	ITexture*	GetBaseTexture(int stage) const
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage) const
	{
		return NULL;
	}

private:

	SHADER_DECLARE_PASS(Unlit);

	IMatVar* m_rangeProps;
	IMatVar* m_bloomSource;

END_SHADER_CLASS