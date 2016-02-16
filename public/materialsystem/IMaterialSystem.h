//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: Equilibrium Material sub-rendering system
//
// TODO:	(DONE) Refactor IMaterialSystem interface, to work in small lib EqMatSystem.dll
//			(DONE) Move some helper functions from scenerenderer interface.
//			(DONE) Make a stub-mode support (no rendering, no shader loading, etc.)
//			(DONE) Preset and dynamically changeable lighting model
//			(DONE) Optimize shader parameter setup
//			Make texture loader (ITextureLoader,something) here and make loaders registrator
//			Add pushing/popping the states
//
// How to use:
//		Taking interface: materials = (IMaterialSystem*)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);
//		Initializing for tools: materials->Init("materials/", "EqShaderAPIEmpty", NULL, FORMAT_NONE, true, true);
//////////////////////////////////////////////////////////////////////////////////

#ifndef IMATERIALSYSTEM_H
#define IMATERIALSYSTEM_H

#include "DebugInterface.h"

#include "renderers/IShaderAPI.h"
#include "renderers/IEqSwapChain.h"

#include "IMaterial.h"
#include "IMatSysShader.h"
#include "ViewParams.h"
#include "scene_def.h"
#include "IDkCore.h"

class CImage;

// interface version for Shaders_*** dlls
#define MATSYSTEM_INTERFACE_VERSION "MaterialSystem_008"

// begin/end resource loading for timer purposes
typedef void (*RESOURCELOADCALLBACK)( void );

// shader registrator's dispatcher
typedef IMaterialSystemShader* (*DISPATCH_CREATE_SHADER)( void );

// shader override registrator's dispatcher
typedef const char* (*DISPATCH_OVERRIDE_SHADER)( void );

// device lost/restore callback
typedef bool (*DEVLICELOSTRESTORE)( void );

// Lighting model for material system
enum MaterialLightingMode_e
{
	MATERIAL_LIGHT_UNLIT = 0,	// no lighting, fully white (also, you can use it for ambient rendering)
	MATERIAL_LIGHT_DEFERRED,	// deferred shading. Full dynamic lighting
	MATERIAL_LIGHT_FORWARD,		// forward shading, also for use in deferred, if you setting non-state lighting model, it will be fully forward
};

struct shaderfactory_t
{
	DISPATCH_CREATE_SHADER dispatcher;
	const char *shader_name;
};

typedef struct Vertex2D
{
	Vertex2D()
	{
		m_vTexCoord = vec2_zero;
		m_vColor = color4_white;
	}

    Vertex2D(const Vector2D& p, const Vector2D& t)
    {
        m_vPosition = p;
        m_vTexCoord = t;
		m_vColor = color4_white;
    }

	Vertex2D(const Vector2D& p, const Vector2D& t,const Vector4D& color)
	{
		m_vPosition = p;
		m_vTexCoord = t;
		m_vColor = color;
	}

	void Set(const Vector2D& p, const Vector2D& t,const Vector4D& color)
	{
		m_vPosition = p;
		m_vTexCoord = t;
		m_vColor = color;
	}

    Vector2D		m_vPosition;
    Vector2D		m_vTexCoord;
	Vector4D		m_vColor;
}Vertex2D_t;

typedef struct Vertex3D
{
	Vertex3D()
	{
		m_vPosition = vec3_zero;
		m_vTexCoord = vec2_zero;
		m_vColor = color4_white;
	}

    Vertex3D(const Vector3D& p, const Vector2D& t)
    {
        m_vPosition = p;
        m_vTexCoord = t;
		m_vColor = color4_white;
    }

	Vertex3D(const Vector3D& p, const Vector2D& t,const Vector4D& color)
	{
		m_vPosition = p;
		m_vTexCoord = t;
		m_vColor = color;
	}
    Vector3D		m_vPosition;
    Vector2D		m_vTexCoord;
	Vector4D		m_vColor;
}Vertex3D_t;

//-----------------------------------------------------
// material system configuration
//-----------------------------------------------------

struct matsystem_render_config_t
{
	// rendering api parameters
	shaderapiinitparams_t shaderapi_params;

	// the basic parameters that materials system can handle

	// preset lighting model
	MaterialLightingMode_e	lighting_model;

	// enable fixed-function mode
	bool	ffp_mode;					// use FFP if possible (don't apply any shaders, DEBUG only)

	bool	stubMode;					// run matsystem in stub mode (deny material usage)?

	bool	lowShaderQuality;
	bool	editormode;					// enable editor mode
	bool	threadedloader;

