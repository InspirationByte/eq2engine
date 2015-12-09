//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basis Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CBaseSingle : public CBaseShader
{
public:
	CBaseSingle()
	{
		m_pBaseTexture			= NULL;
		m_pNightLightTexture	= NULL;
		m_pCubemap				= NULL;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		SHADER_PASS(AmbientInst) = NULL;
		SHADER_FOGPASS(AmbientInst) = NULL;

		m_fSpecularScale = 1.0f;
	}

	void InitTextures()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		SHADER_PARAM_TEXTURE_NOERROR(NightLight, m_pNightLightTexture);

		// load cubemap if available
		if (materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CBaseSingle::SetupBaseTexture);

		SetParameterFunctor(SHADERPARAM_COLOR, &CBaseSingle::SetColorModulation);
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Ambient))
			return true;

		bool bDeferredShading = (materials->GetLightingModel() == MATERIAL_LIGHT_DEFERRED);
		bool bHasAnyTranslucency = (m_nFlags & MATERIAL_FLAG_TRANSPARENT) || (m_nFlags & MATERIAL_FLAG_ADDITIVE) || (m_nFlags & MATERIAL_FLAG_MODULATE);

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		SHADER_PARAM_FLOAT(SpecularScale, m_fSpecularScale);

		bool bBaseTextureSpecularAlpha;
		SHADER_PARAM_BOOL(BaseTextureSpecularAlpha, bBaseTextureSpecularAlpha);

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		// define modulate parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_MODULATE), "MODULATE");

		bool useCubemap = (m_nFlags & MATERIAL_FLAG_USE_ENVCUBEMAP) || (m_pCubemap != NULL);

		// define cubemap parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(useCubemap, "CUBEMAP")

		SHADER_BEGIN_DEFINITION(bBaseTextureSpecularAlpha, "USE_BASETEXTUREALPHA_SPECULAR")
			SHADER_DECLARE_SIMPLE_DEFINITION(true, "USE_SPECULAR");
		SHADER_END_DEFINITION;
		
		SHADER_DECLARE_SIMPLE_DEFINITION((m_pNightLightTexture != NULL), "HAS_NIGHTLIGHT_TEXTURE");

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

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

		return true;
	}

	void SetupShader()
	{
		if(IsError())
			return;

		if(!materials->IsInstancingEnabled())
			SHADER_BIND_PASS_FOGSELECT(Ambient)
		else
			SHADER_BIND_PASS_FOGSELECT(AmbientInst)
	}

	void SetupConstants()
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
		g_pShaderAPI->SetTexture(m_pBaseTexture, "BaseTextureSampler", 0);
		g_pShaderAPI->SetTexture(m_pNightLightTexture, "NightLightSampler", 1);
	}

	const char* GetName()
	{
		return "BaseStatic";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return m_pBaseTexture;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Ambient);
	}

private:

	ITexture*			m_pBaseTexture;
	ITexture*			m_pNightLightTexture;
	ITexture*			m_pCubemap;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	SHADER_DECLARE_PASS(AmbientInst);
	SHADER_DECLARE_FOGPASS(AmbientInst);

	float				m_fSpecularScale;
};

DEFINE_SHADER(BaseStatic, CBaseSingle)