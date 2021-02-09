//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium material sub-rendering system
//
// Features: - Cached texture loading
//			 - Material management
//			 - Shader management
//////////////////////////////////////////////////////////////////////////////////

#ifndef CMATERIALSYSTEM_H
#define CMATERIALSYSTEM_H

#include <unordered_map>

#include "IMaterialSystem.h"
#include "DynamicMesh.h"

#include "utils/DkList.h"
#include "platform/Platform.h"
#include "material.h"

#include "scene_def.h"

#include "utils/eqthread.h"

using namespace Threading;

struct proxyfactory_t
{
	char* name;
	PROXY_DISPATCHER disp;
};

struct shaderoverride_t
{
	char* shadername;
	DISPATCH_OVERRIDE_SHADER	function;
};

class IRenderLibrary;

typedef std::unordered_map<ushort,IRenderState*> blendStateMap_t;
typedef std::unordered_map<ubyte,IRenderState*> depthStateMap_t;
typedef std::unordered_map<ubyte,IRenderState*> rasterStateMap_t;

struct DKMODULE;

class CMaterialSystem : public IMaterialSystem
{
	friend class CMaterial;

public:
	CMaterialSystem();
	~CMaterialSystem();

	// Initialize material system
	// materialsDirectory - you can determine a full path on hard disk
	// szShaderAPI - shader API that will be used. On NULL will set to default Shader API (DX9)
	// config - material system configuration. Must be fully filled
	bool							Init(	const char* materialsDirectory,
													const char* szShaderAPI,
													matsystem_render_config_t &config
													);

	// shutdowns material system, unloading all.
	void							Shutdown();

	// Loads and adds new shader library.
	bool							LoadShaderLibrary(const char* libname);

	// is matsystem in stub mode? (no rendering)
	bool							IsInStubMode();

	// returns configuration that can be modified in realtime (shaderapi settings can't be modified)
	matsystem_render_config_t&		GetConfiguration();

	// returns material path
	const char*						GetMaterialPath() const;

	//-----------------------------
	// Resource operations
	//-----------------------------

	// returns the default material capable to use with MatSystem's GetDynamicMesh()
	IMaterial*						GetDefaultMaterial() const;

	// returns white texture (used for wireframe of shaders that can't use FFP modes,notexture modes, etc.)
	ITexture*						GetWhiteTexture() const;

	// returns luxel test texture (used for lightmap test)
	ITexture*						GetLuxelTestTexture() const;

	// creates new material with defined parameters
	IMaterial*						CreateMaterial(const char* szMaterialName, kvkeybase_t* params);

	// Finds or loads material (if findExisting is false then it will be loaded as new material instance)
	IMaterial*						GetMaterial(const char* szMaterialName);

	// checks material for existence
	bool							IsMaterialExist(const char* szMaterialName);

	// Creates material system shader
	IMaterialSystemShader*			CreateShaderInstance(const char* szShaderName);

	// Loads textures, compiles shaders. Called after level loading
	void							PreloadNewMaterials();

	// begins preloading zone of materials when GetMaterial calls
	void							BeginPreloadMarker();

	// ends preloading zone of materials when GetMaterial calls
	void							EndPreloadMarker();

	// waits for material loader thread is finished
	void							Wait();

	// loads material or sends it to loader thread
	void							PutMaterialToLoadingQueue(IMaterial* pMaterial);

	// returns material count which is currently loading or awaiting for load
	int								GetLoadingQueue() const;

	// Reloads material vars, shaders and textures without touching a material pointers.
	void							ReloadAllMaterials();

	// releases non-used materials
	void							ReleaseUnusedMaterials();

	// Frees materials or decrements it's reference count
	void							FreeMaterial(IMaterial *pMaterial);

	void							FreeMaterials();
	void							ClearRenderStates();

	//-----------------------------

	// returns the dynamic mesh
	IDynamicMesh*					GetDynamicMesh() const;

	//-----------------------------
	// Helper rendering operations
	// TODO: remove this
	//-----------------------------

