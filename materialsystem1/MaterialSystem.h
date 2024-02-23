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
	EqString			name;
	PROXY_FACTORY_CB	func;
};

struct ShaderOverride
{
	EqString			shaderName;
	OVERRIDE_SHADER_CB	func;
};

class IRenderLibrary;
class CMaterial;
struct DKMODULE;

class CMaterialSystem : public IMaterialSystem
{
	friend class CMaterial;

public:
	CMaterialSystem();
	~CMaterialSystem();

	bool						IsInitialized() const { return (m_renderLibrary != nullptr); }

	bool						Init(const MaterialsInitSettings& config);
	void						Shutdown();

	bool						LoadShaderLibrary(const char* libname);
	MatSysRenderSettings&		GetConfiguration();

	const char*					GetMaterialPath() const;
	const char*					GetMaterialSRCPath() const;

	IShaderAPI*					GetShaderAPI() const;

	void						RegisterProxy(PROXY_FACTORY_CB dispfunc, const char* pszName);
	IMaterialProxy*				CreateProxyByName(const char* pszName);

	void						RegisterShader(const ShaderFactory& factory);
	void						RegisterShaderOverride(const char* shaderName, OVERRIDE_SHADER_CB func);

	void						AddDestroyLostCallbacks(DEVICE_LOST_RESTORE_CB destroy, DEVICE_LOST_RESTORE_CB restore);
	void						RemoveLostRestoreCallbacks(DEVICE_LOST_RESTORE_CB destroy, DEVICE_LOST_RESTORE_CB restore);

	void						PrintLoadedMaterials() const;

	void						GetPolyOffsetSettings(float& depthBias, float& depthBiasSlopeScale) const;

	//-----------------------------
	// Swap chains

	bool						BeginFrame(ISwapChain* swapChain);
	bool						EndFrame();

	ITexturePtr					GetCurrentBackbuffer() const;
	ITexturePtr					GetDefaultDepthBuffer() const;

	void						SetDeviceBackbufferSize(int wide, int tall);
	void						SetDeviceFocused(bool inFocus);

	ISwapChain*					CreateSwapChain(const RenderWindowInfo& windowInfo);
	void						DestroySwapChain(ISwapChain* chain);

	bool						SetWindowed(bool enable);
	bool						IsWindowed() const;

	bool						CaptureScreenshot( CImage &img );

	//-----------------------------
	// Resource operations

	// returns the default material capable to use with MatSystem's GetDynamicMesh()
	const IMaterialPtr&			GetDefaultMaterial() const;
	const ITexturePtr&			GetWhiteTexture(ETextureDimension texDimension = TEXDIMENSION_2D) const;
	const ITexturePtr&			GetErrorCheckerboardTexture(ETextureDimension texDimension = TEXDIMENSION_2D) const;
								
	IMaterialPtr				CreateMaterial(const char* szMaterialName, const KVSection* params, int instanceFormatId = 0);
	IMaterialPtr				GetMaterial(const char* szMaterialName, int instanceFormatId = 0);
	bool						IsMaterialExist(const char* szMaterialName) const;
								
	const ShaderFactory*		GetShaderFactory(const char* szShaderName, int instanceFormatId);
	MatSysShaderPipelineCache&	GetRenderPipelineCache(int shaderNameHash);
								
	void						PreloadNewMaterials();
	void						WaitAllMaterialsLoaded();
								
	void						QueueLoading(const IMaterialPtr& pMaterial);
	int							GetLoadingQueue() const;
								
	void						ReloadAllMaterials();
	void						ReleaseUnusedMaterials();
								
	void						FreeMaterial(IMaterial *pMaterial);
								
	void						FreeMaterials();

	void						SetProxyDeltaTime(float deltaTime);

	//-----------------------------
	// Shader dynamic states

	void						SetFogInfo(const FogInfo &info);
	void						GetFogInfo(FogInfo &info) const;

	void						SetEnvironmentMapTexture(const ITexturePtr& pEnvMapTexture);
	const ITexturePtr&			GetEnvironmentMapTexture() const;

	//------------------
	// Materials or shader static states

	MatVarProxyUnk				FindGlobalMaterialVar(int nameHash) const;
	MatVarProxyUnk				FindGlobalMaterialVarByName(const char* pszVarName) const;
	MatVarProxyUnk				GetGlobalMaterialVarByName(const char* pszVarName, const char* defaultValue);

	//-----------------------------
	// Rendering projection helper operations

