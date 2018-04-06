//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: View renderer for game (also includes editor renderer)
//
// TODO: DX9 and DX10 view renderer separation
//////////////////////////////////////////////////////////////////////////////////

#include "ViewRenderer.h"
#include "BaseRenderableObject.h"
#include "materialsystem/MeshBuilder.h"

#ifndef EDITOR
#include "EqGameLevel.h"
#include "idkphysics.h"
#	ifndef EQLC
#	include "IEngineHost.h"
#	endif // EQLC
#else
#include "../Editor/EditorMainFrame.h"
#include "../Editor/RenderWindows.h"
#endif // EDITOR

#ifdef EQLC
//#define USE_SINGLE_CUBEMAPRENDER
#include "../utils/eqlc/eqlc.h"
#else

// we're using DirectX 10
// #define USE_SINGLE_CUBEMAPRENDER

#include "IDebugOverlay.h"
#endif

#include "studio_egf.h"

// declare the viewrenderer as the global
#ifndef EDITOR
static CViewRenderer g_viewrenderer;
IViewRenderer* viewrenderer = &g_viewrenderer;
#else
IViewRenderer* viewrenderer = NULL;
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

	bool bIsChanged = materials->GetConfiguration().lighting_model != newModel;

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
ConVar r_showPortals("r_showPortals", "0", "Shows visible portals", CV_ARCHIVE);

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

CViewRenderer::CViewRenderer()
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

	m_pOcclusionBuffer = NULL;

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
void CViewRenderer::InitializeResources()
{
	if(!g_pShaderAPI)
		return;

	// begin preloading zone - critical cache section
	materials->BeginPreloadMarker();

#ifndef EDITOR
	m_pActiveAreaList = NULL;
#endif // EDITOR

	m_numViewRooms = 0;
	m_viewRooms[0] = -1;
	m_viewRooms[1] = -1;

	//g_nVRGraphBucket = debugoverlay->Graph_AddBucket("VR matsys bind", ColorRGBA(0,1,0,1), 1.0f, 0.01f);

	// initialize occlusion buffer
	m_pOcclusionBuffer = (float*)malloc(r_occlusionBuffersize.GetInt()*r_occlusionBuffersize.GetInt() * sizeof(float));

	m_nOcclusionBufferSize = r_occlusionBuffersize.GetInt();

	m_OcclusionRasterizer.SetImage(m_pOcclusionBuffer, m_nOcclusionBufferSize, m_nOcclusionBufferSize);
	m_OcclusionRasterizer.SetEnableDepthMode(true);
	m_OcclusionRasterizer.SetAdditiveColor(false);
	m_OcclusionRasterizer.Clear(1.0f); // clear to white

	if(materials->GetConfiguration().lighting_model == MATERIAL_LIGHT_DEFERRED && !m_bDSInit)
	{
		int nDSLightModel = g_studioModelCache->PrecacheModel("models/sphere_1x1.egf");

		if(nDSLightModel == -1)
		{
			CrashMsg("Deferred Shading is not initialized, can't load model \"models/sphere_1x1.egf\"");
			exit(-1);
		}

		m_pDSlightModel = g_studioModelCache->GetModel(nDSLightModel);

		if(!m_pDSlightModel)
		{
			CrashMsg("Deferred Shading initialization error\n");
			exit(-1);
		}

		Msg("Initializing DS targets...\n");

		int screenWide = 0;
		int screenTall = 0;

#ifndef EDITOR

#ifndef EQLC
		screenWide = g_pEngineHost->GetWindowSize().x;
		screenTall = g_pEngineHost->GetWindowSize().y;

		screenWide *= r_ds_gBufferSizeScale.GetFloat();
		screenTall *= r_ds_gBufferSizeScale.GetFloat();
#else
		screenWide = g_lightmapSize;
		screenWide = g_lightmapSize;
#endif

#else
		//g_editormainframe->GetMaxRenderWindowSize(screenWide, screenTall);
		screenWide = m_nViewportW;
		screenTall = m_nViewportH;
#endif

		int nDSBuffersSize = 0;

		ETextureFormat gbuf_diffuse_fmt		= FORMAT_RGBA8;
		ETextureFormat gbuf_normal_fmt		= FORMAT_RGBA8;
		ETextureFormat gbuf_depth_fmt		= FORMAT_RG32F;
		ETextureFormat gbuf_material1_fmt	= FORMAT_RGBA8;

		// TODO: also check HDR enabled

		if(g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D10)
		{
			// TODO: get supported format by g_pShaderAPI->CheckFormat()
			gbuf_normal_fmt = FORMAT_RGBA16F;
		}

		// Init GBuffer textures
		m_pGBufferTextures[GBUF_DIFFUSE] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_diff", screenWide, screenTall, gbuf_diffuse_fmt, TEXFILTER_NEAREST, TEXADDRESS_CLAMP);
		m_pGBufferTextures[GBUF_DIFFUSE]->Ref_Grab();

		m_pGBufferTextures[GBUF_NORMALS] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_norm", screenWide, screenTall, gbuf_normal_fmt, TEXFILTER_NEAREST, TEXADDRESS_CLAMP);
		m_pGBufferTextures[GBUF_NORMALS]->Ref_Grab();

		m_pGBufferTextures[GBUF_DEPTH] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_depth", screenWide, screenTall, gbuf_depth_fmt, TEXFILTER_NEAREST, TEXADDRESS_CLAMP);
		m_pGBufferTextures[GBUF_DEPTH]->Ref_Grab();

		m_pGBufferTextures[GBUF_MATERIALMAP1] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_mat1", screenWide, screenTall, gbuf_material1_fmt, TEXFILTER_NEAREST, TEXADDRESS_CLAMP);
		m_pGBufferTextures[GBUF_MATERIALMAP1]->Ref_Grab();

		nDSBuffersSize += screenWide*screenTall * GetBytesPerPixel(gbuf_diffuse_fmt);
		nDSBuffersSize += screenWide*screenTall * GetBytesPerPixel(gbuf_normal_fmt);
		nDSBuffersSize += screenWide*screenTall * GetBytesPerPixel(gbuf_depth_fmt);
		nDSBuffersSize += screenWide*screenTall * GetBytesPerPixel(gbuf_material1_fmt);

		float fSize = ((float)nDSBuffersSize / 1024.0f);

		MsgWarning("GBUFFER usage is %g MB\n", (fSize / 1024.0f));

		//--------------------------------------------------------------------------------------
		// The caustics and flashlight reflector simulator
		//--------------------------------------------------------------------------------------

		// create texture first and then material
		m_pSpotCausticTex = g_pShaderAPI->CreateNamedRenderTarget("_rt_caustics", r_photonBufferSize.GetInt(), r_photonBufferSize.GetInt(), FORMAT_RGBA8, TEXFILTER_LINEAR, TEXADDRESS_CLAMP);
		m_pSpotCausticTex->Ref_Grab();

		// TODO: needs to initialize m_pCausticsGBuffer

		m_pSpotCaustics = materials->GetMaterial("engine/flashlightreflector");
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
			{ 0, 2, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position
		};

		m_pSpotCausticsFormat = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));

		//--------------------------------------------------------------------------------------

		// THIS IS ONLY DX10
		//m_pGBufferTextures[GBUF_MATERIALMAP2] = g_pShaderAPI->CreateNamedRenderTarget("_rt_gbuf_mat2", screenWide, screenTall, FORMAT_RGBA8, TEXFILTER_NEAREST, TEXADDRESS_WRAP);

		Msg("Initializing DS materials...\n");

		m_pDSAmbient = materials->GetMaterial("engine/deferred/ds_ambient");
		m_pDSAmbient->Ref_Grab();

		// Init lighting materials
		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][0] = materials->GetMaterial("engine/deferred/ds_pointlight");

		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][0]->Ref_Grab();

		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][1] = materials->GetMaterial("engine/deferred/ds_pointlight_shadow");

		m_pDSLightMaterials[DLT_OMNIDIRECTIONAL][1]->Ref_Grab();

		m_pDSLightMaterials[DLT_SPOT][0] = materials->GetMaterial("engine/deferred/ds_spotlight");
		
		m_pDSLightMaterials[DLT_SPOT][0]->Ref_Grab();

		m_pDSLightMaterials[DLT_SPOT][1] = materials->GetMaterial("engine/deferred/ds_spotlight_shadow");

		m_pDSLightMaterials[DLT_SPOT][1]->Ref_Grab();

		m_pDSSpotlightReflector = materials->GetMaterial("engine/deferred/ds_spotlight_shadow_reflector");
		m_pDSSpotlightReflector->Ref_Grab();

		m_pDSLightMaterials[DLT_SUN][0] = materials->GetMaterial("engine/deferred/ds_sunlight");
		m_pDSLightMaterials[DLT_SUN][1] = m_pDSLightMaterials[DLT_SUN][0];

		m_pDSLightMaterials[DLT_SUN][0]->Ref_Grab();
		m_pDSLightMaterials[DLT_SUN][0]->Ref_Grab();

		m_bDSInit = true;
	}

	// init shadowmapping textures and materials
	if(materials->GetConfiguration().enableShadows && (materials->GetConfiguration().lighting_model != MATERIAL_LIGHT_UNLIT) && !m_bShadowsInit)
	{
		m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][0]	= materials->GetMaterial("engine/pointdepth");
		m_pShadowmapDepthwrite[DLT_SPOT][0]				= materials->GetMaterial("engine/spotdepth");
		m_pShadowmapDepthwrite[DLT_SUN][0]				= materials->GetMaterial("engine/sundepth");

		m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][1]	= materials->GetMaterial("engine/pointdepth_skin");
		m_pShadowmapDepthwrite[DLT_SPOT][1]				= materials->GetMaterial("engine/spotdepth_skin");
		m_pShadowmapDepthwrite[DLT_SUN][1]				= materials->GetMaterial("engine/sundepth_skin");

		m_pShadowmapDepthwrite[DLT_OMNIDIRECTIONAL][2]	= materials->GetMaterial("engine/pointdepth_simple");
		m_pShadowmapDepthwrite[DLT_SPOT][2]				= materials->GetMaterial("engine/spotdepth_simple");
		m_pShadowmapDepthwrite[DLT_SUN][2]				= materials->GetMaterial("engine/sundepth_simple");