	// options that can be changed in real time
	bool	enableShadows;				// enable shadows?
	bool	enableSpecular;				// enable specular lighting reflections?
	bool	enableBumpmapping;			// enable bump maps?

	bool	wireframeMode;				// matsystem wireframe mode
};

//------------------------------------------------------------------------
// Material system render parameters
//------------------------------------------------------------------------
class IMaterialRenderParamCallbacks
{
public:
	~IMaterialRenderParamCallbacks() {}

	// for direct control of states in application
	virtual void	OnPreApplyMaterial( IMaterial* pMaterial ) = 0;

	// parameters used by shaders, in user shaders
	virtual void*	GetRenderUserParams()				{return NULL;}
	virtual int		GetRenderUserParamsTypeSignature()	{return 0;}
};

//----------------------------------------------------------------------------------------------------------------------
// Material system inteface
//----------------------------------------------------------------------------------------------------------------------
class IMaterialSystem : public ICoreModuleInterface
{
public:

	// Initialize material system
	// materialsDirectory - you can determine a full path on hard disk
	// szShaderAPI - shader API that will be used. On NULL will set to default Shader API (DX9)
	// config - material system configuration. Must be fully filled
	virtual bool							Init(	const char* materialsDirectory,
													const char* szShaderAPI,
													matsystem_render_config_t &config
													) = 0;

	// shutdowns material system, unloading all.
	virtual void							Shutdown() = 0;

	// Loads and adds new shader library.
	virtual bool							LoadShaderLibrary(const char* libname) = 0;

	// is matsystem in stub mode? (no rendering)
	virtual bool							IsInStubMode() = 0;

	// returns configuration that can be modified in realtime (shaderapi settings can't be modified)
	virtual matsystem_render_config_t&		GetConfiguration() = 0;

	// returns material path
	virtual char*							GetMaterialPath() = 0;

	//-----------------------------
	// Material/Shader operations
	//-----------------------------

	// Finds or loads material (if findExisting option specified, or not found in cache)
	virtual IMaterial*						FindMaterial(const char* szMaterialName, bool findExisting = false) = 0;

	// Creates material system shader
	virtual IMaterialSystemShader*			CreateShaderInstance(const char* szShaderName) = 0;

	// checks material for existence
	virtual bool							IsMaterialExist(const char* szMaterialName) = 0;

	//-----------------------------
	// Loading operations
	//-----------------------------

	// Loads textures, compiles shaders. Called after level loading
	virtual void							PreloadNewMaterials() = 0;

	// begins preloading zone of materials when FindMaterial calls
	virtual void							BeginPreloadMarker() = 0;

	// ends preloading zone of materials when FindMaterial calls
	virtual void							EndPreloadMarker() = 0;

	// loads material or sends it to loader thread
	virtual void							PutMaterialToLoadingQueue(IMaterial* pMaterial) = 0;
	virtual int								GetLoadingQueue() const = 0;

	// Reloads materials, flushes shaders, textures without touching a material pointers.
	virtual void							ReloadAllMaterials(bool bTouchTextures = true,bool bTouchShaders = true, bool wait = false) = 0;

	// Reloads all textures loaded by materials
	virtual void							ReloadAllTextures() = 0;

	//-----------------------------
	// Free operations
	//-----------------------------

	// Unloads all textures
	virtual void							FreeAllTextures() = 0;

	// Frees all materials (if bFreeAll is true, will free locked)
	virtual void							FreeMaterials(bool bFreeAll = false) = 0;

	// Frees materials or decrements it's reference count
	virtual void							FreeMaterial(IMaterial *pMaterial) = 0;

	//-----------------------------
	// Helper operations
	//-----------------------------

	// sets up a 2D mode (also sets up view and world matrix)
	virtual void							Setup2D(float wide, float tall) = 0;

