//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Skybox 2D shader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

//--------------------------------------
// Basic cubemap skybox shader
//--------------------------------------

BEGIN_SHADER_CLASS(Skybox)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Unlit) = nullptr;
		m_flags |= MATERIAL_FLAG_SKY;
	}

	// Initialize textures
	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE(BaseTexture, m_baseTexture);
	}

	SHADER_INIT_RHI()
	{
		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "SkyBox");

		return true;
	}

	const ITexturePtr& GetBaseTexture(int stage) const {return m_baseTexture.Get();}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	// Sets constants
	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		// do depth testing for my type of skybox (looks like quake 3/unreal tournament style skyboxes)
		g_matSystem->SetDepthStates(true, false);
		g_matSystem->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO);
		g_matSystem->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	
		Matrix4x4 wvp;
		g_matSystem->GetWorldViewProjection(wvp);

		wvp = !wvp;

		Vector3D camPos(wvp.rows[0].w, wvp.rows[1].w, wvp.rows[2].w);

		// camera direction
		g_renderAPI->SetShaderConstant(StringToHashConst("camPos"), camPos * 2.0f);

		g_renderAPI->SetShaderConstant(StringToHashConst("AmbientColor"), g_matSystem->GetAmbientColor());

		// setup base texture
		g_renderAPI->SetTexture(StringToHashConst("BaseTextureSampler"), m_baseTexture.Get());
	}

	SHADER_DECLARE_PASS(Unlit);
	MatTextureProxy		m_baseTexture;

END_SHADER_CLASS