	// draws primitives
	void							DrawPrimitivesFFP(	ER_PrimitiveType type, Vertex3D_t *pVerts, int nVerts,
														ITexture* pTexture = NULL, const ColorRGBA &color = color4_white,
														BlendStateParam_t* blendParams = NULL, DepthStencilStateParams_t* depthParams = NULL,
														RasterizerStateParams_t* rasterParams = NULL);

	// draws primitives for 2D
	void							DrawPrimitives2DFFP(	ER_PrimitiveType type, Vertex2D_t *pVerts, int nVerts,
															ITexture* pTexture = NULL, const ColorRGBA &color = color4_white,
															BlendStateParam_t* blendParams = NULL, DepthStencilStateParams_t* depthParams = NULL,
															RasterizerStateParams_t* rasterParams = NULL);

	//-----------------------------
	// Shader dynamic states
	//-----------------------------

	ER_CullMode						GetCurrentCullMode();
	void							SetCullMode(ER_CullMode cullMode);

	void							SetSkinningEnabled( bool bEnable );
	bool							IsSkinningEnabled();

	void							SetInstancingEnabled( bool bEnable );
	bool							IsInstancingEnabled();


	void							SetFogInfo(const FogInfo_t &info);
	void							GetFogInfo(FogInfo_t &info);

	void							SetAmbientColor(const ColorRGBA &color);
	ColorRGBA						GetAmbientColor();

	void							SetLight(dlight_t* pLight);
	dlight_t*						GetLight();

	// lighting/shading model selection
	void							SetCurrentLightingModel(EMaterialLightingMode lightingModel);
	EMaterialLightingMode			GetCurrentLightingModel();

	//---------------------------
	// $env_cubemap texture for use in shaders
	void							SetEnvironmentMapTexture( ITexture* pEnvMapTexture );
	ITexture*						GetEnvironmentMapTexture();

	//-----------------------------
	// RHI render states setup
	//-----------------------------

	// sets blending
	void							SetBlendingStates(const BlendStateParam_t& blend);

	// sets depth stencil state
	void							SetDepthStates(const DepthStencilStateParams_t& depth);

	// sets rasterizer extended mode
	void							SetRasterizerStates(const RasterizerStateParams_t& raster);


	// sets blending
	void							SetBlendingStates(	ER_BlendFactor nSrcFactor,
																ER_BlendFactor nDestFactor,
																ER_BlendFunction nBlendingFunc = BLENDFUNC_ADD,
																int colormask = COLORMASK_ALL
																);

	// sets depth stencil state
	void							SetDepthStates(	bool bDoDepthTest,
															bool bDoDepthWrite,
															ER_CompareFunc depthCompFunc = COMP_LEQUAL);

	// sets rasterizer extended mode
	void							SetRasterizerStates(	ER_CullMode nCullMode,
																	ER_FillMode nFillMode = FILL_SOLID,
																	bool bMultiSample = true,
																	bool bScissor = false,
																	bool bPolyOffset = false
																	);

	//------------------
	// Materials or shader static states

	IMaterial*						GetBoundMaterial();

	void							SetShaderParameterOverriden(ShaderDefaultParams_e param, bool set = true);

	bool							BindMaterial( IMaterial* pMaterial, int flags = MATERIAL_BIND_PREAPPLY);
	void							Apply();

	// sets the custom rendering callbacks
	// useful for proxy updates, setting up constants that shader objects can't access by themselves
	void							SetMaterialRenderParamCallback( IMaterialRenderParamCallbacks* callback );
	IMaterialRenderParamCallbacks*	GetMaterialRenderParamCallback();

	//-----------------------------
	// Rendering projection helper operations

	// sets up a 2D mode (also sets up view and world matrix)
	void							Setup2D(float wide, float tall);

