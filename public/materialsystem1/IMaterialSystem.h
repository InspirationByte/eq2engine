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
#include "renderers/IShaderAPI.h"
#include "renderers/IEqSwapChain.h"
#include "scene_def.h"

#include "IMaterial.h"
#include "IMatSysShader.h"
#include "IMaterialVar.h"
#include "IMaterialProxy.h"

struct FogInfo_t;
struct dlight_t;

class CImage;
class CViewParams;
class IDynamicMesh;
class IMaterialRenderParamCallbacks;

typedef void					(*RESOURCELOADCALLBACK)(void);
typedef IMaterialSystemShader* (*DISPATCH_CREATE_SHADER)(void);
typedef const char* (*DISPATCH_OVERRIDE_SHADER)(void);
typedef bool					(*DEVLICELOSTRESTORE)(void);
typedef IMaterialProxy* (*PROXY_DISPATCHER)(void);

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

struct shaderfactory_t
{
	DISPATCH_CREATE_SHADER dispatcher;
	const char* shader_name;
};
typedef Array<shaderfactory_t> FactoryList;

typedef struct Vertex2D
{
	Vertex2D()
	{
		texCoord = vec2_zero;
		color = color_white.pack();
	}

	Vertex2D(const Vector2D& p, const Vector2D& t)
	{
		position = p;
		texCoord = t;
		color = color_white.pack();
	}

	Vertex2D(const Vector2D& p, const Vector2D& t, const Vector4D& c)
	{
		position = p;
		texCoord = t;
		color = MColor(c).pack();
	}

	Vertex2D(const Vector2D& p, const Vector2D& t, const MColor& c)
	{
		position = p;
		texCoord = t;
		color = c.pack();
	}

	void Set(const Vector2D& p, const Vector2D& t, const Vector4D& c)
	{
		position = p;
		texCoord = t;
		color = MColor(c).pack();
	}

	void Set(const Vector2D& p, const Vector2D& t, const MColor& c)
	{
		position = p;
		texCoord = t;
		color = c.pack();
	}

	static Vertex2D Interpolate(const Vertex2D& a, const Vertex2D& b, float fac)
	{
		return Vertex2D(lerp(a.position, b.position, fac), lerp(a.texCoord, b.texCoord, fac), lerp(MColor(a.color).v, MColor(b.color).v, fac));
	}

	Vector2D		position;
	Vector2D		texCoord;
	uint			color;
}Vertex2D_t;

typedef struct Vertex3D
{
	Vertex3D()
	{
		position = vec3_zero;
		texCoord = vec2_zero;
		color = color_white;
	}

	Vertex3D(const Vector3D& p, const Vector2D& t)
	{
		position = p;
		texCoord = t;
		color = color_white;
	}

	Vertex3D(const Vector3D& p, const Vector2D& t, const Vector4D& c)
	{
		position = p;
		texCoord = t;
		color = c;
	}
	Vector3D		position;
	Vector2D		texCoord;
	ColorRGBA		color;
}Vertex3D_t;

//-----------------------------------------------------
// material system configuration
//-----------------------------------------------------

struct matsystem_render_config_t
{
	EMaterialLightingMode	lightingModel{ MATERIAL_LIGHT_UNLIT };

	bool	ffpMode{ false };			// use FFP if possible (don't apply any shaders, DEBUG only)
	bool	stubMode{ false };			// run matsystem in stub mode (don't render anything)

	bool	lowShaderQuality{ false };
	bool	editormode{ false };		// enable editor mode
	bool	threadedloader{ true };
	int		flushThresh{ 1000 };		// flush (unload) threshold in frames

	bool	enableShadows{ true };		// enable shadows?
	bool	enableSpecular{ true };		// enable specular lighting reflections?
	bool	enableBumpmapping{ true };	// enable bump maps?

	bool	wireframeMode{ false };		// matsystem wireframe mode
	bool	overdrawMode{ false };		// matsystem overdraw mode
};

