//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Error Shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CEQLCPointLight : public CBaseShader
{
public:
	CEQLCPointLight()
	{
		SHADER_PASS(Program) = NULL;
		m_bDirMap = false;
	}

	void InitTextures()
	{
		// set texture setup
		SetParameterFunctor(SHADERPARAM_COLOR, &CEQLCPointLight::SetColorModulation);
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Additive);

		SHADER_PARAM_BOOL(Direction, m_bDirMap)

		if(m_bDirMap)
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Translucent);
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Program))
			return true;

		// required
		SHADERDEFINES_BEGIN

		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_RECEIVESHADOWS), "SHADOWMAP");

		if(!m_bDirMap)
		{
			SHADER_FIND_OR_COMPILE(Program, "EQLCPointLight")
		}
		else
		{
			SHADER_FIND_OR_COMPILE(Program, "EQLCPointLight_Dir")
		}

		return true;
	}


	void ParamSetup_RasterState()
	{
		materials->SetRasterizerStates(CULL_NONE, (FillMode_e)materials->GetConfiguration().wireframeMode);
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Program);
	}

	SHADER_SETUP_CONSTANTS()
	{
		if(IsError())
			return;

		m_depthtest = false;
		m_depthwrite = false;

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);

		ITexture* masks[2] = {materials->GetWhiteTexture(), materials->GetLight()->pMaskTexture};
		int has_mask = (materials->GetLight()->pMaskTexture > 0);

		// setup light mask
		g_pShaderAPI->SetTexture(masks[has_mask], "MaskTexture", 4);

		Vector3D lightPosView = materials->GetLight()->position;

		g_pShaderAPI->SetShaderConstantVector4D("lightPos", Vector4D(lightPosView, 1.0 / materials->GetLight()->radius.x));
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("lightColor", Vector4D(materials->GetLight()->color*materials->GetLight()->intensity, materials->GetLight()->intensity));
	}

	const char* GetName()
	{
		return "EQLCPointLight";
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

	SHADER_DECLARE_PASS(Program);
	bool				m_bDirMap;
};

DEFINE_SHADER(EQLCPointLight,CEQLCPointLight)