#ifdef EQLC
		m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]		= g_pShaderAPI->CreateNamedRenderTarget("_rt_cubedepth",2048,2048,FORMAT_R32F, TEXFILTER_NEAREST, TEXADDRESS_CLAMP, COMP_GREATER, TEXFLAG_CUBEMAP);
		m_pShadowMaps[DLT_SPOT][0]					= g_pShaderAPI->CreateNamedRenderTarget("_rt_spotdepth",2048,2048,FORMAT_R32F, TEXFILTER_NEAREST, TEXADDRESS_CLAMP, COMP_GREATER );

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
		ER_TextureFilterMode shadowMapFilter = TEXFILTER_NEAREST;

		// Direct3D10 can use only depth textures to render shadowmaps
		if(g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D10)
		{
			shadowMapFilter = TEXFILTER_LINEAR;

			m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]	= g_pShaderAPI->CreateNamedRenderTarget("_rt_cubedepth",512,512,FORMAT_D16, shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER, TEXFLAG_CUBEMAP | TEXFLAG_RENDERSLICES | TEXFLAG_SAMPLEDEPTH);
			m_pShadowMaps[DLT_SPOT][0]				= g_pShaderAPI->CreateNamedRenderTarget("_rt_spotdepth",1024,1024,FORMAT_D16,shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER, TEXFLAG_SAMPLEDEPTH);

			// Sun has two depth textures
			m_pShadowMaps[DLT_SUN][0]				= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth1",2048,2048,FORMAT_D16,shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER, TEXFLAG_SAMPLEDEPTH);
		
			m_pShadowMaps[DLT_SUN][1]				= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth2",2048,2048,FORMAT_D16,shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER, TEXFLAG_SAMPLEDEPTH);
		}
		else
		{
			m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]	= g_pShaderAPI->CreateNamedRenderTarget("_rt_cubedepth",512,512,FORMAT_R32F, shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER, TEXFLAG_CUBEMAP);
			m_pShadowMaps[DLT_SPOT][0]				= g_pShaderAPI->CreateNamedRenderTarget("_rt_spotdepth",1024,1024,FORMAT_R32F,shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER);

			// Sun has two depth textures
			m_pShadowMaps[DLT_SUN][0]				= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth1",2048,2048,FORMAT_R32F,shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER);
		
			m_pShadowMaps[DLT_SUN][1]				= g_pShaderAPI->CreateNamedRenderTarget("_rt_sundepth2",2048,2048,FORMAT_R32F,shadowMapFilter, TEXADDRESS_CLAMP, COMP_GREATER);
		}

		m_pShadowMaps[DLT_OMNIDIRECTIONAL][0]->Ref_Grab();
		m_pShadowMaps[DLT_SPOT][0]->Ref_Grab();

		m_pShadowMaps[DLT_SUN][0]->Ref_Grab();
		m_pShadowMaps[DLT_SUN][1]->Ref_Grab();
	
		// TODO: drop Direct3D9 support
		if(g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D9)
		{
			// FIXME: THIS is very weird
			// this will be removed when engine will work on DirectX 10 API
#ifndef USE_SINGLE_CUBEMAPRENDER
			m_pShadowMapDepth[0]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap1", 512, 512, FORMAT_D16, TEXFILTER_NEAREST, TEXADDRESS_CLAMP, COMP_NEVER);
			m_pShadowMapDepth[1]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap2", 1024, 1024, FORMAT_D16, TEXFILTER_NEAREST, TEXADDRESS_CLAMP, COMP_NEVER);
			m_pShadowMapDepth[2]						= g_pShaderAPI->CreateNamedRenderTarget("_srt_depthmap3", 2048, 2048, FORMAT_D16, TEXFILTER_NEAREST, TEXADDRESS_CLAMP, COMP_NEVER);

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
		}
#endif // EQLC
		
		m_bShadowsInit = true;
	}

	// estimated memory: 36 mb on 1680x1050 with DS

	materials->EndPreloadMarker();
}

// called when world is unloading
void CViewRenderer::ShutdownResources()
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

	free(m_pOcclusionBuffer);
	m_pOcclusionBuffer = NULL;
	m_OcclusionRasterizer.SetImage(NULL,0,0);

	m_pActiveAreaList = NULL;
}

// update visibility from current view
void CViewRenderer::UpdateAreaVisibility( int nRenderFlags )
{
#ifndef EDITOR
	if(!m_pActiveAreaList)
		return;

	ResetAreaVisibilityStates();

#ifdef EQLC
	// in eqlc camera is outside
	if(m_nRenderMode == VDM_LIGHTING)
	{
		// draw all room
		for(int i = 0; i < g_pLevel->m_numRooms; i++)
		{
			m_pActiveAreaList->area_list[i].doRender = true;
			m_pActiveAreaList->area_list[i].planeSets.setNum(0, false);
		}

		return;
	}
#endif

	m_numViewRooms = g_pLevel->GetRoomsForPoint( m_pCurrentView->GetOrigin(), m_viewRooms);

	if(m_numViewRooms == 0)
	{
		// draw all rooms if camera is outside
		for(int i = 0; i < g_pLevel->m_numRooms; i++)
		{
			m_pActiveAreaList->area_list[i].doRender = true;
			m_pActiveAreaList->area_list[i].planeSets.setNum(0, false);
		}
	}
	else
	{
		for(int i = 0; i < m_numViewRooms; i++)
		{
			portalStack_t stack;
			stack.portal = NULL; // inital point
			stack.area_index = m_viewRooms[i];

			FloodViewThroughPortal_r(&stack, nRenderFlags, true);
		}
	}

	m_pActiveAreaList->updated = true;
#endif
}

void CViewRenderer::ResetAreaVisibilityStates()
{
#ifndef EDITOR
	// reset state if view changed
	if(!m_pActiveAreaList)
		return;

	for(int i = 0; i < g_pLevel->m_numRooms; i++)
	{
		m_pActiveAreaList->area_list[i].doRender = false;
		m_pActiveAreaList->area_list[i].planeSets.setNum(0, false);
	}

	m_pActiveAreaList->updated = false;

	m_numViewRooms = 0;
	m_viewRooms[0] = -1;
	m_viewRooms[1] = -1;
#endif // EDITOR
}

