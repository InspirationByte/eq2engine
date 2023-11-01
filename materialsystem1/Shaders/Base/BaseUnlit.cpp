//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(BaseUnlit)

	bool IsSupportVertexFormat(int nameHash) const
	{
		// must support any vertex
		return true;
	}

	SHADER_INIT_PARAMS()
	{
		m_colorVar = m_material->GetMaterialVar("color", "[1 1 1 1]");
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture, m_baseTexture);

		// set texture setup
		if(m_baseTexture.IsValid())
			SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RENDERPASS_PIPELINE()
	{
		if(SHADER_PASS(Unlit))
			return true;

		bool fogEnable = true;
		SHADER_PARAM_BOOL_NEG(NoFog, fogEnable, false)

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_flags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		bool bVertexColor = false;
		SHADER_PARAM_BOOL(StudioVertexColor, bVertexColor, false);
		SHADER_DECLARE_SIMPLE_DEFINITION(bVertexColor, "STUDIOVERTEXCOLOR");

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
		SHADER_DECLARE_SIMPLE_DEFINITION(fogEnable, "DOFOG");

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
		if (g_matSystem->IsSkinningEnabled())
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
		SetupDefaultParameter(SHADERPARAM_BONETRANSFORMS);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation(IShaderAPI* renderAPI)
	{
		ColorRGBA setColor = m_colorVar.Get() * g_matSystem->GetAmbientColor();

		renderAPI->SetShaderConstant(StringToHashConst("AmbientColor"), setColor);
	}

	void SetupBaseTexture0(IShaderAPI* renderAPI)
	{
		ITexturePtr setupTexture = g_matSystem->GetConfiguration().wireframeMode ? g_matSystem->GetWhiteTexture() : m_baseTexture.Get();

		renderAPI->SetTexture(StringToHashConst("BaseTextureSampler"), setupTexture);
	}

	const ITexturePtr& GetBaseTexture(int stage) const {return m_baseTexture.Get();}

	MatTextureProxy	m_baseTexture;
	MatVec4Proxy	m_colorVar;

	SHADER_DECLARE_PASS(Unlit);
	SHADER_DECLARE_FOGPASS(Unlit);

	SHADER_DECLARE_PASS(Unlit_noskin);
	SHADER_DECLARE_FOGPASS(Unlit_noskin);

END_SHADER_CLASS
