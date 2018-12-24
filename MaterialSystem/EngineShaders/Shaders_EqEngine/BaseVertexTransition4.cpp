//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base vertex transition - 4 texture lerping shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "../vars_generic.h"

BEGIN_SHADER_CLASS(BaseVertexTransition)

	SHADER_INIT_PARAMS()
	{
		memset(m_pBaseTextures, 0, sizeof(m_pBaseTextures));
		memset(m_pBumpTextures, 0, sizeof(m_pBumpTextures));
		memset(m_pSpecularTex, 0, sizeof(m_pSpecularTex));

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE(BaseTexture,			m_pBaseTextures[0]);
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture2,	m_pBaseTextures[1]);
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture3,	m_pBaseTextures[2]);
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture4,	m_pBaseTextures[3]);

		if(materials->GetConfiguration().enableBumpmapping)
		{
			SHADER_PARAM_TEXTURE_NOERROR(BumpMap,	m_pBumpTextures[0]);
			SHADER_PARAM_TEXTURE_NOERROR(BumpMap2,	m_pBumpTextures[1]);
			SHADER_PARAM_TEXTURE_NOERROR(BumpMap3,	m_pBumpTextures[2]);
			SHADER_PARAM_TEXTURE_NOERROR(BumpMap4,	m_pBumpTextures[3]);
		}

		if(materials->GetConfiguration().enableSpecular)
		{
			SHADER_PARAM_TEXTURE_NOERROR(Specular,	m_pSpecularTex[0]);
			SHADER_PARAM_TEXTURE_NOERROR(Specula2,	m_pSpecularTex[1]);
			SHADER_PARAM_TEXTURE_NOERROR(Specular3,	m_pSpecularTex[2]);
			SHADER_PARAM_TEXTURE_NOERROR(Specular4,	m_pSpecularTex[3]);
		}

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTextures);

		// set bumpmap setup, if disabled - keep empty functor
		if(materials->GetConfiguration().enableBumpmapping)
			SetParameterFunctor(SHADERPARAM_BUMPMAP, &ThisShaderClass::SetupBumpTextures);

		if(m_pSpecularTex[0] || m_pSpecularTex[1] || m_pSpecularTex[2] || m_pSpecularTex[3])
		{
			SetParameterFunctor(SHADERPARAM_SPECULARILLUM, &ThisShaderClass::SetupSpecularTextures);
		}

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);

		m_nFlags |= MATERIAL_FLAG_ISTEXTRANSITION;
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Ambient))
			return true;

		bool bDeferredShading = (materials->GetConfiguration().lighting_model == MATERIAL_LIGHT_DEFERRED);

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		bool bSpecularIllum1 = false;
		bool bSpecularIllum2 = false;
		bool bSpecularIllum3 = false;
		bool bSpecularIllum4 = false;
		bool bParallax1 = false;
		bool bParallax2 = false;
		bool bParallax3 = false;
		bool bParallax4 = false;
		float fParallaxScale1 = 1.0f;
		float fParallaxScale2 = 1.0f;
		float fParallaxScale3 = 1.0f;
		float fParallaxScale4 = 1.0f;

		// illumination from specular alpha
		SHADER_PARAM_BOOL(SpecularIllum, bSpecularIllum1, false);
		SHADER_PARAM_BOOL(SpecularIllum2, bSpecularIllum2, false);
		SHADER_PARAM_BOOL(SpecularIllum3, bSpecularIllum3, false);
		SHADER_PARAM_BOOL(SpecularIllum4, bSpecularIllum4, false);

		// parallax as the bump map alpha
		SHADER_PARAM_BOOL(Parallax, bParallax1, false);
		SHADER_PARAM_BOOL(Parallax2, bParallax2, false);
		SHADER_PARAM_BOOL(Parallax3, bParallax3, false);
		SHADER_PARAM_BOOL(Parallax4, bParallax4, false);

		// parallax scale
		SHADER_PARAM_FLOAT(ParallaxScale, fParallaxScale1, 1.0f);
		SHADER_PARAM_FLOAT(ParallaxScale1, fParallaxScale2, 1.0f);
		SHADER_PARAM_FLOAT(ParallaxScale2, fParallaxScale3, 1.0f);
		SHADER_PARAM_FLOAT(ParallaxScale3, fParallaxScale4, 1.0f);

		// base usage table
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pBaseTextures[0] != NULL), "USE_BASE1");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pBaseTextures[1] != NULL), "USE_BASE2");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pBaseTextures[2] != NULL), "USE_BASE3");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pBaseTextures[3] != NULL), "USE_BASE4");

		// bump usage table
		SHADER_BEGIN_DEFINITION((m_pBumpTextures[0] != NULL), "USE_BUMP1")
			// define parallax
			SHADER_BEGIN_DEFINITION(bParallax1, "PARALLAX1")
				SHADER_ADD_FLOAT_DEFINITION("PARALLAX1_SCALE", fParallaxScale1);
			SHADER_END_DEFINITION
		SHADER_END_DEFINITION

		SHADER_BEGIN_DEFINITION((m_pBumpTextures[1] != NULL), "USE_BUMP2");
			// define parallax
			SHADER_BEGIN_DEFINITION(bParallax2, "PARALLAX2")
				SHADER_ADD_FLOAT_DEFINITION("PARALLAX2_SCALE", fParallaxScale2);
			SHADER_END_DEFINITION
		SHADER_END_DEFINITION
		SHADER_BEGIN_DEFINITION((m_pBumpTextures[2] != NULL), "USE_BUMP3");
			// define parallax
			SHADER_BEGIN_DEFINITION(bParallax3, "PARALLAX3")
				SHADER_ADD_FLOAT_DEFINITION("PARALLAX3_SCALE", fParallaxScale3);
			SHADER_END_DEFINITION
		SHADER_END_DEFINITION
		SHADER_BEGIN_DEFINITION((m_pBumpTextures[3] != NULL), "USE_BUMP4");
			// define parallax
			SHADER_BEGIN_DEFINITION(bParallax4, "PARALLAX4")
				SHADER_ADD_FLOAT_DEFINITION("PARALLAX4_SCALE", fParallaxScale4);
			SHADER_END_DEFINITION
		SHADER_END_DEFINITION

		// define specular usage
		SHADER_BEGIN_DEFINITION((m_pSpecularTex[0] != NULL), "USE_SPEC1")
			// define self illumination
			SHADER_DECLARE_SIMPLE_DEFINITION(bSpecularIllum1, "SELFILLUMINATION1")
		SHADER_END_DEFINITION

		// define specular usage
		SHADER_BEGIN_DEFINITION((m_pSpecularTex[1] != NULL), "USE_SPEC2")
			// define self illumination
			SHADER_DECLARE_SIMPLE_DEFINITION(bSpecularIllum2, "SELFILLUMINATION2")
		SHADER_END_DEFINITION

		// define specular usage
		SHADER_BEGIN_DEFINITION((m_pSpecularTex[2] != NULL), "USE_SPEC3")
			// define self illumination
			SHADER_DECLARE_SIMPLE_DEFINITION(bSpecularIllum3, "SELFILLUMINATION3")
		SHADER_END_DEFINITION

		// define specular usage
		SHADER_BEGIN_DEFINITION((m_pSpecularTex[3] != NULL), "USE_SPEC4")
			// define self illumination
			SHADER_DECLARE_SIMPLE_DEFINITION(bSpecularIllum4, "SELFILLUMINATION4")
		SHADER_END_DEFINITION

		// initialize shaders
		if(bDeferredShading)
		{
			// disable FFP alphatesting, let use the shader for better perfomance
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &ThisShaderClass::ParamSetup_AlphaModel_Solid);

			// alphatesting because stock DX9 and OGL sucks
			SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BVT_Deferred");

			// compile with fog
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BVT_Deferred");
		}
		else
		{
			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BVT_Ambient");

			// compile with fog
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BVT_Ambient");
		}

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_FOGSELECT(Ambient);
	}

	SHADER_SETUP_CONSTANTS()
	{
		if(IsError())
			return;

		// setup all parameters
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);
		SetupDefaultParameter(SHADERPARAM_BUMPMAP);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation()
	{
		// c0 - ambient color
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());
	}

	void SetupBaseTextures()
	{
		// setup base textures
		g_pShaderAPI->SetTexture(m_pBaseTextures[0], NULL, 0);
		g_pShaderAPI->SetTexture(m_pBaseTextures[1], NULL, 1);
		g_pShaderAPI->SetTexture(m_pBaseTextures[2], NULL, 2);
		g_pShaderAPI->SetTexture(m_pBaseTextures[3], NULL, 3);

		if(materials->GetConfiguration().wireframeMode || r_showlightmaps->GetInt() == 1)
		{
			// setup base textures
			g_pShaderAPI->SetTexture(materials->GetWhiteTexture(), NULL, 0);
			g_pShaderAPI->SetTexture(materials->GetWhiteTexture(), NULL, 1);
			g_pShaderAPI->SetTexture(materials->GetWhiteTexture(), NULL, 2);
			g_pShaderAPI->SetTexture(materials->GetWhiteTexture(), NULL, 3);
		}
	}

	void SetupBumpTextures()
	{
		// setup bumps
		g_pShaderAPI->SetTexture(m_pBumpTextures[0], NULL, 4);
		g_pShaderAPI->SetTexture(m_pBumpTextures[1], NULL, 5);
		g_pShaderAPI->SetTexture(m_pBumpTextures[2], NULL, 6);
		g_pShaderAPI->SetTexture(m_pBumpTextures[3], NULL, 7);
	}

	
	void SetupSpecularTextures()
	{
		/*
		TODO: support BVT4 specular maps
		// setup bumps
		g_pShaderAPI->SetTexture(m_pSpecularTex[0], 8);
		g_pShaderAPI->SetTexture(m_pSpecularTex[1], 9);
		g_pShaderAPI->SetTexture(m_pSpecularTex[2], 10);
		g_pShaderAPI->SetTexture(m_pSpecularTex[3], 11);
		*/
	}
	
	ITexture*	GetBaseTexture(int stage)
	{
		return m_pBaseTextures[stage];
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return m_pBumpTextures[stage];
	}

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Ambient);
	}

	ITexture*			m_pBaseTextures[4];
	ITexture*			m_pBumpTextures[4];
	ITexture*			m_pSpecularTex[4];

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

END_SHADER_CLASS