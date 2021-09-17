//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: UnLit shader. Used for model
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

//--------------------------------------
// Basic cubemap skybox shader
//--------------------------------------
BEGIN_SHADER_CLASS(BaseParticle)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Particle) = NULL;
		SHADER_FOGPASS(Particle) = NULL;

		m_pBaseTexture			= NULL;
		
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);
	}

	SHADER_INIT_RHI()
	{
		// just skip if we already have shader
		if(SHADER_PASS(Particle))
			return true;

		bool useAdvLighting = false;

		SHADER_PARAM_BOOL(AdvancedLighting, useAdvLighting, false);

		SHADERDEFINES_BEGIN

		// full scene shadow mapping
		SHADER_BEGIN_DEFINITION(materials->GetConfiguration().enableShadows, "SHADOWMAP");
			// TODO: shadowmap quality variable
			SHADER_DECLARE_SIMPLE_DEFINITION(true, "SHADOWMAP_HQ");
		SHADER_END_DEFINITION;

		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ADDITIVE), "ADDITIVE")

		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		SHADER_DECLARE_SIMPLE_DEFINITION(useAdvLighting, "ADVANCED_LIGHTING");

		SHADER_FIND_OR_COMPILE(Particle, "BaseParticle")

		SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG")

		SHADER_FIND_OR_COMPILE(Particle_fog, "BaseParticle")

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		FogInfo_t fg;
		materials->GetFogInfo(fg);

		SHADER_BIND_PASS_FOGSELECT(Particle);
	}

	SHADER_SETUP_CONSTANTS()
	{
		// If we has shader
		if(!IsError())
		{
			materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO);

			SetupDefaultParameter(SHADERPARAM_TRANSFORM);

			SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

			SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
			SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
			SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
			SetupDefaultParameter(SHADERPARAM_COLOR);
			SetupDefaultParameter(SHADERPARAM_FOG);
		}
		else
		{
			// Set error texture
			g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
		}
	}

	void SetupBaseTexture0()
	{
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetAdditiveColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	ITexture*	GetBaseTexture(int stage) {return m_pBaseTexture;}
	ITexture*	GetBumpTexture(int stage) {return NULL;}

	SHADER_DECLARE_PASS(Particle);
	SHADER_DECLARE_FOGPASS(Particle);

	ITexture*			m_pBaseTexture;
END_SHADER_CLASS