//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basis Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "vars_generic.h"
#include "IDkCore.h"

//ConVar r_decals_depthbias("decal_depthbias", "0.000005f", "Decal depth bias", CV_ARCHIVE);
//ConVar r_decals_slopedepthbias("decal_slopedepthbias", "0", "Decal depth bias", CV_ARCHIVE);

class CBaseDecal : public CBaseShader
{
public:
	CBaseDecal()
	{
		m_pBaseTexture	= NULL;
		m_pBumpTexture	= NULL;
		m_pSpecIllum	= NULL;
		m_pCubemap		= NULL;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		m_bSpecularIllum = false;
		m_bParallax = false;
		m_fParallaxScale = 1.0f;
		m_fSpecularScale = 0.0f;
		m_bCubeMapLighting = false;
		m_fCubemapLightingScale = 1.0f;
	}

	void InitParams()
	{
		if(!m_bInitialized && !m_bIsError)
		{
			CBaseShader::InitParams();
			m_depthwrite = false;
		}
	}

	void InitTextures()
	{
		m_nAddressMode = ADDRESSMODE_CLAMP;

		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		if(materials->GetConfiguration().enableBumpmapping)
			SHADER_PARAM_TEXTURE_NOERROR(Bumpmap, m_pBumpTexture);

		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Specular, m_pSpecIllum);

		// load cubemap if available
		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CBaseDecal::SetupBaseTexture);

		if(m_pBumpTexture)
			SetParameterFunctor(SHADERPARAM_BUMPMAP, &CBaseDecal::SetupBumpmap);

		if(m_pSpecIllum)
			SetParameterFunctor(SHADERPARAM_SPECULARILLUM, &CBaseDecal::SetupSpecular);

		SetParameterFunctor(SHADERPARAM_COLOR, &CBaseDecal::SetColorModulation);
	}

	void ParamSetup_AlphaModel_Solid()
	{
		// setup default alphatesting from shaderapi
		materials->SetBlendingStates( BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDFUNC_ADD, (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE) );
	}

	void ParamSetup_AlphaModel_Alphatest()
	{
		// setup default alphatesting from shaderapi
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD, (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE));
	}

	void ParamSetup_AlphaModel_Translucent()
	{
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD, (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE));
		m_depthwrite = false;
	}

	void ParamSetup_AlphaModel_Modulate()
	{
		// setup default alphatesting from shaderapi
		materials->SetBlendingStates(BLENDFACTOR_DST_COLOR, BLENDFACTOR_SRC_COLOR, BLENDFUNC_ADD, (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE));
		m_depthwrite = false;
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Ambient))
			return true;

		bool bDeferredShading = (materials->GetLightingModel() == MATERIAL_LIGHT_DEFERRED);
		bool bIsModulated = (m_nFlags & MATERIAL_FLAG_MODULATE) > 0;

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		// illumination from specular green channel
		SHADER_PARAM_BOOL(SpecularIllum, m_bSpecularIllum);

		SHADER_PARAM_BOOL(CubeMapLighting, m_bCubeMapLighting);

		SHADER_PARAM_FLOAT(CubeMapLightingScale, m_fCubemapLightingScale);

		// parallax as the bump map alpha
		SHADER_PARAM_BOOL(Parallax, m_bParallax);

		// parallax scale
		SHADER_PARAM_FLOAT(ParallaxScale, m_fParallaxScale);

		if(m_pSpecIllum)
			m_fSpecularScale = 1.0f;

		// parallax scale
		SHADER_PARAM_FLOAT(SpecularScale, m_fSpecularScale);

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define bump mapping
		SHADER_BEGIN_DEFINITION((m_pBumpTexture != NULL), "BUMPMAP")

			// define parallax
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bParallax, "PARALLAX")

		SHADER_END_DEFINITION

		// define cubemap parameter.
		SHADER_BEGIN_DEFINITION((m_pCubemap != NULL), "CUBEMAP")
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bCubeMapLighting, "CUBEMAP_LIGHTING")
		SHADER_END_DEFINITION

		// define specular usage
		SHADER_BEGIN_DEFINITION((m_pSpecIllum != NULL), "SPECULAR")
			// define self illumination
			SHADER_DECLARE_SIMPLE_DEFINITION(m_bSpecularIllum, "SELFILLUMINATION")
		SHADER_END_DEFINITION

		// define modulate parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_MODULATE), "MODULATE");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(materials->GetConfiguration().editormode, "EDITOR");

		if(bDeferredShading && !bIsModulated)
		{
			// disable FFP alphatesting, let use the shader for better perfomance
			//SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseDecal::ParamSetup_AlphaModel_Solid);

			// alphatesting because stock DX9 and OGL sucks
			SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BaseDecal_Deferred");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");

			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BaseDecal_Deferred");
		}
		else
		{
			// compile without fog
			SHADER_FIND_OR_COMPILE(Ambient, "BaseDecal_Ambient");

			// define fog parameter.
			SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");

			// compile with fog
			SHADER_FIND_OR_COMPILE(Ambient_fog, "BaseDecal_Ambient");
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

		g_pShaderAPI->SetShaderConstantFloat("PARALLAX_SCALE", m_fParallaxScale);
		g_pShaderAPI->SetShaderConstantFloat("CUBEMAP_LIGHTING_SCALE", m_fCubemapLightingScale);
		g_pShaderAPI->SetShaderConstantFloat("SPECULAR_SCALE", m_fSpecularScale);
	}

	void ParamSetup_Cubemap()
	{
		g_pShaderAPI->SetTexture(m_pCubemap, "CubemapTexture", 12);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", (m_nFlags & MATERIAL_FLAG_TRANSPARENT) ? materials->GetAmbientColor() : ColorRGBA(1,1,1,1));
	}

	void SetupBaseTexture()
	{
		ITexture* pSetupTexture = (materials->GetConfiguration().wireframeMode || r_showlightmaps->GetInt() == 1) ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	void SetupBumpmap()
	{
		g_pShaderAPI->SetTexture(m_pBumpTexture, "BumpMapSampler", 4);
	}

	void SetupSpecular()
	{
		g_pShaderAPI->SetTexture(m_pSpecIllum, "SpecularMapSampler", 8);
	}

	const char* GetName()
	{
		return "Decal";
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

private:

	ITexture*			m_pBaseTexture;
	ITexture*			m_pBumpTexture;
	ITexture*			m_pSpecIllum;
	ITexture*			m_pCubemap;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	bool				m_bSpecularIllum;
	bool				m_bParallax;
	float				m_fParallaxScale;
	float				m_fSpecularScale;
	bool				m_bCubeMapLighting;
	float				m_fCubemapLightingScale;
};

DEFINE_SHADER(Decal, CBaseDecal)