struct matsystem_init_config_t
{
	shaderAPIParams_t			shaderApiParams;

	EqString					rendererName;		// shaderAPI library filename
	EqString					materialsPath;		// regular (retail) materials file paths
	EqString					materialsSRCPath;	// DEV materials file source paths

	matsystem_render_config_t	renderConfig;
};

//----------------------------------------------------------------------------------------------------------------------
// Material system inteface
//----------------------------------------------------------------------------------------------------------------------
class IMaterialSystem : public IEqCoreModule
{
public:
	CORE_INTERFACE("E1_MaterialSystem_023")

	// Initialize material system
	// szShaderAPI - shader API that will be used. On NULL will set to default Shader API (DX9)
	// config - material system configuration. Must be fully filled
	virtual bool							Init(const matsystem_init_config_t& config) = 0;

	// shutdowns material system, unloading all.
	virtual void							Shutdown() = 0;

	// Loads and adds new shader library.
	virtual bool							LoadShaderLibrary(const char* libname) = 0;

	// is matsystem in stub mode? (no rendering)
	virtual bool							IsInStubMode() const = 0;

	// returns configuration that can be modified in realtime (shaderapi settings can't be modified)
	virtual matsystem_render_config_t&		GetConfiguration() = 0;

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
	// Helper rendering operations
	// TODO: remove this
	//-----------------------------

	// draws primitives
	virtual void							DrawPrimitivesFFP(ER_PrimitiveType type, Vertex3D_t* pVerts, int nVerts,
		const ITexturePtr& pTexture = nullptr, const ColorRGBA& color = color_white,
		BlendStateParam_t* blendParams = nullptr, DepthStencilStateParams_t* depthParams = nullptr,
		RasterizerStateParams_t* rasterParams = nullptr) = 0;

	// draws primitives for 2D
	virtual void							DrawPrimitives2DFFP(ER_PrimitiveType type, Vertex2D_t* pVerts, int nVerts,
		const ITexturePtr& pTexture = nullptr, const ColorRGBA& color = color_white,
		BlendStateParam_t* blendParams = nullptr, DepthStencilStateParams_t* depthParams = nullptr,
		RasterizerStateParams_t* rasterParams = nullptr) = 0;

	//-----------------------------
	// Shader dynamic states
	//-----------------------------

	virtual ER_CullMode						GetCurrentCullMode() const = 0;
	virtual void							SetCullMode(ER_CullMode cullMode) = 0;

	virtual void							SetSkinningEnabled(bool bEnable) = 0;
	virtual bool							IsSkinningEnabled() const = 0;

	virtual void							SetInstancingEnabled(bool bEnable) = 0;
	virtual bool							IsInstancingEnabled() const = 0;


	virtual void							SetFogInfo(const FogInfo_t& info) = 0;
	virtual void							GetFogInfo(FogInfo_t& info) const = 0;

	virtual void							SetAmbientColor(const ColorRGBA& color) = 0;
	virtual ColorRGBA						GetAmbientColor() const = 0;

	virtual void							SetLight(dlight_t* pLight) = 0;
	virtual dlight_t*						GetLight() const = 0;

	// lighting/shading model selection
	virtual void							SetCurrentLightingModel(EMaterialLightingMode lightingModel) = 0;
	virtual EMaterialLightingMode			GetCurrentLightingModel() const = 0;

	//-----------------------------
	// RHI render states setup
	//-----------------------------

	// sets blending
	virtual void							SetBlendingStates(const BlendStateParam_t& blend) = 0;

	// sets depth stencil state
	virtual void							SetDepthStates(const DepthStencilStateParams_t& depth) = 0;

	// sets rasterizer extended mode
	virtual void							SetRasterizerStates(const RasterizerStateParams_t& raster) = 0;


