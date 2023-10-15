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
#include "renderers/IShaderAPI.h"
#include "renderers/IEqSwapChain.h"

#include "IMaterial.h"
#include "IMatSysShader.h"
#include "IMaterialVar.h"
#include "IMaterialProxy.h"
#include "SceneDefs.h"
#include "RenderDefs.h"

class CImage;
class IDynamicMesh;
class IMatSysRenderCallbacks;

typedef void					(*RESOURCELOADCALLBACK)(void);
typedef const char*				(*DISPATCH_OVERRIDE_SHADER)(void);
typedef bool					(*DEVLICELOSTRESTORE)(void);
typedef IMaterialProxy*			(*PROXY_DISPATCHER)(void);

// Lighting model for material system
enum EMaterialLightingMode
{
	MATERIAL_LIGHT_UNLIT = 0,	// no lighting, fully white (also, you can use it for ambient rendering)
	MATERIAL_LIGHT_DEFERRED,	// deferred shading. Full dynamic lighting
	MATERIAL_LIGHT_FORWARD,		// forward shading, also for use in deferred, if you setting non-state lighting model, it will be fully forward
};

// material bind flags
enum EMaterialBindFlags
{
	MATERIAL_BIND_PREAPPLY = (1 << 0),
	MATERIAL_BIND_KEEPOVERRIDE = (1 << 1),
};

//-----------------------------------------------------
// material system configuration
//-----------------------------------------------------

struct MaterialsRenderSettings
{
	EMaterialLightingMode	lightingModel{ MATERIAL_LIGHT_UNLIT };

	int					materialFlushThresh{ 1000 };	// flush (unload) threshold in frames
	bool				ffpMode{ false };				// use FFP if possible (don't apply any shaders, DEBUG only)
	bool				stubMode{ false };				// run matsystem in stub mode (don't render anything)

	bool				lowShaderQuality{ false };
	bool				editormode{ false };			// enable editor mode
	bool				threadedloader{ true };

	bool				enableShadows{ true };			// enable shadows?
	bool				enableSpecular{ true };			// enable specular lighting reflections?
	bool				enableBumpmapping{ true };		// enable bump maps?

	bool				wireframeMode{ false };			// matsystem wireframe mode
	bool				overdrawMode{ false };			// matsystem overdraw mode
};

struct MaterialsInitSettings
{
	MaterialsRenderSettings	renderConfig;
	ShaderAPIParams		shaderApiParams;

	EqString			rendererName;		// shaderAPI library filename
	EqString			materialsPath;		// regular (retail) materials file paths
	EqString			materialsSRCPath;	// DEV materials file source paths
	EqString			texturePath;		// texture path (.DDS only)
	EqString			textureSRCPath;		// texture sources path (.TGA only)
};

//----------------------------------------------------------------------------------------------------------------------
// Material system inteface
//----------------------------------------------------------------------------------------------------------------------
class IMaterialSystem : public IEqCoreModule
{
public:
	CORE_INTERFACE("E1_MaterialSystem_024")

	// Initialize material system
	// szShaderAPI - shader API that will be used. On NULL will set to default Shader API (DX9)
	// config - material system configuration. Must be fully filled
	virtual bool							Init(const MaterialsInitSettings& config) = 0;

	// shutdowns material system, unloading all.
	virtual void							Shutdown() = 0;

	// Loads and adds new shader library.
	virtual bool							LoadShaderLibrary(const char* libname) = 0;

	// is matsystem in stub mode? (no rendering)
	virtual bool							IsInStubMode() const = 0;

	// returns configuration that can be modified in realtime (shaderapi settings can't be modified)
	virtual MaterialsRenderSettings&		GetConfiguration() = 0;

	// returns material path
	virtual const char*						GetMaterialPath() const = 0;
	virtual const char*						GetMaterialSRCPath() const = 0;

	//-----------------------------
	// Resource operations
	//-----------------------------

	// returns the default material capable to use with MatSystem's GetDynamicMesh()
	virtual const IMaterialPtr&				GetDefaultMaterial() const = 0;

	// returns white texture (used for wireframe of shaders that can't use FFP modes,notexture modes, etc.)
	virtual	const ITexturePtr&				GetWhiteTexture() const = 0;

	// returns luxel test texture (used for lightmap test)
	virtual	const ITexturePtr&				GetLuxelTestTexture() const = 0;

	// creates new material with defined parameters
	virtual IMaterialPtr					CreateMaterial(const char* szMaterialName, KVSection* params) = 0;

	// Finds or loads material (if findExisting is false then it will be loaded as new material instance)
	virtual IMaterialPtr					GetMaterial(const char* szMaterialName) = 0;