int CViewRenderer::GetViewRooms(int rooms[2])
{
	rooms[0] = m_viewRooms[0];
	rooms[1] = m_viewRooms[1];

	return m_numViewRooms;
}

// updates visibility for render list
void CViewRenderer::UpdateRenderlistVisibility( IRenderList* pRenderList )
{
#ifndef EDITOR
	/*
	for(int i = 0; i < g_pLevel->m_numRooms; i++)
	{
		CBaseRenderableObject* pObject = pRenderList->GetRenderable(i);
		//if()
	}
	*/
#endif
}

// sets new area list
void CViewRenderer::SetAreaList(renderAreaList_t* pList)
{
#ifndef EDITOR
	m_pActiveAreaList = pList;
#endif
}

// returns current area list
renderAreaList_t* CViewRenderer::GetAreaList()
{
#ifndef EDITOR
	return m_pActiveAreaList;
#else
	return NULL;
#endif
}

// sets view
void CViewRenderer::SetView(CViewParams* pParams)
{
	m_pCurrentView = pParams;
}

CViewParams* CViewRenderer::GetView()
{
	return m_pCurrentView;
}

// sets scene info
void CViewRenderer::SetSceneInfo(sceneinfo_t &info)
{
	m_sceneinfo = info;
}

// returns scene info
sceneinfo_t& CViewRenderer::GetSceneInfo()
{
	return m_sceneinfo;
}



#ifndef EDITOR

ConVar r_visOcclusion("r_visOcclusion", "0", "Enable occlusion system", CV_ARCHIVE);
ConVar r_visOcclusionDump("r_visOcclusionDump", "0", "Dump occlusion depth to raw texture file", CV_ARCHIVE);

Vector4D vertex_to_dc(Vector4D &vertex)
{
	Vector4D dst;

	float w = vertex.w;
	float iw = 1.0f / w;

	dst = vertex;

	dst.w = iw;

	return dst;
}

bool PointToScreen_Z_Custom(const Vector3D& point, Vector3D& screen, Matrix4x4 &wwp, float fScreenSize)
{
	Matrix4x4 worldToScreen = wwp;
	bool behind = false;

	Vector4D outMat;
	Vector4D transpos(point,1.0f);

	outMat = worldToScreen*transpos;

	if(outMat.w < 0)
		behind = true;

	float zDiv = outMat.w == 0.0f ? 1.0f :
		(1.0f / outMat.w);

	float w_sign = sign(outMat.w);

	if(w_sign < 0)
		zDiv = -outMat.w;

	float vW,vH;
	vW = fScreenSize;
	vH = fScreenSize;

	screen.x = ((vW * outMat.x * zDiv) + vW) * 0.5f;
	screen.y = ((vH - (vH * (outMat.y * zDiv)))) * 0.5f;
	screen.z = outMat.z*w_sign;

	return behind;
}

void CViewRenderer::RenderOccluders()
{
#ifndef EQLC

	int nOccludersVisible = 0;
	m_nOccluderTriangles = 0;

	// occlusion type 1
	if(g_pLevel->m_bOcclusionAvailable && r_visOcclusion.GetInt() == 1)
	{
		m_OcclusionRasterizer.SetColorClampDiscardEnable( true, m_sceneinfo.m_fZNear, m_sceneinfo.m_fZFar);
		m_OcclusionRasterizer.Clear(m_sceneinfo.m_fZFar);
		m_OcclusionRasterizer.SetTestMode(false);

		for(int i = 0; i < g_pLevel->m_numOcclusionSurfs; i++)
		{
			eqocclusionsurf_t* pSurf = &g_pLevel->m_pOcclusionSurfaces[i];
			if(!m_frustum.IsBoxInside(pSurf->mins.x,pSurf->maxs.x, pSurf->mins.y,pSurf->maxs.y, pSurf->mins.z,pSurf->maxs.z))
				continue;

			nOccludersVisible++;

			for(int j = 0; j < pSurf->numindices; j+=3)
			{
				Vector3D v0 = g_pLevel->m_vOcclusionVerts[g_pLevel->m_nOcclusionIndices[pSurf->firstindex+j]];
				Vector3D v1 = g_pLevel->m_vOcclusionVerts[g_pLevel->m_nOcclusionIndices[pSurf->firstindex+j+1]];
				Vector3D v2 = g_pLevel->m_vOcclusionVerts[g_pLevel->m_nOcclusionIndices[pSurf->firstindex+j+2]];

				if(!m_frustum.IsTriangleInside(v0,v1,v2))
					continue;

				Matrix4x4 tViewProj = m_viewprojection;

				Vector3D tv0;
				Vector3D tv1;
				Vector3D tv2;

				float half_size = m_nOcclusionBufferSize;

				float sign_w0 = PointToScreen_Z_Custom(v0,tv0,tViewProj, half_size) ? -1 : 1;
				float sign_w1 = PointToScreen_Z_Custom(v1,tv1,tViewProj, half_size) ? -1 : 1;
				float sign_w2 = PointToScreen_Z_Custom(v2,tv2,tViewProj, half_size) ? -1 : 1;

				/*
				float inv_z0 = 1.0f/tv0.z; //(tv40.w == 0.0f) ? 1.0f : (1.0f / tv40.w);
				float inv_z1 = 1.0f/tv1.z; //(tv41.w == 0.0f) ? 1.0f : (1.0f / tv41.w);
				float inv_z2 = 1.0f/tv2.z; //(tv42.w == 0.0f) ? 1.0f : (1.0f / tv42.w);

				tv0 *= Vector3D(inv_z0,inv_z0,1.0f)*sign_w0;
				tv1 *= Vector3D(inv_z1,inv_z1,1.0f)*sign_w1;
				tv2 *= Vector3D(inv_z2,inv_z2,1.0f)*sign_w2;
				*/

				//Msg("z value: %g %g %g\n", inv_z0, inv_z1,inv_z2);

				m_nOccluderTriangles++;
				/*
				// TODO: correct view-space
				tv0 += Vector3D(1,1,0);
				tv1 += Vector3D(1,1,0);
				tv2 += Vector3D(1,1,0);

				float half_size = m_nOcclusionBufferSize*0.5f;

				tv0 *= Vector3D(half_size,half_size,1.0f);
				tv1 *= Vector3D(half_size,half_size,1.0f);
				tv2 *= Vector3D(half_size,half_size,1.0f);
				*/

				m_OcclusionRasterizer.DrawTriangleZ(tv0, tv1, tv2);
				
				/*
				m_OcclusionRasterizer.DrawLine(tv0.z,tv0.xy(), tv1.z,tv1.xy());
				m_OcclusionRasterizer.DrawLine(tv1.z,tv1.xy(), tv2.z,tv2.xy());
				m_OcclusionRasterizer.DrawLine(tv2.z,tv2.xy(), tv0.z,tv0.xy());
				*/
				
			}
		}
	}

	/*
	debugoverlay->Text(ColorRGBA(1,1,1,1), "Visible occluders: %d\n", nOccludersVisible);
	debugoverlay->Text(ColorRGBA(1,1,1,1), "  occluder triangles: %d\n", m_nOccluderTriangles);


	if(r_vis_occlusiondump.GetBool())
	{
		r_vis_occlusiondump.SetBool(false);

		IFile* pFile = g_fileSystem->Open("occ_dump.raw", "wb", SP_ROOT);
		ubyte* pConv = (ubyte*)malloc(m_nOcclusionBufferSize*m_nOcclusionBufferSize);

		for(int i = 0; i < m_nOcclusionBufferSize*m_nOcclusionBufferSize; i++)
		{
			pConv[i] = (ubyte)(((m_pOcclusionBuffer[i] - m_sceneinfo.m_fZNear) / (m_sceneinfo.m_fZFar - m_sceneinfo.m_fZNear)) * 255.0f);
		}

		g_fileSystem->Write(pConv,1,m_nOcclusionBufferSize*m_nOcclusionBufferSize,pFile);
		g_fileSystem->Close(pFile);
		
		free(pConv);
	}
	*/
#endif
}

