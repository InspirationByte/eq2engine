//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Deferred ambient
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"
#include "../vars_generic.h"

class CDeferredAmbient : public CBaseShader
{
public:
	CDeferredAmbient()
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
	void InitTextures()
	{
		SHADER_PARAM_RENDERTARGET_FIND(Albedo, m_pRTAlbedo);
		SHADER_PARAM_RENDERTARGET_FIND(Normal, m_pRTNormals);
		SHADER_PARAM_RENDERTARGET_FIND(Depth, m_pRTDepth);
		SHADER_PARAM_RENDERTARGET_FIND(MaterialMap1, m_pRTMatMap);

		SHADER_PARAM_TEXTURE(SSAODirectionMap, m_pRotationMap);

		SetParameterFunctor(SHADERPARAM_ALPHASETUP, &CDeferredAmbient::ParamSetup_AlphaModel_Translucent);

		//m_pRotationMap = g_pShaderAPI->LoadTexture("engine/rot_map", TEXFILTER_LINEAR, ADDRESSMODE_WRAP);
		//AddTextureToAutoremover(&m_pRotationMap);
	}

	// Initialize shader(s)
	bool InitShaders()
	{
		// just skip if we already have shader
		if(SHADER_PASS(Ambient))
			return true;

		// required
		SHADERDEFINES_BEGIN

		SHADER_FIND_OR_COMPILE(Ambient, "DeferredAmbient")

		SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG") // for compile it's always enabled

		SHADER_FIND_OR_COMPILE(Ambient_fog, "DeferredAmbient")

		SetParameterFunctor(SHADERPARAM_COLOR, &CDeferredAmbient::SetupColors);

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

	// Return real shader name
	const char* GetName()
	{
		return "DeferredAmbient";
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_FOGSELECT(Ambient);
	}

	// Sets constants
	void SetupConstants()
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

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Ambient);
	}

private:
	SHADER_DECLARE_PASS(Ambient);
	SHADER_DECLARE_FOGPASS(Ambient);

	// ambient needs only depth and albedo
	ITexture* m_pRTAlbedo;
	ITexture* m_pRTDepth;
	ITexture* m_pRTNormals;
	ITexture* m_pRTMatMap;
	ITexture* m_pRotationMap;

	IRenderState* m_pDepthStencilState;
};

DEFINE_SHADER(DeferredAmbient, CDeferredAmbient)