	// checks material for existence
	virtual bool							IsMaterialExist(const char* szMaterialName) const = 0;

	// Creates material system shader
	virtual IMaterialSystemShader*			CreateShaderInstance(const char* szShaderName) = 0;

	// Loads textures, compiles shaders. Called after level loading
	virtual void							PreloadNewMaterials() = 0;

	// waits for material loader thread is finished
	virtual void							Wait() = 0;

	// loads material or sends it to loader thread
	virtual void							PutMaterialToLoadingQueue(const IMaterialPtr& pMaterial) = 0;

	// returns material count which is currently loading or awaiting for load
	virtual int								GetLoadingQueue() const = 0;

	// Reloads material vars, shaders and textures without touching a material pointers.
	virtual void							ReloadAllMaterials() = 0;

	// releases non-used materials
	virtual void							ReleaseUnusedMaterials() = 0;

	// Frees materials
	virtual void							FreeMaterial(IMaterial* pMaterial) = 0;

	//-----------------------------

	virtual IDynamicMesh*					GetDynamicMesh() const = 0;

	//-----------------------------
	
	// draw primitives with default material
	virtual void							DrawDefaultUP(EPrimTopology type, int vertFVF, const void* verts, int numVerts,
															const ITexturePtr& texture = nullptr, const MColor &color = color_white,
															BlendStateParams* blendParams = nullptr, DepthStencilStateParams* depthParams = nullptr,
															RasterizerStateParams* rasterParams = nullptr) = 0;

	template<typename VERT>
	void									DrawDefaultUP(EPrimTopology type, const VERT* verts, int numVerts,
															const ITexturePtr& texture = nullptr, const ColorRGBA& color = color_white,
															BlendStateParams* blendParams = nullptr, DepthStencilStateParams* depthParams = nullptr,
															RasterizerStateParams* rasterParams = nullptr);

	template<typename ARRAY_TYPE>
	void									DrawDefaultUP(EPrimTopology type, const ARRAY_TYPE& verts,
															const ITexturePtr& texture = nullptr, const ColorRGBA& color = color_white,
															BlendStateParams* blendParams = nullptr, DepthStencilStateParams* depthParams = nullptr,
															RasterizerStateParams* rasterParams = nullptr);

	//-----------------------------
	// Shader dynamic states
	//-----------------------------

	virtual ECullMode						GetCurrentCullMode() const = 0;
	virtual void							SetCullMode(ECullMode cullMode) = 0;

	virtual void							SetSkinningEnabled(bool bEnable) = 0;
	virtual bool							IsSkinningEnabled() const = 0;

	virtual void							SetInstancingEnabled(bool bEnable) = 0;
	virtual bool							IsInstancingEnabled() const = 0;


	virtual void							SetFogInfo(const FogInfo& info) = 0;
	virtual void							GetFogInfo(FogInfo& info) const = 0;

	virtual void							SetAmbientColor(const ColorRGBA& color) = 0;
	virtual ColorRGBA						GetAmbientColor() const = 0;

	// lighting/shading model selection
	virtual void							SetCurrentLightingModel(EMaterialLightingMode lightingModel) = 0;
	virtual EMaterialLightingMode			GetCurrentLightingModel() const = 0;

	//-----------------------------
	// RHI render states setup
	//-----------------------------

	// sets blending
	virtual void							SetBlendingStates(const BlendStateParams& blend) = 0;

	// sets depth stencil state
	virtual void							SetDepthStates(const DepthStencilStateParams& depth) = 0;

	// sets rasterizer extended mode
	virtual void							SetRasterizerStates(const RasterizerStateParams& raster) = 0;


	// sets blending
	virtual void							SetBlendingStates(EBlendFactor nSrcFactor,
																EBlendFactor nDestFactor,
																EBlendFunction nBlendingFunc = BLENDFUNC_ADD,
																int colormask = COLORMASK_ALL) = 0;

	// sets depth stencil state
	virtual void							SetDepthStates(bool bDoDepthTest,
															bool bDoDepthWrite,
															ECompareFunc depthCompFunc = COMPFUNC_LEQUAL) = 0;

	// sets rasterizer extended mode
	virtual void							SetRasterizerStates(ECullMode nCullMode,
																EFillMode nFillMode = FILL_SOLID,
																bool bMultiSample = true,
																bool bScissor = false,
																bool bPolyOffset = false) = 0;

	//------------------
	// Materials or shader static states

	virtual void							SetProxyDeltaTime(float deltaTime) = 0;

	virtual IMaterialPtr					GetBoundMaterial() const = 0;

	virtual void							SetShaderParameterOverriden(int /*EShaderParamSetup*/ param, bool set = true) = 0;

