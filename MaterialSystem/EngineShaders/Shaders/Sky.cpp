//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Skybox 2D shader
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

//--------------------------------------
// Basic cubemap skybox shader
//--------------------------------------

BEGIN_SHADER_CLASS(Skybox)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = NULL;
		m_nBaseTexture = NULL;
		m_nFlags |= MATERIAL_FLAG_ISSKY;
	}

	// Initialize textures
	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE(BaseTexture, m_nBaseTexture);
	}

	SHADER_INIT_RHI()
	{
		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "SkyBox");

		return true;	
	}

	ITexture* GetBaseTexture(int stage) {return m_nBaseTexture;}
	ITexture* GetBumpTexture(int stage) {return NULL;}

	void SetupShader()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	// Sets constants
	void SetupConstants()
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

	SHADER_DECLARE_PASS(Unlit);
	ITexture*		m_nBaseTexture;

END_SHADER_CLASS