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

#pragma once
#include "materialsystem1/IMaterialSystem.h"
#include "DynamicMesh.h"

struct proxyfactory_t
{
	EqString name;
	PROXY_DISPATCHER disp;
};

struct shaderoverride_t
{
	EqString shadername;
	DISPATCH_OVERRIDE_SHADER	function;
};

class IRenderLibrary;
class IRenderState;
class CMaterial;
struct DKMODULE;

class CMaterialSystem : public IMaterialSystem
{
	friend class CMaterial;

public:
	CMaterialSystem();
	~CMaterialSystem();

	// Initialize material system
	// szShaderAPI - shader API that will be used. On NULL will set to default Shader API (DX9)
	// config - material system configuration. Must be fully filled
	bool							Init(const matsystem_init_config_t& config);

	// shutdowns material system, unloading all.
	void							Shutdown();

	// Loads and adds new shader library.
	bool							LoadShaderLibrary(const char* libname);

	// is matsystem in stub mode? (no rendering)
	bool							IsInStubMode() const;

	// returns configuration that can be modified in realtime (shaderapi settings can't be modified)
	matsystem_render_config_t&		GetConfiguration();

	// returns material path
	const char*						GetMaterialPath() const;
	const char*						GetMaterialSRCPath() const;

	//-----------------------------
	// Resource operations
	//-----------------------------

	// returns the default material capable to use with MatSystem's GetDynamicMesh()
	const IMaterialPtr&				GetDefaultMaterial() const;

	// returns white texture (used for wireframe of shaders that can't use FFP modes,notexture modes, etc.)
	const ITexturePtr&				GetWhiteTexture() const;

	// returns luxel test texture (used for lightmap test)
	const ITexturePtr&				GetLuxelTestTexture() const;

	// creates new material with defined parameters
	IMaterialPtr					CreateMaterial(const char* szMaterialName, KVSection* params);

	// Finds or loads material (if findExisting is false then it will be loaded as new material instance)
	IMaterialPtr					GetMaterial(const char* szMaterialName);

	// checks material for existence
	bool							IsMaterialExist(const char* szMaterialName) const;

	// Creates material system shader
	IMaterialSystemShader*			CreateShaderInstance(const char* szShaderName);

	// Loads textures, compiles shaders. Called after level loading
	void							PreloadNewMaterials();

	// waits for material loader thread is finished
	void							Wait();

	// loads material or sends it to loader thread
	void							PutMaterialToLoadingQueue(const IMaterialPtr& pMaterial);

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
														const ITexturePtr& pTexture = nullptr, const ColorRGBA &color = color_white,
														BlendStateParam_t* blendParams = nullptr, DepthStencilStateParams_t* depthParams = nullptr,
														RasterizerStateParams_t* rasterParams = nullptr);

	// draws primitives for 2D
	void							DrawPrimitives2DFFP(	ER_PrimitiveType type, Vertex2D_t *pVerts, int nVerts,
															const ITexturePtr& pTexture = nullptr, const ColorRGBA &color = color_white,
															BlendStateParam_t* blendParams = nullptr, DepthStencilStateParams_t* depthParams = nullptr,
															RasterizerStateParams_t* rasterParams = nullptr);

	//-----------------------------
	// Shader dynamic states
	//-----------------------------

	ER_CullMode						GetCurrentCullMode() const;
	void							SetCullMode(ER_CullMode cullMode);

	void							SetSkinningEnabled( bool bEnable );
	bool							IsSkinningEnabled() const;

	void							SetInstancingEnabled( bool bEnable );
	bool							IsInstancingEnabled() const;


	void							SetFogInfo(const FogInfo_t &info);
	void							GetFogInfo(FogInfo_t &info) const;

	void							SetAmbientColor(const ColorRGBA &color);
	ColorRGBA						GetAmbientColor() const;

	void							SetLight(dlight_t* pLight);
	dlight_t*						GetLight() const;

	// lighting/shading model selection
	void							SetCurrentLightingModel(EMaterialLightingMode lightingModel);
	EMaterialLightingMode			GetCurrentLightingModel() const;

	//---------------------------
	// $env_cubemap texture for use in shaders
	void							SetEnvironmentMapTexture(const ITexturePtr& pEnvMapTexture);
	const ITexturePtr&				GetEnvironmentMapTexture() const;

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
															ER_CompareFunc depthCompFunc = COMPFUNC_LEQUAL);

	// sets rasterizer extended mode
	void							SetRasterizerStates(	ER_CullMode nCullMode,
																	ER_FillMode nFillMode = FILL_SOLID,
																	bool bMultiSample = true,
																	bool bScissor = false,
																	bool bPolyOffset = false
																	);

	//------------------
	// Materials or shader static states

	void							SetProxyDeltaTime(float deltaTime);

	IMaterialPtr					GetBoundMaterial() const;

	void							SetShaderParameterOverriden(int param, bool set = true);

	// global variables
	MatVarProxyUnk					FindGlobalMaterialVar(int nameHash) const;
	MatVarProxyUnk					FindGlobalMaterialVarByName(const char* pszVarName) const;
	MatVarProxyUnk					GetGlobalMaterialVarByName(const char* pszVarName, const char* defaultValue);

	bool							BindMaterial(IMaterial* pMaterial, int flags = MATERIAL_BIND_PREAPPLY);
	void							Apply();

	// sets the custom rendering callbacks
	// useful for proxy updates, setting up constants that shader objects can't access by themselves
	void							SetMaterialRenderParamCallback( IMaterialRenderParamCallbacks* callback );
	IMaterialRenderParamCallbacks*	GetMaterialRenderParamCallback() const;

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
	bool							BeginFrame(IEqSwapChain* swapChain);

	// tells device to end and present frame. Also swapchain can be overriden.
	bool							EndFrame();

	// resizes device back buffer. Must be called if window resized, etc
	void							SetDeviceBackbufferSize(int wide, int tall);

	// reports device focus state to render lib
	void							SetDeviceFocused(bool inFocus);

	// creates additional swap chain
	IEqSwapChain*					CreateSwapChain(void* windowHandle);

	void							DestroySwapChain(IEqSwapChain* chain);

	// window/fullscreen mode changing; returns false if fails
	bool							SetWindowed(bool enable);
	bool							IsWindowed() const;
	
	// captures screenshot to CImage data
	bool							CaptureScreenshot( CImage &img );

	//-----------------------------
	// Internal operations
	//-----------------------------

	// returns RHI device interface
	IShaderAPI*						GetShaderAPI() const;

	void							RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName);
	IMaterialProxy*					CreateProxyByName(const char* pszName);

	void							RegisterShader(const char* pszShaderName,DISPATCH_CREATE_SHADER dispatcher_creation);
	void							RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function);

	// device lost/restore callbacks
	void							AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore);
	void							RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore);

	void							PrintLoadedMaterials();

	bool							IsInitialized() const {return (m_renderLibrary != nullptr);}
	const char*						GetInterfaceName() const {return MATSYSTEM_INTERFACE_VERSION;}

