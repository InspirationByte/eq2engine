//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: View renderer for game
//////////////////////////////////////////////////////////////////////////////////

#ifndef VIEWRENDERER_H
#define VIEWRENDERER_H

#include "IViewRenderer.h"
#include "Utils/rasterizer.h"
#include "eqlevel.h"

enum GBufferRT_e
{
	GBUF_DIFFUSE = 0,
	GBUF_NORMALS,
	GBUF_DEPTH,
	GBUF_MATERIALMAP1,

	GBUF_TEXTURES		// GBuffer RT's count
};

class CEqRoom;
class CEqRoomVolume;
class CEqAreaPortal;

struct portalStack_t
{
	portalStack_t()
	{
		numPortalPlanes = 0;
		portal = NULL;
		next = NULL;
		area_index = -1;
	};

	portalStack_t*	next;

	int				area_index;
	CEqAreaPortal*	portal;

	Plane			portalPlanes[MAX_PORTAL_PLANES+1];
	int				numPortalPlanes;
};

//------------------------------------------------
// View renderer class
//------------------------------------------------

class CViewRenderer : public IViewRenderer
{
public:
	CViewRenderer();

	//------------------------------------------------

	// called when map is loading
	void					InitializeResources();

	// called when world is unloading
	void					ShutdownResources();

	//------------------------------------------------

	// sets view
	void					SetView(CViewParams* pParams);
	CViewParams*			GetView();

#ifndef EDITOR
	// draws world
	void					DrawWorld(int nRenderFlags);

	// floods view through world portals
	void					FloodViewThroughPortal_r(portalStack_t* pStack, int nRenderFlags, bool insidePortal = false);

	void					DrawRoom(renderArea_t* pArea, int nRenderFlags);
	void					DrawRoomVolume(renderArea_t* pCurArea, CEqRoomVolume* pRoom, int nRenderFlags);
	void					DrawSurfaceEx(renderArea_t* pCurArea, eqlevelsurf_t *pSurface, int nRenderFlags);
#endif

#ifdef EQLC
	void					SetLightmapTextureIndex(int nLMIndex);
#endif // EQLC

	// draws render list using the current view, also you can specify the render target
	void					DrawRenderList(IRenderList* pRenderList, int nRenderFlags);

	// draws model part with existing world transform (used internally by renderers)
	void					DrawModelPart(IEqModel*	pModel, int nModel, int nGroup, int nRenderFlags, slight_t* pStaticLights, Matrix4x4* pBones);

//------------------------------------------------

	// sets the rendertargets
	void					SetupRenderTargets(int nNumRTs = 0, ITexture** ppRenderTargets = NULL, ITexture* pDepthTarget = NULL, int* nCubeFaces = NULL);

	// sets rendertargets to backbuffers
	void					SetupRenderTargetToBackbuffer();

	// returns the current set rendertargets
	void					GetCurrentRenderTargets(int* nNumRTs, ITexture** pRenderTargets, 
													ITexture** pDepthTarget, 
													int* nCubeFaces);

	//------------------------------------------------

	// allocates new light
	dlight_t*				AllocLight();

	// adds light to current frame
	void					AddLight(dlight_t* pLight);

	// removes light
	void					RemoveLight(int idx);

	// updates lights
	void					UpdateLights(float fDt);

	// retuns light
	dlight_t*				GetLight(int index);

	// returns light count
	int						GetLightCount();

	// draws deferred ambient
	void					DrawDeferredAmbient();

	// draws deferred lighting
	void					DrawDeferredCurrentLighting(bool bShadowLight);

	// draws debug data
	void					DrawDebug();

	// updates material
	void					UpdateDepthSetup(bool bUpdateRT = false, VRSkinType_e nSkin = VR_SKIN_NONE);

	//------------------------------------------------

	// sets the render mode
	void					SetDrawMode(ViewDrawMode_e mode);

	// returns draw mode
	ViewDrawMode_e			GetDrawMode();

	// sets cubemap texture index
	void					SetCubemapIndex( int nCubeIndex );

	// returns the specified matrix
	Matrix4x4				GetMatrix(ER_MatrixMode mode);

