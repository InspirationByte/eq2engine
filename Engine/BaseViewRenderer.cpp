//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base view renderer implementation
//////////////////////////////////////////////////////////////////////////////////

#include "BaseViewRenderer.h"
#include "BaseRenderableObject.h"
#include "studio_egf.h"

#ifdef EDITOR
#include "../Editor/EditorMainFrame.h"
#include "../Editor/RenderWindows.h"
#endif

#ifdef EQLC
//#define USE_SINGLE_CUBEMAPRENDER
#include "../utils/eqlc/eqlc.h"
#else

// we're using DirectX 10
// #define USE_SINGLE_CUBEMAPRENDER

#include "IDebugOverlay.h"
#endif

static slight_t s_default_no_lights[8] = {slight_t(vec4_zero,vec4_zero)};

ConVar r_useLigting("r_useLigting", "1", "Use lighting", CV_ARCHIVE);

void lighting_callback(ConVar* pVar,char const* pszOldValue)
{
	MaterialLightingMode_e newModel = MATERIAL_LIGHT_UNLIT;
	
	if(r_useLigting.GetBool())
	{
		newModel = MATERIAL_LIGHT_FORWARD;
		if(pVar->GetBool())
			newModel = MATERIAL_LIGHT_DEFERRED;
	}

	bool bIsChanged = materials->GetLightingModel() != newModel;

	if(bIsChanged)
	{
		viewrenderer->ShutdownResources();
		
		materials->GetConfiguration().lighting_model = newModel;

		// reload materials
		materials->ReloadAllMaterials();

		viewrenderer->InitializeResources();
	}
}

ConVar r_advancedLighting("r_advancedLighting", "0", lighting_callback, "Advanced lighting model", CV_ARCHIVE);

void bump_callback(ConVar* pVar, char const* pszOldValue)
{
	bool bChanged = (materials->GetConfiguration().enableBumpmapping != pVar->GetBool());

	materials->GetConfiguration().enableBumpmapping = pVar->GetBool();

	if(bChanged)
		materials->ReloadAllMaterials();
}

void specular_callback(ConVar* pVar, char const* pszOldValue)
{
	bool bChanged = (materials->GetConfiguration().enableSpecular != pVar->GetBool());

	materials->GetConfiguration().enableSpecular = pVar->GetBool();

	if(bChanged)
		materials->ReloadAllMaterials();
}

void shadows_callback(ConVar* pVar, char const* pszOldValue)
{
	bool bChanged = (materials->GetConfiguration().enableShadows != pVar->GetBool());

	materials->GetConfiguration().enableShadows = pVar->GetBool();

	if(bChanged)
	{
		viewrenderer->ShutdownResources();
		materials->ReloadAllMaterials();
		viewrenderer->InitializeResources();
	}
}

ConVar r_bumpMapping("r_bumpmapping", "0", bump_callback, "Enable bump mapping", CV_ARCHIVE);
ConVar r_specular("r_specularmap", "0", specular_callback, "Enable specular maps", CV_ARCHIVE);
ConVar r_shadows("r_shadows", "0", shadows_callback, "Enable shadow maps", CV_ARCHIVE);
ConVar r_occlusionBuffersize("r_occlusionBuffersize", "512", "Occlusion buffer size (change needs alt-tab)", CV_ARCHIVE);

CBaseViewRenderer::CBaseViewRenderer()
{
	m_nViewportH = m_nViewportW = 512;
	m_nCubeFaceId = 0;

	m_pCurrentView = NULL;

	m_nRenderMode = VDM_AMBIENT;
	memset(m_pLights, 0, sizeof(m_pLights));
	m_numLights = 0;

	m_sceneinfo.m_bFogEnable = false;
	m_sceneinfo.m_cAmbientColor = color3_white;
	m_sceneinfo.m_cFogColor = color3_white;
	m_sceneinfo.m_fFogNear = 0;
	m_sceneinfo.m_fFogRange = 1500;
	m_sceneinfo.m_fZNear = 1;
	m_sceneinfo.m_fZFar = 15000;

	memset(m_pTargets,0,sizeof(m_pTargets));
	m_numMRTs = 0;
	m_pDepthTarget = NULL;

	memset(m_pGBufferTextures,0,sizeof(m_pGBufferTextures));
	memset(m_pShadowMaps,0,sizeof(m_pShadowMaps));

	memset(m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL], 0, sizeof(m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL]));
	memset(m_pShadowmapDepthwrite[DLT_SPOT], 0, sizeof(m_pShadowmapDepthwrite[DLT_SPOT]));
	memset(m_pShadowmapDepthwrite[DLT_SUN], 0, sizeof(m_pShadowmapDepthwrite[DLT_SUN]));

	memset(m_pDSLightMaterials[DLT_OMNIDIRECTIONAL], 0, sizeof(m_pDSLightMaterials[DLT_OMNIDIRECTIONAL]));
	memset(m_pDSLightMaterials[DLT_SPOT], 0, sizeof(m_pDSLightMaterials[DLT_SPOT]));
	memset(m_pDSLightMaterials[DLT_SUN], 0, sizeof(m_pDSLightMaterials[DLT_SUN]));

	m_pDSSpotlightReflector = NULL;

	//----------------------------------------------------------

	m_pSpotCaustics = NULL;
	m_pSpotCausticTex = NULL;
	m_pSpotCausticsBuffer = NULL;
	m_pSpotCausticsFormat = NULL;

	memset(m_pCausticsGBuffer, 0, sizeof(m_pCausticsGBuffer));
	
	//----------------------------------------------------------

	memset(m_pShadowMaps[DLT_OMNIDIRECTIONAL], 0, sizeof(m_pShadowMaps[DLT_OMNIDIRECTIONAL]));
	memset(m_pShadowMaps[DLT_SPOT], 0, sizeof(m_pShadowMaps[DLT_SPOT]));
	memset(m_pShadowMaps[DLT_SUN], 0, sizeof(m_pShadowMaps[DLT_SUN]));

	memset(m_pShadowMapDepth,0,sizeof(m_pShadowMapDepth));

	m_pDSAmbient = NULL;

	m_bCurrentSkinningPrepared = false;

	m_bDSInit = false;
	m_bShadowsInit = false;

	m_numViewRooms = 0;
	m_viewRooms[0] = -1;
	m_viewRooms[1] = -1;

#ifdef EQLC
	m_nLightmapID = 0;
#endif
}

//int g_nVRGraphBucket = -1;

ConVar r_ds_gBufferSizeScale("r_ds_gBufferSizeScale", "1.0", "GBuffer size scale for perfomance issues", CV_ARCHIVE);
ConVar r_photonCount("r_photonCount", "256", "Photon buffer size", CV_ARCHIVE);
ConVar r_photonBufferSize("r_photonBufferSize", "256", "Photon buffer size", CV_ARCHIVE);

