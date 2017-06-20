//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Error Shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CEditorFlatColor : public CBaseShader
{
public:
	CEditorFlatColor()
	{
		m_pProgram = NULL;
	}

	void InitTextures()
	{
		// set texture setup
		SetParameterFunctor(SHADERPARAM_COLOR, &CEditorFlatColor::SetColorModulation);
	}

	bool InitShaders()
	{
		if(m_pProgram)
			return true;

		EqString defines;

		// Download shader from disk (shaders\*.shader)
		m_pProgram = g_pShaderAPI->CreateNewShaderProgram("EditorFlatColor");

		AddShaderToAutoremover(&m_pProgram);

		return g_pShaderAPI->LoadShadersFromFile(m_pProgram,"EditorFlatColor", defines.GetData(), NULL);
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		g_pShaderAPI->SetShader(m_pProgram);
	}

	SHADER_SETUP_CONSTANTS()
	{
		if(IsError())
			return;

		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);

		//m_depthwrite = true;

		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);

	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());

		if(materials->GetAmbientColor().w < 1.0f)
		{
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CEditorFlatColor::ParamSetup_AlphaModel_Translucent);
		}
		else
		{
			m_depthwrite = true;
			SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CEditorFlatColor::ParamSetup_AlphaModel_Solid);
		}
	}

	const char* GetName()
	{
		return "EditableSurfaceFlat";
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

	IShaderProgram*		m_pProgram;
};

DEFINE_SHADER(EditorFlatColor,CEditorFlatColor)