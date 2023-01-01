//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(BaseUnlit)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture = nullptr;

		SHADER_PASS(Unlit) = nullptr;
		SHADER_FOGPASS(Unlit) = nullptr;

		m_colorVar = GetAssignedMaterial()->GetMaterialVar("color", "[1 1 1 1]");

		m_nFlags |= MATERIAL_FLAG_SKINNED;
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture, m_pBaseTexture);

		// set texture setup
		if(m_pBaseTexture)
			SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		// compile without fog
		{
			SHADER_FIND_OR_COMPILE(Unlit_noskin, "BaseUnlit");
		}
		
		{
			EqString oldDefines = defines;
			EqString oldFindQuery = findQuery;

			SHADER_DECLARE_SIMPLE_DEFINITION(true, "SKIN");
			SHADER_FIND_OR_COMPILE(Unlit, "BaseUnlit");

			defines = oldDefines;
			findQuery = oldFindQuery;
		}

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");

		// compile with fog
		{
			SHADER_FIND_OR_COMPILE(Unlit_noskin_fog, "BaseUnlit");
		}

		{
			SHADER_DECLARE_SIMPLE_DEFINITION(true, "SKIN");
			SHADER_FIND_OR_COMPILE(Unlit_fog, "BaseUnlit");
		}

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if (materials->IsSkinningEnabled())
		{
			SHADER_BIND_PASS_FOGSELECT(Unlit)
		}
		else
		{
			SHADER_BIND_PASS_FOGSELECT(Unlit_noskin)
		}
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = m_colorVar.GetVector4() * materials->GetAmbientColor();

		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetupBaseTexture0()
	{
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	ITexture*	GetBaseTexture(int stage) const {return m_pBaseTexture;}
	ITexture*	GetBumpTexture(int stage) const {return nullptr;}

	ITexture*	m_pBaseTexture;
	MatVarProxy	m_colorVar;

	SHADER_DECLARE_PASS(Unlit);
	SHADER_DECLARE_FOGPASS(Unlit);

	SHADER_DECLARE_PASS(Unlit_noskin);
	SHADER_DECLARE_FOGPASS(Unlit_noskin);

END_SHADER_CLASS
