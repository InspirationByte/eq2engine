//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Sky shader for driver syndicate (dynamic weather)
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(DrvSynSky)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = NULL;

		m_color = GetAssignedMaterial()->GetMaterialVar("color", "[1 1 1 1]");
		m_base1 = GetAssignedMaterial()->GetMaterialVar("Base1", "$base1");
		m_base2 = GetAssignedMaterial()->GetMaterialVar("Base2", "$base1");
		m_interpFac = GetAssignedMaterial()->GetMaterialVar("Interp", "0.0");

		m_depthtest = true;
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
		SHADER_FIND_OR_COMPILE(Unlit, "Sky");

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

		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", m_color->GetVector4());
		g_pShaderAPI->SetShaderConstantFloat("InterpFactor", m_interpFac->GetFloat());
	}

	void SetupBaseTextures()
	{
		g_pShaderAPI->SetTexture(m_base1->GetTexture(), "Base1", 0);
		g_pShaderAPI->SetTexture(m_base2->GetTexture(), "Base2", 1);
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

private:

	SHADER_DECLARE_PASS(Unlit);

	IMatVar* m_base1;
	IMatVar* m_base2;
	IMatVar* m_interpFac;
	IMatVar* m_color;

END_SHADER_CLASS