private:

	void							CreateMaterialInternal(CRefPtr<CMaterial> material, KVSection* params);
	void							CreateWhiteTexture();
	void							InitDefaultMaterial();

	matsystem_render_config_t		m_config;

	IRenderLibrary*					m_renderLibrary;			// render library.
	DKMODULE*						m_rendermodule;				// render dll.

	IShaderAPI*						m_shaderAPI;				// the main renderer interface
	EqString						m_materialsPath;			// material path
	EqString						m_materialsSRCPath;			// material sources path

	MaterialVarBlock				m_globalMaterialVars;

	Array<DKMODULE*>				m_shaderLibs{ PP_SL };				// loaded shader libraries
	Array<shaderfactory_t>			m_shaderFactoryList{ PP_SL };		// registered shaders
	Array<shaderoverride_t>			m_shaderOverrideList{ PP_SL };		// shader override functors
	Array<proxyfactory_t>			m_proxyFactoryList{ PP_SL };

	Map<int, IMaterial*>			m_loadedMaterials{ PP_SL };			// loaded material list
	ER_CullMode						m_cullMode;					// culling mode. For shaders. TODO: remove, and check matrix handedness.

	CDynamicMesh					m_dynamicMesh;

	//-------------------------------------------------------------------------

	Map<ushort, IRenderState*>		m_blendStates{ PP_SL };
	Map<ushort, IRenderState*>		m_depthStates{ PP_SL };
	Map<ushort, IRenderState*>		m_rasterStates{ PP_SL };

	IMaterialRenderParamCallbacks*	m_preApplyCallback;

	EMaterialLightingMode			m_curentLightingModel;		// dynamic-changeable lighting model. Used as state

	Matrix4x4						m_matrices[5];				// matrix modes

	IMaterialPtr					m_pDefaultMaterial;
	IMaterialPtr					m_overdrawMaterial;
	IMaterialPtr					m_setMaterial;				// currently binded material
	uint							m_paramOverrideMask;		// parameter setup mask for overrides

	ITexturePtr						m_currentEnvmapTexture;
	ITexturePtr						m_whiteTexture;
	ITexturePtr						m_luxelTestTexture;

	Array<DEVLICELOSTRESTORE>		m_lostDeviceCb{ PP_SL };
	Array<DEVLICELOSTRESTORE>		m_restoreDeviceCb{ PP_SL };

	FogInfo_t						m_fogInfo;
	ColorRGBA						m_ambColor;

	dlight_t*						m_currentLight;

	CEqTimer						m_proxyTimer;

	uint							m_frame;
	float							m_proxyDeltaTime;

	bool							m_skinningEnabled;
	bool							m_instancingEnabled;
	bool							m_deviceActiveState;
};
