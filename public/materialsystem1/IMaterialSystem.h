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
#include "renderers/ISwapChain.h"

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

//-----------------------------------------------------
// material system configuration
//-----------------------------------------------------

struct MaterialsRenderSettings
{
	int			materialFlushThresh{ 1000 };	// flush (unload) threshold in frames

	bool		lowShaderQuality{ false };
	bool		editormode{ false };			// enable editor mode
	bool		threadedloader{ true };

	bool		enableShadows{ true };			// enable shadows?
	bool		wireframeMode{ false };			// matsystem wireframe mode
	bool		overdrawMode{ false };			// matsystem overdraw mode
};

struct MaterialsInitSettings
{
	MaterialsRenderSettings	renderConfig;
	ShaderAPIParams		shaderApiParams;

	EqString	rendererName;		// shaderAPI library filename
	EqString	materialsPath;		// regular (retail) materials file paths
	EqString	materialsSRCPath;	// DEV materials file source paths
	EqString	texturePath;		// texture path (.DDS only)
	EqString	textureSRCPath;		// texture sources path (.TGA only)
};

//----------------------------------------------------------------------------------------------------------------------
// Material system inteface
//----------------------------------------------------------------------------------------------------------------------
class IMaterialSystem : public IEqCoreModule
{
public:
	CORE_INTERFACE("E1_MaterialSystem_026")

	// Initialize material system
	// szShaderAPI - shader API that will be used. On NULL will set to default Shader API (DX9)
	// config - material system configuration. Must be fully filled
	virtual bool					Init(const MaterialsInitSettings& config) = 0;
	virtual void					Shutdown() = 0;

	virtual bool					LoadShaderLibrary(const char* libname) = 0;

	virtual MaterialsRenderSettings&	GetConfiguration() = 0;

	// returns material path
	virtual const char*				GetMaterialPath() const = 0;
	virtual const char*				GetMaterialSRCPath() const = 0;

	//-----------------------------
	// Internal operations

	virtual IShaderAPI*				GetShaderAPI() const = 0;