	void						Setup2D(float wide, float tall);
	void						SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar);
	void						SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar);
	void						SetMatrix(EMatrixMode mode, const Matrix4x4 &matrix);
	void						GetMatrix(EMatrixMode mode, Matrix4x4 &matrix) const;

	void						GetViewProjection(Matrix4x4& matrix) const;
	void						GetWorldViewProjection(Matrix4x4 &matrix) const;
	int							GetCameraChangeId() const { return m_cameraChangeId; }

	void						GetCameraParams(MatSysCamera& cameraParams, const Matrix4x4& proj, const Matrix4x4& view, const Matrix4x4& world = identity4) const;

	//-----------------------------
	// Helper rendering operations

	// returns the dynamic mesh
	IDynamicMeshPtr				GetDynamicMesh();
	void						ReleaseDynamicMesh(int id);

	// returns temp buffer with data written. SubmitQueuedCommands uploads it to GPU
	GPUBufferView				GetTransientUniformBuffer(const void* data, int64 size);
	GPUBufferView				GetTransientVertexBuffer(const void* data, int64 size);

	void						QueueCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers);
	void						QueueCommandBuffer(const IGPUCommandBuffer* cmdBuffer);

	void						SubmitQueuedCommands();

	void						UpdateMaterialProxies(IMaterial* material, IGPUCommandRecorder* commandRecorder, bool force = false) const;

	bool						SetupMaterialPipeline(IMaterial* material, ArrayCRef<RenderBufferInfo> uniformBuffers, EPrimTopology primTopology, const MeshInstanceFormatRef& meshInstFormat, const RenderPassContext& passContext);
	void						SetupDrawCommand(const RenderDrawCmd& drawCmd, const RenderPassContext& passContext);
	bool						SetupDrawDefaultUP(EPrimTopology primTopology, int vertFVF, const void* verts, int numVerts, const RenderPassContext& passContext);

private:

	void						CreateMaterialInternal(CRefPtr<CMaterial> material, const KVSection* params);
	void						CreateWhiteTexture();
	void						CreateErrorTexture();
	void						CreateDefaultDepthTexture();
	void						InitDefaultMaterial();

	void						FramePrepareInternal();
	void						QueueCommitInternalBuffers();

	MatSysRenderSettings		m_config;

	IRenderLibrary*				m_renderLibrary{ nullptr };	// render library.
	DKMODULE*					m_rendermodule{ nullptr };	// render dll.

	IShaderAPI*					m_shaderAPI{ nullptr };		// the main renderer interface
	EqString					m_materialsPath;			// material path
	EqString					m_materialsSRCPath;			// material sources path

	MaterialVarBlock			m_globalMaterialVars;

	Array<DKMODULE*>			m_shaderLibs{ PP_SL };				// loaded shader libraries
	Map<int, ShaderFactory>		m_shaderFactoryList{ PP_SL };		// registered shaders
	Array<ShaderOverride>		m_shaderOverrideList{ PP_SL };		// shader override functors
	Array<ShaderProxyFactory>	m_proxyFactoryList{ PP_SL };

	Map<int, MatSysShaderPipelineCache> m_renderPipelineCache{ PP_SL };
	Map<int, IMaterial*>		m_loadedMaterials{ PP_SL };			// loaded material list

	Array<CDynamicMesh>			m_dynamicMeshes{ PP_SL };
	Array<int>					m_freeDynamicMeshes{ PP_SL };
	IVertexFormat*				m_dynamicMeshVertexFormat{ nullptr };

	//-------------------------------------------------------------------------
	IVector2D					m_backbufferSize{ 800, 600 };

	mutable Matrix4x4			m_viewProjMatrix{ identity4 };
	mutable Matrix4x4			m_wvpMatrix{ identity4 };
	mutable Matrix4x4			m_matrices[5]{ identity4 };

	Array<IGPUCommandBufferPtr>	m_pendingCmdBuffers{ PP_SL };
	IGPUCommandRecorderPtr		m_proxyUpdateCmdRecorder;
	IGPUCommandRecorderPtr		m_bufferCmdRecorder;

	struct TransientBufferCollection
	{
		IGPUBufferPtr	buffers[8];
		int64			bufferOffsets[8]{ 0 };
		int				bufferIdx{ 0 };
	};

	TransientBufferCollection	m_transientUniformBuffers;
	TransientBufferCollection	m_transientVertexBuffers;

	IMaterialPtr				m_defaultMaterial;
	IMaterialPtr				m_overdrawMaterial;

	ITexturePtr					m_currentEnvmapTexture;
	ITexturePtr					m_whiteTexture[TEXDIMENSION_COUNT];
	ITexturePtr					m_errorTexture[TEXDIMENSION_COUNT];
	ITexturePtr					m_defaultDepthTexture;

	Array<DEVICE_LOST_RESTORE_CB>	m_lostDeviceCb{ PP_SL };
	Array<DEVICE_LOST_RESTORE_CB>	m_restoreDeviceCb{ PP_SL };

	FogInfo						m_fogInfo;

	CEqTimer					m_proxyTimer;

	uint						m_frame{ 0 };
	float						m_proxyDeltaTime{ 0.0f };
	bool						m_frameBegun{ false };

	int							m_cameraChangeId{ 0 };
	mutable uint8				m_matrixDirty{ UINT8_MAX };
};