// called when map is loading
void CBaseViewRenderer::InitializeResources()
{
	if(!g_pShaderAPI)
		return;

	// begin preloading zone - critical cache section
	materials->BeginPreloadMarker();

	m_numViewRooms = 0;
	m_viewRooms[0] = -1;
	m_viewRooms[1] = -1;

	//g_nVRGraphBucket = debugoverlay->Graph_AddBucket("VR matsys bind", ColorRGBA(0,1,0,1), 1.0f, 0.01f);

	if(materials->GetLightingModel() == MATERIAL_LIGHT_DEFERRED && !m_bDSInit)
	{
		int nDSLightModel = g_pModelCache->PrecacheModel("models/sphere_1x1.egf");

		if(nDSLightModel == -1)
		{
			CrashMsg("Deferred Shading is not initialized, can't load model \"models/sphere_1x1.egf\"");
			exit(-1);
		}

		m_pDSlightModel = g_pModelCache->GetModel(nDSLightModel);

		if(!m_pDSlightModel)
		{
			CrashMsg("Deferred Shading initialization error\n");
			exit(-1);
		}

		Msg("Initializing DS targets...\n");

		int screenWide;
		int screenTall;

#ifndef EDITOR

#ifndef EQLC
		screenWide = g_pEngineHost->GetWindowSize().x;
		screenTall = g_pEngineHost->GetWindowSize().y;
#else
		screenWide = g_lightmapSize;
		screenWide = g_lightmapSize;
#endif

#else
		//g_editormainframe->GetMaxRenderWindowSize(screenWide, screenTall);
		screenWide = m_nViewportW;
		screenTall = m_nViewportH;
#endif

		screenWide *= r_ds_gBufferSizeScale.GetFloat();
		screenTall *= r_ds_gBufferSizeScale.GetFloat();

		int nDSBuffersSize = 0;

		// Init GBuffer textures
		m_pGBufferTextures[GBUF_DIFFUSE] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_diff", screenWide, screenTall, FORMAT_RGBA8, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP);
		m_pGBufferTextures[GBUF_DIFFUSE]->Ref_Grab();

		m_pGBufferTextures[GBUF_NORMALS] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_norm", screenWide, screenTall, FORMAT_RGBA8, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP);
		m_pGBufferTextures[GBUF_NORMALS]->Ref_Grab();

		m_pGBufferTextures[GBUF_DEPTH] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_depth", screenWide, screenTall, FORMAT_RG32F, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP);
		m_pGBufferTextures[GBUF_DEPTH]->Ref_Grab();

		m_pGBufferTextures[GBUF_MATERIALMAP1] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_mat1", screenWide, screenTall, FORMAT_RGBA8, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP);
		m_pGBufferTextures[GBUF_MATERIALMAP1]->Ref_Grab();

		nDSBuffersSize += screenWide*screenTall*4;
		nDSBuffersSize *= 5;

		float fSize = ((float)nDSBuffersSize / 1024.0f);

		MsgWarning("GBUFFER usage is %g MB\n", (fSize / 1024.0f));

		//--------------------------------------------------------------------------------------
		// The caustics and flashlight reflector simulator
		//--------------------------------------------------------------------------------------

		// create texture first and then material
		m_pSpotCausticTex = g_pShaderAPI->CreateNamedRenderTarget("_rt_caustics", r_photonBufferSize.GetInt(), r_photonBufferSize.GetInt(), FORMAT_RGBA8, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);
		m_pSpotCausticTex->Ref_Grab();

		// TODO: needs to initialize m_pCausticsGBuffer

		m_pSpotCaustics = materials->FindMaterial("engine/flashlightreflector");
		m_pSpotCaustics->Ref_Grab();

		int NUM_PHOTONS = (r_photonCount.GetInt()*r_photonCount.GetInt());

		// init photon map VB
		Vector2D* photons = new Vector2D[NUM_PHOTONS];

		for(int i = 0; i < r_photonCount.GetInt(); i++)
		{
			for(int j = 0; j < r_photonCount.GetInt(); j++)
			{
				Vector2D coords(float(i) / float(r_photonCount.GetInt()), 1.0f - (float(j) / float(r_photonCount.GetInt())));

				photons[i*r_photonCount.GetInt()+j] = coords;
			}
		}

		m_pSpotCausticsBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, NUM_PHOTONS, sizeof(Vector2D), photons);

		delete [] photons;

		VertexFormatDesc_t pFormat[] = {
			{ 0, 2, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position
		};

		m_pSpotCausticsFormat = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));

		//--------------------------------------------------------------------------------------

		// THIS IS ONLY DX10
		//m_pGBufferTextures[GBUF_MATERIALMAP2] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_mat2", screenWide, screenTall, FORMAT_RGBA8, TEXFILTER_NEAREST, ADDRESSMODE_WRAP);

		Msg("Initializing DS materials...\n");

		m_pDSAmbient = materials->FindMaterial("engine/deferred/ds_ambient");
		m_pDSAmbient->Ref_Grab();

		// Init lighting materials
		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][0] = materials->FindMaterial("engine/deferred/ds_pointlight");

		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][0]->Ref_Grab();

		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][1] = materials->FindMaterial("engine/deferred/ds_pointlight_shadow");

		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][1]->Ref_Grab();

		m_pDSLightMaterials[DLT_SPOT][0] = materials->FindMaterial("engine/deferred/ds_spotlight");
		
		m_pDSLightMaterials[DLT_SPOT][0]->Ref_Grab();

		m_pDSLightMaterials[DLT_SPOT][1] = materials->FindMaterial("engine/deferred/ds_spotlight_shadow");

		m_pDSLightMaterials[DLT_SPOT][1]->Ref_Grab();

		m_pDSSpotlightReflector = materials->FindMaterial("engine/deferred/ds_spotlight_shadow_reflector");
		m_pDSSpotlightReflector->Ref_Grab();

		m_pDSLightMaterials[DLT_SUN][0] = materials->FindMaterial("engine/deferred/ds_sunlight");
		m_pDSLightMaterials[DLT_SUN][1] = m_pDSLightMaterials[DLT_SUN][0];

		m_pDSLightMaterials[DLT_SUN][0]->Ref_Grab();
		m_pDSLightMaterials[DLT_SUN][0]->Ref_Grab();

		m_bDSInit = true;
	}

	// init shadowmapping textures and materials
	if(materials->GetConfiguration().enableShadows && (materials->GetLightingModel() != MATERIAL_LIGHT_UNLIT) && !m_bShadowsInit)
	{
		m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][0]	= materials->FindMaterial("engine/pointdepth");
		m_pShadowmapDepthwrite[DLT_SPOT][0]				= materials->FindMaterial("engine/spotdepth");
		m_pShadowmapDepthwrite[DLT_SUN][0]				= materials->FindMaterial("engine/sundepth");

		m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][1]	= materials->FindMaterial("engine/pointdepth_skin");
		m_pShadowmapDepthwrite[DLT_SPOT][1]				= materials->FindMaterial("engine/spotdepth_skin");
		m_pShadowmapDepthwrite[DLT_SUN][1]				= materials->FindMaterial("engine/sundepth_skin");

		m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][2]	= materials->FindMaterial("engine/pointdepth_simple");
		m_pShadowmapDepthwrite[DLT_SPOT][2]				= materials->FindMaterial("engine/spotdepth_simple");
		m_pShadowmapDepthwrite[DLT_SUN][2]				= materials->FindMaterial("engine/sundepth_simple");