	// sets up 3D mode, projection
	virtual void							SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar) = 0;

	// sets up 3D mode, orthogonal
	virtual void							SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar) = 0;

	//-----------------------------
	// Helper rendering operations (warning, may be slow)
	//-----------------------------

	// draws primitives
	virtual void							DrawPrimitivesFFP(	PrimitiveType_e type, Vertex3D_t *pVerts, int nVerts,
																ITexture* pTexture = NULL, const ColorRGBA &color = color4_white,
																BlendStateParam_t* blendParams = NULL, DepthStencilStateParams_t* depthParams = NULL,
																RasterizerStateParams_t* rasterParams = NULL) = 0;

	// draws primitives for 2D
	virtual void							DrawPrimitives2DFFP(	PrimitiveType_e type, Vertex2D_t *pVerts, int nVerts,
																	ITexture* pTexture = NULL, const ColorRGBA &color = color4_white,
																	BlendStateParam_t* blendParams = NULL, DepthStencilStateParams_t* depthParams = NULL,
																	RasterizerStateParams_t* rasterParams = NULL) = 0;

	//-----------------------------
	// Pre-render operations
	//-----------------------------

	// returns white texture (used for wireframe of shaders that can't use FFP modes,notexture modes, etc.)
	virtual	ITexture*						GetWhiteTexture() = 0;

	// returns luxel test texture (used for lightmap test)
	virtual	ITexture*						GetLuxelTestTexture() = 0;

	// Returns current cull mode
	virtual CullMode_e						GetCurrentCullMode() = 0;

	// Sets new cull mode
	virtual void							SetCullMode(CullMode_e cullMode) = 0;

	//-------------------

	// skinning mode (dynamic, for egf, etc)
	virtual void							SetSkinningEnabled( bool bEnable ) = 0;

	// is skinning enabled?
	virtual bool							IsSkinningEnabled() = 0;

	//------------------

	// instancing
	virtual void							SetInstancingEnabled( bool bEnable ) = 0;

	// is instancing enabled?
	virtual bool							IsInstancingEnabled() = 0;

	//------------------


	// Binds the material. All shader constants will be set up with it
	virtual bool							BindMaterial( IMaterial *pMaterial, bool preApply = true ) = 0;

	// Applies current material
	virtual void							Apply() = 0;

	// returns bound material
	virtual IMaterial*						GetBoundMaterial() = 0;

	// non-realtime Lighting model setup, requries ReloadAllMaterials call
	virtual void							SetLightingModel(MaterialLightingMode_e lightingModel) = 0;

	// Lighting model (e.g shadow maps or light maps)
	virtual MaterialLightingMode_e			GetLightingModel() = 0;

	// sets a fog info
	virtual void							SetFogInfo(const FogInfo_t &info) = 0;
	// returns fog info
	virtual void							GetFogInfo(FogInfo_t &info) = 0;

	// updates all materials and proxies. Also provides reloading of materials
	virtual	void							Update(float dt) = 0;

	// waits for material loader thread is finished
	virtual void							Wait() = 0;

//-----------------------------
// transform operations
//-----------------------------

	// sets up a matrix, projection, view, and world
	virtual void							SetMatrix(MatrixMode_e mode, const Matrix4x4 &matrix) = 0;

	// returns a typed matrix
	virtual void							GetMatrix(MatrixMode_e mode, Matrix4x4 &matrix) = 0;

	// retunrs multiplied matrix
	virtual void							GetWorldViewProjection(Matrix4x4 &matrix) = 0;

	// sets an ambient light. Also used as main color
	virtual void							SetAmbientColor(const ColorRGBA &color) = 0;

	// returns current ambient color
	virtual ColorRGBA						GetAmbientColor() = 0;

	// sets current light for processing in shaders (only lighting shader. Non-lighing shaders will ignore it.)
	virtual void							SetLight(dlight_t* pLight) = 0;

	// returns current light.
	virtual dlight_t*						GetLight() = 0;

	// sets current lighting model as realtime state
	virtual void							SetCurrentLightingModel(MaterialLightingMode_e lightingModel) = 0;

	// returns current lighting model realtime state
	virtual MaterialLightingMode_e			GetCurrentLightingModel() = 0;

	// sets pre-apply callback
	virtual void							SetMaterialRenderParamCallback( IMaterialRenderParamCallbacks* callback ) = 0;

	// returns current pre-apply callback
	virtual IMaterialRenderParamCallbacks*	GetMaterialRenderParamCallback() = 0;

	// sets $env_cubemap texture for use in shaders
	virtual void							SetEnvironmentMapTexture( ITexture* pEnvMapTexture ) = 0;

	// returns $env_cubemap texture used in shaders
	virtual ITexture*						GetEnvironmentMapTexture() = 0;

