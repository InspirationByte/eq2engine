//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Post processing effect combiner
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem1/BaseShader.h"

BEGIN_SHADER_CLASS(GaussianBlur)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = NULL;

		m_blurProps = GetAssignedMaterial()->GetMaterialVar("BlurProps", "[0.6 40 100 100]");
		m_blurSource = GetAssignedMaterial()->GetMaterialVar("BlurSource", "");

		m_depthtest = false;
		m_depthwrite = false;
	}

	SHADER_INIT_TEXTURES()
	{
		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTextures);
	}

	SHADER_INIT_RHI()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "GaussianBlur");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);

		ITexture* bloomTex = m_blurSource->GetTexture();

		Vector4D textureSizeProps;

		if (bloomTex)
		{
			textureSizeProps.x = bloomTex->GetWidth();
			textureSizeProps.y = bloomTex->GetHeight();

			textureSizeProps.z = 1.0f / textureSizeProps.x;
			textureSizeProps.w = 1.0f / textureSizeProps.y;
		}

		g_pShaderAPI->SetShaderConstantVector4D("BlurProps", m_blurProps->GetVector4());
		g_pShaderAPI->SetShaderConstantVector4D("TextureSize", textureSizeProps);		
	}

	void SetupBaseTextures()
	{
		g_pShaderAPI->SetTexture(m_blurSource->GetTexture(), "BaseTexture", 0);
	}

	ITexture*	GetBaseTexture(int stage) const
	{
		return NULL;
	}

	ITexture*	GetBumpTexture(int stage) const
	{
		return NULL;
	}

private:

	SHADER_DECLARE_PASS(Unlit);

	IMatVar* m_blurProps;
	IMatVar* m_blurSource;

END_SHADER_CLASS