#ifdef EQLC
		m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]		= g_pShaderAPI->CreateNamedRenderTarget("_rt_cubedepth",2048,2048,FORMAT_R32F, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP, COMP_GREATER, TEXFLAG_CUBEMAP);
		m_pShadowMaps[DLT_SPOT][0]					= g_pShaderAPI->CreateNamedRenderTarget("_rt_spotdepth",2048,2048,FORMAT_R32F, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP, COMP_GREATER );

		// Sun has two depth textures
		//m_pShadowMaps[DLT_SUN][0]					= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth1",2048,2048,FORMAT_R16F,TEXFILTER_NEAREST);
		//m_pShadowMaps[DLT_SUN][1]					= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth2",2048,2048,FORMAT_R16F,TEXFILTER_NEAREST);
		
		m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]->Ref_Grab();
		m_pShadowMaps[DLT_SPOT][0]->Ref_Grab();

		//m_pShadowMaps[DLT_SUN][0]->Ref_Grab();
		//m_pShadowMaps[DLT_SUN][1]->Ref_Grab();

		// FIXME: THIS is very weird
		// this will be removed when engine will work on DirectX 10 API
		m_pShadowMapDepth[0]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap1",2048,2048,FORMAT_D32F,TEXFILTER_NEAREST);
		m_pShadowMapDepth[0]->Ref_Grab();

#ifdef USE_SINGLE_CUBEMAPRENDER
		m_pShadowMapDepth[1]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap2",2048,2048,FORMAT_D32F,TEXFILTER_NEAREST);
		m_pShadowMapDepth[2]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap3",2048,2048,FORMAT_D32F,TEXFILTER_NEAREST);
		m_pShadowMapDepth[3]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap4",2048,2048,FORMAT_D32F,TEXFILTER_NEAREST);
		m_pShadowMapDepth[4]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap5",2048,2048,FORMAT_D32F,TEXFILTER_NEAREST);
		m_pShadowMapDepth[5]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap6",2048,2048,FORMAT_D32F,TEXFILTER_NEAREST);

		m_pShadowMapDepth[1]->Ref_Grab();
		m_pShadowMapDepth[2]->Ref_Grab();
		m_pShadowMapDepth[3]->Ref_Grab();
		m_pShadowMapDepth[4]->Ref_Grab();
		m_pShadowMapDepth[5]->Ref_Grab();
#endif // USE_SINGLE_CUBEMAPRENDER
#else
		Filter_e shadowMapFilter = TEXFILTER_NEAREST;

		if(g_pShaderAPI->GetShaderAPIClass() == ShaderAPIClass_e::SHADERAPI_DIRECT3D10)
			shadowMapFilter = TEXFILTER_LINEAR;

		m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]		= g_pShaderAPI->CreateNamedRenderTarget("_rt_cubedepth",512,512,FORMAT_R32F, shadowMapFilter, ADDRESSMODE_CLAMP, COMP_GREATER, TEXFLAG_CUBEMAP);
		m_pShadowMaps[DLT_SPOT][0]					= g_pShaderAPI->CreateNamedRenderTarget("_rt_spotdepth",1024,1024,FORMAT_R32F,shadowMapFilter, ADDRESSMODE_CLAMP, COMP_GREATER);

		// Sun has two depth textures
		m_pShadowMaps[DLT_SUN][0]					= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth1",2048,2048,FORMAT_R32F,shadowMapFilter, ADDRESSMODE_CLAMP, COMP_GREATER);
		
		m_pShadowMaps[DLT_SUN][1]					= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth2",2048,2048,FORMAT_R32F,shadowMapFilter, ADDRESSMODE_CLAMP, COMP_GREATER);

		m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]->Ref_Grab();
		m_pShadowMaps[DLT_SPOT][0]->Ref_Grab();

		m_pShadowMaps[DLT_SUN][0]->Ref_Grab();
		m_pShadowMaps[DLT_SUN][1]->Ref_Grab();
		
		// FIXME: THIS is very weird
		// this will be removed when engine will work on DirectX 10 API
#ifndef USE_SINGLE_CUBEMAPRENDER
		m_pShadowMapDepth[0]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap1", 512, 512, FORMAT_D16, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP, COMP_NEVER, TEXTURE_FLAG_DEPTHBUFFER);
		m_pShadowMapDepth[1]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap2", 1024, 1024, FORMAT_D16, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP, COMP_NEVER, TEXTURE_FLAG_DEPTHBUFFER);
		m_pShadowMapDepth[2]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap3", 2048, 2048, FORMAT_D16, TEXFILTER_NEAREST, ADDRESSMODE_CLAMP, COMP_NEVER, TEXTURE_FLAG_DEPTHBUFFER);

		m_pShadowMapDepth[0]->Ref_Grab();
		m_pShadowMapDepth[1]->Ref_Grab();
		m_pShadowMapDepth[2]->Ref_Grab();
#else
		m_pShadowMapDepth[0]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap1", 2048, 2048, FORMAT_D16, TEXFILTER_NEAREST);
		m_pShadowMapDepth[1]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap2", 512, 512, FORMAT_D16, TEXFILTER_NEAREST);
		m_pShadowMapDepth[2]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap3", 512, 512, FORMAT_D16, TEXFILTER_NEAREST);
		m_pShadowMapDepth[3]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap4", 512, 512, FORMAT_D16, TEXFILTER_NEAREST);
		m_pShadowMapDepth[4]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap5", 512, 512, FORMAT_D16, TEXFILTER_NEAREST);
		m_pShadowMapDepth[5]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap6", 512, 512, FORMAT_D16, TEXFILTER_NEAREST);

		m_pShadowMapDepth[0]->Ref_Grab();
		m_pShadowMapDepth[1]->Ref_Grab();
		m_pShadowMapDepth[2]->Ref_Grab();
		m_pShadowMapDepth[3]->Ref_Grab();
		m_pShadowMapDepth[4]->Ref_Grab();
		m_pShadowMapDepth[5]->Ref_Grab();
#endif
#endif // EQLC
		
		m_bShadowsInit = true;
	}

	// estimated memory: 36 mb on 1680x1050 with DS

	materials->EndPreloadMarker();
}

