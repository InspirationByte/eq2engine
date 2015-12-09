//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Deferred shading Point light material
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "vars_generic.h"

class CDeferredPointLight : public CBaseShader
{
public:
	CDeferredPointLight()
	{
		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		m_pRTAlbedo = NULL;
		m_pRTDepth = NULL;
		m_pRTMatMap = NULL;
		m_pRTNormals = NULL;
	}

	// Initialize textures
	void InitTextures()
	{
		SHADER_PARAM_RENDERTARGET_FIND(Albedo, m_pRTAlbedo);
		SHADER_PARAM_RENDERTARGET_FIND(Normal, m_pRTNormals);
		SHADER_PARAM_RENDERTARGET_FIND(Depth, m_pRTDepth);
		SHADER_PARAM_RENDERTARGET_FIND(MaterialMap1, m_pRTMatMap);
	}

	// Initialize shader(s)
	bool InitShaders()
	{
		// just skip if we already have shader
		if(SHADER_PASS(Ambient))
			return true;

		// required
		SHADERDEFINES_BEGIN

		SHADER_DECLARE_SIMPLE_DEFINITION((m_nFlags & MATERIAL_FLAG_RECEIVESHADOWS), "SHADOWMAP");

		SHADER_FIND_OR_COMPILE(Ambient, "DeferredPointLight")

		SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG") // for compile it's always enabled

		SHADER_FIND_OR_COMPILE(Ambient_fog, "DeferredPointLight")

		SetParameterFunctor(SHADERPARAM_COLOR, &CDeferredPointLight::SetupColors);
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Additive_Fog);

		return true;
	}

	// Return real shader name
	const char* GetName()
	{
		return "DeferredPointLight";
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_FOGSELECT(Ambient);
	}

	// Sets constants
	void SetupConstants()
	{
		// If we has shader
		if(!IsError())
		{
			SetupDefaultParameter(SHADERPARAM_TRANSFORM);
			SetupDefaultParameter(SHADERPARAM_COLOR);
			SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
			SetupDefaultParameter(SHADERPARAM_FOG);

			// set needed cull mode
			materials->SetRasterizerStates(materials->GetCurrentCullMode(), FILL_SOLID);
		
			// set depth state
			//materials->SetDepthStates(false, false);
			//g_pShaderAPI->SetDepthRange(0.0f, 0.9f);

			// Apply the samplers 0, 1, 2, 3 as albedo, normals, depth, material map
			g_pShaderAPI->SetTexture(m_pRTAlbedo, "Albedo", 0);
			g_pShaderAPI->SetTexture(m_pRTNormals, "Normals", 1);
			g_pShaderAPI->SetTexture(m_pRTDepth, "Depth", 2);
			g_pShaderAPI->SetTexture(m_pRTMatMap, "MaterialMap1", 3);

			ITexture* masks[2] = {materials->GetWhiteTexture(), materials->GetLight()->pMaskTexture};
			int has_mask = (materials->GetLight()->pMaskTexture > 0);

			// setup light mask
			g_pShaderAPI->SetTexture(masks[has_mask], "MaskTexture", 4);

			Matrix4x4 view, proj, world;
			materials->GetMatrix(MATRIXMODE_VIEW, view);
			materials->GetMatrix(MATRIXMODE_PROJECTION, proj);
			materials->GetMatrix(MATRIXMODE_WORLD, world);

			// get camera pos
			FogInfo_t fog;
			materials->GetFogInfo(fog);

			Vector3D lightPosView = materials->GetLight()->position-fog.viewPos;

			g_pShaderAPI->SetShaderConstantMatrix4("WorldView", view*world);
			g_pShaderAPI->SetShaderConstantVector3D("LightPos", lightPosView);

			// retranslate
			view = view*translate(fog.viewPos);

			g_pShaderAPI->SetShaderConstantMatrix4("InvViewProj", !(proj*view)); // inverse view projection
			g_pShaderAPI->SetShaderConstantMatrix4("InvView", !view);
			g_pShaderAPI->SetShaderConstantMatrix4("Proj", proj);

			int x,y,vW,vH;
			g_pShaderAPI->GetViewport(x,y,vW,vH);

			g_pShaderAPI->SetShaderConstantVector2D("ScreenTexel", Vector2D(1.0f/vW, 1.0f/vH));

		}
	}

	void SetupColors()
	{
		// Light color, and inv light radius
		g_pShaderAPI->SetShaderConstantVector4D("LightColorAndInvRad", Vector4D(materials->GetLight()->color * materials->GetLight()->curintensity * r_lightscale->GetFloat(), 1.0 / materials->GetLight()->radius.x));
	}

	ITexture* GetBaseTexture(int n)		{return NULL;}
	ITexture* GetBumpTexture(int n)		{return NULL;}

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Ambient);
	}

private:
	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	// ambient needs only depth and albedo
	ITexture* m_pRTAlbedo;
	ITexture* m_pRTDepth;
	ITexture* m_pRTNormals;
	ITexture* m_pRTMatMap;
};

DEFINE_SHADER(DeferredPointLight, CDeferredPointLight)