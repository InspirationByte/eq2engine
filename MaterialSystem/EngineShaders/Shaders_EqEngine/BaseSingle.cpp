//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basic lightmapped static mesh shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "../vars_generic.h"

BEGIN_SHADER_CLASS(BaseSingle)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture	= NULL;
		m_pBumpTexture	= NULL;
		m_pSpecIllum	= NULL;
		m_pCubemap		= NULL;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		SHADER_PASS(AmbientInst) = NULL;
		SHADER_FOGPASS(AmbientInst) = NULL;

		m_bSpecularIllum = false;
		m_bParallax = false;
		m_fParallaxScale = 1.0f;
		m_fSpecularScale = 0.0f;
		m_bCubeMapLighting = false;
		m_fCubemapLightingScale = 1.0f;
	}

	SHADER_INIT_TEXTURES()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		if(materials->GetConfiguration().enableBumpmapping)
			SHADER_PARAM_TEXTURE_NOERROR(Bumpmap, m_pBumpTexture);

		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Specular, m_pSpecIllum);

		// load cubemap if available
		if (materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);

		if(m_pBumpTexture)
			SetParameterFunctor(SHADERPARAM_BUMPMAP, &ThisShaderClass::SetupBumpmap);

		if(m_pSpecIllum)
			SetParameterFunctor(SHADERPARAM_SPECULARILLUM, &ThisShaderClass::SetupSpecular);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Ambient))
			return true;

		bool bDeferredShading = (materials->GetConfiguration().lighting_model == MATERIAL_LIGHT_DEFERRED);
		bool bHasAnyTranslucency = (m_nFlags & MATERIAL_FLAG_TRANSPARENT) || (m_nFlags & MATERIAL_FLAG_ADDITIVE) || (m_nFlags & MATERIAL_FLAG_MODULATE);

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		// illumination from specular green channel
		SHADER_PARAM_BOOL(SpecularIllum, m_bSpecularIllum, false);

		SHADER_PARAM_BOOL(CubeMapLighting, m_bCubeMapLighting, false);

		SHADER_PARAM_FLOAT(CubeMapLightingScale, m_fCubemapLightingScale, 1.0f);

		// parallax as the bump map alpha
		SHADER_PARAM_BOOL(Parallax, m_bParallax, false);

		// parallax scale
		SHADER_PARAM_FLOAT(ParallaxScale, m_fParallaxScale, 1.0f);

		if(m_pSpecIllum)
			m_fSpecularScale = 1.0f;

		// parallax scale
		SHADER_PARAM_FLOAT(SpecularScale, m_fSpecularScale, m_fSpecularScale);

		bool bVertexSpecular = false;
		SHADER_PARAM_BOOL(VertexSpecular, bVertexSpecular, false);

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define modulate parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_MODULATE), "MODULATE");

		// define bump mapping
		SHADER_BEGIN_DEFINITION((m_pBumpTexture != NULL), "BUMPMAP")

			// define parallax
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bParallax, "PARALLAX")

		SHADER_END_DEFINITION

		bool useCubemap = (m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP) || (m_pCubemap != NULL);

		// define cubemap parameter.
		SHADER_BEGIN_DEFINITION(useCubemap, "CUBEMAP")
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bCubeMapLighting, "CUBEMAP_LIGHTING")
		SHADER_END_DEFINITION

		SHADER_DECLARE_SIMPLE_DEFINITION(bVertexSpecular, "VERTEXSPECULAR");

		// define specular usage
		SHADER_BEGIN_DEFINITION((m_pSpecIllum != NULL), "SPECULAR")
			// define self illumination
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bSpecularIllum, "SELFILLUMINATION");
			
		SHADER_END_DEFINITION

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		if(bDeferredShading && !bHasAnyTranslucency)
		{
			// disable FFP alphatesting, let use the shader for better perfomance
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &ThisShaderClass::ParamSetup_AlphaModel_Solid);

			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BaseSingle_Deferred");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BaseSingle_Deferred");
		}
		else
		{
			bool bUseLightmaps = (!materials->GetConfiguration().editormode);
			SHADER_DECLARE_SIMPLE_DEFINITION(bDeferredShading || bUseLightmaps, "LIGHTMAP");

			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BaseSingle_Ambient");
			SHADER_FIND_OR_COMPILE(AmbientInst, "BaseSingle_Inst_Ambient");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BaseSingle_Ambient");
			SHADER_FIND_OR_COMPILE(AmbientInst_fog, "BaseSingle_Inst_Ambient");
		}

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		g_pShaderAPI->Reset( STATE_RESET_SHADERCONST );

		if(materials->IsInstancingEnabled())
			SHADER_BIND_PASS_FOGSELECT(AmbientInst)
		else
			SHADER_BIND_PASS_FOGSELECT(Ambient)
	}

	SHADER_SETUP_CONSTANTS()
	{
		if(IsError())
			return;

		SetupDefaultParameter( SHADERPARAM_TRANSFORM );

		SetupDefaultParameter( SHADERPARAM_BASETEXTURE );
		SetupDefaultParameter( SHADERPARAM_BUMPMAP );
		SetupDefaultParameter( SHADERPARAM_SPECULARILLUM );
		SetupDefaultParameter( SHADERPARAM_CUBEMAP );

		SetupDefaultParameter( SHADERPARAM_ALPHASETUP );
		SetupDefaultParameter( SHADERPARAM_DEPTHSETUP );
		SetupDefaultParameter( SHADERPARAM_RASTERSETUP );

		SetupDefaultParameter( SHADERPARAM_COLOR );
		SetupDefaultParameter( SHADERPARAM_FOG );

		g_pShaderAPI->SetShaderConstantFloat("PARALLAX_SCALE", m_fParallaxScale);
		g_pShaderAPI->SetShaderConstantFloat("CUBEMAP_LIGHTING_SCALE", m_fCubemapLightingScale);
		g_pShaderAPI->SetShaderConstantFloat("SPECULAR_SCALE", m_fSpecularScale);
	}

	void ParamSetup_Cubemap()
	{
		if (m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP)
			g_pShaderAPI->SetTexture(materials->GetEnvironmentMapTexture(), "CubemapTexture", 12);
		else
			g_pShaderAPI->SetTexture(m_pCubemap, "CubemapTexture", 12);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetupBaseTexture()
	{
		ITexture* pSetupTexture = (materials->GetConfiguration().wireframeMode || r_showlightmaps->GetInt() == 1) ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	void SetupBumpmap()
	{
		g_pShaderAPI->SetTexture(m_pBumpTexture, "BumpMapSampler", 4 );
	}

	void SetupSpecular()
	{
		g_pShaderAPI->SetTexture(m_pSpecIllum, "SpecularMapSampler", 8);
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return m_pBaseTexture;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return m_pBumpTexture;
	}

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Ambient);
	}

	ITexture*			m_pBaseTexture;
	ITexture*			m_pBumpTexture;
	ITexture*			m_pSpecIllum;
	ITexture*			m_pCubemap;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	SHADER_DECLARE_PASS(AmbientInst);
	SHADER_DECLARE_FOGPASS(AmbientInst);

	bool				m_bSpecularIllum;
	bool				m_bParallax;
	float				m_fParallaxScale;
	float				m_fSpecularScale;
	bool				m_bCubeMapLighting;
	float				m_fCubemapLightingScale;

END_SHADER_CLASS