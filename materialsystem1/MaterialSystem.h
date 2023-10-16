//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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

struct ShaderProxyFactory
{
	EqString name;
	PROXY_DISPATCHER disp;
};

struct ShaderOverride
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
	bool							Init(const MaterialsInitSettings& config);

	// shutdowns material system, unloading all.
	void							Shutdown();

	// Loads and adds new shader library.
	bool							LoadShaderLibrary(const char* libname);

	// returns configuration that can be modified in realtime (shaderapi settings can't be modified)
	MaterialsRenderSettings&		GetConfiguration();

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
	void							WaitAllMaterialsLoaded();

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
	//-----------------------------

	void							DrawDefaultUP(EPrimTopology type, int vertFVF, const void* verts, int numVerts,
													const ITexturePtr& pTexture = nullptr, const MColor &color = color_white,
													BlendStateParams* blendParams = nullptr, DepthStencilStateParams* depthParams = nullptr,
													RasterizerStateParams* rasterParams = nullptr);

	//-----------------------------
	// Shader dynamic states
	//-----------------------------

	ECullMode						GetCurrentCullMode() const;
	void							SetCullMode(ECullMode cullMode);

	void							SetSkinningEnabled( bool bEnable );
	bool							IsSkinningEnabled() const;

	void							SetInstancingEnabled( bool bEnable );
	bool							IsInstancingEnabled() const;

	void							SetFogInfo(const FogInfo &info);
	void							GetFogInfo(FogInfo &info) const;

	void							SetAmbientColor(const ColorRGBA &color);
	ColorRGBA						GetAmbientColor() const;

	void							SetEnvironmentMapTexture(const ITexturePtr& pEnvMapTexture);
	const ITexturePtr&				GetEnvironmentMapTexture() const;

	//-----------------------------
	// RHI render states setup
	//-----------------------------

	void							SetBlendingStates(const BlendStateParams& blend);
	void							SetBlendingStates(EBlendFactor nSrcFactor, EBlendFactor nDestFactor, EBlendFunction nBlendingFunc = BLENDFUNC_ADD, int colormask = COLORMASK_ALL);

	void							SetDepthStates(const DepthStencilStateParams& depth);
	void							SetDepthStates(bool bDoDepthTest, bool bDoDepthWrite, ECompareFunc depthCompFunc = COMPFUNC_LEQUAL);

	void							SetRasterizerStates(const RasterizerStateParams& raster);
	void							SetRasterizerStates(ECullMode nCullMode,EFillMode nFillMode = FILL_SOLID, bool bMultiSample = true, bool bScissor = false, bool bPolyOffset = false);

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
	void							SetRenderCallbacks( IMatSysRenderCallbacks* callback );
	IMatSysRenderCallbacks*			GetRenderCallbacks() const;

	//-----------------------------
	// Rendering projection helper operations

	// sets up a 2D mode (also sets up view and world matrix)
	void							Setup2D(float wide, float tall);

	// sets up 3D mode, projection
	void							SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar);

	// sets up 3D mode, orthogonal
	void							SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar);

	// sets up a matrix, projection, view, and world
	void							SetMatrix(EMatrixMode mode, const Matrix4x4 &matrix);

	// returns a typed matrix
	void							GetMatrix(EMatrixMode mode, Matrix4x4 &matrix);

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

private:

	void							CreateMaterialInternal(CRefPtr<CMaterial> material, KVSection* params);
	void							CreateWhiteTexture();
	void							InitDefaultMaterial();

	MaterialsRenderSettings			m_config;

	IRenderLibrary*					m_renderLibrary{ nullptr };	// render library.
	DKMODULE*						m_rendermodule{ nullptr };	// render dll.

	IShaderAPI*						m_shaderAPI{ nullptr };		// the main renderer interface
	EqString						m_materialsPath;			// material path
	EqString						m_materialsSRCPath;			// material sources path

	MaterialVarBlock				m_globalMaterialVars;

	Array<DKMODULE*>				m_shaderLibs{ PP_SL };				// loaded shader libraries
	Array<ShaderFactory>			m_shaderFactoryList{ PP_SL };		// registered shaders
	Array<ShaderOverride>			m_shaderOverrideList{ PP_SL };		// shader override functors
	Array<ShaderProxyFactory>		m_proxyFactoryList{ PP_SL };

	Map<int, IMaterial*>			m_loadedMaterials{ PP_SL };			// loaded material list
	ECullMode						m_cullMode{ CULL_BACK };			// culling mode. For shaders. TODO: remove, and check matrix handedness.

	CDynamicMesh					m_dynamicMesh;

	//-------------------------------------------------------------------------
	IVector2D						m_backbufferSize{ 800, 600 };
	Map<ushort, IRenderState*>		m_blendStates{ PP_SL };
	Map<ushort, IRenderState*>		m_depthStates{ PP_SL };
	Map<ushort, IRenderState*>		m_rasterStates{ PP_SL };

	IMatSysRenderCallbacks*			m_preApplyCallback{ nullptr };

	Matrix4x4						m_viewProjMatrix{ identity4 };
	Matrix4x4						m_wvpMatrix{ identity4 };
	Matrix4x4						m_matrices[5]{ identity4 };

	IMaterialPtr					m_defaultMaterial;
	IMaterialPtr					m_overdrawMaterial;
	IMaterialPtr					m_setMaterial;						// currently bound material
	uint							m_paramOverrideMask{ UINT_MAX };	// parameter setup mask for overrides

	ITexturePtr						m_currentEnvmapTexture;
	ITexturePtr						m_whiteTexture;
	ITexturePtr						m_luxelTestTexture;

	Array<DEVLICELOSTRESTORE>		m_lostDeviceCb{ PP_SL };
	Array<DEVLICELOSTRESTORE>		m_restoreDeviceCb{ PP_SL };

	FogInfo							m_fogInfo;
	ColorRGBA						m_ambColor;

	CEqTimer						m_proxyTimer;

	uint							m_frame{ 0 };
	float							m_proxyDeltaTime{ 0.0f };

	uint8							m_matrixDirty{ UINT8_MAX };
	bool							m_skinningEnabled{ false };
	bool							m_instancingEnabled{ false };
	bool							m_deviceActiveState{ true };
};
