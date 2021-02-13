//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Depth write shader for lighting
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

BEGIN_SHADER_CLASS(DepthWriteLighting)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Depth) = NULL;
	}

	SHADER_INIT_TEXTURES()
	{
		// does nothing
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Depth))
			return true;

		bool bCubicOmni = false;
		bool bDepthRemap = true; // depth conversion for sun

		int nSkinned = 0;

		SHADER_PARAM_BOOL(Omni, bCubicOmni, false);
		SHADER_PARAM_BOOL(DepthRemap, bDepthRemap, true);
		SHADER_PARAM_INT(Skin, nSkinned, 0);

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// cubic omni definition
		SHADER_DECLARE_SIMPLE_DEFINITION(bCubicOmni, "CUBIC_OMNI");

		// depth conversion
		SHADER_DECLARE_SIMPLE_DEFINITION(bDepthRemap, "DEPTH_REMAP");

		// compile
		if(nSkinned)
		{
			if(nSkinned == 1)
			{
				SHADER_FIND_OR_COMPILE(Depth, "DepthWriteLighting_Skinned");
			}
			else if(nSkinned == 2)
			{
				SHADER_FIND_OR_COMPILE(Depth, "DepthWriteLighting_SimpleSkinned");
			}
		}
		else
		{
			SHADER_FIND_OR_COMPILE(Depth, "DepthWriteLighting");
		}

		return true;
	}

	void ParamSetup_Transform()
	{
		if(materials->GetLight()->nType == DLT_OMNIDIRECTIONAL)
		{
			CBaseShader::ParamSetup_Transform();
		}
		else
		{
			// override world-view-projection
			Matrix4x4 wvp_matrix = materials->GetLight()->lightWVP;

			Matrix4x4 worldtransform = identity4();
			materials->GetMatrix(MATRIXMODE_WORLD, worldtransform);

			g_pShaderAPI->SetShaderConstantMatrix4("WVP", wvp_matrix*worldtransform);
			g_pShaderAPI->SetShaderConstantMatrix4("World", worldtransform);

			// setup texture transform 
			//TODO: 2D texture matrix (Matrix2x2)
			SetupVertexShaderTextureTransform(m_pBaseTextureTransformVar, m_pBaseTextureScaleVar, "BaseTextureTransform");
		}
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Depth);
	}

	SHADER_SETUP_CONSTANTS()
	{
		if(IsError())
			return;

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_FOG);

		g_pShaderAPI->SetShaderConstantVector3D("LightParams", Vector3D(1, materials->GetLight()->radius.x, 1.0f / materials->GetLight()->radius.x));
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

	SHADER_DECLARE_PASS(Depth);
END_SHADER_CLASS