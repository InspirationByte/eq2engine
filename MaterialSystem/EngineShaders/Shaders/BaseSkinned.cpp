//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basic skinned shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(BaseSkinned)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture	= NULL;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;
		SHADER_PASS(Ambient_noskin) = NULL;
		SHADER_FOGPASS(Ambient_noskin) = NULL;

		SHADER_PASS(AmbientInst) = NULL;
		SHADER_FOGPASS(AmbientInst) = NULL;
		SHADER_PASS(AmbientInst_noskin) = NULL;
		SHADER_FOGPASS(AmbientInst_noskin) = NULL;

		m_nFlags |= MATERIAL_FLAG_SKINNED;
	}

	SHADER_INIT_TEXTURES()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Ambient))
			return true;

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(materials->GetConfiguration().editormode, "EDITOR");

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Ambient, "BaseSkinned_Ambient");
		SHADER_FIND_OR_COMPILE(Ambient_noskin, "BaseSkinned_Ambient_NoSkin");
		SHADER_FIND_OR_COMPILE(AmbientInst, "BaseSkinned_Inst_Ambient");
		SHADER_FIND_OR_COMPILE(AmbientInst_noskin, "BaseSkinned_Inst_Ambient_NoSkin");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
		// compile with fog
		SHADER_FIND_OR_COMPILE(Ambient_fog, "BaseSkinned_Ambient");
		SHADER_FIND_OR_COMPILE(Ambient_noskin_fog, "BaseSkinned_Ambient_NoSkin");
		SHADER_FIND_OR_COMPILE(AmbientInst_fog, "BaseSkinned_Inst_Ambient");
		SHADER_FIND_OR_COMPILE(AmbientInst_noskin_fog, "BaseSkinned_Inst_Ambient_NoSkin");

		return true;
	}

	void SetupShader()
	{
		if(materials->IsSkinningEnabled())
		{
			if(materials->IsInstancingEnabled())
				SHADER_BIND_PASS_FOGSELECT(AmbientInst)
			else
				SHADER_BIND_PASS_FOGSELECT(Ambient)
		}
		else
		{
			if(materials->IsInstancingEnabled())
				SHADER_BIND_PASS_FOGSELECT(AmbientInst_noskin)
			else
				SHADER_BIND_PASS_FOGSELECT(Ambient_noskin)
		}
	}

	void SetupConstants()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);
		SetupDefaultParameter(SHADERPARAM_BUMPMAP);
		SetupDefaultParameter(SHADERPARAM_SPECULARILLUM);
		SetupDefaultParameter(SHADERPARAM_CUBEMAP);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);

		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetConfiguration().wireframeMode ? ColorRGBA(0,1,1,1) : materials->GetAmbientColor());
	}

	void SetupBaseTexture()
	{
		ITexture* pSetupTexture = (materials->GetConfiguration().wireframeMode) ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTexture", 0);
	}

	ITexture*	GetBaseTexture(int stage)	{ return m_pBaseTexture; }
	ITexture*	GetBumpTexture(int stage)	{ return NULL; }

	ITexture*			m_pBaseTexture;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	SHADER_DECLARE_PASS(Ambient_noskin);
	SHADER_DECLARE_FOGPASS(Ambient_noskin);

	SHADER_DECLARE_PASS(AmbientInst);
	SHADER_DECLARE_FOGPASS(AmbientInst);

	SHADER_DECLARE_PASS(AmbientInst_noskin);
	SHADER_DECLARE_FOGPASS(AmbientInst_noskin);

END_SHADER_CLASS