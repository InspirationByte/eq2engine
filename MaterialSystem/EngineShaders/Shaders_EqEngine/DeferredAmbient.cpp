//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Deferred ambient
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "../vars_generic.h"

BEGIN_SHADER_CLASS(DeferredAmbient)

	SHADER_INIT_PARAMS()
	{
		SHADER_PASS(Ambient) = NULL;
		SHADER_FOGPASS(Ambient) = NULL;

		m_pRTAlbedo = NULL;
		m_pRTDepth = NULL;
		m_pRTMatMap = NULL;
		m_pRTNormals = NULL;
		m_pDepthStencilState = NULL;
		m_pRotationMap = NULL;
	}

	// Initialize textures
	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_RENDERTARGET_FIND(Albedo, m_pRTAlbedo);
		SHADER_PARAM_RENDERTARGET_FIND(Normal, m_pRTNormals);
		SHADER_PARAM_RENDERTARGET_FIND(Depth, m_pRTDepth);
		SHADER_PARAM_RENDERTARGET_FIND(MaterialMap1, m_pRTMatMap);

		SHADER_PARAM_TEXTURE(SSAODirectionMap, m_pRotationMap);

		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &ThisShaderClass::ParamSetup_AlphaModel_Translucent);

		//m_pRotationMap = g_pShaderAPI->LoadTexture("engine/rot_map", TEXFILTER_LINEAR, TEXADDRESS_WRAP);
		//AddTextureToAutoremover(&m_pRotationMap);
	}

	// Initialize shader(s)
	SHADER_INIT_RHI()
	{
		// just skip if we already have shader
		if(SHADER_PASS(Ambient))
			return true;

		// required
		SHADERDEFINES_BEGIN

		SHADER_FIND_OR_COMPILE(Ambient, "DeferredAmbient")

		SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG") // for compile it's always enabled

		SHADER_FIND_OR_COMPILE(Ambient_fog, "DeferredAmbient")

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetupColors);

		/*
		DepthStencilStateParams_t depth_params;
		depth_params.nStencilFail = STENCILFUNC_REPLACE;
		depth_params.nStencilPass = STENCILFUNC_REPLACE;
		depth_params.nStencilFunc = COMP_EQUAL;
		depth_params.doStencilTest = true;

		m_pDepthStencilState = g_pShaderAPI->CreateDepthStencilState(depth_params);
		m_UsedRenderStates.append( &m_pDepthStencilState );
		*/

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_FOGSELECT(Ambient);
	}

	// Sets constants
	SHADER_SETUP_CONSTANTS()
	{
		// If we has shader
		if(!IsError())
		{
			SetupDefaultParameter(SHADERPARAM_TRANSFORM);
			SetupDefaultParameter(SHADERPARAM_FOG);
			SetupDefaultParameter(SHADERPARAM_COLOR);
			SetupDefaultParameter(SHADERPARAM_ALPHASETUP);

			// set needed cull mode
			materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
		
			// set depth state
			materials->SetDepthStates(false, false);
			//g_pShaderAPI->SetDepthStencilState( m_pDepthStencilState );

			// Apply the samplers 0, 1, 2, 3 as albedo, normals, depth, transparency
			g_pShaderAPI->SetTexture(m_pRTAlbedo, "Albedo", 0);
			g_pShaderAPI->SetTexture(m_pRTNormals, "Normals", 1);
			g_pShaderAPI->SetTexture(m_pRTDepth, "Depth", 2);
			g_pShaderAPI->SetTexture(m_pRTMatMap, "MaterialMap1", 3);
			g_pShaderAPI->SetTexture(m_pRotationMap, "SSAODirSampler", 4);
			
			//

			Matrix4x4 view, proj, world;
			materials->GetMatrix(MATRIXMODE_VIEW, view);
			materials->GetMatrix(MATRIXMODE_PROJECTION, proj);
			materials->GetMatrix(MATRIXMODE_WORLD, world);

			// get camera pos
			FogInfo_t fog;
			materials->GetFogInfo(fog);

			// retranslate
			view = view*translate(fog.viewPos);

			g_pShaderAPI->SetShaderConstantMatrix4("InvViewProj", !(proj*view)); // inverse view projection
			g_pShaderAPI->SetShaderConstantMatrix4("DS_VP", proj*view); // inverse view projection
		}
	}

	void SetupColors()
	{
		// Ambient color
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor()); // ambient color
	}

	ITexture* GetBaseTexture(int n)		{return NULL;}
	ITexture* GetBumpTexture(int n)		{return NULL;}

	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	// ambient needs only depth and albedo
	ITexture* m_pRTAlbedo;
	ITexture* m_pRTDepth;
	ITexture* m_pRTNormals;
	ITexture* m_pRTMatMap;
	ITexture* m_pRotationMap;

	IRenderState* m_pDepthStencilState;
END_SHADER_CLASS