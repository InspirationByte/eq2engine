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
		m_rangeVar = GetAssignedMaterial()->GetMaterialVar("Range", "[0.94 0.95]");
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);

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

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_COLOR);

		g_pShaderAPI->SetShaderConstantVector2D("SDFRange", m_rangeVar->GetVector2());
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	ITexture*	GetBaseTexture(int stage) {return NULL;}
	ITexture*	GetBumpTexture(int stage) {return NULL;}

	SHADER_DECLARE_PASS(Unlit);

	IMatVar*	m_rangeVar;
END_SHADER_CLASS