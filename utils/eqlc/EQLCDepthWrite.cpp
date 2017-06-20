//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Depth write shader for lighting
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CEQLCDepthWriteLighting : public CBaseShader
{
public:
	CEQLCDepthWriteLighting()
	{
		SHADER_PASS(Depth) = NULL;
	}

	void InitTextures()
	{
		// does nothing
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Depth))
			return true;

		bool bCubicOmni = false;
		bool bDepthRemap = true; // depth conversion for sun

		bool bSkinned = false;

		SHADER_PARAM_BOOL(Omni,bCubicOmni);
		SHADER_PARAM_BOOL(DepthRemap,bDepthRemap);
		SHADER_PARAM_BOOL(Skin,bSkinned);

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// cubic omni definition
		SHADER_DECLARE_SIMPLE_DEFINITION(bCubicOmni, "CUBIC_OMNI");

		// depth conversion
		SHADER_DECLARE_SIMPLE_DEFINITION(bDepthRemap, "DEPTH_REMAP");

		// compile
		if(bSkinned)
		{
			SHADER_FIND_OR_COMPILE(Depth, "EQLCDepthWriteLighting_Skinned");
		}
		else
		{
			SHADER_FIND_OR_COMPILE(Depth, "EQLCDepthWriteLighting");
		}

		return true;
	}

	void ParamSetup_RasterState()
	{
		materials->SetRasterizerStates(CULL_BACK, FILL_SOLID);
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
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_FOG);

		g_pShaderAPI->SetShaderConstantVector3D("LightParams", Vector3D(1, materials->GetLight()->radius.x, 1.0f / materials->GetLight()->radius.x));
	}

	const char* GetName()
	{
		return "EQLCDepthWriteLighting";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

private:
	SHADER_DECLARE_PASS(Depth);
};

DEFINE_SHADER(EQLCDepthWriteLighting, CEQLCDepthWriteLighting)