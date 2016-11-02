//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Depth map shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class SH_BlockLight : public CBaseShader
{
public:
	SH_BlockLight();

	// Sets parameters and compiles shader
	void InitParams();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	// Return real shader name
	const char* GetName()
	{
		return "BlockLight";
	}

	ITexture* GetBaseTexture(int n)	{return m_pBaseTexture;}
	ITexture* GetBumpTexture(int n)	{return NULL;}
	bool IsHasAlphaTexture()		{return false;}
	bool IsCastsShadows()			{return true;}

	// Sets constants
	void SetupConstants();

	void SetupShader();

	void SetupBaseTexture0();

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Unlit);
	}

private:
	SHADER_DECLARE_PASS(Unlit);
	SHADER_DECLARE_FOGPASS(Unlit);

	ITexture*		m_pBaseTexture;
};

//--------------------------------------
// BaseUnlit
//--------------------------------------

SH_BlockLight::SH_BlockLight() : CBaseShader()
{
	SHADER_PASS(Unlit) = NULL;
	SHADER_FOGPASS(Unlit) = NULL;

	m_pBaseTexture = NULL;
}

//--------------------------------------
// Init parameters
//--------------------------------------

void SH_BlockLight::InitParams()
{
	if(!IsInitialized() && !IsError())
	{
		m_nFlags |= MATERIAL_FLAG_CASTSHADOWS | MATERIAL_FLAG_INVISIBLE;

		// Call base classinitialization
		CBaseShader::InitParams();
	}
}

void SH_BlockLight::InitTextures()
{
	// parse material variables
	SHADER_PARAM_TEXTURE(BaseTexture, m_pBaseTexture);

	// set texture setup
	SetParameterFunctor(SHADERPARAM_BASETEXTURE, &SH_BlockLight::SetupBaseTexture0);
}

void SH_BlockLight::SetupBaseTexture0()
{
	ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;

	g_pShaderAPI->SetTexture(pSetupTexture, 0);
}

bool SH_BlockLight::InitShaders()
{
	if(SHADER_PASS(Unlit))
		return true;

	// begin shader definitions
	SHADERDEFINES_BEGIN;

	// compile without fog
	SHADER_FIND_OR_COMPILE(Unlit, "BaseUnlit");

	// define fog parameter.
	SHADER_DECLARE_SIMPLE_DEFINITION(m_fogenabled, "DOFOG");

	// compile with fog
	SHADER_FIND_OR_COMPILE(Unlit_fog, "BaseUnlit");

	return true;
}

//--------------------------------------
// Set Constants
//--------------------------------------

void SH_BlockLight::SetupShader()
{
	if(IsError())
		return;

	SHADER_BIND_PASS_FOGSELECT(Unlit);
}


void SH_BlockLight::SetupConstants()
{
	if(IsError())
		return;

	SetupDefaultParameter(SHADERPARAM_TRANSFORM);

	SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

	SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
	SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
	SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
	SetupDefaultParameter(SHADERPARAM_COLOR);
	SetupDefaultParameter(SHADERPARAM_FOG);
}

// DECLARE SHADER!
DEFINE_SHADER(BlockLight,SH_BlockLight)