// called when world is unloading
void CBaseViewRenderer::ShutdownResources()
{
	if(m_bDSInit)
	{
		m_bDSInit = false;

		for(int i = 0; i < GBUF_TEXTURES; i++)
		{
			g_pShaderAPI->FreeTexture(m_pGBufferTextures[i]);
			m_pGBufferTextures[i] = NULL;
		}

		materials->FreeMaterial(m_pDSAmbient);

		m_pDSAmbient = NULL;

		for(int i = 0; i < DLT_COUNT; i++)
		{
			materials->FreeMaterial(m_pDSLightMaterials[i][0]);
			materials->FreeMaterial(m_pDSLightMaterials[i][1]);
		}

		materials->FreeMaterial(m_pSpotCaustics);
		g_pShaderAPI->FreeTexture(m_pSpotCausticTex);
		g_pShaderAPI->DestroyVertexBuffer(m_pSpotCausticsBuffer);
		g_pShaderAPI->DestroyVertexFormat(m_pSpotCausticsFormat);

		materials->FreeMaterial(m_pDSSpotlightReflector);
		m_pDSSpotlightReflector = NULL;

		m_pSpotCaustics = NULL;
		m_pSpotCausticTex = NULL;
		m_pSpotCausticsBuffer = NULL;
		m_pSpotCausticsFormat = NULL;

		memset(m_pDSLightMaterials[DLT_OMNIDIRECTIONAL], 0, sizeof(m_pDSLightMaterials[DLT_OMNIDIRECTIONAL]));
		memset(m_pDSLightMaterials[DLT_SPOT], 0, sizeof(m_pDSLightMaterials[DLT_SPOT]));
		memset(m_pDSLightMaterials[DLT_SUN], 0, sizeof(m_pDSLightMaterials[DLT_SUN]));
	}

	if(m_bShadowsInit)
	{
		for(int i = 0; i < 6; i++)
			g_pShaderAPI->FreeTexture( m_pShadowMapDepth[i] );

		g_pShaderAPI->FreeTexture( m_pShadowMaps[DLT_OMNIDIRECTIONAL][0] );
		g_pShaderAPI->FreeTexture( m_pShadowMaps[DLT_SPOT][0] );
		g_pShaderAPI->FreeTexture( m_pShadowMaps[DLT_SUN][0] );
		g_pShaderAPI->FreeTexture( m_pShadowMaps[DLT_SUN][1] );

		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][0] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][1] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][2] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_SPOT][0] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_SPOT][1] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_SPOT][2] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_SUN][0] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_SUN][1] );
		materials->FreeMaterial( m_pShadowmapDepthwrite[DLT_SUN][2] );

		memset( m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL], 0, sizeof(m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL]) );
		memset( m_pShadowmapDepthwrite[DLT_SPOT], 0, sizeof(m_pShadowmapDepthwrite[DLT_SPOT]) );
		memset( m_pShadowmapDepthwrite[DLT_SUN], 0, sizeof(m_pShadowmapDepthwrite[DLT_SUN]) );

		memset( m_pShadowMaps[DLT_OMNIDIRECTIONAL], 0, sizeof(m_pShadowMaps[DLT_OMNIDIRECTIONAL]) );
		memset( m_pShadowMaps[DLT_SPOT], 0, sizeof(m_pShadowMaps[DLT_SPOT]) );
		memset( m_pShadowMaps[DLT_SUN], 0, sizeof(m_pShadowMaps[DLT_SUN]) );

		memset(m_pShadowMapDepth,0,sizeof(m_pShadowMapDepth));

		m_bShadowsInit = false;
	}

	for(int i = 0; i < m_numLights; i++)
		PPFree( m_pLights[i] );

	memset(m_pLights, 0, sizeof(m_pLights));
	m_numLights = 0;
}

// sets view
void CBaseViewRenderer::SetView(CViewParams* pParams)
{
	m_pCurrentView = pParams;
}

CViewParams* CBaseViewRenderer::GetView()
{
	return m_pCurrentView;
}

// sets scene info
void CBaseViewRenderer::SetSceneInfo(sceneinfo_t &info)
{
	m_sceneinfo = info;
}

// returns scene info
sceneinfo_t& CBaseViewRenderer::GetSceneInfo()
{
	return m_sceneinfo;
}

extern ConVar r_wireframe;

// -------------------------------------------

void CBaseViewRenderer::SetModelInstanceChanged()
{
	m_bCurrentSkinningPrepared = false;
}

// draws render list using the current view, also you can specify the render target
void CBaseViewRenderer::DrawRenderList(IRenderList* pRenderList, int nRenderFlags)
{
	/*
	if(pRenderTarget)
	{
		viewSizeX = pRenderTarget->GetWidth();
		viewSizeY = pRenderTarget->GetHeight();
	}
	*/

	materials->SetCullMode(CULL_BACK);

	if(!(nRenderFlags & VR_FLAG_CUBEMAP))
		BuildViewMatrices(m_nViewportW, m_nViewportH, nRenderFlags);

#ifdef USE_SINGLE_CUBEMAPRENDER
	int nCubePass = 0;
redraw:
#endif // USE_SINGLE_CUBEMAPRENDER

	if( nRenderFlags & VR_FLAG_CUBEMAP )
	{
#ifdef USE_SINGLE_CUBEMAPRENDER
		SetCubemapIndex(nCubePass);
#endif // USE_SINGLE_CUBEMAPRENDER
		BuildViewMatrices(m_nViewportW, m_nViewportH, nRenderFlags);

		if( m_nRenderMode == VDM_SHADOW )
			UpdateDepthSetup( true );
	}

	materials->SetSkinningEnabled( false );

	pRenderList->Render( nRenderFlags, NULL );

#ifdef USE_SINGLE_CUBEMAPRENDER
	if((nRenderFlags & VR_FLAG_CUBEMAP) && nCubePass < 5)
	{
		nCubePass++;
		goto redraw;
	}

	// reset
	SetCubemapIndex(0);
#endif // USE_SINGLE_CUBEMAPRENDER
}

#ifdef EQLC
void CViewRenderer::SetLightmapTextureIndex(int nLMIndex)
{
	m_nLightmapID = nLMIndex;
}
#endif // EQLC

extern ConVar r_force_softwareskinning;

