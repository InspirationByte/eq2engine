//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Deferred shading Point light material
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "vars_generic.h"

class CFlashlightReflector : public CBaseShader
{
public:
	CFlashlightReflector()
	{
		SHADER_PASS(Ambient) = NULL;
	}

	// Initialize textures
	void InitTextures()
	{

	}

	// Initialize shader(s)
	bool InitShaders()
	{
		// just skip if we already have shader
		if(SHADER_PASS(Ambient))
			return true;

		// required
		SHADERDEFINES_BEGIN

		SHADER_FIND_OR_COMPILE(Ambient, "FlashlightReflector")

		SetParameterFunctor(SHADERPARAM_COLOR, &CFlashlightReflector::SetupColors);
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CBaseShader::ParamSetup_AlphaModel_Additive);

		return true;
	}

	// Return real shader name
	const char* GetName()
	{
		return "FlashlightReflector";
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Ambient);
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
			SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
			SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);

			// set needed cull mode
			materials->SetRasterizerStates(materials->GetCurrentCullMode(), FILL_SOLID);

			Matrix4x4 view, proj, world;
			materials->GetMatrix(MATRIXMODE_VIEW, view);
			materials->GetMatrix(MATRIXMODE_PROJECTION, proj);
			materials->GetMatrix(MATRIXMODE_WORLD, world);

			ITexture* masks[2] = {materials->GetWhiteTexture(), materials->GetLight()->pMaskTexture};
			int has_mask = (materials->GetLight()->pMaskTexture > 0);

			// setup light mask
			g_pShaderAPI->SetTexture(masks[has_mask], "MaskTexture", VERTEX_TEXTURE_INDEX(0));

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
};

DEFINE_SHADER(FlashlightReflector, CFlashlightReflector)