/*
	
//---------------------------------------------
// Point light shader
//---------------------------------------------

class SHDeferredPointlight : public CBaseShader
{
public:
	SHDeferredPointlight();

	// Sets parameters and compiles shader
	void InitParams();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	// Return real shader name
	const char* GetName()
	{
		return "DS_PointLight";
	}

	ITexture* GetBaseTexture(int n)		{return NULL;}
	ITexture* GetBumpTexture(int n)		{return NULL;}
	bool IsHasAlphaTexture()		{return false;}
	bool IsCastsShadows()			{return false;}

	void Unload(bool bUnloadShaders = true, bool bUnloadTextures = true)
	{
		if(bUnloadShaders)
		{
			SHADER_PASS_UNLOAD(Lighting);

			//SHADER_FOGPASS_UNLOAD(Ambient);
		}
	}

	// Sets constants
	void SetupConstants();

private:
	SHADER_DECLARE_PASS(Lighting);
	// SHADER_DECLARE_FOGPASS(Lighting);

	// ambient needs only depth and albedo
	ITexture* m_pRTAlbedo;
	ITexture* m_pRTNormals;
	ITexture* m_pRTDepth;
	ITexture* m_pRTMatMap1;
};

//--------------------------------------
// BaseUnlit
//--------------------------------------

SHDeferredPointlight::SHDeferredPointlight() : CBaseShader()
{
	SHADER_PASS(Lighting)		= NULL;
	//SHADER_FOGPASS(Lighting)		= NULL;

	m_pRTAlbedo = NULL;
	m_pRTDepth = NULL;
	m_pRTNormals = NULL;
	m_pRTMatMap1 = NULL;
}

//--------------------------------------
// Init parameters
//--------------------------------------

void SHDeferredPointlight::InitParams()
{
	if(!GetAssignedMaterial()->IsError() && !IsInitialized() && !IsError())
	{
		// Call base classinitialization
		CBaseShader::InitParams();
	}
}

void SHDeferredPointlight::InitTextures()
{
	IMatVar *mv_albedo = GetAssignedMaterial()->FindMaterialVar("albedo");
	IMatVar *mv_depth = GetAssignedMaterial()->FindMaterialVar("depth");
	IMatVar *mv_normal = GetAssignedMaterial()->FindMaterialVar("normal");
	IMatVar *mv_mm1 = GetAssignedMaterial()->FindMaterialVar("materialmap1");

	if(!m_pRTAlbedo && mv_albedo)
	{
		m_pRTAlbedo = g_pShaderAPI->FindTexture(mv_albedo->GetString());

		if(!m_pRTAlbedo)
			m_pRTAlbedo = g_pShaderAPI->GetErrorTexture();
	}


	if(!m_pRTDepth && mv_depth)
	{
		m_pRTDepth = g_pShaderAPI->FindTexture(mv_depth->GetString());

		if(!m_pRTDepth)
			m_pRTDepth = g_pShaderAPI->GetErrorTexture();
	}

	if(!m_pRTNormals && mv_normal)
	{
		m_pRTNormals = g_pShaderAPI->FindTexture(mv_normal->GetString());

		if(!m_pRTNormals)
			m_pRTNormals = g_pShaderAPI->GetErrorTexture();
	}

	if(!m_pRTMatMap1 && mv_mm1)
	{
		m_pRTMatMap1 = g_pShaderAPI->FindTexture(mv_mm1->GetString());

		if(!m_pRTMatMap1)
			m_pRTMatMap1 = g_pShaderAPI->GetErrorTexture();
	}
}

bool SHDeferredPointlight::InitShaders()
{
	// just skip if we already have shader
	if(SHADER_PASS(Lighting))
		return true;

	// required
	SHADERDEFINES_BEGIN

	//SHADER_DECLARE_SIMPLE_DEFINITION(!m_bNotShadowed, "RECEIVESHADOW")
	SHADER_DECLARE_SIMPLE_DEFINITION(r_phong->GetBool(), "PHONG")

	
	int x,y,vW,vH;
	g_pShaderAPI->GetViewport(x,y,vW,vH);
	

	SHADER_ADD_FLOAT_DEFINITION("SCREENTEXEL_X", 1.0f/vW);
	SHADER_ADD_FLOAT_DEFINITION("SCREENTEXEL_Y", 1.0f/vH);

	SHADER_FIND_OR_COMPILE(Lighting, "DeferredPointlight")

	//SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG") // for compile it's always enabled

	//SHADER_FIND_OR_COMPILE(Lighting_fog, "DeferredPointlight")

	return true;
}

//--------------------------------------
// Set Constants
//--------------------------------------


void SHDeferredPointlight::SetupConstants()
{
	// If we has shader
	if(!IsError())
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		// Setup standard shader
		//SHADER_BIND_PASS_FOGSELECT(Lighting);
		SHADER_BIND_PASS_SIMPLE(Lighting);

		// set needed cull mode
		g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT, FILL_SOLID);
		
		// set depth state
		g_pShaderAPI->ChangeDepthStateEx(false, false);

		g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

		// Apply the samplers 0, 1, 2, 3 as albedo, normals, depth, transparency
		g_pShaderAPI->SetTexture(m_pRTAlbedo, 0);
		g_pShaderAPI->SetTexture(m_pRTDepth, 2);
		g_pShaderAPI->SetTexture(m_pRTNormals, 3);
		g_pShaderAPI->SetTexture(m_pRTMatMap1, 4);

		if(!materials->GetLight()->pMaskTexture)
			g_pShaderAPI->SetTexture(materials->GetWhiteTexture(), 11);
		else
			g_pShaderAPI->SetTexture(materials->GetLight()->pMaskTexture, 11);

#ifndef DS_SPHERE
		int vW,vH;
		g_pShaderAPI->GetViewport(vW,vH);

		// window dims
		Vector4D scaleBias2D(2.0f / vW, 2.0f / (-vH), -1.0f, 1.0f);
		g_pShaderAPI->SetVertexShaderConstantVector4D(90,scaleBias2D);
#else
		Matrix4x4 lightSphereRotation = identity4();
		Matrix4x4 lightSphereTransform = translate(materials->GetLight()->position) * lightSphereRotation
			* scale(
				materials->GetLight()->radius.x,
				materials->GetLight()->radius.y,
				materials->GetLight()->radius.z
		);

		//g_pShaderAPI->SetVertexShaderConstantMatrix4(98, viewrenderer->GetMatrix(MATRIXMODE_VIEW) * lightSphereTransform);
		//g_pShaderAPI->SetVertexShaderConstantMatrix4(90, viewrenderer->GetMatrix(MATRIXMODE_PROJECTION) * (viewrenderer->GetMatrix(MATRIXMODE_VIEW) * lightSphereTransform));
		//g_pShaderAPI->SetPixelShaderConstantMatrix4(15, viewrenderer->GetMatrix(MATRIXMODE_PROJECTION));
#endif

		Matrix3x3 light_rotation = materials->GetLight()->lightRotation;

		g_pShaderAPI->SetPixelShaderConstantVector3D(25, materials->GetLight()->radius);   // doom3 radius
		g_pShaderAPI->SetPixelShaderConstantMatrix4(26, Matrix4x4(	Vector4D(light_rotation.rows[0],0),
																	Vector4D(light_rotation.rows[1],0),
																	Vector4D(light_rotation.rows[2],0),
																	Vector4D(0,0,0, 1)));

		g_pShaderAPI->SetVertexShaderConstantFloat(4, materials->GetLight()->radius.x);

		g_pShaderAPI->SetPixelShaderConstantMatrix4(2, !viewrenderer->GetViewProjectionMatrix()); // inverse view projection
		g_pShaderAPI->SetPixelShaderConstantMatrix4(8, !viewrenderer->GetMatrix(MATRIXMODE_VIEW));		// inverse view

		// light position was sent to vertex shader first
		g_pShaderAPI->SetVertexShaderConstantVector3D(38, materials->GetLight()->position);

		// light color
		g_pShaderAPI->SetPixelShaderConstantVector4D(1, Vector4D(materials->GetLight()->color * materials->GetLight()->intensity*r_lightscale->GetFloat(),1.0 / materials->GetLight()->radius.x));

		// setup zfar/znear
		//g_pShaderAPI->SetPixelShaderConstantVector2D(14, Vector2D(scenerenderer->GetSceneInfo()->m_fZNear, scenerenderer->GetSceneInfo()->m_fZFar));

		// view origin
		//g_pShaderAPI->SetPixelShaderConstantVector3D(12, scenerenderer->GetView()->GetOrigin());

		// bind shadowmap
		//scenerenderer->BindShadowBuffers();
	}
	else
	{
		// Set error texture
		g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
	}
}

// DECLARE SHADER!
DEFINE_SHADER(DeferredPointLight,SHDeferredPointlight)

	/*

//---------------------------------------------
// Spot light shader
//---------------------------------------------

class SHDeferredSpotlight : public CBaseShader
{
public:
	SHDeferredSpotlight();

	// Sets parameters and compiles shader
	void InitParams();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	// Return real shader name
	const char* GetName()
	{
		return "DS_SpotLight";
	}

	ITexture* GetBaseTexture()		{return NULL;}
	ITexture* GetBumpTexture()		{return NULL;}
	bool IsHasAlphaTexture()		{return false;}
	bool IsCastsShadows()			{return false;}

	void Unload(bool bUnloadShaders = true, bool bUnloadTextures = true)
	{
		if(bUnloadShaders)
		{
			SHADER_PASS_UNLOAD(Lighting);

			//SHADER_FOGPASS_UNLOAD(Ambient);
		}
	}

	// Sets constants
	void SetConstants();

private:
	SHADER_DECLARE_PASS(Lighting);
	// SHADER_DECLARE_FOGPASS(Lighting);

	// ambient needs only depth and albedo
	ITexture* m_pRTAlbedo;
	ITexture* m_pRTNormals;
	ITexture* m_pRTDepth;
	ITexture* m_pRTMatMap1;
};

SHDeferredSpotlight::SHDeferredSpotlight() : CBaseShader()
{
	SHADER_PASS(Lighting)		= NULL;
	//SHADER_FOGPASS(Lighting)		= NULL;

	m_pRTAlbedo = NULL;
	m_pRTAlbedo= NULL;
	m_pRTDepth = NULL;
	m_pRTNormals = NULL;
	m_pRTMatMap1 = NULL;
}

//--------------------------------------
// Init parameters
//--------------------------------------

void SHDeferredSpotlight::InitParams()
{
	if(!GetAssignedMaterial()->IsError() && !IsInitialized() && !IsError())
	{
		// Call base classinitialization
		CBaseShader::InitParams();
	}
}

void SHDeferredSpotlight::InitTextures()
{
	IMatVar *mv_albedo = GetAssignedMaterial()->FindMaterialVar("albedo");
	IMatVar *mv_depth = GetAssignedMaterial()->FindMaterialVar("depth");
	IMatVar *mv_normal = GetAssignedMaterial()->FindMaterialVar("normal");
	IMatVar *mv_mm1 = GetAssignedMaterial()->FindMaterialVar("materialmap1");

	if(!m_pRTAlbedo && mv_albedo)
	{
		m_pRTAlbedo = g_pShaderAPI->FindTexture(mv_albedo->GetString());

		if(!m_pRTAlbedo)
			m_pRTAlbedo = g_pShaderAPI->GetErrorTexture();
	}

	if(!m_pRTDepth && mv_depth)
	{
		m_pRTDepth = g_pShaderAPI->FindTexture(mv_depth->GetString());

		if(!m_pRTDepth)
			m_pRTDepth = g_pShaderAPI->GetErrorTexture();
	}

	if(!m_pRTNormals && mv_normal)
	{
		m_pRTNormals = g_pShaderAPI->FindTexture(mv_normal->GetString());

		if(!m_pRTNormals)
			m_pRTNormals = g_pShaderAPI->GetErrorTexture();
	}

	if(!m_pRTMatMap1 && mv_mm1)
	{
		m_pRTMatMap1 = g_pShaderAPI->FindTexture(mv_mm1->GetString());

		if(!m_pRTMatMap1)
			m_pRTMatMap1 = g_pShaderAPI->GetErrorTexture();
	}

}

bool SHDeferredSpotlight::InitShaders()
{
	// just skip if we already have shader
	if(SHADER_PASS(Lighting))
		return true;

	// required
	SHADERDEFINES_BEGIN()

	SHADER_DECLARE_SIMPLE_DEFINITION(!m_bNotShadowed, "RECEIVESHADOW")
	SHADER_DECLARE_SIMPLE_DEFINITION(r_phong->GetBool(), "PHONG")

	int vW,vH;
	g_pShaderAPI->GetViewport(vW,vH);


	SHADER_ADD_FLOAT_DEFINITION("SCREENTEXEL_X", 1.0f/vW);
	SHADER_ADD_FLOAT_DEFINITION("SCREENTEXEL_Y", 1.0f/vH);

	SHADER_FIND_OR_COMPILE(Lighting, "DeferredSpotlight")


	//SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG") // for compile it's always enabled

	//SHADER_FIND_OR_COMPILE(Lighting_fog, "DeferredPointlight")

	return true;
}

//--------------------------------------
// Set Constants
//--------------------------------------

void SHDeferredSpotlight::SetConstants()
{
	// If we has shader
	if(!IsError())
	{
		// Setup standard shader
		//SHADER_BIND_PASS_FOGSELECT(Lighting);
		SHADER_BIND_PASS_SIMPLE(Lighting);

		// set needed cull mode
		g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT, FILL_SOLID);
		
		// set depth state
		g_pShaderAPI->ChangeDepthStateEx(false, false);

		g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

		// Apply the samplers 0, 1, 2, 3 as albedo, normals, depth, transparency
		g_pShaderAPI->SetTexture(m_pRTAlbedo, 0);
		g_pShaderAPI->SetTexture(m_pRTDepth, 2);
		g_pShaderAPI->SetTexture(m_pRTNormals, 3);
		g_pShaderAPI->SetTexture(m_pRTMatMap1, 4);

		// apply spot mask
		g_pShaderAPI->SetTexture(scenerenderer->GetCurrentLight()->mask, 5); // s5

		if(!(scenerenderer->GetCurrentLight()->nFlags & LFLAG_MATRIXSET))
		{
			Matrix4x4 spotProj = identity4() * perspectiveMatrixX(DEG2RAD(scenerenderer->GetCurrentLight()->fov), 512, 512, 1.0f, scenerenderer->GetCurrentLight()->radius);
			Matrix4x4 modelview = rotateZXY(DEG2RAD(-scenerenderer->GetCurrentLight()->angles.x), DEG2RAD(-scenerenderer->GetCurrentLight()->angles.y), DEG2RAD(-scenerenderer->GetCurrentLight()->angles.z));
			modelview.translate(-scenerenderer->GetCurrentLight()->position);

			scenerenderer->GetCurrentLight()->spotmatrix = spotProj*modelview;
			scenerenderer->GetCurrentLight()->nFlags |= LFLAG_MATRIXSET;
		}

#ifndef DS_SPHERE
		int vW,vH;
		g_pShaderAPI->GetViewport(vW,vH);

		// window dims
		Vector4D scaleBias2D(2.0f / vW, 2.0f / (-vH), -1.0f, 1.0f);
		g_pShaderAPI->SetVertexShaderConstantVector4D(90,scaleBias2D);
#else
		Matrix4x4 lightSphereRotation = identity4();
		Matrix4x4 lightSphereTransform = translate(scenerenderer->GetCurrentLight()->position) * lightSphereRotation
			* scale(
				scenerenderer->GetCurrentLight()->radius,
				scenerenderer->GetCurrentLight()->radius,
				scenerenderer->GetCurrentLight()->radius
		);

		g_pShaderAPI->SetVertexShaderConstantMatrix4(98, scenerenderer->GetModelView() * lightSphereTransform);
		g_pShaderAPI->SetVertexShaderConstantMatrix4(90, scenerenderer->GetProjection() * (scenerenderer->GetModelView() * lightSphereTransform));
		g_pShaderAPI->SetPixelShaderConstantMatrix4(19, scenerenderer->GetProjection());
#endif

		g_pShaderAPI->SetVertexShaderConstantFloat(4, scenerenderer->GetCurrentLight()->radius);

		g_pShaderAPI->SetPixelShaderConstantMatrix4(2, !scenerenderer->GetWorldToScreen()); // inverse view projection
		g_pShaderAPI->SetPixelShaderConstantMatrix4(8, !scenerenderer->GetModelView());		// inverse view

		// light position was sent to vertex shader first
		g_pShaderAPI->SetVertexShaderConstantVector3D(38, scenerenderer->GetCurrentLight()->position);

		// light color
		g_pShaderAPI->SetPixelShaderConstantVector4D(1, Vector4D(scenerenderer->GetCurrentLight()->color * scenerenderer->GetCurrentLight()->intensity*r_lightscale->GetFloat(),1.0 / scenerenderer->GetCurrentLight()->radius));

		// setup zfar/znear
		g_pShaderAPI->SetPixelShaderConstantVector3D(14, Vector3D(scenerenderer->GetSceneInfo()->m_fZNear, scenerenderer->GetSceneInfo()->m_fZFar, scenerenderer->GetCurrentLight()->radius));

		// spot matrix for pixel shader
		g_pShaderAPI->SetPixelShaderConstantMatrix4(15, scenerenderer->GetCurrentLight()->spotmatrix); // r15 - r18

		// view origin
		g_pShaderAPI->SetPixelShaderConstantVector3D(12, scenerenderer->GetView()->GetOrigin());

		// bind shadowmap
		scenerenderer->BindShadowBuffers();
	}
	else
	{
		// Set error texture
		g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
	}
}

// DECLARE SHADER!
DEFINE_SHADER(DeferredSpotLight,SHDeferredSpotlight)

//---------------------------------------------
// Sun light shader
//---------------------------------------------

class SHDeferredSunlight : public CBaseShader
{
public:
	SHDeferredSunlight();

	// Sets parameters and compiles shader
	void InitParams();

	// Initialize textures
	void InitTextures();

	// Initialize shader(s)
	bool InitShaders();

	// Return real shader name
	const char* GetName()
	{
		return "DS_SunLight";
	}

	ITexture* GetBaseTexture()		{return NULL;}
	ITexture* GetBumpTexture()		{return NULL;}
	bool IsHasAlphaTexture()		{return false;}
	bool IsCastsShadows()			{return false;}

	void Unload(bool bUnloadShaders = true, bool bUnloadTextures = true)
	{
		if(bUnloadShaders)
		{
			SHADER_PASS_UNLOAD(Lighting);

			SHADER_FOGPASS_UNLOAD(Lighting);
		}
	}

	// Sets constants
	void SetConstants();

private:
	SHADER_DECLARE_PASS(Lighting);
	SHADER_DECLARE_FOGPASS(Lighting);

	// ambient needs only depth and albedo
	ITexture* m_pRTAlbedo;
	ITexture* m_pRTNormals;
	ITexture* m_pRTDepth;

	ITexture* m_pRTMaterialMap1;

	ITexture* m_pRainOnFloorTextures;
	ITexture* m_pRainOnWallTexture;

	IMatVar*	m_pRainTextureFrame;

	bool m_bRainMap;
};

SHDeferredSunlight::SHDeferredSunlight() : CBaseShader()
{
	SHADER_PASS(Lighting)		= NULL;
	SHADER_FOGPASS(Lighting)		= NULL;

	m_pRTAlbedo = NULL;
	m_pRTDepth = NULL;
	m_pRTNormals = NULL;
	m_pRTMaterialMap1 = NULL;

	m_pRainOnFloorTextures = NULL;
	m_pRainOnWallTexture = NULL;

	m_pRTMaterialMap1 = NULL;

	m_pRainTextureFrame = NULL;

	m_bRainMap = false;
}

//--------------------------------------
// Init parameters
//--------------------------------------

void SHDeferredSunlight::InitParams()
{
	if(!GetAssignedMaterial()->IsError() && !IsInitialized() && !IsError())
	{
		// Call base classinitialization
		CBaseShader::InitParams();
	}
}

void SHDeferredSunlight::InitTextures()
{
	// required
	m_pRainTextureFrame			= GetAssignedMaterial()->FindMaterialVar("RainTextureFrame");

	if(!m_pRainTextureFrame)
	{
		m_pRainTextureFrame = GetAssignedMaterial()->CreateMaterialVar("RainTextureFrame");
		m_pRainTextureFrame->SetInt(0);
	}

	IMatVar *mv_albedo = GetAssignedMaterial()->FindMaterialVar("albedo");
	IMatVar *mv_depth = GetAssignedMaterial()->FindMaterialVar("depth");
	IMatVar *mv_normal = GetAssignedMaterial()->FindMaterialVar("normal");
	IMatVar *mv_matmap1 = GetAssignedMaterial()->FindMaterialVar("materials1");

	IMatVar *mv_rainmap = GetAssignedMaterial()->FindMaterialVar("rainmap");
	IMatVar *mv_rain_onfloor = GetAssignedMaterial()->FindMaterialVar("rainmap_floor");
	IMatVar *mv_rain_onwall = GetAssignedMaterial()->FindMaterialVar("rainmap_wall");

	if(mv_matmap1)
		m_pRTMaterialMap1 = g_pShaderAPI->FindTexture(mv_matmap1->GetString());

	if(mv_rainmap)
		m_bRainMap = mv_rainmap->GetInt() > 0;

	if(!m_pRainOnFloorTextures && m_bRainMap && mv_rain_onfloor)
		m_pRainOnFloorTextures = g_pShaderAPI->LoadTexture(mv_rain_onfloor->GetString(), TEXFILTER_LINEAR);

	if(!m_pRainOnWallTexture && m_bRainMap && mv_rain_onwall)
		m_pRainOnWallTexture = g_pShaderAPI->LoadTexture(mv_rain_onwall->GetString(), TEXFILTER_LINEAR);

	if(!m_pRTAlbedo && mv_albedo)
	{
		m_pRTAlbedo = g_pShaderAPI->FindTexture(mv_albedo->GetString());

		if(!m_pRTAlbedo)
			m_pRTAlbedo = g_pShaderAPI->GetErrorTexture();
	}

	if(!m_pRTDepth && mv_depth)
	{
		m_pRTDepth = g_pShaderAPI->FindTexture(mv_depth->GetString());

		if(!m_pRTDepth)
			m_pRTDepth = g_pShaderAPI->GetErrorTexture();
	}

	if(!m_pRTNormals && mv_depth)
	{
		m_pRTNormals = g_pShaderAPI->FindTexture(mv_normal->GetString());

		if(!m_pRTNormals)
			m_pRTNormals = g_pShaderAPI->GetErrorTexture();
	}
}

bool SHDeferredSunlight::InitShaders()
{
	// just skip if we already have shader
	if(SHADER_PASS(Lighting))
		return true;

	// required
	SHADERDEFINES_BEGIN()

	SHADER_DECLARE_SIMPLE_DEFINITION(r_phong->GetBool(), "PHONG")
	SHADER_DECLARE_SIMPLE_DEFINITION(!m_bNotShadowed, "RECEIVESHADOW")
	SHADER_DECLARE_SIMPLE_DEFINITION(!m_bRainMap, "RAINMAP")

	SHADER_FIND_OR_COMPILE(Lighting, m_bRainMap ? "DeferredRainMap" : "DeferredSunlight")

	SHADER_DECLARE_SIMPLE_DEFINITION(true, "DOFOG") // for compile it's always enabled

	SHADER_FIND_OR_COMPILE(Lighting_fog, m_bRainMap ? "DeferredRainMap" : "DeferredSunlight")

	return true;
}

//--------------------------------------
// Set Constants
//--------------------------------------

HOOK_TO_CVAR(m_far)
HOOK_TO_CVAR(m_near)

HOOK_TO_CVAR(r_sunshadowscale)

ConVar rainmap_func("rainmap_func","2");

//ConVar r_shadowquality("r_shadowquality", "0", "Shadow quality mode", CV_ARCHIVE);

void SHDeferredSunlight::SetConstants()
{
	// If we has shader
	if(!IsError())
	{
		// Setup standard shader
		SHADER_BIND_PASS_FOGSELECT(Lighting);

		// set needed cull mode
		g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT, FILL_SOLID);
		
		// set depth state
		g_pShaderAPI->ChangeDepthStateEx(false, false);

		if(m_bRainMap)
			g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_ONE, BLENDFACTOR_ONE, rainmap_func.GetInt());
		else
			g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_ONE, BLENDFACTOR_ONE, BLENDFUNC_ADD);

		// Apply the samplers 0, 1, 2, 3 as albedo, normals, depth, transparency
		g_pShaderAPI->SetTexture(m_pRTAlbedo, 0);
		g_pShaderAPI->SetTexture(m_pRTDepth, 2);
		g_pShaderAPI->SetTexture(m_pRTNormals, 3);
		g_pShaderAPI->SetTexture(m_pRTMaterialMap1, 4);

		if(m_bRainMap)
		{
			if(m_pRainTextureFrame->GetInt() >= m_pRainOnFloorTextures->GetAnimatedTextureFrames())
				m_pRainTextureFrame->SetInt(0);

			m_pRainOnFloorTextures->SetAnimatedTextureFrame(m_pRainTextureFrame->GetInt());

			g_pShaderAPI->SetTexture(m_pRainOnFloorTextures, 9);
			//g_pShaderAPI->SetTexture(m_pRainOnWallTexture, 5);

			static ITexture* s_pCubemap = g_pShaderAPI->LoadTexture("skybox/rain_night", TEXFILTER_LINEAR);

			g_pShaderAPI->SetTexture(s_pCubemap, 8);

			static ITexture *rainReflection = g_pShaderAPI->LoadTexture("_rt_framebuffer", TEXFILTER_LINEAR);

			g_pShaderAPI->CopyFramebufferToTexture(rainReflection);

			g_pShaderAPI->SetTexture(rainReflection, 7);
		}

		int vW,vH;
		g_pShaderAPI->GetViewport(vW,vH);

		// window dims
		Vector4D scaleBias2D(2.0f / vW, 2.0f / (-vH), -1.0f, 1.0f);
		g_pShaderAPI->SetVertexShaderConstantVector4D(90,scaleBias2D);

		g_pShaderAPI->SetPixelShaderConstantMatrix4(2, !scenerenderer->GetWorldToScreen()); // inverse view projection
		g_pShaderAPI->SetPixelShaderConstantMatrix4(8, !scenerenderer->GetModelView());		// inverse view

		g_pShaderAPI->SetPixelShaderConstantMatrix4(26, scenerenderer->GetModelView()); // inverse view projection

		Vector3D lightDir;
		AngleVectors(scenerenderer->GetCurrentLight()->angles*Vector3D(-1,1,1),&lightDir,NULL,NULL);

		// r38 lightdirection as like lightPos
		g_pShaderAPI->SetVertexShaderConstantVector3D(38, Vector3D(-lightDir.x,lightDir.y,-lightDir.z));

		// light color
		g_pShaderAPI->SetPixelShaderConstantVector4D(1, Vector4D(scenerenderer->GetCurrentLight()->color * scenerenderer->GetCurrentLight()->intensity*r_lightscale->GetFloat(),1.0 / scenerenderer->GetCurrentLight()->radius));

		// setup zfar/znear
		g_pShaderAPI->SetPixelShaderConstantVector3D(14, Vector3D(scenerenderer->GetSceneInfo()->m_fZNear, scenerenderer->GetSceneInfo()->m_fZFar, scenerenderer->GetCurrentLight()->radius));

		g_pShaderAPI->SetPixelShaderConstantVector3D(15, scenerenderer->GetCurrentLight()->position);

		// spot matrix for pixel shader
		g_pShaderAPI->SetPixelShaderConstantMatrix4(16, scenerenderer->GetSunMatrix(0)); // r16 - r19
		g_pShaderAPI->SetPixelShaderConstantMatrix4(20, scenerenderer->GetSunMatrix(1)); // r16 - r19

		// view origin
		g_pShaderAPI->SetPixelShaderConstantVector3D(12, scenerenderer->GetView()->GetOrigin());

		// bind shadowmap
		scenerenderer->BindShadowBuffers();
	}
	else
	{
		// Set error texture
		g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());
	}
}

// DECLARE SHADER!
DEFINE_SHADER(DeferredSunLight,SHDeferredSunlight)
*/