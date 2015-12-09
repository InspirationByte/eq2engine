//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basis Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "vars_generic.h"

class CWaterShader : public CBaseShader
{
public:
	CWaterShader()
	{
		m_pBaseTexture	= NULL;
		m_pBumpTexture	= NULL;
		m_pCubemap		= NULL;
		m_pBumpFrame	= NULL;

		m_pUnderwater = NULL;
		m_pReflection = NULL;
		m_pDepth = NULL;

		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		m_nFlags |= MATERIAL_FLAG_TRANSPARENT | MATERIAL_FLAG_WATER;
	}

	void InitParams()
	{
		if(!m_bInitialized && !m_bIsError)
		{
			CBaseShader::InitParams();

			m_bNoCull = true;

			m_pBumpFrame = m_pAssignedMaterial->GetMaterialVar("bumptextureframe", 0);
		}
	}

	void InitTextures()
	{
		// load textures from parameters
		SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

		if(materials->GetConfiguration().enableBumpmapping)
			SHADER_PARAM_TEXTURE_NOERROR(Bumpmap, m_pBumpTexture);

		// load cubemap if available
		if(materials->GetConfiguration().enableSpecular)
			SHADER_PARAM_TEXTURE_NOERROR(Cubemap, m_pCubemap);

		m_pUnderwater = g_pShaderAPI->FindTexture("_rt_underwater");
		m_pReflection = g_pShaderAPI->FindTexture("_rt_reflection");
		m_pDepth = g_pShaderAPI->FindTexture("_rt_gbuf_depth");

		if(m_pUnderwater)
			m_pUnderwater->Ref_Grab();

		if(m_pReflection)
			m_pReflection->Ref_Grab();

		if(m_pDepth)
			m_pDepth->Ref_Grab();

		AddTextureToAutoremover(&m_pUnderwater);
		AddTextureToAutoremover(&m_pReflection);
		AddTextureToAutoremover(&m_pDepth);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CWaterShader::SetupBaseTexture);

		if(m_pBumpTexture)
			SetParameterFunctor(SHADERPARAM_BUMPMAP, &CWaterShader::SetupBumpmap);