	// sets blending
	virtual void							SetBlendingStates(ER_BlendFactor nSrcFactor,
		ER_BlendFactor nDestFactor,
		ER_BlendFunction nBlendingFunc = BLENDFUNC_ADD,
		int colormask = COLORMASK_ALL
	) = 0;

	// sets depth stencil state
	virtual void							SetDepthStates(bool bDoDepthTest,
		bool bDoDepthWrite,
		ER_CompareFunc depthCompFunc = COMPFUNC_LEQUAL) = 0;

	// sets rasterizer extended mode
	virtual void							SetRasterizerStates(ER_CullMode nCullMode,
		ER_FillMode nFillMode = FILL_SOLID,
		bool bMultiSample = true,
		bool bScissor = false,
		bool bPolyOffset = false
	) = 0;

	//------------------
	// Materials or shader static states

	virtual void							SetProxyDeltaTime(float deltaTime) = 0;

	virtual IMaterialPtr					GetBoundMaterial() const = 0;

	virtual void							SetShaderParameterOverriden(int /*ShaderDefaultParams_e*/ param, bool set = true) = 0;

	// global variables
	virtual MatVarProxyUnk					FindGlobalMaterialVarByName(const char* pszVarName) const = 0;
	virtual MatVarProxyUnk					GetGlobalMaterialVarByName(const char* pszVarName, const char* defaultValue = nullptr) = 0;

	virtual MatVarProxyUnk					FindGlobalMaterialVar(int nameHash) const = 0;

	virtual bool							BindMaterial(IMaterial* pMaterial, int flags = MATERIAL_BIND_PREAPPLY) = 0;
	virtual void							Apply() = 0;

	// sets the custom rendering callbacks
	// useful for proxy updates, setting up constants that shader objects can't access by themselves
	virtual void							SetMaterialRenderParamCallback(IMaterialRenderParamCallbacks* callback) = 0;
	virtual IMaterialRenderParamCallbacks*	GetMaterialRenderParamCallback() const = 0;

	//-----------------------------
	// Rendering projection helper operations

	// sets up a 2D mode (also sets up view and world matrix)
	virtual void							Setup2D(float wide, float tall) = 0;

	// sets up 3D mode, projection
	virtual void							SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar) = 0;

	// sets up 3D mode, orthogonal
	virtual void							SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar) = 0;

	// sets up a matrix, projection, view, and world
	virtual void							SetMatrix(ER_MatrixMode mode, const Matrix4x4& matrix) = 0;

	// returns a typed matrix
	virtual void							GetMatrix(ER_MatrixMode mode, Matrix4x4& matrix) = 0;

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

extern IMaterialSystem* materials;

#define DECLARE_INTERNAL_SHADERS()       \
	FactoryList* s_internalShaderReg = nullptr;                            \
	FactoryList& _InternalShaderList() { if(!s_internalShaderReg) s_internalShaderReg = new FactoryList(PP_SL); return *s_internalShaderReg; }

#define REGISTER_INTERNAL_SHADERS()								\
	for(int i = 0; i < _InternalShaderList().numElem(); i++)	\
		materials->RegisterShader( _InternalShaderList()[i].shader_name, _InternalShaderList()[i].dispatcher );

extern FactoryList& _InternalShaderList();

#define DEFINE_SHADER(stringName, className)								\
	static IMaterialSystemShader* C##className##Factory( void )						\
	{																				\
		IMaterialSystemShader *pShader = static_cast< IMaterialSystemShader * >(new className()); 	\
		return pShader;																\
	}																				\
	class C_ShaderClassFactoryFoo													\
	{																				\
	public:																			\
		C_ShaderClassFactoryFoo( void )											\
		{																			\
			shaderfactory_t factory;												\
			factory.dispatcher = &C##className##Factory;							\
			factory.shader_name = stringName;										\
			_InternalShaderList().append(factory);						\
		}																			\
	};																				\
	static C_ShaderClassFactoryFoo g_CShaderClassFactoryFoo;