	// sets up 3D mode, projection
	void							SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar);

	// sets up 3D mode, orthogonal
	void							SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar);

	// sets up a matrix, projection, view, and world
	void							SetMatrix(ER_MatrixMode mode, const Matrix4x4 &matrix);

	// returns a typed matrix
	void							GetMatrix(ER_MatrixMode mode, Matrix4x4 &matrix);

	// retunrs multiplied matrix
	void							GetWorldViewProjection(Matrix4x4 &matrix);

	//-----------------------------
	// Swap chains
	//-----------------------------

	// tells device to begin frame
	bool							BeginFrame();

	// tells device to end and present frame. Also swapchain can be overriden.
	bool							EndFrame(IEqSwapChain* swapChain);

	// resizes device back buffer. Must be called if window resized, etc
	void							SetDeviceBackbufferSize(int wide, int tall);

	// reports device focus state to render lib
	void							SetDeviceFocused(bool inFocus);

	// creates additional swap chain
	IEqSwapChain*					CreateSwapChain(void* windowHandle);

	void							DestroySwapChain(IEqSwapChain* chain);

	// captures screenshot to CImage data
	bool							CaptureScreenshot( CImage &img );

	//-----------------------------
	// Internal operations
	//-----------------------------

	// returns RHI device interface
	IShaderAPI*						GetShaderAPI();

	void							RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName);
	IMaterialProxy*					CreateProxyByName(const char* pszName);

	void							RegisterShader(const char* pszShaderName,DISPATCH_CREATE_SHADER dispatcher_creation);
	void							RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function);

	// use this if you want to reduce "frametime jumps" when matsystem loads textures
	void							SetResourceBeginEndLoadCallback(RESOURCELOADCALLBACK begin, RESOURCELOADCALLBACK end);

	// device lost/restore callbacks
	void							AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore);
	void							RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore);

	void							PrintLoadedMaterials();

	bool							IsInitialized() const {return (m_renderLibrary != NULL);}
	const char*						GetInterfaceName() const {return MATSYSTEM_INTERFACE_VERSION;}

private:

	void							CreateWhiteTexture();
	void							InitDefaultMaterial();

	matsystem_render_config_t		m_config;

	IRenderLibrary*					m_renderLibrary;					// render library.
	DKMODULE*						m_rendermodule;					// render dll.

	IShaderAPI*						m_shaderAPI;					// the main renderer interface
	EqString						m_materialsPath;				// material path

	DkList<DKMODULE*>				m_shaderLibs;				// loaded shader libraries

	DkList<shaderfactory_t>			m_shaderFactoryList;						// registered shaders
	DkList<shaderoverride_t>		m_shaderOverrideList;			// shader override functors
	DkList<proxyfactory_t>			m_proxyFactoryList;

	DkList<IMaterial*>				m_loadedMaterials;				// loaded material list
	ER_CullMode						m_cullMode;				// culling mode. For shaders. TODO: remove, and check matrix handedness.

	CDynamicMesh					m_dynamicMesh;

	//-------------------------------------------------------------------------

	blendStateMap_t					m_blendStates;
	depthStateMap_t					m_depthStates;
	rasterStateMap_t				m_rasterStates;

	IMaterialRenderParamCallbacks*	m_preApplyCallback;

	EMaterialLightingMode			m_curentLightingModel;	// dynamic-changeable lighting model. Used as state
	bool							m_skinningEnabled;
	bool							m_instancingEnabled;

	Matrix4x4						m_matrices[5];					// matrix modes

	IMaterial*						m_setMaterial;				// currently binded material
	uint							m_paramOverrideMask;			// parameter setup mask for overrides


	IMaterial*						m_overdrawMaterial;

	ITexture*						m_currentEnvmapTexture;

	DkList<DEVLICELOSTRESTORE>		m_lostDeviceCb;
	DkList<DEVLICELOSTRESTORE>		m_restoreDeviceCb;

	ITexture*						m_whiteTexture;
	ITexture*						m_luxelTestTexture;

	IMaterial*						m_pDefaultMaterial;

	FogInfo_t						m_fogInfo;
	ColorRGBA						m_ambColor;

	dlight_t*						m_currentLight;

	bool							m_deviceActiveState;

	CEqMutex						m_ProxyMutex[4];
	CEqMutex						m_Mutex;

	bool							m_forcePreloadMaterials;
	uint							m_frame;
};

#endif //CMATERIALSYSTEM_H