	// global variables
	virtual MatVarProxyUnk					FindGlobalMaterialVarByName(const char* pszVarName) const = 0;
	virtual MatVarProxyUnk					GetGlobalMaterialVarByName(const char* pszVarName, const char* defaultValue = nullptr) = 0;

	virtual MatVarProxyUnk					FindGlobalMaterialVar(int nameHash) const = 0;

	template<typename PROXY = MatVarProxyUnk>
	PROXY									FindGlobalMaterialVar(int nameHash) const
	{
		return PROXY(FindGlobalMaterialVar(nameHash));
	}

	virtual bool							BindMaterial(IMaterial* pMaterial, int flags = MATERIAL_BIND_PREAPPLY) = 0;
	virtual void							Apply() = 0;

	// sets the custom rendering callbacks
	// useful for proxy updates, setting up constants that shader objects can't access by themselves
	virtual void							SetRenderCallbacks(IMatSysRenderCallbacks* callback) = 0;
	virtual IMatSysRenderCallbacks*			GetRenderCallbacks() const = 0;

	//-----------------------------
	// Rendering projection helper operations

	// sets up a 2D mode (also sets up view and world matrix)
	virtual void							Setup2D(float wide, float tall) = 0;

	// sets up 3D mode, projection
	virtual void							SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar) = 0;

	// sets up 3D mode, orthogonal
	virtual void							SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar) = 0;

	// sets up a matrix, projection, view, and world
	virtual void							SetMatrix(EMatrixMode mode, const Matrix4x4& matrix) = 0;

	// returns a typed matrix
	virtual void							GetMatrix(EMatrixMode mode, Matrix4x4& matrix) = 0;

	// retunrs multiplied matrix
	virtual void							GetWorldViewProjection(Matrix4x4& matrix) = 0;

	//-----------------------------
	// Swap chains
	//-----------------------------

	// tells device to begin frame
	virtual bool							BeginFrame(IEqSwapChain* swapChain) = 0;

	// tells device to end and present frame. Also swapchain can be overriden.
	virtual bool							EndFrame() = 0;

	// resizes device back buffer. Must be called if window resized, etc
	virtual void							SetDeviceBackbufferSize(int wide, int tall) = 0;

	// reports device focus mode
	virtual void							SetDeviceFocused(bool inFocus) = 0;

	// creates additional swap chain
	virtual IEqSwapChain*					CreateSwapChain(void* windowHandle) = 0;
	virtual void							DestroySwapChain(IEqSwapChain* chain) = 0;

	// window/fullscreen mode changing; returns false if fails
	virtual bool							SetWindowed(bool enable) = 0;
	virtual bool							IsWindowed() const = 0;

	// captures screenshot to CImage data
	virtual bool							CaptureScreenshot(CImage& img) = 0;

	//-----------------------------
	// Internal operations
	//-----------------------------

	// returns RHI device interface
	virtual IShaderAPI*						GetShaderAPI() const = 0;

	virtual void							RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName) = 0;
	virtual IMaterialProxy*					CreateProxyByName(const char* pszName) = 0;

	virtual void							RegisterShader(const char* pszShaderName, DISPATCH_CREATE_SHADER dispatcher_creation) = 0;
	virtual void							RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function) = 0;

	// device lost/restore callbacks
	virtual void							AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore) = 0;
	virtual void							RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore) = 0;

	virtual void							PrintLoadedMaterials() = 0;
};

template<typename VERT>
void IMaterialSystem::DrawDefaultUP(EPrimTopology type, const VERT* verts, int numVerts, const ITexturePtr& texture, const ColorRGBA& color,
		BlendStateParams* blendParams, DepthStencilStateParams* depthParams, RasterizerStateParams* rasterParams)
{
	const void* vertPtr = reinterpret_cast<void*>(&verts);
	const int vertFVF = typename VertexFVFResolver<VERT>::value;
	DrawDefaultUP(type, vertFVF, vertPtr, N, texture, color, blendParams, depthParams, rasterParams);
}

template<typename ARRAY_TYPE>
void IMaterialSystem::DrawDefaultUP(EPrimTopology type, const ARRAY_TYPE& verts, const ITexturePtr& texture, const ColorRGBA& color,
		BlendStateParams* blendParams, DepthStencilStateParams* depthParams, RasterizerStateParams* rasterParams)
{
	const int vertFVF = typename VertexFVFResolver<typename ARRAY_TYPE::ITEM>::value;
	DrawDefaultUP(type, vertFVF, verts.ptr(), verts.numElem(), texture, color, blendParams, depthParams, rasterParams);
}

extern IMaterialSystem* g_matSystem;