//-----------------------------
// State setup
//-----------------------------

	// sets blending
	virtual void							SetBlendingStates(const BlendStateParam_t& blend) = 0;

	// sets depth stencil state
	virtual void							SetDepthStates(const DepthStencilStateParams_t& depth) = 0;

	// sets rasterizer extended mode
	virtual void							SetRasterizerStates(const RasterizerStateParams_t& raster) = 0;


	// sets blending
	virtual void							SetBlendingStates(	BlendingFactor_e nSrcFactor,
																BlendingFactor_e nDestFactor,
																BlendingFunction_e nBlendingFunc = BLENDFUNC_ADD,
																int colormask = COLORMASK_ALL
																) = 0;

	// sets depth stencil state
	virtual void							SetDepthStates(	bool bDoDepthTest,
															bool bDoDepthWrite,
															CompareFunc_e depthCompFunc = COMP_LEQUAL) = 0;

	// sets rasterizer extended mode
	virtual void							SetRasterizerStates(	CullMode_e nCullMode,
																	FillMode_e nFillMode = FILL_SOLID,
																	bool bMultiSample = true,
																	bool bScissor = false
																	) = 0;

	//-----------------------------
	// Frame operations
	//-----------------------------

	// tells device to begin frame
	virtual bool							BeginFrame() = 0;

	// tells device to end and present frame. Also swapchain can be overriden.
	virtual bool							EndFrame(IEqSwapChain* swapChain) = 0;

	// resizes device back buffer. Must be called if window resized, etc
	virtual void							SetDeviceBackbufferSize(int wide, int tall) = 0;

	// creates additional swap chain
	virtual IEqSwapChain*					CreateSwapChain(void* windowHandle) = 0;

	//-----------------------------
	// Internal operations
	//-----------------------------

	// returns proxy factory interface (material parameter controller)
	virtual IProxyFactory*					GetProxyFactory() = 0;

	// returns shader interface of this material system (shader render api)
	virtual IShaderAPI*						GetShaderAPI() = 0;

	// captures screenshot to CImage data
	virtual bool							CaptureScreenshot( CImage &img ) = 0;

	// Registers new shader.
	virtual void							RegisterShader(const char* pszShaderName,DISPATCH_CREATE_SHADER dispatcher_creation) = 0;

	// registers overrider for shaders
	virtual void							RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function) = 0;

	// use this if you want to reduce "frametime jumps" when matsystem loads textures
	virtual void							SetResourceBeginEndLoadCallback(RESOURCELOADCALLBACK begin, RESOURCELOADCALLBACK end) = 0;

	// use this if you have objects that must be destroyed when device is lost
	virtual void							AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore) = 0;

	// removes callbacks from list
	virtual void							RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore) = 0;

	// prints loaded materials to console
	virtual void							PrintLoadedMaterials() = 0;
};

extern IMaterialSystem* materials;

#define DECLARE_INTERNAL_SHADERS()       \
	DkList<shaderfactory_t>* s_internalShaderReg = NULL;                            \
	DkList<shaderfactory_t>& _InternalShaderList() { if(!s_internalShaderReg) s_internalShaderReg = new DkList<shaderfactory_t>(); return *s_internalShaderReg; }

#define REGISTER_INTERNAL_SHADERS()								\
	for(int i = 0; i < _InternalShaderList().numElem(); i++)	\
		materials->RegisterShader( _InternalShaderList()[i].shader_name, _InternalShaderList()[i].dispatcher );

#ifdef EQSHADER_LIBRARY

#define DEFINE_SHADER(localName,className)											\
	static IMaterialSystemShader* C##className##Factory( void )						\
	{																				\
		IMaterialSystemShader *pShader = static_cast< IMaterialSystemShader * >(new className()); 	\
		return pShader;																\
	}																				\
	class C_Shader##localName##Foo													\
	{																				\
	public:																			\
		C_Shader##localName##Foo( void )											\
		{																			\
			static IMaterialSystem* matSystem =	(IMaterialSystem *)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);	\
			matSystem->RegisterShader( #localName, &C##className##Factory );		\
		}																			\
	};																				\
	static C_Shader##localName##Foo g_CShader##localName##Foo;
#else

#define DEFINE_SHADER(localName,className)											\
	static IMaterialSystemShader* C##className##Factory( void )						\
	{																				\
		IMaterialSystemShader *pShader = static_cast< IMaterialSystemShader * >(new className()); 	\
		return pShader;																\
	}																				\
	class C_Shader##localName##Foo													\
	{																				\
	public:																			\
		C_Shader##localName##Foo( void )											\
		{																			\
			extern DkList<shaderfactory_t>& _InternalShaderList();					\
			shaderfactory_t factory;												\
			factory.dispatcher = &C##className##Factory;							\
			factory.shader_name = #localName;										\
			int idx = _InternalShaderList().append(factory);						\
		}																			\
	};																				\
	static C_Shader##localName##Foo g_CShader##localName##Foo;

#endif


#endif //IMATERIALSYSTEM_H