	// returns view-projection matrix
	Matrix4x4				GetViewProjectionMatrix();

	// view frustum
	Volume*					GetViewFrustum();

	// sets scene info
	void					SetSceneInfo(sceneinfo_t &info);

	// returns scene info
	sceneinfo_t&			GetSceneInfo();

	// builds view matrices before rendering of view
	void					BuildViewMatrices(int width, int height, int nRenderFlags);

	// backbuffer viewport size
	void					SetViewportSize(int width, int height, bool bSetDeviceViewport = true);

	// backbuffer viewport size
	void					GetViewportSize(int &width, int &height);

	//------------------------------------------------

	// resets skinning state
	void					SetModelInstanceChanged();

	// returns lighting color and direction
	void					GetLightingSampleAtPosition(const Vector3D &position, ColorRGB &color);

	// fills array with eight nearest lights to point
	void					GetNearestEightLightsForPoint(const Vector3D &position, slight_t *pLights);

	// sets new area list
	void					SetAreaList(renderAreaList_t* pList);

	// returns current area list
	renderAreaList_t*		GetAreaList();

	// update visibility from current view
	void					UpdateAreaVisibility( int nRenderFlags );

	// updates render list visibility
	void					UpdateRenderlistVisibility( IRenderList* pRenderList );

	void					ResetAreaVisibilityStates();

	int						GetViewRooms(int rooms[2]);

	Vertex2D_t*				GetViewSpaceMesh(int* size);

	void					SetupShadowDepth(int nLightType, int sunIndex, int cubeIndex);

protected:

	void					RenderOccluders();

	bool					OcclusionTestBBox(Vector3D &mins, Vector3D &maxs);

	void					UpdateRendertargetSetup();

	renderAreaList_t*		m_pActiveAreaList;

	CViewParams*			m_pCurrentView;

	ITexture*				m_pTargets[MAX_MRTS];
	int						m_numMRTs;
	int						m_nCubeFaces[MAX_MRTS];
	ITexture*				m_pDepthTarget;
	
	// per-frame lights
	dlight_t*				m_pLights[MAX_VISIBLE_LIGHTS];
	int						m_numLights;

	// keep current light always
	dlight_t*				m_pCurrLight;

	ViewDrawMode_e			m_nRenderMode;

	Matrix4x4				m_matrices[4];
	Matrix4x4				m_viewprojection;

	Volume					m_frustum;

	sceneinfo_t				m_sceneinfo;

	int						m_nViewportW;
	int						m_nViewportH;
	int						m_nCubeFaceId;

	// ----------------
	// Engine resources
	// ----------------

	// occlusion buffer
	float*					m_pOcclusionBuffer;
	int						m_nOcclusionBufferSize;
	CRasterizer<float>		m_OcclusionRasterizer;

	// DS materials and targets
	ITexture*				m_pGBufferTextures[GBUF_TEXTURES];
	IMaterial*				m_pDSLightMaterials[DLT_COUNT][2];
	IMaterial*				m_pDSAmbient;

	IMaterial*				m_pDSSpotlightReflector;

	IMaterial*				m_pSpotCaustics;
	ITexture*				m_pSpotCausticTex;
	ITexture*				m_pCausticsGBuffer[2]; // GBUF_DIFFUSE and GBUF_NORMALS only

	IVertexBuffer*			m_pSpotCausticsBuffer;
	IVertexFormat*			m_pSpotCausticsFormat;

	// Shadow maps
	ITexture*				m_pShadowMaps[DLT_COUNT][2];
	ITexture*				m_pShadowMapDepth[6];
	IMaterial*				m_pShadowmapDepthwrite[DLT_COUNT][3];

	IEqModel*			m_pDSlightModel;

	bool					m_bCurrentSkinningPrepared;

	bool					m_bDSInit;
	bool					m_bShadowsInit;

	int						m_nOccluderTriangles;

	int						m_viewRooms[2];
	int						m_numViewRooms;

#ifdef EQLC
	int						m_nLightmapID;
#endif
};

#endif // VIEWRENDERER_H