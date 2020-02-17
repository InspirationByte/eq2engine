//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Deferred shading Point light material
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "../vars_generic.h"

BEGIN_SHADER_CLASS(FlashlightReflector)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Ambient) = NULL;
	}

	// Initialize textures
	SHADER_INIT_TEXTURES()
	{

	}

	// Initialize shader(s)
	SHADER_INIT_RHI()
	{
		// just skip if we already have shader
		if(SHADER_PASS(Ambient))
			return true;

		// required
		SHADERDEFINES_BEGIN

		SHADER_FIND_OR_COMPILE(Ambient, "FlashlightReflector")

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetupColors);
		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &ThisShaderClass::ParamSetup_AlphaModel_Additive);

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Ambient);
	}

	// Sets constants
	SHADER_SETUP_CONSTANTS()
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

	SHADER_DECLARE_PASS(Ambient);
END_SHADER_CLASS