#define BBOX_STRIP_VERTS(min, max) \
	Vector3D(min.x, max.y, max.z),\
	Vector3D(max.x, max.y, max.z),\
	Vector3D(min.x, max.y, min.z),\
	Vector3D(max.x, max.y, min.z),\
	Vector3D(min.x, min.y, min.z),\
	Vector3D(max.x, min.y, min.z),	\
	Vector3D(min.x, min.y, max.z),	\
	Vector3D(max.x, min.y, max.z),	\
	Vector3D(max.x, min.y, max.z),	\
	Vector3D(max.x, min.y, min.z),	\
	Vector3D(max.x, min.y, min.z),	\
	Vector3D(max.x, max.y, min.z),	\
	Vector3D(max.x, min.y, max.z),	\
	Vector3D(max.x, max.y, max.z),	\
	Vector3D(min.x, min.y, max.z),	\
	Vector3D(min.x, max.y, max.z),	\
	Vector3D(min.x, min.y, min.z),	\
	Vector3D(min.x, max.y, min.z)

// That's very bad.
bool CViewRenderer::OcclusionTestBBox(Vector3D &mins, Vector3D &maxs)
{
	if(g_pLevel->m_bOcclusionAvailable && r_visOcclusion.GetBool() == 1)
	{
		m_OcclusionRasterizer.SetTestMode(true);
		m_OcclusionRasterizer.ResetOcclusionTest();

		Vector3D verts[] = {BBOX_STRIP_VERTS(mins, maxs)};

		int num_verts = elementsOf(verts);

		for(int i = 0; i < num_verts; i+=2)
		{
			Vector3D v0 = verts[i];
			Vector3D v1 = verts[i+1];
			Vector3D v2 = verts[i+2];

			if(!m_frustum.IsTriangleInside(v0,v1,v2))
				continue;

			Matrix4x4 tViewProj = m_viewprojection;

			Vector3D tv0;
			Vector3D tv1;
			Vector3D tv2;

			float half_size = m_nOcclusionBufferSize;

			float sign_w0 = PointToScreen_Z_Custom(v0,tv0,tViewProj, half_size) ? -1 : 1;
			float sign_w1 = PointToScreen_Z_Custom(v1,tv1,tViewProj, half_size) ? -1 : 1;
			float sign_w2 = PointToScreen_Z_Custom(v2,tv2,tViewProj, half_size) ? -1 : 1;

			m_OcclusionRasterizer.DrawTriangleZ(tv0, tv1, tv2);
		}

		int numPixels = m_OcclusionRasterizer.GetDrawnPixels();

		return (numPixels > 0);
	}
	else if(g_pLevel->m_bOcclusionAvailable && (r_visOcclusion.GetInt() == 2))
	{
		// second occlusion type

		BoundingBox bbox(mins, maxs);

		ubyte visiblePoints = 0xFF;

		for(int k = 0; k < 8; k++)
		{
			for(int i = 0; i < g_pLevel->m_numOcclusionSurfs; i++)
			{
				eqocclusionsurf_t* pSurf = &g_pLevel->m_pOcclusionSurfaces[i];
				if(!m_frustum.IsBoxInside(pSurf->mins.x,pSurf->maxs.x, pSurf->mins.y,pSurf->maxs.y, pSurf->mins.z,pSurf->maxs.z))
					continue;

				for(int j = 0; j < pSurf->numindices; j+=3)
				{
					Vector3D v0 = g_pLevel->m_vOcclusionVerts[g_pLevel->m_nOcclusionIndices[pSurf->firstindex+j]];
					Vector3D v1 = g_pLevel->m_vOcclusionVerts[g_pLevel->m_nOcclusionIndices[pSurf->firstindex+j+1]];
					Vector3D v2 = g_pLevel->m_vOcclusionVerts[g_pLevel->m_nOcclusionIndices[pSurf->firstindex+j+2]];

					if(!m_frustum.IsTriangleInside(v0,v1,v2))
						continue;

					Vector3D vPos = GetView()->GetOrigin();

					float fraction = 1.0f;

					if(IsRayIntersectsTriangle(v0,v1,v2,bbox.GetVertex(k),bbox.GetVertex(k)-vPos,fraction, true))
					{
						visiblePoints &= ~(1 << (k+1));
					}
				}
			}
		}

		if(visiblePoints == 0x0)
			return false;
	}

	return true;
}