// draws model with parameters
void CBaseViewRenderer::DrawModelPart(IEqModel*	pModel, int nModel, int nGroup, int nRenderFlags, slight_t* pStaticLights, Matrix4x4* pBones)
{
	IMaterial* pMaterial = pModel->GetMaterial(nModel, nGroup);

	if((nRenderFlags & VR_FLAG_NO_TRANSLUCENT) && (pMaterial->GetShader()->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		return;

	if((nRenderFlags & VR_FLAG_NO_OPAQUE) && !(pMaterial->GetShader()->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		return;

	if( !pBones || r_force_softwareskinning.GetBool() )
		materials->SetSkinningEnabled( false );

	if(!(nRenderFlags & VR_FLAG_NO_MATERIALS))
	{
		materials->BindMaterial(pMaterial, false);
	}
	else
	{
		if(m_nRenderMode == VDM_SHADOW)
		{
			UpdateDepthSetup( false, materials->IsSkinningEnabled() ? VR_SKIN_EGFMODEL : VR_SKIN_NONE);

#ifdef EQLC
			pMaterial->GetShader()->InitParams();
#endif

			float fAlphatestFac = 0.0f;

			if(pMaterial->GetFlags() & MATERIAL_FLAG_ALPHATESTED)
				fAlphatestFac = 50.0f;

			g_pShaderAPI->SetShaderConstantFloat("AlphaTestFactor", fAlphatestFac);
			g_pShaderAPI->SetTexture(pMaterial->GetShader()->GetBaseTexture(0));
			g_pShaderAPI->ApplyTextures();
		}
	}

	// do skinning
	if( materials->IsSkinningEnabled() || r_force_softwareskinning.GetBool() )
	{
		if(!m_bCurrentSkinningPrepared || !r_force_softwareskinning.GetBool())
			pModel->PrepareForSkinning( pBones );

		m_bCurrentSkinningPrepared = true;
	}

	if(!(nRenderFlags & VR_FLAG_NO_MATERIALS))
	{
		slight_t* pSetupLights = pStaticLights;

		if(!pSetupLights)
			pSetupLights = s_default_no_lights;

		// model lighting
		g_pShaderAPI->SetShaderConstantArrayVector4D("ModelLights", (Vector4D*)&pSetupLights[0].origin_radius, 16);
	}

	pModel->DrawGroup( nModel, nGroup );

	materials->SetSkinningEnabled( false );
}

// updates material
void CBaseViewRenderer::UpdateDepthSetup(bool bUpdateRT, VRSkinType_e nSkin)
{
	if(!materials->GetLight())
	{
#ifdef USE_SINGLE_CUBEMAPRENDER
		UpdateRendertargetSetup();
#endif
		return;
	}

	// attach shadowmap buffers to shader
	int nLightType = materials->GetLight()->nType;

	// reattach shadowmap render target
	if(bUpdateRT)
	{
#ifdef USE_SINGLE_CUBEMAPRENDER
		if(nLightType == DLT_OMNIDIRECTIONAL)
			g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][0], m_nCubeFaceId, m_pShadowMapDepth[m_nCubeFaceId]);
#else
		//g_pShaderAPI->ChangeRenderTarget( m_pShadowMaps[nLightType][0], m_nCubeFaceId, m_pShadowMapDepth[0] );
#endif // USE_SINGLE_CUBEMAPRENDER
	}
	else
	{
		// bind depth material, without apply operation
		materials->BindMaterial( m_pShadowmapDepthwrite[nLightType][nSkin], true );
	}
}

//------------------------------------------------------------------------------------------

void CBaseViewRenderer::UpdateRendertargetSetup()
{
	if(m_numMRTs > 0)
	{
		g_pShaderAPI->ChangeRenderTargetToBackBuffer();
		g_pShaderAPI->ChangeRenderTargets( m_pTargets, m_numMRTs, m_nCubeFaces, m_pDepthTarget );
	}
	else
	{
		g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	}
}

// sets the rendertargets
void CBaseViewRenderer::SetupRenderTargets(int nNumRTs, ITexture** ppRenderTargets, ITexture* pDepthTarget, int* nCubeFaces)
{
	//memcpy(m_pTargets,ppRenderTargets,sizeof(ITexture*)*nNumRTs);
	for(int i = 0; i < nNumRTs; i++)
		m_pTargets[i] = ppRenderTargets[i];

	m_numMRTs = nNumRTs;
	m_pDepthTarget = pDepthTarget;

	if(!nCubeFaces)
	{
		int target_ids[MAX_MRTS] = {0,0,0,0,0,0,0,0};
		memcpy(m_nCubeFaces, target_ids,sizeof(int)*nNumRTs);
	}
	else
	{
		memcpy(m_nCubeFaces, nCubeFaces,sizeof(int)*nNumRTs);
	}

	UpdateRendertargetSetup();
}

// sets rendertargets to backbuffers
void CBaseViewRenderer::SetupRenderTargetToBackbuffer()
{
	m_numMRTs = 0;
	UpdateRendertargetSetup();
}

// returns the current set rendertargets
void CBaseViewRenderer::GetCurrentRenderTargets(int* nNumRTs, ITexture** ppRenderTargets, 
											ITexture** ppDepthTarget, 
											int* nCubeFaces)
{
	for(int i = 0; i < m_numMRTs; i++)
		ppRenderTargets[i] = m_pTargets[i];

	*ppDepthTarget = m_pDepthTarget;

	*nNumRTs = m_numMRTs;

	memcpy(nCubeFaces, m_nCubeFaces,sizeof(int)*m_numMRTs);
}

//------------------------------------------------------------------------------------------

// allocates new light
dlight_t* CBaseViewRenderer::AllocLight()
{
	dlight_t* pLight = (dlight_t*)PPAlloc( sizeof(dlight_t) );
	SetLightDefaults(pLight);

	// always need white texture
	pLight->pMaskTexture = materials->GetWhiteTexture();

	return pLight;
}

// adds light to current frame
void CBaseViewRenderer::AddLight(dlight_t* pLight)
{
	if(materials->GetLightingModel() == MATERIAL_LIGHT_UNLIT)
	{
		PPFree( pLight );
		return;
	}

	if(m_numLights == MAX_VISIBLE_LIGHTS)
	{
		PPFree( pLight );
		return;
	}

	/*
	bool isMisc = (light->nFlags & LFLAG_MISCLIGHT) > 0;
	bool isRainMap = (light->nFlags & LFLAG_RAINMAP) > 0;

	if(!r_pointlighting.GetBool() && light->fov == -1 && !(light->nFlags & LFLAG_ISSUN) ||
		(light->nFlags & LFLAG_ISSUN) && !r_sunlighting.GetBool() ||
		!r_spotlighting.GetBool() && light->fov != -1 && !(light->nFlags & LFLAG_ISSUN))
	{
		delete light;
		return;
	}

	// misc lights is not allowed for forward shading due low perfomance
	if(isMisc && (materials->GetLightingModel() != MATERIAL_LIGHT_DEFERRED || !r_misclights.GetBool()) || isRainMap && !r_shadows->GetBool())
	{
		delete light;
		return;
	}

	if(light->radius3 == Vector3D(1))
		light->radius3 = Vector3D(abs(light->radius));
	else
		light->radius = abs(length(light->radius3));
		*/

	m_pLights[m_numLights] = pLight;
	m_numLights++;
}

// removes light
void CBaseViewRenderer::RemoveLight(int idx)
{
	if ( idx >= m_numLights || idx < 0 )
		return;

	dlight_t *pLight = m_pLights[idx];

	m_numLights--;

	if ( m_numLights > 0 && idx != m_numLights )
	{
		m_pLights[idx] = m_pLights[m_numLights];
		m_pLights[m_numLights] = NULL;
	}

	PPFree(pLight);
}

// removes light
void CBaseViewRenderer::UpdateLights(float fDt)
{
	for(int i = 0; i < m_numLights; i++)
	{
		if(m_pLights[i]->dietime < 0)
		{
			RemoveLight(i);
			i--;
		}
		else
		{
			if(m_pLights[i]->curfadetime < 0)
				m_pLights[i]->curfadetime = m_pLights[i]->fadetime;

			if(m_pLights[i]->curintensity < 0)
				m_pLights[i]->curintensity = m_pLights[i]->intensity;

			m_pLights[i]->dietime -= fDt;
			m_pLights[i]->curfadetime -= fDt;

			if(m_pLights[i]->curfadetime > 0)
				m_pLights[i]->curintensity = m_pLights[i]->intensity * (m_pLights[i]->curfadetime / m_pLights[i]->fadetime);
		}
	}
}

#define VERTS_SUBDIVS 4
#define VERTS_SUBDIVS_COUNT ((VERTS_SUBDIVS+1)*2*(VERTS_SUBDIVS+1)*2)

void GetViewspaceMesh(Vertex2D_t* verts)
{
	const int			nScreenVerts = (VERTS_SUBDIVS+1)*(VERTS_SUBDIVS+1);
	static Vertex2D_t	pVerts[nScreenVerts];

	// compute vertices
	for(int c = 0; c < (VERTS_SUBDIVS+1); c++)
	{
		for(int r = 0; r < (VERTS_SUBDIVS+1); r++)
		{
			int vertex_id = r + c*(VERTS_SUBDIVS+1);

			float tc_x = (float)(r)/(float)VERTS_SUBDIVS;
			float tc_y = (float)(c)/(float)VERTS_SUBDIVS;

			float v_pos_x = tc_x;
			float v_pos_y = tc_y;

			Vector2D texCoord(tc_x,tc_y);

			pVerts[vertex_id] = Vertex2D_t(Vector2D(v_pos_x,v_pos_y), texCoord);
		}
	}

	int nTriangle = 0;

	for(int c = 0; c < VERTS_SUBDIVS; c++)
	{
		for(int r = 0; r < VERTS_SUBDIVS; r++)
		{
			int index1 = r + c*(VERTS_SUBDIVS+1);
			int index2 = r + (c+1)*(VERTS_SUBDIVS+1);
			int index3 = (r+1) + c*(VERTS_SUBDIVS+1);
			int index4 = (r+1) + (c+1)*(VERTS_SUBDIVS+1);

			verts[nTriangle*3] = pVerts[index1];
			verts[nTriangle*3+1] = pVerts[index2];
			verts[nTriangle*3+2] = pVerts[index3];

			nTriangle++;

			verts[nTriangle*3] = pVerts[index3];
			verts[nTriangle*3+1] = pVerts[index2];
			verts[nTriangle*3+2] = pVerts[index4];

			nTriangle++;
		}
	}
}

Vertex2D_t* CBaseViewRenderer::GetViewSpaceMesh(int* size)
{
	static Vertex2D_t* quad_sub = NULL;

	if(quad_sub == NULL)
	{
		// alloc in automatic memory remover
		quad_sub = PPAllocStructArray(Vertex2D_t, VERTS_SUBDIVS_COUNT);

		GetViewspaceMesh( quad_sub );
	}

	if(size)
		*size = VERTS_SUBDIVS_COUNT;

	return quad_sub;
}

// draws deferred ambient
void CBaseViewRenderer::DrawDeferredAmbient()
{
	// restore view
	//g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	UpdateRendertargetSetup();

	// setup 3d first
	BuildViewMatrices(m_nViewportW, m_nViewportH, 0);

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	materials->BindMaterial(m_pDSAmbient, false);

	// setup 2D here
	materials->Setup2D(m_nViewportW, m_nViewportH);

	Matrix4x4 wvp_matrix;
	materials->GetWorldViewProjection(wvp_matrix);

	Matrix4x4 worldtransform = identity4();
	materials->GetMatrix(MATRIXMODE_WORLD, worldtransform);

	g_pShaderAPI->SetShaderConstantMatrix4("MVP", wvp_matrix);
	g_pShaderAPI->SetShaderConstantMatrix4("World", worldtransform);

	g_pShaderAPI->Apply();

	Vertex2D_t* amb_quad_sub = GetViewSpaceMesh(NULL);

	IMeshBuilder* pMeshBuilder = g_pShaderAPI->CreateMeshBuilder();

	float wTexel = 1.0f;//1.0f / float(m_nViewportW);
	float hTexel = 1.0f;//1.0f / float(m_nViewportH);

	pMeshBuilder->Begin(PRIM_TRIANGLES);
		for(int i = 0; i < VERTS_SUBDIVS_COUNT; i++)
		{
			pMeshBuilder->Position3fv(Vector3D((amb_quad_sub[i].m_vPosition * Vector2D((float)m_nViewportW+wTexel,(float)m_nViewportH+hTexel) - Vector2D(wTexel, hTexel)),0));
			pMeshBuilder->TexCoord2fv( amb_quad_sub[i].m_vTexCoord );
			pMeshBuilder->AdvanceVertex();
		}
	pMeshBuilder->End();

	g_pShaderAPI->DestroyMeshBuilder(pMeshBuilder);
}

ConVar r_ds_debug("r_ds_debug", "-1", "Debug deferred shading target", CV_CHEAT);
ConVar r_debug_caustics("r_debug_caustics", "0", "Debug caustics texture", CV_CHEAT);

ITexture* g_pDebugTexture = NULL;

void OnShowTextureChanged(ConVar* pVar,char const* pszOldValue)
{
	if(g_pDebugTexture)
		g_pShaderAPI->FreeTexture(g_pDebugTexture);

	g_pDebugTexture = g_pShaderAPI->FindTexture( pVar->GetString() );

	if(g_pDebugTexture)
		g_pDebugTexture->Ref_Grab();
}

ConVar r_showTexture("r_showTexture", "", OnShowTextureChanged, "if r_debugShowTexture enabled, it shows the selected texture in overlay", CV_CHEAT);

// draws debug data
void CBaseViewRenderer::DrawDebug()
{
	if(r_ds_debug.GetInt() != -1)
	{
		g_pShaderAPI->ChangeRenderTargetToBackBuffer();

		// setup 2d
		float w,h;

#ifndef EDITOR
#ifndef EQLC
		w = g_pEngineHost->GetWindowSize().x;
		h = g_pEngineHost->GetWindowSize().y;
#else
		w = g_lightmapSize;
		h = g_lightmapSize;
#endif
		materials->Setup2D(w, h);
#else
		int wi,hi;
		g_editormainframe->GetMaxRenderWindowSize( wi, hi );

		w = wi;
		h = hi;

		materials->Setup2D(w, h);
#endif

		Vertex2D_t gbuf_view[] = { MAKETEXQUAD(0, 0, w, h, 0) };

		materials->SetDepthStates(false,false);
		materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, gbuf_view, elementsOf(gbuf_view),m_pGBufferTextures[r_ds_debug.GetInt()], color4_white);//, &blend);
	}

#if !defined(EDITOR) && !defined(EQLC)
	if(r_debug_caustics.GetBool())
	{
		materials->Setup2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y);
	
		Vertex2D_t light_depth[] = { MAKETEXQUAD(0, 0, 512, 512, 0) };
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, light_depth, elementsOf(light_depth),m_pSpotCausticTex);
	}

	// more universal thing
	if( g_pDebugTexture )
	{
		materials->Setup2D( g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y );

		int w, h;
		w = g_pDebugTexture->GetWidth();
		h = g_pDebugTexture->GetHeight();

		if(h > g_pEngineHost->GetWindowSize().y)
		{
			float fac = g_pEngineHost->GetWindowSize().y/float(h);

			w *= fac;
			h *= fac;
		}
	
		Vertex2D_t light_depth[] = { MAKETEXQUAD(0, 0, w, h, 0) };
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, light_depth, elementsOf(light_depth),g_pDebugTexture);
	}

