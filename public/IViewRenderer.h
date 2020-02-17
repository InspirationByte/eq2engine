//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: View renderer for game
//////////////////////////////////////////////////////////////////////////////////

#ifndef IVIEWRENDERER_H
#define IVIEWRENDERER_H

#include "materialsystem/IMaterialSystem.h"
#include "IEngineWorld.h"
#include "ViewParams.h"
#include "RenderList.h"

#include "scene_def.h"

#define MAX_VISIBLE_LIGHTS		512			// 512 visible light sources for DS

enum ViewDrawMode_e
{
	VDM_AMBIENT = 0,				// ambient rendering, GBuffer fill
	VDM_AMBIENTMODULATE,			// ambient rendering, GBuffer fill
	VDM_SHADOW,						// shadowmap render
	VDM_LIGHTING,					// lighting rendering, DS ambient + lights + forward translucent
};

enum VRSkinType_e
{
	VR_SKIN_NONE = 0,		// no skinning
	VR_SKIN_EGFMODEL,		// egf model skinning shader
	VR_SKIN_SIMPLEMODEL,	// simple model skinning shader
};

enum ViewRenderFlags_e
{
	VR_FLAG_ORTHOGONAL			= (1 << 0),	// build orthogonal projection instead of perspective
	VR_FLAG_CUBEMAP				= (1 << 1), // cubemap mode
	VR_FLAG_ONLY_SKY			= (1 << 3), // renders only sky
	VR_FLAG_NO_MATERIALS		= (1 << 4), // don't bind any materials and don't reset the shader api
	VR_FLAG_NO_TRANSLUCENT		= (1 << 5),	// exclude translucent materials from rendering
	VR_FLAG_NO_OPAQUE			= (1 << 6),	// exclude opaque materials from rendering
	VR_FLAG_SKIPVISUPDATE		= (1 << 7),	// skips visibility update
	VR_FLAG_MODEL_LIST			= (1 << 8), // is model list (render list flag), for instancing support
	VR_FLAG_NO_MODULATEDECALS	= (1 << 9),	// engine flag, prevents modulate decals rendering
	VR_FLAG_WATERREFLECTION		= (1 << 10), // flag used by water reflection rendering
	VR_FLAG_CUSTOM_FRUSTUM		= (1 << 11), // disables frustum computation by BuildViewMatrices

	// next flags are model rendering flags, used by game
	VR_FLAG_ALWAYSVISIBLE		= (1 << 12),
};

class IEqModel;
struct decal_t;

//------------------------------------------------
// View renderer class
//------------------------------------------------
class IViewRenderer
{
public:
	//------------------------------------------------
	// Rendering resources. Fully controlled by engine
	//------------------------------------------------

	// called when map is loading
	virtual void					InitializeResources() = 0;

	// called when world is unloading
	virtual void					ShutdownResources() = 0;

	//------------------------------------------------

	// sets view
	virtual void					SetView(CViewParams* pParams) = 0;

	// returns current view
	virtual CViewParams*			GetView() = 0;

	// sets scene info
	virtual void					SetSceneInfo(sceneinfo_t &info) = 0;

	// returns scene info
	virtual sceneinfo_t&			GetSceneInfo() = 0;

#ifndef EDITOR
	// draws world
	virtual void					DrawWorld(int nRenderFlags) = 0;
#endif

#ifdef EQLC
	virtual void					SetLightmapTextureIndex(int nLMIndex) = 0;
#endif // EQLC

	// draws render list using the current view, also you can specify the render target
	virtual void					DrawRenderList(IRenderList* pRenderList, int nRenderFlags) = 0;

	// draws model part with existing world transform (used internally by renderers)
	virtual void					DrawModelPart(IEqModel*	pModel, int nModel, int nGroup, int nRenderFlags, slight_t* pStaticLights, Matrix4x4* pBones) = 0;

	//------------------------------------------------

	// allocates new light
	virtual dlight_t*				AllocLight() = 0;

	// adds light to current frame
	virtual void					AddLight(dlight_t* pLight) = 0;

	// removes light
	virtual void					RemoveLight(int idx) = 0;

	// updates lights
	virtual void					UpdateLights(float fDt) = 0;

	// retuns light
	virtual dlight_t*				GetLight(int index) = 0;

	// returns light count
	virtual int						GetLightCount() = 0;

	// draws deferred ambient
	virtual void					DrawDeferredAmbient() = 0;

	// draws deferred lighting
	virtual void					DrawDeferredCurrentLighting(bool bShadowLight) = 0;

	// draws debug data
	virtual void					DrawDebug() = 0;

	// updates material
	virtual void					UpdateDepthSetup(bool bUpdateRT = false, VRSkinType_e nSkin = VR_SKIN_NONE) = 0;

	//------------------------------------------------

	// sets the render mode
	virtual void					SetDrawMode(ViewDrawMode_e mode) = 0;

	// returns draw mode
	virtual ViewDrawMode_e			GetDrawMode() = 0;

	// sets cubemap texture index
	virtual void					SetCubemapIndex( int nCubeIndex ) = 0;

	// returns the specified matrix
	virtual Matrix4x4				GetMatrix(ER_MatrixMode mode) = 0;

	// returns view-projection matrix
	virtual Matrix4x4				GetViewProjectionMatrix() = 0;

	// view frustum
	virtual Volume&					GetViewFrustum() = 0;

	//------------------------------------------------

	// builds view matrices before rendering of view
	virtual void					BuildViewMatrices(int width, int height, int nRenderFlags) = 0;

	// backbuffer viewport size
	virtual void					SetViewportSize(int width, int height, bool bSetDeviceViewport = true) = 0;

	// backbuffer viewport size
	virtual void					GetViewportSize(int &width, int &height) = 0;

	// sets the rendertargets
	virtual void					SetupRenderTargets(int nNumRTs = 0, ITexture** ppRenderTargets = NULL, ITexture* pDepthTarget = NULL, int* nCubeFaces = NULL) = 0;

	// sets rendertargets to backbuffers
	virtual void					SetupRenderTargetToBackbuffer() = 0;

	// returns the current set rendertargets
	virtual void					GetCurrentRenderTargets(int* nNumRTs, ITexture** pRenderTargets, 
															ITexture** pDepthTarget, 
															int* nCubeFaces) = 0;

	//------------------------------------------------

	// resets skinning state
	virtual void					SetModelInstanceChanged() = 0;

	// returns lighting color and direction
	virtual void					GetLightingSampleAtPosition(const Vector3D &position, ColorRGB &color) = 0;

	// fills array with eight nearest lights to point
	virtual void					GetNearestEightLightsForPoint(const Vector3D &position, slight_t *pLights) = 0;

	// sets new area list
	virtual void					SetAreaList(renderAreaList_t* pList) = 0;

	// returns current area list
	virtual renderAreaList_t*		GetAreaList() = 0;

	// update visibility from current view
	virtual void					UpdateAreaVisibility( int nRenderFlags ) = 0;

	// updates visibility for render list
	virtual void					UpdateRenderlistVisibility( IRenderList* pRenderList ) = 0;

	// resets visibility state
	virtual void					ResetAreaVisibilityStates() = 0;

	// returns room count and room ids
	virtual int						GetViewRooms(int rooms[2]) = 0;

	// returns screen rectangle (it's here because it's single and already subivided for better fillrate)
	virtual Vertex2D_t*				GetViewSpaceMesh(int* size) = 0;
};

// extern the view renderer, as like the old 'scenerenderer'
extern IViewRenderer* viewrenderer;

#endif // IVIEWRENDERER_H