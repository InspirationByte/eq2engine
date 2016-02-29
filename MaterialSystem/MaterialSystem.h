//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech material sub-rendering system
//
// Features: - Cached texture loading
//			 - Material management
//			 - Shader management
//////////////////////////////////////////////////////////////////////////////////

#ifndef CMATERIALSYSTEM_H
#define CMATERIALSYSTEM_H

#include "utils/DkList.h"
#include "platform/Platform.h"
#include "materialsystem/IMaterialSystem.h"
#include "material.h"
#include <map>

#include "utils/eqthread.h"

using namespace Threading;

struct shaderoverride_t
{
	char* shadername;
	DISPATCH_OVERRIDE_SHADER	function;
};

class IRenderLibrary;

typedef std::map<ushort,IRenderState*> blendStateMap_t;
typedef std::map<ubyte,IRenderState*> depthStateMap_t;
typedef std::map<ubyte,IRenderState*> rasterStateMap_t;

struct DKMODULE;

class CMaterialSystem : public IMaterialSystem
{
public:
	CMaterialSystem();

	~CMaterialSystem();

	friend class CMaterial;

	bool							Init(	const char* materialsDirectory,
											const char* szShaderAPI,
											matsystem_render_config_t &config
									);			// Initializes material system

	void							Shutdown();

	matsystem_render_config_t&		GetConfiguration();

	bool							IsInStubMode();							// is matsystem in stub mode?

	// returns material path
	char*							GetMaterialPath() {return (char*)m_szMaterialsdir.GetData();}

	bool							LoadShaderLibrary(const char* libname);	// Adds new shader library

	IMaterial*						FindMaterial(const char* szMaterialName,bool findExisting = false);	// Finds material

	bool							IsMaterialExist(const char* szMaterialName);

	// Finds shader
	IMaterialSystemShader*			CreateShaderInstance(const char* szShaderName);

	// Preload materials that loaded, but not initialized
	void							PreloadNewMaterials();

	// begins preloading zone of materials when FindMaterial calls
	void							BeginPreloadMarker();

	// ends preloading zone of materials when FindMaterial calls
	void							EndPreloadMarker();

	// loads material or sends it to loader thread
	void							PutMaterialToLoadingQueue(IMaterial* pMaterial);

	int								GetLoadingQueue() const;

	// Reloads materials
	void							ReloadAllMaterials(bool bTouchTextures = true,bool bTouchShaders = true, bool wait = false);

	// Reloads textures
	void							ReloadAllTextures();

	// Frees all textures
	void							FreeAllTextures();

	// Frees all materials (if bFreeAll is positive, will free locked)
	void							FreeMaterials(bool bFreeAll = false);

	// Frees material
	void							FreeMaterial(IMaterial *pMaterial);

	// Setting rendering material. Applies shader constants and more needed things
	// NOTENOTE: Do reset() though renderer.
	bool							BindMaterial(IMaterial *pMaterial,bool doApply = true);

	// Applies current material
	void							Apply();

	// returns bound material
	IMaterial*						GetBoundMaterial();

	// Registers new shader
	void							RegisterShader(const char* pszShaderName,DISPATCH_CREATE_SHADER dispatcher_creation);

	// registers overrider for shaders
	void							RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function);

	// Culling mode for model
	CullMode_e						GetCurrentCullMode()				{return m_nCurrentCullMode;}
	void							SetCullMode(CullMode_e cullMode)	{m_nCurrentCullMode = cullMode;}

	// skinning mode
	void							SetSkinningEnabled( bool bEnable );
	bool							IsSkinningEnabled();

	// instancing mode
	void							SetInstancingEnabled( bool bEnable );
	bool							IsInstancingEnabled();

	// Lighting model (e.g shadow maps or light maps)
	void							SetLightingModel(MaterialLightingMode_e lightingModel);

	// Lighting model (e.g shadow maps or light maps)
	MaterialLightingMode_e			GetLightingModel();

	// transform operations
	void							SetMatrix(MatrixMode_e mode, const Matrix4x4 &matrix);							// sets up a matrix, projection, view, and world
	void							GetMatrix(MatrixMode_e mode, Matrix4x4 &matrix);							// returns a typed matrix

	void							GetWorldViewProjection(Matrix4x4 &matrix);									// retunrs multiplied matrix

	void							SetAmbientColor(const ColorRGBA &color);											// sets an ambient light
	ColorRGBA						GetAmbientColor();

	void							SetLight(dlight_t* pLight);													// sets current light for processing in shaders
	dlight_t*						GetLight();

	void							SetCurrentLightingModel(MaterialLightingMode_e lightingModel);				// sets current lighting model as state
	MaterialLightingMode_e			GetCurrentLightingModel();													// returns current lighting model state

	// sets pre-apply callback
	void							SetMaterialRenderParamCallback( IMaterialRenderParamCallbacks* callback );

	// returns current pre-apply callback
	IMaterialRenderParamCallbacks*	GetMaterialRenderParamCallback();

	// sets $env_cubemap texture for use in shaders
	void							SetEnvironmentMapTexture( ITexture* pEnvMapTexture );

	// returns $env_cubemap texture used in shaders
	ITexture*						GetEnvironmentMapTexture();

	ITexture*						GetWhiteTexture();
	ITexture*						GetLuxelTestTexture();


	//-----------------------------
	// Frame operations
	//-----------------------------

	bool							BeginFrame();																// tells 3d device to begin frame
	bool							EndFrame(IEqSwapChain* swapChain = NULL);																	// tells 3d device to end and present frame

	void							SetDeviceBackbufferSize(int wide, int tall);								// resizes device back buffer. Must be called if window resized

	// creates additional swap chain
	IEqSwapChain*					CreateSwapChain(void* windowHandle);
	void							DestroySwapChain(IEqSwapChain* swapChain);

	//-----------------------------

	void							SetFogInfo(const FogInfo_t &info);												// sets a fog info
	void							GetFogInfo(FogInfo_t &info);												// returns fog info

	//-----------------------------
	// Helper operations
	//-----------------------------

	void							Setup2D(float wide, float tall);														// sets up a 2D mode
	void							SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar);			// sets up 3D mode, projection
	void							SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar);	// sets up 3D mode, orthogonal

	//-----------------------------
	// Helper rendering operations (warning, slow)
	//-----------------------------

	void							DrawPrimitivesFFP(PrimitiveType_e type, Vertex3D_t *pVerts, int nVerts,
												ITexture* pTexture = NULL, const ColorRGBA &color = color4_white,
												BlendStateParam_t* blendParams = NULL, DepthStencilStateParams_t* depthParams = NULL,
												RasterizerStateParams_t* rasterParams = NULL);			// draws 3D primitives

	void							DrawPrimitives2DFFP(PrimitiveType_e type, Vertex2D_t *pVerts, int nVerts,
												ITexture* pTexture = NULL, const ColorRGBA &color = color4_white,
												BlendStateParam_t* blendParams = NULL, DepthStencilStateParams_t* depthParams = NULL,
												RasterizerStateParams_t* rasterParams = NULL);			// draws 2D primitives