#endif // EDITOR
}

ConVar r_skipdslighting("r_skipdslighting", "0", "Skips deferred lighitng", CV_CHEAT);

// draws deferred lighting
void CBaseViewRenderer::DrawDeferredCurrentLighting(bool bShadowLight)
{
	if(r_skipdslighting.GetBool())
		return;

	if(materials->GetLight() == NULL)
		return;

	// first, do this
	UpdateRendertargetSetup();

	BuildViewMatrices(m_nViewportW, m_nViewportH, 0);

	// setup material by current light and draw something
	// (sphere or spot)

	int nLightType = materials->GetLight()->nType;

	Matrix4x4 lightSphereWorldMat;
	
	if(nLightType == DLT_OMNIDIRECTIONAL)
	{
		lightSphereWorldMat = translate(materials->GetLight()->position)*scale(materials->GetLight()->radius.x,materials->GetLight()->radius.y,materials->GetLight()->radius.z);
	}
	else if (nLightType == DLT_SPOT)
	{
		float fFovScale = (1.0f - (cos(DEG2RAD(materials->GetLight()->fovWH.z))*sin(DEG2RAD(materials->GetLight()->fovWH.z))))*materials->GetLight()->radius.x*6;
		lightSphereWorldMat = translate(materials->GetLight()->position)*Matrix4x4(materials->GetLight()->lightRotation)*scale(fFovScale,fFovScale,materials->GetLight()->radius.x*2);

		if( materials->GetLight()->nFlags & LFLAG_SIMULATE_REFLECTOR )
		{
			materials->Setup2D(m_nViewportW, m_nViewportH);

			// set the render target
			g_pShaderAPI->ChangeRenderTargetToBackBuffer();
			g_pShaderAPI->ChangeRenderTarget(m_pSpotCausticTex, 0, NULL);

			g_pShaderAPI->Clear( true,false,false );

			// draw caustics firts
			materials->BindMaterial(m_pSpotCaustics, false);

			g_pShaderAPI->SetVertexFormat( m_pSpotCausticsFormat );
			g_pShaderAPI->SetVertexBuffer( m_pSpotCausticsBuffer, 0 );

			materials->SetMatrix(MATRIXMODE_WORLD, lightSphereWorldMat);

			// apply current shadowmap as s5, s6 for sun lod
			g_pShaderAPI->SetTexture(m_pShadowMaps[nLightType][0], "ShadowMapSampler",  VERTEX_TEXTURE_INDEX(1));

			materials->Apply();

			int NUM_PHOTONS = (r_photonCount.GetInt()*r_photonCount.GetInt());

			g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_POINTS, 0, NUM_PHOTONS);

			g_pShaderAPI->Reset();

			// reset RT setup
			UpdateRendertargetSetup();
		}
	}
	else if (nLightType == DLT_SUN)
	{
		// setup 2d
		g_pShaderAPI->Reset(STATE_RESET_VBO);

		materials->SetCullMode(CULL_FRONT);

		materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
		materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);

		materials->BindMaterial(m_pDSLightMaterials[nLightType][bShadowLight], false);

		if(materials->GetBoundMaterial()->GetShader())
			materials->GetBoundMaterial()->GetShader()->SetupConstants();

		materials->Setup2D(m_nViewportW, m_nViewportH);

		Matrix4x4 wvp_matrix;
		materials->GetWorldViewProjection(wvp_matrix);

		Matrix4x4 worldtransform = identity4();
		materials->GetMatrix(MATRIXMODE_WORLD, worldtransform);


		g_pShaderAPI->SetShaderConstantMatrix4("WVP", wvp_matrix);
		g_pShaderAPI->SetShaderConstantMatrix4("World", worldtransform);
		//g_pShaderAPI->SetVertexShaderConstantMatrix4(SHADERCONST_WORLDVIEWPROJ, wvp_matrix);
		//g_pShaderAPI->SetVertexShaderConstantMatrix4(SHADERCONST_WORLDTRANSFORM, worldtransform);

		// apply current shadowmap as s5, s6 for sun lod
		g_pShaderAPI->SetTexture(m_pShadowMaps[nLightType][0], "ShadowMapSampler1", 5);
		g_pShaderAPI->SetTexture(m_pShadowMaps[nLightType][1], "ShadowMapSampler2", 6);

		g_pShaderAPI->Apply();

		Vertex2D_t* amb_quad_sub = GetViewSpaceMesh(NULL);

		IMeshBuilder* pMeshBuilder = g_pShaderAPI->CreateMeshBuilder();

		float wTexel = 1.0f;//1.0f / float(m_nViewportW);
		float hTexel = 1.0f;//1.0f / float(m_nViewportH);

		pMeshBuilder->Begin(PRIM_TRIANGLES);
			for(int i = 0; i < VERTS_SUBDIVS_COUNT; i++)
			{
				pMeshBuilder->Position3fv(Vector3D((amb_quad_sub[i].m_vPosition * Vector2D((float)m_nViewportW+wTexel,(float)m_nViewportH+hTexel) - Vector2D(wTexel, hTexel)),0));
				pMeshBuilder->TexCoord2fv( amb_quad_sub[i].m_vTexCoord );
				pMeshBuilder->AdvanceVertex();
			}
		pMeshBuilder->End();

		g_pShaderAPI->DestroyMeshBuilder(pMeshBuilder);
		/*
		materials->Setup2D(m_nViewportW, m_nViewportH);
		Vertex2D_t tmprect[] = { MAKETEXQUAD(0, 0,(float) 512, (float)512, 0) };
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), m_pShadowMaps[nLightType][0]);
		*/
		return;
	}

	materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
	materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);

	materials->SetCullMode(CULL_FRONT);

	// check light inside
	if(viewrenderer->GetView() && nLightType <= DLT_SPOT)
	{
		Vector3D lVec = viewrenderer->GetView()->GetOrigin() - materials->GetLight()->position;
		float fRad = dot(lVec, lVec);
		float fLightRad = materials->GetLight()->radius.x;

		if(fRad > fLightRad*fLightRad)
		{
			if(nLightType == DLT_OMNIDIRECTIONAL)
			{
				materials->SetCullMode(CULL_BACK);
				materials->SetDepthStates(true, false);
			}
			else
				materials->SetDepthStates(false, false);
		}
		else
		{
			materials->SetDepthStates(false, false);
		}
	}

	materials->SetMatrix(MATRIXMODE_WORLD, lightSphereWorldMat);

	if (nLightType == DLT_SPOT && r_shadows.GetBool())
	{
		if( materials->GetLight()->nFlags & LFLAG_SIMULATE_REFLECTOR )
		{
			materials->BindMaterial(m_pDSSpotlightReflector, false);
			g_pShaderAPI->SetTexture(m_pSpotCausticTex, "FlashlightReflector", 7);
		}
		else
			materials->BindMaterial(m_pDSLightMaterials[nLightType][bShadowLight], false);
	}
	else
		materials->BindMaterial(m_pDSLightMaterials[nLightType][bShadowLight], false);

	// apply current shadowmap as s5, s6 for sun lod
	g_pShaderAPI->SetTexture(m_pShadowMaps[nLightType][0], "ShadowMapSampler",  5);
	g_pShaderAPI->SetTexture(m_pShadowMaps[nLightType][1], "ShadowMapSampler2", 6);

	m_pDSlightModel->DrawGroup( 0, 0 );

	//m_pDSlightModel->DrawGroup( nLightType, 0 );
}

