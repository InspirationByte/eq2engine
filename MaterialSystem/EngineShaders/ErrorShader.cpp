//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Error Shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CErrorShader : public CBaseShader
{
public:
	CErrorShader()
	{
		m_pProgram = NULL;
		m_pBaseTexture = NULL;
	}

	void InitTextures()
	{
		if(m_pBaseTexture)
			return;

		// parse material variables
		IMatVar *m_BaseTextureName = GetAssignedMaterial()->FindMaterialVar("BaseTexture");

		if(m_BaseTextureName)
			m_pBaseTexture = g_pShaderAPI->LoadTexture(m_BaseTextureName->GetString(),m_nTextureFilter,m_nAddressMode);
		else
			m_pBaseTexture = g_pShaderAPI->GenerateErrorTexture();

		AddTextureToAutoremover(&m_pBaseTexture);

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CErrorShader::SetupBaseTexture0);
		SetParameterFunctor(SHADERPARAM_COLOR, &CErrorShader::SetColorModulation);
	}

	bool InitShaders()
	{
		if(m_pProgram)
			return true;

		EqString defines;

		// Download shader from disk (shaders\*.shader)
		m_pProgram = g_pShaderAPI->CreateNewShaderProgram("BaseUnlit");

		AddShaderToAutoremover(&m_pProgram);

		return g_pShaderAPI->LoadShadersFromFile(m_pProgram,"BaseUnlit", defines.GetData(), NULL);
	}

	void SetupShader()
	{
		if(IsError())
			return;

		g_pShaderAPI->SetShader(m_pProgram);
	}

	void SetupConstants()
	{
		if(IsError())
			return;

		g_pShaderAPI->SetShader(m_pProgram);

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());
	}

	void SetupBaseTexture0()
	{
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	const char* GetName()
	{
		return "Error";
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
		return m_pProgram;
	}

private:

	ITexture*			m_pBaseTexture;
	IShaderProgram*		m_pProgram;
};

DEFINE_SHADER(Error,CErrorShader)