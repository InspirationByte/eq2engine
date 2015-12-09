//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Skybox 2D shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

//--------------------------------------
// Generic 2D shader
//--------------------------------------

class CSkyboxSimpleShader : public CBaseShader
{
public:
	CSkyboxSimpleShader();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	// Return real shader name
	const char* GetName()
	{
		return "sky";
	}

	ITexture* GetBaseTexture(int stage) {return m_nBaseTexture;}
	ITexture* GetBumpTexture(int stage) {return NULL;}

	// Sets constants
	void SetupConstants();

	void SetupShader();

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Unlit);
	}


private:

	SHADER_DECLARE_PASS(Unlit);

	ITexture*		m_nBaseTexture;
};

//--------------------------------------
// BaseUnlit
//--------------------------------------

CSkyboxSimpleShader::CSkyboxSimpleShader() : CBaseShader()
{
	SHADER_PASS(Unlit) = NULL;

	m_nBaseTexture = NULL;

	m_nFlags |= MATERIAL_FLAG_ISSKY;
}

//--------------------------------------
// Init parameters
//--------------------------------------

// Initialize textures
void CSkyboxSimpleShader::InitTextures()
{
	SHADER_PARAM_TEXTURE(BaseTexture, m_nBaseTexture);
}

// Initialize shader(s)
bool CSkyboxSimpleShader::InitShaders()
{
	// just skip if we already have shader
	//if(SHADER_PASS(Unlit))
	//	return true;

	// begin shader definitions
	SHADERDEFINES_BEGIN;

	// compile without fog
	SHADER_FIND_OR_COMPILE(Unlit, "SkyBox");

	return true;
}

void CSkyboxSimpleShader::SetupShader()
{
	if(IsError())
		return;

	SHADER_BIND_PASS_SIMPLE(Unlit);
}

//--------------------------------------
// Set Constants
//--------------------------------------
void CSkyboxSimpleShader::SetupConstants()
{
	// If we has shader
	if(!m_bIsError)
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		// do depth testing for my type of skybox (looks like quake 3/unreal tournament style skyboxes)

		materials->SetDepthStates(true, false);
		materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO);
		materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	
		Matrix4x4 wvp;
		materials->GetWorldViewProjection(wvp);

		wvp = !wvp;

		Vector3D camPos(wvp.rows[0].w, wvp.rows[1].w, wvp.rows[2].w);

		// camera direction
		g_pShaderAPI->SetShaderConstantVector3D("camPos", camPos*2.0f);

		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());

		// setup base texture
		g_pShaderAPI->SetTexture(m_nBaseTexture, "BaseTextureSampler", 0);
	}
}

// DECLARE SHADER!
DEFINE_SHADER(sky,CSkyboxSimpleShader)