//-----------------------------
// State setup
//-----------------------------

	// sets blending
	void							SetBlendingStates(const BlendStateParam_t& blend);

	// sets depth stencil state
	void							SetDepthStates(const DepthStencilStateParams_t& depth);

	// sets rasterizer extended mode
	void							SetRasterizerStates(const RasterizerStateParams_t& raster);

	// sets blending
	void							SetBlendingStates(	BlendingFactor_e nSrcFactor,
														BlendingFactor_e nDestFactor,
														BlendingFunction_e nBlendingFunc,
														int colormask = COLORMASK_ALL
														);

	// sets depth stencil state
	void							SetDepthStates(	bool bDoDepthTest,
													bool bDoDepthWrite,
													CompareFunc_e depthCompFunc = COMP_LEQUAL);

	// sets rasterizer extended mode
	void							SetRasterizerStates(CullMode_e nCullMode,
														FillMode_e nFillMode = FILL_SOLID,
														bool bMultiSample = true,
														bool bScissor = false
														);

//**********************************
// Material system draw*() functions
//**********************************

	// return proxy factory interface
	IProxyFactory*					GetProxyFactory() {return proxyfactory;}

	// returns shader interface of this material system
	IShaderAPI*						GetShaderAPI() {return m_pShaderAPI;}

	// captures screenshot to CImage data
	bool							CaptureScreenshot( CImage &img );

	// update all materials and proxies
	void							Update( float dt );

	// waits for material loader thread is finished
	void							Wait();

	void							SetResourceBeginEndLoadCallback(RESOURCELOADCALLBACK begin, RESOURCELOADCALLBACK end);

	// use this if you have objects that must be destroyed when device is lost
	void							AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore);

	// removes callbacks from list
	void							RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore);

	// prints loaded materials to console
	void							PrintLoadedMaterials();

	bool							IsInitialized() const {return (m_pRenderLib != NULL);}
	const char*						GetInterfaceName() const {return MATSYSTEM_INTERFACE_VERSION;}

private:

	void							CreateWhiteTexture();
	void							InitDefaultMaterial();

	matsystem_render_config_t		m_config;

	IRenderLibrary*					m_pRenderLib;					// render library.
	DKMODULE*						m_rendermodule;					// render dll.

	IShaderAPI*						m_pShaderAPI;					// the main renderer interface
	EqString						m_szMaterialsdir;				// material path

	DkList<DKMODULE*>				m_hShaderLibraries;				// loaded shader libraries

	DkList<shaderfactory_t>			m_hShaders;						// registered shaders
	DkList<shaderoverride_t>		m_ShaderOverrideList;			// shader override functors

	DkList<IMaterial*>				m_pLoadedMaterials;				// loaded material list
	CullMode_e						m_nCurrentCullMode;				// culling mode. For shaders. TODO: remove, and check matrix handedness.

	//-------------------------------------------------------------------------

	blendStateMap_t					m_blendStates;
	depthStateMap_t					m_depthStates;
	rasterStateMap_t				m_rasterStates;

	IMaterialRenderParamCallbacks*	m_preApplyCallback;

	MaterialLightingMode_e			m_nCurrentLightingShaderModel;	// dynamic-changeable lighting model. Used as state
	bool							m_bIsSkinningEnabled;
	bool							m_instancingEnabled;

	Matrix4x4						m_matrices[4];					// matrix modes

	IMaterial*						m_pCurrentMaterial;				// currently binded material
	IMaterial*						m_pOverdrawMaterial;

	ITexture*						m_pEnvmapTexture;

	int								m_nMaterialChanges;				// material binds per frame count

	DkList<DEVLICELOSTRESTORE>		m_pDeviceLostCb;
	DkList<DEVLICELOSTRESTORE>		m_pDeviceRestoreCb;

	ITexture*						m_pWhiteTexture;
	ITexture*						m_pLuxelTestTexture;

	IMaterial*						m_pDefaultMaterial;

	FogInfo_t						m_fogInfo;
	ColorRGBA						m_ambColor;

	dlight_t*						m_pCurrentLight;

	bool							m_bDeviceState;

	float							m_fCurrFrameTime;

	CEqMutex						m_Mutex;

	bool							m_bPreloadingMarker;
	uint32							m_frame;
};

#endif //CMATERIALSYSTEM_H
