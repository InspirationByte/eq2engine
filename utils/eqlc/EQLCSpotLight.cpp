//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Error Shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CEQLCSpotLight : public CBaseShader
{
public:
	CEQLCSpotLight()
	{
		SHADER_PASS(Program) = NULL;
		m_bDirMap = false;
	}

	void InitTextures()
	{
		// set texture setup
		SetParameterFunctor(SHADERPARAM_COLOR, &CEQLCSpotLight::SetColorModulation);
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
			SHADER_FIND_OR_COMPILE(Program, "EQLCSpotLight")
		}
		else
		{
			SHADER_FIND_OR_COMPILE(Program, "EQLCSpotLight_Dir")
		}

		return true;
	}

	void ParamSetup_RasterState()
	{
		materials->SetRasterizerStates(CULL_NONE, (ER_FillMode)materials->GetConfiguration().wireframeMode);
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

		Vector3D lightPosView = materials->GetLight()->position;

		g_pShaderAPI->SetShaderConstantVector4D("lightPos", Vector4D(lightPosView, 1.0 / materials->GetLight()->radius.x));
		g_pShaderAPI->SetShaderConstantFloat("lightRad", materials->GetLight()->radius.x);

		g_pShaderAPI->SetShaderConstantMatrix4("spot_mvp", materials->GetLight()->lightWVP);

		ITexture* masks[2] = {materials->GetWhiteTexture(), materials->GetLight()->pMaskTexture};
		int has_mask = (materials->GetLight()->pMaskTexture > 0);

		// setup light mask
		g_pShaderAPI->SetTexture(masks[has_mask], "MaskTexture", 4);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("lightColor", Vector4D(materials->GetLight()->color*materials->GetLight()->intensity, materials->GetLight()->intensity));
	}

	const char* GetName()
	{
		return "EQLCSpotLight";
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

	bool m_bDirMap;
};

DEFINE_SHADER(EQLCSpotLight,CEQLCSpotLight)