// backbuffer viewport size
void CBaseViewRenderer::SetViewportSize(int width, int height, bool bSetDeviceViewport)
{
	// store viewport
	m_nViewportW = width;
	m_nViewportH = height;

	if(bSetDeviceViewport)
		g_pShaderAPI->SetViewport(0, 0, m_nViewportW, m_nViewportH);
}

// backbuffer viewport size
void CBaseViewRenderer::GetViewportSize(int &width, int &height)
{
	width = m_nViewportW;
	height = m_nViewportH;
}

ConVar r_ortho("r_ortho", "0", "Orthogonal mode", CV_CHEAT);
ConVar r_ortho_size("r_ortho_size", "1024", "Orthogonal size", CV_CHEAT);

void CBaseViewRenderer::BuildViewMatrices(int width, int height, int nRenderFlags)
{
	if(m_pCurrentView)
	{
		Vector3D vRadianRotation			= VDEG2RAD(m_pCurrentView->GetAngles());

		// this is negated matrix
		Matrix4x4 viewMatrix;
		
		if(nRenderFlags & VR_FLAG_CUBEMAP)
		{
			m_matrices[MATRIXMODE_PROJECTION] = cubeProjectionMatrixD3D(m_sceneinfo.m_fZNear, m_sceneinfo.m_fZFar);
			viewMatrix = cubeViewMatrix( m_nCubeFaceId );
		}
		else
		{
#ifdef EDITOR
			m_matrices[MATRIXMODE_PROJECTION]	= perspectiveMatrixY(DEG2RAD(65), width, height, m_sceneinfo.m_fZNear, m_sceneinfo.m_fZFar);
#else
			m_matrices[MATRIXMODE_PROJECTION]	= perspectiveMatrixY(DEG2RAD(m_pCurrentView->GetFOV()), width, height, m_sceneinfo.m_fZNear, m_sceneinfo.m_fZFar);
#endif
			if(r_ortho.GetBool())
				m_matrices[MATRIXMODE_PROJECTION]	= orthoMatrix(-r_ortho_size.GetFloat(), r_ortho_size.GetFloat(), -r_ortho_size.GetFloat(), r_ortho_size.GetFloat(), m_sceneinfo.m_fZNear, m_sceneinfo.m_fZFar);

			viewMatrix = rotateZXY(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);
		}

		viewMatrix.translate(-m_pCurrentView->GetOrigin());

		m_matrices[MATRIXMODE_VIEW]			= viewMatrix;
		m_matrices[MATRIXMODE_WORLD]		= identity4();

		// store the viewprojection matrix for some purposes
		m_viewprojection = m_matrices[MATRIXMODE_PROJECTION] * m_matrices[MATRIXMODE_VIEW];

		materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
		materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);

		if(materials->GetLight() && (materials->GetLight()->nType != DLT_OMNIDIRECTIONAL) && (materials->GetLight()->nFlags & LFLAG_MATRIXSET))
			m_viewprojection = materials->GetLight()->lightWVP;

		if(!(nRenderFlags & VR_FLAG_CUSTOM_FRUSTUM))
			m_frustum.LoadAsFrustum(m_viewprojection);

		FogInfo_t fog;
		materials->GetFogInfo(fog);
		fog.viewPos = m_pCurrentView->GetOrigin();
		materials->SetFogInfo(fog);
	}
	else
	{
		MsgError("NO VIEW\n");
	}
}