// draws world
void CViewRenderer::DrawWorld( int nRenderFlags )
{
	if(!g_pLevel->IsLoaded())
		return;

	if(!m_pActiveAreaList)
		return;

	if(!(nRenderFlags & VR_FLAG_CUBEMAP))
		BuildViewMatrices(m_nViewportW, m_nViewportH, nRenderFlags);

#ifdef USE_SINGLE_CUBEMAPRENDER
	int nCubePass = 0;

redraw:
#endif // USE_SINGLE_CUBEMAPRENDER

	// TODO: draw occluders
	// RenderOccluders();

	if(nRenderFlags & VR_FLAG_CUBEMAP)
	{
#ifdef USE_SINGLE_CUBEMAPRENDER
		SetCubemapIndex(nCubePass);
#endif // USE_SINGLE_CUBEMAPRENDER

		BuildViewMatrices(m_nViewportW, m_nViewportH, nRenderFlags);

		if(m_nRenderMode == VDM_SHADOW)
			UpdateDepthSetup(true);
	}

	// update vis
	if(!(nRenderFlags & VR_FLAG_SKIPVISUPDATE) || (nRenderFlags & VR_FLAG_CUBEMAP))
		UpdateAreaVisibility( nRenderFlags );

	if(nRenderFlags & VR_FLAG_ONLY_SKY)
		UpdateRendertargetSetup();

	// render marked areas
	for(int i = 0; i < g_pLevel->m_numRooms; i++)
	{
#ifdef EQLC
		if( m_nRenderMode == VDM_LIGHTING || m_pActiveAreaList->area_list[i].doRender )
		{
			// eqlc needs skipping of visibility because lighting performs in 2D
			if( m_nRenderMode == VDM_LIGHTING )
				m_pActiveAreaList->area_list[i].planeSets.setNum(0, false);
#else
		if(m_pActiveAreaList->area_list[i].doRender)
		{
#endif
			DrawRoom(&m_pActiveAreaList->area_list[i], nRenderFlags);
		}
	}
	
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

ConVar r_debug_sectors("r_debugSectors", "0", "Show debug sector boxes", CV_CHEAT);
ConVar r_sector_max_surfaces("r_sectorDrawSurfaceCount", "-1", "Set debug maximum of surfaces in sector", CV_CHEAT);
ConVar r_drawWorld("r_drawWorld", "1", "Draw world objects", CV_CHEAT);

int ClipVertsAgainstPlane(Plane &plane, int vertexCount, Vector3D** pVerts)
{
	bool* negative = (bool*)stackalloc(vertexCount+2);

	float t;
	Vector3D v, v1, v2;

	Vector3D* vertex = *pVerts;

	// classify vertices
	int negativeCount = 0;
	for (int i = 0; i < vertexCount; i++)
	{
		negative[i] = !(plane.ClassifyPoint(vertex[i]) == CP_FRONT || plane.ClassifyPoint(vertex[i]) == CP_ONPLANE);
		negativeCount += negative[i];
	}
	
	if(negativeCount == vertexCount)
	{
		PPFree(vertex);
		return 0; // completely culled, we have no polygon
	}

	if(negativeCount == 0)
	{
		return -1;			// not clipped
	}

	Vector3D* newVertex = (Vector3D*)alloca(sizeof(Vector3D)*(vertexCount+2));

	int count = 0;

	for (int i = 0; i < vertexCount; i++)
	{
		// prev is the index of the previous vertex
		int	prev = (i != 0) ? i - 1 : vertexCount - 1;
		
		if (negative[i])
		{
			if (!negative[prev])
			{
				Vector3D v1 = vertex[prev];
				Vector3D v2 = vertex[i];

				// Current vertex is on negative side of plane,
				// but previous vertex is on positive side.
				Vector3D edge = v1 - v2;
				t = plane.Distance(v1) / dot(plane.normal, edge);

				newVertex[count] = lerp(v1,v2,t);
				count++;
			}
		}
		else
		{
			if (negative[prev])
			{
				Vector3D v1 = vertex[i];
				Vector3D v2 = vertex[prev];

				// Current vertex is on positive side of plane,
				// but previous vertex is on negative side.
				Vector3D edge = v1 - v2;

				t = plane.Distance(v1) / dot(plane.normal, edge);

				newVertex[count] = lerp(v1,v2,t);
				
				count++;
			}
			
			// include current vertex
			newVertex[count] = vertex[i];
			count++;
		}
	}

	PPFree( vertex );
	*pVerts = PPAllocStructArray(Vector3D, count);

	memcpy(*pVerts, newVertex, sizeof(Vector3D)*count);
	
	// return new clipped vertex count
	return count;
}

// floods view through world portals
void CViewRenderer::FloodViewThroughPortal_r(portalStack_t* pStack, int nRenderFlags, bool insidePortal)
{
	portalStack_t newStack;

	portalStack_t*	pCheckStack = NULL;
	//CEqRoom*		pCheckRoom = NULL;

	renderArea_t*	pArea = NULL;

	if(pStack->area_index != -1)
		pArea = &m_pActiveAreaList->area_list[pStack->area_index];

	if(pArea == NULL)
		return;

	if(pStack->numPortalPlanes > 0)
	{
		areaPlaneSet_t planeSet;

		planeSet.numPortalPlanes = pStack->numPortalPlanes;
		memcpy(planeSet.portalPlanes, pStack->portalPlanes, sizeof(Plane)*planeSet.numPortalPlanes);

		pArea->planeSets.append(planeSet);
	}

	// mark it to be rendered
	pArea->doRender = true;

	CEqRoom* pAreaRoom = &g_pLevel->m_pRooms[pArea->room];

	// do portals here
	for(int i = 0; i <pAreaRoom->room_info.numPortals; i++)
	{
		int portal_id = pAreaRoom->room_info.portals[i];

		CEqAreaPortal* pPortal = &g_pLevel->m_pPortals[portal_id];

		// in frustum?
		if(!m_frustum.IsBoxInside(	pPortal->m_bounding.minPoint.x, pPortal->m_bounding.maxPoint.x,
									pPortal->m_bounding.minPoint.y, pPortal->m_bounding.maxPoint.y,
									pPortal->m_bounding.minPoint.z, pPortal->m_bounding.maxPoint.z))
									continue;

		// get the visible side for that room and check
		int nVisibleSide = pAreaRoom->m_pPortalSides[i];
		
		// don't check if backfacing
		int cp_side = pPortal->m_planes[nVisibleSide].ClassifyPointEpsilon(m_pCurrentView->GetOrigin(), 0.05f);
		if(cp_side == CP_BACK)
			continue;

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for ( pCheckStack = pStack; pCheckStack; pCheckStack = pCheckStack->next )
		{
			if ( pCheckStack->portal == pPortal )
				break; // don't recursively enter a stack
		}

		if ( pCheckStack )
			continue; // already in stack

		int portal_roomid = !nVisibleSide;
		int area_idx = pPortal->portal_info.rooms[portal_roomid];

		newStack.numPortalPlanes = 0;

		/*
		// if we inside portal, instantiate render
		if(insidePortal)
		{
			newStack.next = pStack;
			newStack.portal = pPortal;

			newStack.area_index = area_idx;

			// render room portal if visible
			FloodViewThroughPortal_r(&newStack, nRenderFlags);

			continue;
		}
		*/

		bool completelyClipped = false;

		int			nPortalVerts	= pPortal->portal_info.numVerts[nVisibleSide];
		Vector3D*	portalVerts		= PPAllocStructArray(Vector3D, nPortalVerts);//new Vector3D[nPortalVerts];

		memcpy(portalVerts, pPortal->m_vertices[nVisibleSide], sizeof(Vector3D)*nPortalVerts);

		// clip 'em all
		for(int j = 0; j < pStack->numPortalPlanes; j++)
		{
			int numVerts = ClipVertsAgainstPlane(pStack->portalPlanes[j], nPortalVerts, &portalVerts);

			if(numVerts == -1)
				continue;

			if(numVerts == 0)
			{
				completelyClipped = true;
				break;
			}

			nPortalVerts = numVerts;
		}

		if(completelyClipped)
			continue;

		// this is also a vertex count
		int nAddPlanes = nPortalVerts;

		if(nAddPlanes > MAX_PORTAL_PLANES)
			nAddPlanes = MAX_PORTAL_PLANES;

		int next;

		// make clip planes for portal
		for(int j = 0; j < nAddPlanes; j++)
		{
			next = j+1;
			if ( next == nAddPlanes )
				next = 0;

			Vector3D v0 = m_pCurrentView->GetOrigin() - portalVerts[j];
			Vector3D v1 = m_pCurrentView->GetOrigin() - portalVerts[next];

			Plane pl;
			pl.normal = cross(v0, v1);

			// if it can't be normalized, just remove
			if(dot(pl.normal, pl.normal) < 0.001f)
				continue;

			pl.normal = -normalize(pl.normal);
			pl.offset = -dot(pl.normal, m_pCurrentView->GetOrigin());

#ifndef EQLC
			if(r_showPortals.GetBool())
			{
				Vector3D line_center = (portalVerts[j] + portalVerts[next]) * 0.5f;
				debugoverlay->Line3D(portalVerts[j], portalVerts[next], ColorRGBA(1,1,0,1),  ColorRGBA(1,1,0,1), 0.0f);
				debugoverlay->Line3D(line_center, line_center + pl.normal*25.0f, ColorRGBA(0,0,1,1),  ColorRGBA(0,0,1,1), 0.0f);
			}
#endif // EQLC

			newStack.portalPlanes[newStack.numPortalPlanes] = pl;
			newStack.numPortalPlanes++;
		}

		// we doesn't need these verts.
		PPFree(portalVerts);

		newStack.next = pStack;
		newStack.portal = pPortal;

		newStack.area_index = area_idx;

		// render room portal if visible
		FloodViewThroughPortal_r(&newStack, nRenderFlags);
	}
}

ConVar r_drawroom("r_drawroom", "-1", "Draw single room", CV_CHEAT);
ConVar r_drawroomvolume("r_drawroomvolume", "-1", "Draw single room volume", CV_CHEAT);

// intenal use: Draws world sector
void CViewRenderer::DrawRoom(renderArea_t* pArea, int nRenderFlags)
{
	if(!r_drawWorld.GetBool())
		return;

	materials->SetSkinningEnabled( false );

	CEqRoom* pAreaRoom = &g_pLevel->m_pRooms[pArea->room];

	if(r_drawroom.GetInt() != -1 && r_drawroom.GetInt() != pArea->room)
		return;

#ifdef EQLC
	if(m_nRenderMode == VDM_SHADOW)
#endif
	{
		if(!m_frustum.IsBoxInside(pAreaRoom->m_bounding.minPoint.x, pAreaRoom->m_bounding.maxPoint.x,
								pAreaRoom->m_bounding.minPoint.y, pAreaRoom->m_bounding.maxPoint.y,
								pAreaRoom->m_bounding.minPoint.z, pAreaRoom->m_bounding.maxPoint.z) /*&& r_vis_frustum.GetBool()*/)
								return;
	}
#ifndef EQLC
	if(GetView() && !pAreaRoom->m_bounding.Contains(GetView()->GetOrigin()) && ((m_nOccluderTriangles > 0 && r_visOcclusion.GetInt() == 1) || r_visOcclusion.GetInt() == 2))
	{
		if(!OcclusionTestBBox(pAreaRoom->m_bounding.minPoint, pAreaRoom->m_bounding.maxPoint))
			return;
	}
#endif

#ifndef EQLC
	if(r_debug_sectors.GetBool())
		debugoverlay->Box3D(pAreaRoom->m_bounding.minPoint, pAreaRoom->m_bounding.maxPoint, ColorRGBA(0,1,0,1),0);
#endif // EQLC

	for(int i = 0; i < pAreaRoom->m_numRoomVolumes; i++)
	{
		if(r_drawroomvolume.GetInt() != -1 && r_drawroomvolume.GetInt() != i)
			continue;

		DrawRoomVolume( pArea, &pAreaRoom->m_pRoomVolumes[i], nRenderFlags);
	}
}

ConVar r_drawOnlyLightmapSurfs("r_drawOnlyLightmapSurfs", "-1");

// room volume rendering
void CViewRenderer::DrawRoomVolume(renderArea_t* pCurArea, CEqRoomVolume* pRoomVolume, int nRenderFlags)
{
#ifdef EQLC
	if(m_nRenderMode == VDM_SHADOW)
#endif
	{
		if(!m_frustum.IsBoxInside(	pRoomVolume->m_bounding.minPoint.x, pRoomVolume->m_bounding.maxPoint.x,
									pRoomVolume->m_bounding.minPoint.y, pRoomVolume->m_bounding.maxPoint.y,
									pRoomVolume->m_bounding.minPoint.z, pRoomVolume->m_bounding.maxPoint.z))
								return;

		if(pCurArea)
		{
			if(!pCurArea->IsBoxInside(	pRoomVolume->m_bounding.minPoint.x, pRoomVolume->m_bounding.maxPoint.x,
										pRoomVolume->m_bounding.minPoint.y, pRoomVolume->m_bounding.maxPoint.y,
										pRoomVolume->m_bounding.minPoint.z, pRoomVolume->m_bounding.maxPoint.z))
									return;
		}
	}

	g_pShaderAPI->SetVertexFormat(g_pLevel->m_pVertexFormat_Lit);
	g_pShaderAPI->SetVertexBuffer(g_pLevel->m_pVertexBuffer, 0);
	g_pShaderAPI->SetIndexBuffer(g_pLevel->m_pIndexBuffer);

	materials->SetAmbientColor(ColorRGBA(m_sceneinfo.m_cAmbientColor, 1));

	m_matrices[MATRIXMODE_WORLD] = identity4();

	materials->SetMatrix(MATRIXMODE_WORLD, m_matrices[MATRIXMODE_WORLD]);
	materials->SetCullMode(CULL_BACK);

	g_pShaderAPI->ApplyBuffers();

#ifndef EQLC
	if(r_debug_sectors.GetBool())
		debugoverlay->Box3D(pRoomVolume->m_bounding.minPoint, pRoomVolume->m_bounding.maxPoint, ColorRGBA(0,1,0,1),0);
#endif // EQLC

	int nNumSurfaces = 0;

	for(int i = 0; i < pRoomVolume->m_numDrawSurfaceGroups; i++)
	{
		eqlevelsurf_t *pSurface = &pRoomVolume->m_pDrawSurfaceGroups[i];

		if(r_sector_max_surfaces.GetInt() != -1 && i > r_sector_max_surfaces.GetInt())
			continue;

		// draw surface with extended features
		DrawSurfaceEx( pCurArea, pSurface, nRenderFlags);

		nNumSurfaces++;
	}

#ifndef EQLC
	if(r_debug_sectors.GetBool() && nNumSurfaces)
		debugoverlay->Text(ColorRGBA(1,1,1,1), "Sector surfaces: %d\n", nNumSurfaces);
#endif
}

// draws single material surface
void CViewRenderer::DrawSurfaceEx(renderArea_t* pCurArea, eqlevelsurf_t *pSurface, int nRenderFlags)
{
	if((nRenderFlags & VR_FLAG_ONLY_SKY))
	{
		if(!(pSurface->flags & EQSURF_FLAG_SKY))
			return;
	}
	else
	{
		if((pSurface->flags & EQSURF_FLAG_SKY))
			return;

		bool bHasAnyTranslucency = (pSurface->flags & EQSURF_FLAG_TRANSLUCENT) > 0;

		if((nRenderFlags & VR_FLAG_NO_TRANSLUCENT) && bHasAnyTranslucency)
			return;

		if((nRenderFlags & VR_FLAG_WATERREFLECTION) && (pSurface->flags & EQSURF_FLAG_WATER))
			return;

		if((nRenderFlags & VR_FLAG_NO_OPAQUE) && !bHasAnyTranslucency)
			return;

		if((m_nRenderMode != VDM_SHADOW) && (pSurface->flags & EQSURF_FLAG_BLOCKLIGHT))
			return;
	}

	if(r_drawOnlyLightmapSurfs.GetInt() != -1 && (pSurface->lightmap_id != r_drawOnlyLightmapSurfs.GetInt()))
		return;

#ifdef EQLC
	if((pSurface->lightmap_id != m_nLightmapID) && (m_nRenderMode != VDM_SHADOW))
		return;

	if(m_nRenderMode == VDM_SHADOW)
#endif
	{
		if(!m_frustum.IsBoxInside(	pSurface->mins.x, pSurface->maxs.x,
									pSurface->mins.y, pSurface->maxs.y,
									pSurface->mins.z, pSurface->maxs.z))
			return;

		if(pCurArea)
		{
			if(!pCurArea->IsBoxInside(	pSurface->mins.x, pSurface->maxs.x,
										pSurface->mins.y, pSurface->maxs.y,
										pSurface->mins.z, pSurface->maxs.z))
										return;
		}
	}

	// for forward rendering only (UNDONE)
	if((m_nRenderMode == VDM_LIGHTING) && materials->GetLight())
	{
		if(materials->GetLight()->nType == DLT_OMNIDIRECTIONAL)
		{
			BoundingBox box(materials->GetLight()->lightBoundMin, materials->GetLight()->lightBoundMax);
			BoundingBox surfBox(pSurface->mins, pSurface->maxs);

			if(!box.Intersects(surfBox))
				return;
		}
		else
		{
			Volume light_frustum;
			light_frustum.LoadAsFrustum(materials->GetLight()->lightWVP);

			if(!light_frustum.IsBoxInside(pSurface->mins.x, pSurface->maxs.x,
											pSurface->mins.y, pSurface->maxs.y,
											pSurface->mins.z, pSurface->maxs.z))
											return;
		}
	}
		

	IMaterial* pMaterial = g_pLevel->m_pMaterials[pSurface->material_id];

	if(!pMaterial)
		return;

	HOOK_TO_CVAR(r_wireframe);

	materials->GetConfiguration().wireframeMode = r_wireframe->GetBool();
		
	if(!(nRenderFlags & VR_FLAG_NO_MATERIALS))
	{
		materials->SetMatrix(MATRIXMODE_WORLD, m_matrices[MATRIXMODE_WORLD]);

		materials->BindMaterial(pMaterial, false);

#ifndef EQLC
		if(g_pLevel->m_numLightmaps && pSurface->lightmap_id != -1)
		{
			HOOK_TO_CVAR(r_showlightmaps);

			ITexture* pLightmap = g_pLevel->m_pLightmaps[pSurface->lightmap_id];

			if(r_showlightmaps->GetInt() == 2)
				pLightmap = materials->GetLuxelTestTexture();

			// set lightmap texture
			g_pShaderAPI->SetTexture(pLightmap, "LightmapTexture", 13);
			//g_pShaderAPI->SetTexture(g_pLevel->m_pDirmaps[pSurface->lightmap_id], "DirmapTexture", 14);
		}

		//else
		//	g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture(), "LightmapTexture", 13);
#endif

		if(
			//(materials->GetCurrentLightingModel() == LIGHTING_FORWARD) &&
			(m_nRenderMode == VDM_LIGHTING)
			&& (materials->GetLight() != NULL))
		{
			//Msg("SET ShadowMapSampler\n");

			// attach shadowmap buffers to shader
			int nLightType = materials->GetLight()->nType;

			// apply current shadowmap as s5, s6 for sun lod
			g_pShaderAPI->SetTexture( m_pShadowMaps[nLightType][0], "ShadowMapSampler",  5 );
			g_pShaderAPI->SetTexture( m_pShadowMaps[nLightType][1], "ShadowMapSampler2", 6 );
		}

		materials->Apply();
	}
	else
	{
		if(m_nRenderMode == VDM_SHADOW)
		{
			UpdateDepthSetup();
#ifdef EQLC
			pMaterial->GetShader()->InitParams();
#endif

			float fAlphatestFac = 0.0f;

			if(pMaterial->GetFlags() & MATERIAL_FLAG_ALPHATESTED)
				fAlphatestFac = 50.0f;

			//g_pShaderAPI->SetPixelShaderConstantFloat(0, fAlphatestFac);
			if(pMaterial->GetFlags() & MATERIAL_FLAG_NOCULL)
				materials->SetCullMode(CULL_NONE);

			g_pShaderAPI->SetShaderConstantFloat("AlphaTestFactor", fAlphatestFac);
			g_pShaderAPI->SetTexture( pMaterial->GetBaseTexture(0) );
			g_pShaderAPI->ApplyTextures();

#ifdef EQLC
			//materials->SetRasterizerStates(CULL_NONE);
#endif
		}

		g_pShaderAPI->Apply();
	}

	// increment render counter
	//g_pLevel->m_nMaterialRenderCounter[ pSurface->material_id ]++;

	g_pShaderAPI->SetDepthRange(0.0f, 1.0f);
	
	g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, pSurface->firstindex, pSurface->numindices, pSurface->firstvertex, pSurface->numvertices);
}

#endif // EDITOR

// -------------------------------------------

void CViewRenderer::SetModelInstanceChanged()
{
	m_bCurrentSkinningPrepared = false;
}

// draws render list using the current view, also you can specify the render target
void CViewRenderer::DrawRenderList(IRenderList* pRenderList, int nRenderFlags)
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
void CViewRenderer::DrawModelPart(IEqModel*	pModel, int nModel, int nGroup, int nRenderFlags, slight_t* pStaticLights, Matrix4x4* pBones)
{
	IMaterial* pMaterial = pModel->GetMaterial(nModel);

	if((nRenderFlags & VR_FLAG_NO_TRANSLUCENT) && (pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		return;

	if((nRenderFlags & VR_FLAG_NO_OPAQUE) && !(pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
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

			if(pMaterial->GetFlags() & MATERIAL_FLAG_NOCULL)
				materials->SetCullMode(CULL_NONE);

			g_pShaderAPI->SetShaderConstantFloat("AlphaTestFactor", fAlphatestFac);
			g_pShaderAPI->SetTexture(pMaterial->GetBaseTexture(0));
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
void CViewRenderer::UpdateDepthSetup(bool bUpdateRT, VRSkinType_e nSkin)
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

void CViewRenderer::UpdateRendertargetSetup()
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
void CViewRenderer::SetupRenderTargets(int nNumRTs, ITexture** ppRenderTargets, ITexture* pDepthTarget, int* nCubeFaces)
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
void CViewRenderer::SetupRenderTargetToBackbuffer()
{
	m_numMRTs = 0;
	UpdateRendertargetSetup();
}

// returns the current set rendertargets
void CViewRenderer::GetCurrentRenderTargets(int* nNumRTs, ITexture** ppRenderTargets, 
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
dlight_t* CViewRenderer::AllocLight()
{
	dlight_t* pLight = (dlight_t*)PPAlloc( sizeof(dlight_t) );
	SetLightDefaults(pLight);

	// always need white texture
	pLight->pMaskTexture = materials->GetWhiteTexture();

	return pLight;
}

// adds light to current frame
void CViewRenderer::AddLight(dlight_t* pLight)
{
	if(materials->GetConfiguration().lighting_model == MATERIAL_LIGHT_UNLIT)
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
void CViewRenderer::RemoveLight(int idx)
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
void CViewRenderer::UpdateLights(float fDt)
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
#define VERTS_SUBDIVS_COUNT ((VERTS_SUBDIVS+1)*(VERTS_SUBDIVS+1)*4)

int GetViewspaceMesh(Vertex2D_t* verts)
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

	return nTriangle;
}

Vertex2D_t* CViewRenderer::GetViewSpaceMesh(int* size)
{
	static Vertex2D_t* quad_sub = NULL;
	static int nTriangleCount = 0;

	if(quad_sub == NULL)
	{
		// alloc in automatic memory remover
		quad_sub = PPAllocStructArray(Vertex2D_t, VERTS_SUBDIVS_COUNT);

		nTriangleCount = GetViewspaceMesh( quad_sub );
	}

	if(size)
		*size = nTriangleCount*3;

	return quad_sub;
}

// draws deferred ambient
void CViewRenderer::DrawDeferredAmbient()
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

	int nVerts = 0;
	Vertex2D_t* amb_quad_sub = GetViewSpaceMesh(&nVerts);

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	float wTexel = 1.0f;//1.0f / float(m_nViewportW);
	float hTexel = 1.0f;//1.0f / float(m_nViewportH);

	meshBuilder.Begin(PRIM_TRIANGLES);
		for(int i = 0; i < nVerts; i++)
		{
			meshBuilder.Position3fv(Vector3D((amb_quad_sub[i].position * Vector2D((float)m_nViewportW+wTexel,(float)m_nViewportH+hTexel) - Vector2D(wTexel, hTexel)),0));
			meshBuilder.TexCoord2fv( amb_quad_sub[i].texCoord );
			meshBuilder.AdvanceVertex();
		}
	meshBuilder.End();
}

ConVar r_ds_debug("r_ds_debug", "-1", "Debug deferred shading target", CV_CHEAT);
ConVar r_debug_caustics("r_debug_caustics", "0", "Debug caustics texture", CV_CHEAT);

/*
ITexture* g_pDebugTexture = NULL;

void OnShowTextureChanged(ConVar* pVar,char const* pszOldValue)
{
	if(g_pDebugTexture)
		g_pShaderAPI->FreeTexture(g_pDebugTexture);

	g_pDebugTexture = g_pShaderAPI->FindTexture( pVar->GetString() );

	if(g_pDebugTexture)
		g_pDebugTexture->Ref_Grab();
}
*/

//ConVar r_showTexture("r_showTexture", "", OnShowTextureChanged, "if r_debugShowTexture enabled, it shows the selected texture in overlay", CV_CHEAT);

// draws debug data
void CViewRenderer::DrawDebug()
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

	/*
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
	}*/

#endif // EDITOR
}

ConVar r_skipdslighting("r_skipdslighting", "0", "Skips deferred lighitng", CV_CHEAT);

ConVar r_shadow_pointbias("r_shadow_pointbias", "-0.0001", "Skips deferred lighitng");
ConVar r_shadow_spotbias("r_shadow_spotbias", "-1.0001", "Skips deferred lighitng");
ConVar r_shadow_sunbias("r_shadow_sunbias", "-1.00025", "Skips deferred lighitng");

// draws deferred lighting
void CViewRenderer::DrawDeferredCurrentLighting(bool bShadowLight)
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

	float light_bias[DLT_COUNT] = 
	{
		r_shadow_pointbias.GetFloat(),
		r_shadow_spotbias.GetFloat(),
		r_shadow_sunbias.GetFloat(),
	};

	float fShadowDepthBias = light_bias[nLightType];

	Matrix4x4 lightSphereWorldMat;
	
	if(nLightType == DLT_OMNIDIRECTIONAL)
	{
		lightSphereWorldMat = translate(materials->GetLight()->position)*scale4(materials->GetLight()->radius.x,materials->GetLight()->radius.y,materials->GetLight()->radius.z);
	}
	else if (nLightType == DLT_SPOT)
	{
		float fFovScale = (1.0f - (cos(DEG2RAD(materials->GetLight()->fovWH.z))*sin(DEG2RAD(materials->GetLight()->fovWH.z))))*materials->GetLight()->radius.x*6;
		lightSphereWorldMat = translate(materials->GetLight()->position)*Matrix4x4(materials->GetLight()->lightRotation)*scale4(fFovScale,fFovScale,materials->GetLight()->radius.x*2);

		if( materials->GetLight()->nFlags & LFLAG_SIMULATE_REFLECTOR && bShadowLight )
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

		//if(materials->GetBoundMaterial()->GetState() == MATERIAL_LOAD_OK)
		//	materials->GetBoundMaterial()->GetShader()->SetupConstants();

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

		// setup deth properties
		SetupLightDepthProps(m_sceneinfo, nLightType, fShadowDepthBias);

		g_pShaderAPI->Apply();

		int nVerts = 0;
		Vertex2D_t* amb_quad_sub = GetViewSpaceMesh(&nVerts);

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());

		float wTexel = 1.0f;//1.0f / float(m_nViewportW);
		float hTexel = 1.0f;//1.0f / float(m_nViewportH);

		meshBuilder.Begin(PRIM_TRIANGLES);
			for(int i = 0; i < nVerts; i++)
			{
				meshBuilder.Position3fv(Vector3D((amb_quad_sub[i].position * Vector2D((float)m_nViewportW+wTexel,(float)m_nViewportH+hTexel) - Vector2D(wTexel, hTexel)),0));
				meshBuilder.TexCoord2fv( amb_quad_sub[i].texCoord );
				meshBuilder.AdvanceVertex();
			}
		meshBuilder.End();

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

	// setup deth properties
	SetupLightDepthProps(m_sceneinfo, nLightType, fShadowDepthBias);

	m_pDSlightModel->DrawGroup( 0, 0 );

	//m_pDSlightModel->DrawGroup( nLightType, 0 );
}

// backbuffer viewport size
void CViewRenderer::SetViewportSize(int width, int height, bool bSetDeviceViewport)
{
	// store viewport
	m_nViewportW = width;
	m_nViewportH = height;

	if(bSetDeviceViewport)
		g_pShaderAPI->SetViewport(0, 0, m_nViewportW, m_nViewportH);
}

// backbuffer viewport size
void CViewRenderer::GetViewportSize(int &width, int &height)
{
	width = m_nViewportW;
	height = m_nViewportH;
}

ConVar r_ortho("r_ortho", "0", "Orthogonal mode", CV_CHEAT);
ConVar r_ortho_size("r_ortho_size", "1024", "Orthogonal size", CV_CHEAT);

void CViewRenderer::BuildViewMatrices(int width, int height, int nRenderFlags)
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
				m_matrices[MATRIXMODE_PROJECTION]	= orthoMatrixR(-r_ortho_size.GetFloat(), r_ortho_size.GetFloat(), -r_ortho_size.GetFloat(), r_ortho_size.GetFloat(), m_sceneinfo.m_fZNear, m_sceneinfo.m_fZFar);

			viewMatrix = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);
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
void CViewRenderer::SetCubemapIndex( int nCubeIndex )
{
	m_nCubeFaceId = nCubeIndex;
}

// returns the specified matrix
Matrix4x4 CViewRenderer::GetMatrix(ER_MatrixMode mode)
{
	return m_matrices[mode];
}

// returns view-projection matrix
Matrix4x4 CViewRenderer::GetViewProjectionMatrix()
{
	return m_viewprojection;
}

// view frustum
Volume* CViewRenderer::GetViewFrustum()
{
	return &m_frustum;
}

// sets the render mode
void CViewRenderer::SetDrawMode(ViewDrawMode_e mode)
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
			SetupShadowDepth(nLightType, 0, m_nCubeFaceId);
#endif // USE_SINGLE_CUBEMAPRENDER
		}
		else
		{
			int sunIndex = m_nCubeFaceId;
			SetupShadowDepth(nLightType, sunIndex, 0);

			/*
			// attach shadowmap texture TODO: sun index
			g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][m_nCubeFaceId], 0, m_pShadowMapDepth[nLightType]);
			*/
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

void CViewRenderer::SetupShadowDepth(int nLightType, int sunIndex, int cubeIndex)
{
#ifdef EQLC
	g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][sunIndex], cubeIndex, m_pShadowMapDepth[nLightType]);
#else

	if(g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D9)
	{
		g_pShaderAPI->ChangeRenderTarget(m_pShadowMaps[nLightType][sunIndex], cubeIndex, m_pShadowMapDepth[nLightType]);

		// clear them
		g_pShaderAPI->Clear(true, true, false, vec4_zero, 1.0f);
	}
	else
	{
		// attach shadowmap texture
		g_pShaderAPI->ChangeRenderTargets(NULL, 0, NULL, m_pShadowMaps[nLightType][sunIndex], /*cubeIndex*/POSITIVE_Y);

		// clear it
		g_pShaderAPI->Clear(false, true, false, vec4_zero, 1.0f);
	}
#endif
}

ViewDrawMode_e CViewRenderer::GetDrawMode()
{
	return m_nRenderMode;
}

// retuns light
dlight_t* CViewRenderer::GetLight(int index)
{
	return m_pLights[index];
}

int	CViewRenderer::GetLightCount()
{
	return m_numLights;
}

//------------------------------------------------

Vector3D g_check_pos = vec3_zero;

int comp_eqlevellight_dist(const void* l1,const void* l2)
{
	eqlevellight_t* light1 = (eqlevellight_t*)l1;
	eqlevellight_t* light2 = (eqlevellight_t*)l2;

	Vector3D lvec1 = light1->origin - g_check_pos;
	Vector3D lvec2 = light2->origin - g_check_pos;

	// sort by intensity
	float intensity1 = dot(light1->color, light1->color);
	float intensity2 = dot(light2->color, light2->color);

	float dist1 = dot(lvec1, lvec1);
	float dist2 = dot(lvec2, lvec2);

	return (int)sign(dist1*intensity1 - dist2*intensity2);
}

// returns lighting color and direction
void CViewRenderer::GetLightingSampleAtPosition(const Vector3D &position, ColorRGB &color)
{
#if 0

	// enumerate visible light sources (from list)
#ifndef EDITOR
	if( !g_pLevel->m_numLevelLights )
		return;

	 // HACK: comparison is global
	g_check_pos = position;

	// sort to nearest position
	qsort(g_pLevel->m_pLevelLights, g_pLevel->m_numLevelLights, sizeof(eqlevellight_t), comp_eqlevellight_dist);

	ColorRGB lighting = vec3_zero;

	int nLights = 0;

	for(int i = 0; i < g_pLevel->m_numLevelLights; i++)
	{
		Vector3D lvec = position-g_pLevel->m_pLevelLights[i].origin;
		float srad = g_pLevel->m_pLevelLights[i].radius*g_pLevel->m_pLevelLights[i].radius;

		if(dot(lvec, lvec) <= srad)
		{
			// check light occlusion
			internaltrace_t tr;

			if(!(g_pLevel->m_pLevelLights[i].flags & EQLEVEL_LIGHT_NOTRACE))
				physics->InternalTraceLine(g_pLevel->m_pLevelLights[i].origin, (Vector3D)position, COLLISION_GROUP_WORLD, &tr);

			if((g_pLevel->m_pLevelLights[i].flags & EQLEVEL_LIGHT_NOTRACE) || (tr.fraction > 0.999f))
			{
				float atten = saturate(1.0f - dot(lvec,lvec));

				lighting += g_pLevel->m_pLevelLights[i].color * atten;
			}
		}

		if(nLights > 7) // even with this it's too slow
			break;
	}

	color = lighting;
#endif

#else
	// too slow
	color = ColorRGB(0,0,0);
#endif
}

// fills array with eight nearest lights to point
void CViewRenderer::GetNearestEightLightsForPoint(const Vector3D &position, slight_t *pLights)
{
	memcpy(pLights, s_default_no_lights, sizeof(slight_t)*8);

#ifndef EDITOR
	if( !g_pLevel->m_numLevelLights )
		return;

	 // HACK: comparison is global
	g_check_pos = position;

	// sort to nearest position
	qsort(g_pLevel->m_pLevelLights, g_pLevel->m_numLevelLights, sizeof(eqlevellight_t), comp_eqlevellight_dist);

	int nLights = 0;

	for(int i = 0; i < g_pLevel->m_numLevelLights; i++)
	{
		Vector3D lvec = position-g_pLevel->m_pLevelLights[i].origin;
		float srad = g_pLevel->m_pLevelLights[i].radius*g_pLevel->m_pLevelLights[i].radius;

		if(dot(lvec, lvec) <= srad)
		{
			// check light occlusion
			internaltrace_t tr;

			if(!(g_pLevel->m_pLevelLights[i].flags & EQLEVEL_LIGHT_NOTRACE))
				physics->InternalTraceLine(g_pLevel->m_pLevelLights[i].origin, (Vector3D)position, COLLISION_GROUP_WORLD, &tr);

			if((g_pLevel->m_pLevelLights[i].flags & EQLEVEL_LIGHT_NOTRACE) || (tr.fraction > 0.999f))
			{
				pLights[nLights++] = slight_t(	Vector4D(g_pLevel->m_pLevelLights[i].origin, 1.0f / g_pLevel->m_pLevelLights[i].radius), 
												Vector4D(g_pLevel->m_pLevelLights[i].color, 1.0f));
			}

			if(nLights > 7)
				return;
		}
	}
#endif

}