	virtual void					RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName) = 0;
	virtual IMaterialProxy*			CreateProxyByName(const char* pszName) = 0;

	virtual void					RegisterShader(const char* pszShaderName, DISPATCH_CREATE_SHADER dispatcher_creation) = 0;
	virtual void					RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function) = 0;

	virtual void					AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore) = 0;
	virtual void					RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore) = 0;

	//-----------------------------
	// Swap chains

	virtual bool					BeginFrame(ISwapChain* swapChain) = 0;
	virtual bool					EndFrame() = 0;
	virtual ITexturePtr				GetCurrentBackbuffer() const = 0;
	virtual ITexturePtr				GetDefaultDepthBuffer() const = 0;

	virtual bool					CaptureScreenshot(CImage& img) = 0;

	virtual void					SetDeviceBackbufferSize(int wide, int tall) = 0;
	virtual void					SetDeviceFocused(bool inFocus) = 0;

	virtual ISwapChain*				CreateSwapChain(const RenderWindowInfo& windowInfo) = 0;
	virtual void					DestroySwapChain(ISwapChain* chain) = 0;

	virtual bool					SetWindowed(bool enable) = 0;
	virtual bool					IsWindowed() const = 0;

	//-----------------------------
	// Resource operations

	virtual const IMaterialPtr&		GetDefaultMaterial() const = 0;
	virtual const ITexturePtr&		GetErrorCheckerboardTexture() const = 0;
	virtual	const ITexturePtr&		GetWhiteTexture(ETextureDimension texDimension = TEXDIMENSION_2D) const = 0;

	virtual IMaterialPtr			CreateMaterial(const char* szMaterialName, KVSection* params) = 0;
	virtual IMaterialPtr			GetMaterial(const char* szMaterialName) = 0;
	virtual bool					IsMaterialExist(const char* szMaterialName) const = 0;

	virtual IMatSystemShader*		CreateShaderInstance(const char* szShaderName) = 0;

	virtual void					QueueLoading(const IMaterialPtr& pMaterial) = 0;
	virtual void					PreloadNewMaterials() = 0;
	virtual void					WaitAllMaterialsLoaded() = 0;
	virtual int						GetLoadingQueue() const = 0;

	virtual void					ReloadAllMaterials() = 0;
	virtual void					ReleaseUnusedMaterials() = 0;
	virtual void					FreeMaterial(IMaterial* pMaterial) = 0;

	virtual void					PrintLoadedMaterials() const = 0;

	virtual void					GetPolyOffsetSettings(float& depthBias, float& depthBiasSlopeScale) const = 0;

	//------------------
	// Materials or shader static states

	virtual void					SetProxyDeltaTime(float deltaTime) = 0;

	// global variables
	virtual MatVarProxyUnk			FindGlobalMaterialVarByName(const char* pszVarName) const = 0;
	virtual MatVarProxyUnk			GetGlobalMaterialVarByName(const char* pszVarName, const char* defaultValue = nullptr) = 0;

	virtual MatVarProxyUnk			FindGlobalMaterialVar(int nameHash) const = 0;

	template<typename PROXY = MatVarProxyUnk>
	PROXY							FindGlobalMaterialVar(int nameHash) const { return PROXY(FindGlobalMaterialVar(nameHash)); }

	//-----------------------------
	// Render states 
	// TODO: remove them all, replace with something that would take render pas instead

	virtual void					Setup2D(float wide, float tall) = 0;
	virtual void					SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar) = 0;
	virtual void					SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar) = 0;

	virtual void					SetMatrix(EMatrixMode mode, const Matrix4x4& matrix) = 0;
	virtual void					GetMatrix(EMatrixMode mode, Matrix4x4& matrix) const = 0;

	virtual void					GetViewProjection(Matrix4x4& matrix) const = 0;
	virtual void					GetWorldViewProjection(Matrix4x4& matrix) const = 0;

	virtual void					GetCameraParams(MatSysCamera& cameraParams, bool worldViewProj = false) const = 0;

	// sets the custom rendering callbacks
	// useful for proxy updates, setting up constants that shader objects can't access by themselves
	virtual void					SetRenderCallbacks(IMatSysRenderCallbacks* callback) = 0;
	virtual IMatSysRenderCallbacks* GetRenderCallbacks() const = 0;

	//-----------------------------
	// Drawing
	virtual IDynamicMesh*			GetDynamicMesh() const = 0;

	// returns temp buffer with data written. SubmitCommandBuffers uploads it to GPU
	virtual GPUBufferPtrView		GetTransientUniformBuffer(const void* data, int64 size) = 0;
	virtual GPUBufferPtrView		GetTransientVertexBuffer(const void* data, int64 size) = 0;

	// queues command buffer. Execution order is guaranteed
	virtual void					QueueCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) = 0;
	virtual void					QueueCommandBuffer(const IGPUCommandBuffer* cmdBuffer) = 0;

	// submits all queued command buffers to RHI.
	virtual void					SubmitQueuedCommands() = 0;

	virtual void					SetupMaterialPipeline(IMaterial* material, EPrimTopology primTopology, const MeshInstanceFormatRef& meshInstFormat, int vertexLayoutBits, const void* userData, IGPURenderPassRecorder* rendPassRecorder) = 0;
	virtual void					SetupDrawCommand(const RenderDrawCmd& drawCmd, IGPURenderPassRecorder* rendPassRecorder) = 0;
	virtual bool					SetupDrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, int vertFVF, const void* verts, int numVerts, IGPURenderPassRecorder* rendPassRecorder) = 0;

	template<typename VERT>
	void							SetupDrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const VERT* verts, int numVerts, IGPURenderPassRecorder* rendPassRecorder);

	template<typename ARRAY_TYPE>
	void							SetupDrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const ARRAY_TYPE& verts, IGPURenderPassRecorder* rendPassRecorder);

	//--------------------------------------------------
	// DEPRECATED

	virtual void					SetFogInfo(const FogInfo& info) = 0;
	virtual void					GetFogInfo(FogInfo& info) const = 0;

	virtual void					Draw(const RenderDrawCmd& drawCmd) = 0;

	// draw primitives with default material
	virtual void					DrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, int vertFVF, const void* verts, int numVerts) = 0;

	template<typename VERT>
	void							DrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const VERT* verts, int numVerts);

	template<typename ARRAY_TYPE>
	void							DrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const ARRAY_TYPE& verts);
};

template<typename VERT>
void IMaterialSystem::DrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const VERT* verts, int numVerts)
{
	const void* vertPtr = reinterpret_cast<void*>(&verts);
	const int vertFVF = VertexFVFResolver<VERT>::value;
	DrawDefaultUP(rendPassInfo, primTopology, vertFVF, vertPtr, numVerts);
}

template<typename ARRAY_TYPE>
void IMaterialSystem::DrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const ARRAY_TYPE& verts)
{
	using VERT = typename ARRAY_TYPE::ITEM;
	const int vertFVF = VertexFVFResolver<VERT>::value;
	DrawDefaultUP(rendPassInfo, primTopology, vertFVF, verts.ptr(), verts.numElem());
}

template<typename VERT>
void IMaterialSystem::SetupDrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const VERT* verts, int numVerts, IGPURenderPassRecorder* rendPassRecorder)
{
	const void* vertPtr = reinterpret_cast<void*>(&verts);
	const int vertFVF = VertexFVFResolver<VERT>::value;
	SetupDrawDefaultUP(rendPassInfo, primTopology, vertFVF, vertPtr, numVerts, rendPassRecorder);
}

template<typename ARRAY_TYPE>
void IMaterialSystem::SetupDrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, const ARRAY_TYPE& verts, IGPURenderPassRecorder* rendPassRecorder)
{
	using VERT = typename ARRAY_TYPE::ITEM;
	const int vertFVF = VertexFVFResolver<VERT>::value;
	SetupDrawDefaultUP(rendPassInfo, primTopology, vertFVF, verts.ptr(), verts.numElem(), rendPassRecorder);
}

extern IMaterialSystem* g_matSystem;