// sets cubemap texture index
void CBaseViewRenderer::SetCubemapIndex( int nCubeIndex )
{
	m_nCubeFaceId = nCubeIndex;
}

// returns the specified matrix
Matrix4x4 CBaseViewRenderer::GetMatrix(MatrixMode_e mode)
{
	return m_matrices[mode];
}

// returns view-projection matrix
Matrix4x4 CBaseViewRenderer::GetViewProjectionMatrix()
{
	return m_viewprojection;
}

// view frustum
Volume* CBaseViewRenderer::GetViewFrustum()
{
	return &m_frustum;
}

// sets the render mode
void CBaseViewRenderer::SetDrawMode(ViewDrawMode_e mode)
{
	m_nRenderMode = mode;

	// setup view draw mode
	if(m_nRenderMode <= VDM_AMBIENTMODULATE)
	{
		if(!m_bDSInit || (materials->GetCurrentLightingModel() == MATERIAL_LIGHT_FORWARD))
		{
			UpdateRendertargetSetup();
			return;
		}

		int nCubeFaces[] = {0,0,0,0,0,0,0};

		if(m_nRenderMode == VDM_AMBIENT)
		{
			// setup, with z pass
			g_pShaderAPI->ChangeRenderTargets(m_pGBufferTextures, GBUF_TEXTURES, nCubeFaces, NULL);

			// clear this targets
			g_pShaderAPI->Clear(true,true,false, ColorRGBA(1));
		}
		else
		{
			// apply diffuse only and don't clear
			g_pShaderAPI->ChangeRenderTargets(m_pGBufferTextures, 1, nCubeFaces, NULL);
		}

		m_nCubeFaceId = 0;
	}
	else if(m_nRenderMode == VDM_SHADOW)
	{
		// attach shadowmap buffers to shader
		int nLightType = materials->GetLight()->nType;

		// bind depth material, without apply operation
		materials->BindMaterial( m_pShadowmapDepthwrite[nLightType][0], false );

#ifdef EQLC
		SetViewportSize(2048, 2048);
#else
		SetViewportSize(512, 512);
#endif

		if(nLightType == DLT_OMNIDIRECTIONAL)
		{
			/*
			ITexture* pSingleCube[] = {
				m_pShadowMaps[nLightType][0],
				m_pShadowMaps[nLightType][0],
				m_pShadowMaps[nLightType][0],
				m_pShadowMaps[nLightType][0],
				m_pShadowMaps[nLightType][0],
				m_pShadowMaps[nLightType][0],
			};

			int nCubeIds[] = {0,1,2,3,4,5,6};

			g_pShaderAPI->ChangeRenderTargets(pSingleCube, nCubeIds, m_pShadowMapDepth[m_nCubeFaceId]);
			*/
			
#ifdef USE_SINGLE_CUBEMAPRENDER
			for(int i = 0; i < 6; i++)
			{
				// attach shadowmap texture
				g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][0], i, m_pShadowMapDepth[i]);
				// clear it
				g_pShaderAPI->Clear(true,true,false, ColorRGBA(0.0f));
			}

			// attach shadowmap texture
			g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][0], m_nCubeFaceId, m_pShadowMapDepth[m_nCubeFaceId]);
#else
			// attach shadowmap texture
			g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][0], m_nCubeFaceId, m_pShadowMapDepth[nLightType]);

			// clear it
			g_pShaderAPI->Clear(true,true,false, ColorRGBA(0.0f));
#endif // USE_SINGLE_CUBEMAPRENDER
		}
		else
		{
			// attach shadowmap texture TODO: sun index
			g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][m_nCubeFaceId], 0, m_pShadowMapDepth[nLightType]);

			ColorRGBA clearColor(0.0f);

			// clear it
			g_pShaderAPI->Clear(true,true,false, clearColor);
		}
	}
	else if(m_nRenderMode == VDM_LIGHTING)
	{
		UpdateRendertargetSetup();

		if( materials->GetLight() && m_bShadowsInit )
		{
			int nLightType = materials->GetLight()->nType;

			// apply current shadowmap as s5, s6 for sun lod
			g_pShaderAPI->SetTexture( m_pShadowMaps[nLightType][0], "ShadowMapSampler", 5 );
			g_pShaderAPI->SetTexture( m_pShadowMaps[nLightType][1], "ShadowMapSampler2", 6 );
		}
	}
}

ViewDrawMode_e CBaseViewRenderer::GetDrawMode()
{
	return m_nRenderMode;
}

// retuns light
dlight_t* CBaseViewRenderer::GetLight(int index)
{
	return m_pLights[index];
}

int	CBaseViewRenderer::GetLightCount()
{
	return m_numLights;
}

//------------------------------------------------

// returns lighting color and direction
void CBaseViewRenderer::GetLightingSampleAtPosition(const Vector3D &position, ColorRGBA &color, Vector3D &dir)
{
	// enumerate visible light sources (from list)
}


Vector3D g_check_pos = vec3_zero; // HACK: comparsion is global-level

int comp_eqlevellight_dist(const void* l1,const void* l2)
{
	eqlevellight_t* light1 = (eqlevellight_t*)l1;
	eqlevellight_t* light2 = (eqlevellight_t*)l2;

	Vector3D lvec1 = light1->origin - g_check_pos;
	Vector3D lvec2 = light2->origin - g_check_pos;

	float intensity1 = dot(light1->color, light1->color);
	float intensity2 = dot(light2->color, light2->color);

	float dist1 = dot(lvec1, lvec1);
	float dist2 = dot(lvec2, lvec2);

	return (dist1*intensity2 - dist2*intensity1);
}

#ifndef EDITOR
#include "idkphysics.h"
#endif // EDITOR

// fills array with eight nearest lights to point
void CBaseViewRenderer::GetNearestEightLightsForPoint(const Vector3D &position, slight_t *pLights)
{
	memcpy(pLights, s_default_no_lights, sizeof(slight_t)*8);
}