		SetParameterFunctor(SHADERPARAM_COLOR, &CWaterShader::SetColorModulation);
	}

	void ParamSetup_TextureFrames()
	{
		if(m_pBumpFrame && m_pBumpTexture)
			m_pBumpTexture->SetAnimatedTextureFrame(m_pBumpFrame->GetInt());
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Ambient))
			return true;

		bool bSpecularIllum = false;
		bool bParallax = false;
		float fParallaxScale = 1.0f;
		float fSpecularScale = 0.0f;
		bool bCubeMapLighting = false;
		float fCubemapLightingScale = 1.0f;

		//SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Translucent);

		bool bDeferredShading = (materials->GetLightingModel() == MATERIAL_LIGHT_DEFERRED);

		//------------------------------------------
		// load another shader params here (because we want to use less memory)
		//------------------------------------------

		// illumination from specular green channel
		SHADER_PARAM_BOOL(CubeMapLighting, bCubeMapLighting);

		SHADER_PARAM_FLOAT(CubeMapLightingScale, fCubemapLightingScale);

		// parallax as the bump map alpha
		SHADER_PARAM_BOOL(Parallax, bParallax);

		// parallax scale
		SHADER_PARAM_FLOAT(ParallaxScale, fParallaxScale);

		// parallax scale
		SHADER_PARAM_FLOAT(SpecularScale, fSpecularScale);

		//------------------------------------------
		// begin shader definitions
		//------------------------------------------
		SHADERDEFINES_BEGIN;

		int x,y,vW,vH;
		g_pShaderAPI->GetViewport(x,y,vW,vH);
	
		SHADER_ADD_FLOAT_DEFINITION("SCREENTEXEL_X", 1.0f/vW);
		SHADER_ADD_FLOAT_DEFINITION("SCREENTEXEL_Y", 1.0f/vH);

		// define modulate parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_MODULATE), "MODULATE");

		// define bump mapping
		SHADER_BEGIN_DEFINITION((m_pBumpTexture != NULL), "BUMPMAP")

		SHADER_END_DEFINITION

		// define cubemap parameter.
		SHADER_BEGIN_DEFINITION((m_pCubemap != NULL), "CUBEMAP")
			SHADER_BEGIN_DEFINITION(bCubeMapLighting, "CUBEMAP_LIGHTING")
				SHADER_ADD_FLOAT_DEFINITION("CUBEMAP_LIGHTING_SCALE", fCubemapLightingScale);
			SHADER_END_DEFINITION
		SHADER_END_DEFINITION

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		bool bUseLightmaps = (!materials->GetConfiguration().editormode);
		SHADER_DECLARE_SIMPLE_DEFINITION(bUseLightmaps, "LIGHTMAP");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Ambient, "Water_Ambient");
		SHADER_FIND_OR_COMPILE(Spotlight, "Water_Spotlight");

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");
		
		// compile with fog
		SHADER_FIND_OR_COMPILE(Ambient_fog, "Water_Ambient");
		SHADER_FIND_OR_COMPILE(Spotlight_fog, "Water_Spotlight");

		return true;
	}

	void SetupShader()
	{
		if(IsError())
			return;

		if( materials->GetLight() == NULL )
		{
			g_pShaderAPI->CopyFramebufferToTexture( m_pUnderwater );

			SHADER_BIND_PASS_FOGSELECT( Ambient );

			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CWaterShader::ParamSetup_AlphaModel_Solid);
		}
		else
		{
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CWaterShader::ParamSetup_AlphaModel_Additive);

			if( materials->GetLight()->nType == DLT_SPOT )
			{
				SHADER_BIND_PASS_FOGSELECT( Spotlight );
			}
		}
	}

	void SetupConstants()
	{
		if( IsError() )
			return;

		SetupDefaultParameter( SHADERPARAM_TRANSFORM );

		SetupDefaultParameter( SHADERPARAM_BASETEXTURE );
		SetupDefaultParameter( SHADERPARAM_BUMPMAP );
		SetupDefaultParameter( SHADERPARAM_CUBEMAP );
		SetupDefaultParameter( SHADERPARAM_ANIMFRAME );

		SetupDefaultParameter( SHADERPARAM_ALPHASETUP );
		SetupDefaultParameter( SHADERPARAM_DEPTHSETUP );
		SetupDefaultParameter( SHADERPARAM_RASTERSETUP );

		SetupDefaultParameter( SHADERPARAM_COLOR );
		SetupDefaultParameter( SHADERPARAM_FOG );

		Matrix4x4 view, proj, world;
		materials->GetMatrix(MATRIXMODE_VIEW, view);
		materials->GetMatrix(MATRIXMODE_PROJECTION, proj);
		materials->GetMatrix(MATRIXMODE_WORLD, world);

		g_pShaderAPI->SetShaderConstantMatrix4("WorldView", view*world);

		// get camera pos
		FogInfo_t fog;
		materials->GetFogInfo(fog);

		// retranslate
		view = view*translate(fog.viewPos);

		g_pShaderAPI->SetShaderConstantMatrix4("InvViewProj", !(proj*view)); // inverse view projection

		g_pShaderAPI->SetShaderConstantMatrix4("Proj", proj);

		g_pShaderAPI->SetTexture(m_pDepth, "DepthTex", 7);

		// set light
		if( materials->GetLight() != NULL )
		{
			if( materials->GetLight()->nType == DLT_SPOT )
			{
				ITexture* masks[2] = {materials->GetWhiteTexture(), materials->GetLight()->pMaskTexture};
				int has_mask = (materials->GetLight()->pMaskTexture > 0);

				// setup light mask
				g_pShaderAPI->SetTexture(masks[has_mask], "MaskTexture", 1);

				g_pShaderAPI->SetShaderConstantMatrix4("LightWVP", materials->GetLight()->lightWVP);

				Vector3D lightPosView = materials->GetLight()->position;
				g_pShaderAPI->SetShaderConstantVector3D("LightPos", lightPosView);

				// Light color, and inv light radius
				g_pShaderAPI->SetShaderConstantVector4D("LightColorAndInvRad", Vector4D(materials->GetLight()->color * materials->GetLight()->curintensity * r_lightscale->GetFloat(), 1.0 / materials->GetLight()->radius.x));
				g_pShaderAPI->SetShaderConstantVector4D("LightRadius", Vector4D(materials->GetLight()->radius.x,0,0,0));
			}
		}
		else
		{
			g_pShaderAPI->SetTexture(m_pUnderwater, "UnderwaterTex", 5);
			g_pShaderAPI->SetTexture(m_pReflection, "ReflectionTex", 6);
		}
	}

	void ParamSetup_Cubemap()
	{
		g_pShaderAPI->SetTexture(m_pCubemap, "CubemapTexture", 12);
	}

	void SetColorModulation()
	{
		ColorRGBA setColor = materials->GetAmbientColor();
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", setColor);
	}

	void SetupBaseTexture()
	{
		ITexture* pSetupTexture = (materials->GetConfiguration().wireframeMode || r_showlightmaps->GetBool()) ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	void SetupBumpmap()
	{
		g_pShaderAPI->SetTexture(m_pBumpTexture, "BumpMapSampler", 4 );
	}

	const char* GetName()
	{
		return "Water";
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
	ITexture*			m_pCubemap;

	ITexture*			m_pDepth;
	ITexture*			m_pUnderwater;
	ITexture*			m_pReflection;

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	SHADER_DECLARE_PASS(Spotlight);
	SHADER_DECLARE_FOGPASS(Spotlight);

	IMatVar*			m_pBumpFrame;
};

DEFINE_SHADER(Water, CWaterShader)