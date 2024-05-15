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
using IDynamicMeshPtr = CRefPtr<IDynamicMesh>;

using DEVICE_LOST_RESTORE_CB	= bool (*)(void);
using PROXY_FACTORY_CB			= IMaterialProxy* (*)(void);

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

struct MatSysRenderSettings
{
	int			materialFlushThresh{ 1000 };	// flush (unload) threshold in frames

	bool		editormode{ false };			// enable editor mode
	bool		threadedloader{ true };

	bool		enableShadows{ false };			// enable shadows?
	bool		wireframeMode{ false };			// matsystem wireframe mode
	bool		overdrawMode{ false };			// matsystem overdraw mode
};

struct MaterialsInitSettings
{
	MatSysRenderSettings	renderConfig;
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
	CORE_INTERFACE("E2_MaterialSystem_028")

	// Initialize material system
	// szShaderAPI - shader API that will be used. On NULL will set to default Shader API (DX9)
	// config - material system configuration. Must be fully filled
	virtual bool					Init(const MaterialsInitSettings& config) = 0;
	virtual void					Shutdown() = 0;

	virtual bool					LoadShaderLibrary(const char* libname) = 0;

	virtual MatSysRenderSettings&	GetConfiguration() = 0;

	// returns material path
	virtual const char*				GetMaterialPath() const = 0;
	virtual const char*				GetMaterialSRCPath() const = 0;

	//-----------------------------
	// Internal operations

	virtual IShaderAPI*				GetShaderAPI() const = 0;

	virtual void					RegisterProxy(PROXY_FACTORY_CB dispfunc, const char* pszName) = 0;
	virtual IMaterialProxy*			CreateProxyByName(const char* pszName) = 0;

	virtual void					RegisterShader(const ShaderFactory& factory) = 0;
	virtual void					RegisterShaderOverride(const char* shaderName, OVERRIDE_SHADER_CB func) = 0;

	virtual void					AddDestroyLostCallbacks(DEVICE_LOST_RESTORE_CB destroy, DEVICE_LOST_RESTORE_CB restore) = 0;
	virtual void					RemoveLostRestoreCallbacks(DEVICE_LOST_RESTORE_CB destroy, DEVICE_LOST_RESTORE_CB restore) = 0;

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
	virtual const IMaterialPtr&		GetGridMaterial() const = 0;

	virtual const ITexturePtr&		GetErrorCheckerboardTexture(ETextureDimension texDimension = TEXDIMENSION_2D) const = 0;
	virtual	const ITexturePtr&		GetWhiteTexture(ETextureDimension texDimension = TEXDIMENSION_2D) const = 0;

	virtual IMaterialPtr			CreateMaterial(const char* szMaterialName, const KVSection* params, int instanceFormatId = 0) = 0;
	virtual IMaterialPtr			GetMaterial(const char* szMaterialName, int instanceFormatId = 0) = 0;
	virtual bool					IsMaterialExist(const char* szMaterialName) const = 0;

	virtual const ShaderFactory*	GetShaderFactory(const char* szShaderName, int instanceFormatId) = 0;
	virtual MatSysShaderPipelineCache&	GetRenderPipelineCache(int shaderNameHash) = 0;

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

	virtual int						GetCameraChangeId() const = 0;

	virtual void					GetCameraParams(MatSysCamera& cameraParams, const Matrix4x4& proj, const Matrix4x4& view, const Matrix4x4& world = identity4) const = 0;
	int								GetCameraParams(MatSysCamera& cameraParams);

	//-----------------------------
	// Drawing
	virtual IDynamicMeshPtr			GetDynamicMesh() = 0;
	virtual void					ReleaseDynamicMesh(int id) = 0;

	// returns temp buffer with data written. SubmitCommandBuffers uploads it to GPU
	virtual GPUBufferView			GetTransientUniformBuffer(const void* data, int64 size) = 0;
	virtual GPUBufferView			GetTransientVertexBuffer(const void* data, int64 size) = 0;

	// queues command buffer. Execution order is guaranteed
	virtual void					QueueCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers) = 0;
	virtual void					QueueCommandBuffer(const IGPUCommandBuffer* cmdBuffer) = 0;

	// submits all queued command buffers to RHI.
	virtual void					SubmitQueuedCommands() = 0;
	virtual Future<bool>			SubmitQueuedCommandsAwaitable() = 0;
	virtual void					UpdateMaterialProxies(IMaterial* material, IGPUCommandRecorder* commandRecorder, bool force = false) const = 0;

	virtual bool					SetupMaterialPipeline(IMaterial* material, ArrayCRef<RenderBufferInfo> uniformBuffers, EPrimTopology primTopology, const MeshInstanceFormatRef& meshInstFormat, const RenderPassContext& passContext) = 0;
	virtual void					SetupDrawCommand(const RenderDrawCmd& drawCmd, const RenderPassContext& passContext) = 0;
	virtual bool					SetupDrawDefaultUP(EPrimTopology primTopology, int vertFVF, const void* verts, int numVerts, const RenderPassContext& passContext) = 0;

	template<typename VERT>
	void							SetupDrawDefaultUP(EPrimTopology primTopology, const VERT* verts, int numVerts, const RenderPassContext& passContext);

	template<typename ARRAY_TYPE>
	void							SetupDrawDefaultUP(EPrimTopology primTopology, const ARRAY_TYPE& verts, const RenderPassContext& passContext);

	//--------------------------------------------------
	// DEPRECATED

	virtual void					SetFogInfo(const FogInfo& info) = 0;
	virtual void					GetFogInfo(FogInfo& info) const = 0;
};

inline int IMaterialSystem::GetCameraParams(MatSysCamera& cameraParams)
{
	Matrix4x4 world, world2;
	GetMatrix(MATRIXMODE_PROJECTION, cameraParams.proj);
	GetMatrix(MATRIXMODE_VIEW, cameraParams.view);
	GetMatrix(MATRIXMODE_WORLD, world);
	GetMatrix(MATRIXMODE_WORLD2, world2);

	GetCameraParams(cameraParams, cameraParams.proj, cameraParams.view, world * world2);

	return GetCameraChangeId();
}

template<typename VERT>
void IMaterialSystem::SetupDrawDefaultUP(EPrimTopology primTopology, const VERT* verts, int numVerts, const RenderPassContext& passContext)
{
	const void* vertPtr = reinterpret_cast<void*>(&verts);
	const int vertFVF = VertexFVFResolver<VERT>::value;
	SetupDrawDefaultUP(primTopology, vertFVF, vertPtr, numVerts, passContext);
}

template<typename ARRAY_TYPE>
void IMaterialSystem::SetupDrawDefaultUP(EPrimTopology primTopology, const ARRAY_TYPE& verts, const RenderPassContext& passContext)
{
	using VERT = typename ARRAY_TYPE::ITEM;
	const int vertFVF = VertexFVFResolver<VERT>::value;
	SetupDrawDefaultUP(primTopology, vertFVF, verts.ptr(), verts.numElem(), passContext);
}

extern IMaterialSystem* g_matSystem;
