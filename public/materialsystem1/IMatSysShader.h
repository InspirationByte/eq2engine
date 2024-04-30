//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: MatSystem abstraction of shader
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IMaterial;
class IMatSystemShader;

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

enum ETextureFormat : int;
enum EPrimTopology : int;
enum ECullMode : int;

class IGPURenderPassRecorder;
struct MeshInstanceFormatRef;
struct RenderBufferInfo;
struct RenderPassContext;

class IGPURenderPipeline;
using IGPURenderPipelinePtr = CRefPtr<IGPURenderPipeline>;

class IGPUBindGroup;
using IGPUBindGroupPtr = CRefPtr<IGPUBindGroup>;

using OVERRIDE_SHADER_CB = const char* (*)(int instanceFormatId);
using CREATE_SHADER_CB = IMatSystemShader * (*)();

enum EBindGroupId : int
{
	// Data is never going to be changed during the life time of the material. 
	//   Used for material parameters and textures.
	BINDGROUP_CONSTANT = 0,

	// Persists across single render pass. 
	//   Use for camera & fog properties, material proxy values.
	BINDGROUP_RENDERPASS = 1,

	// Unique for each draw call. 
	//   Use for something that would be unique for drawn object.
	BINDGROUP_TRANSIENT = 2,
};

struct MatSysShaderPipelineCache
{
	Map<uint, IGPURenderPipelinePtr>	pipelines{ PP_SL };
	Threading::CEqReadWriteLock			rwLock;
};

class IMatSystemShader
{
public:
	struct PipelineInputParams;

	virtual ~IMatSystemShader() = default;

	virtual void				Init(IShaderAPI* renderAPI, IMaterial* material) = 0;
	virtual void				Unload() = 0;

	virtual bool				IsInitialized() const = 0;

	virtual void				InitTextures(IShaderAPI* renderAPI) = 0;
	virtual void				InitShader(IShaderAPI* renderAPI) = 0;

	virtual const char*			GetName() const = 0;
	virtual int					GetNameHash() const = 0;

	virtual int					GetFlags() const = 0;

	virtual const ITexturePtr&	GetBaseTexture(int stage = 0) const = 0;
	virtual const ITexturePtr&	GetBumpTexture(int stage = 0) const = 0;

	virtual bool				SetupRenderPass(IShaderAPI* renderAPI, const PipelineInputParams& pipelineParams, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext, IMaterial* originalMaterial) = 0;
	virtual void				UpdateProxy(IGPUCommandRecorder* cmdRecorder) const = 0;

	struct PipelineInputParams
	{
		ArrayCRef<ETextureFormat>		colorTargetFormat;
		ETextureFormat					depthTargetFormat;
		const MeshInstanceFormatRef&	meshInstFormat;
		EPrimTopology					primitiveTopology{ (EPrimTopology)0 };
		ECullMode						cullMode{ (EPrimTopology)0 };

		int								multiSampleCount{ 1 };
		uint32							multiSampleMask{ 0xffffffff };
		bool							multiSampleAlphaToCoverage{ false };

		bool							depthReadOnly{ false };
		bool							skipFragmentPipeline{ false };
	};
};

struct ShaderFactory
{
	ArrayCRef<int>		vertexLayoutIds{ nullptr };
	CREATE_SHADER_CB	func;
	const char*			shaderName;
};
using FactoryList = Array<ShaderFactory>;

extern FactoryList& _InternalShaderList();

#define SHADER_VERTEX_ID(name)	StringToHashConst(#name)

#define DECLARE_INTERNAL_SHADERS()  \
	FactoryList* s_internalShaderReg = nullptr; \
	FactoryList& _InternalShaderList() { if(!s_internalShaderReg) s_internalShaderReg = new FactoryList(PP_SL); return *s_internalShaderReg; }

#define REGISTER_INTERNAL_SHADERS()	\
	for(const ShaderFactory& factory : _InternalShaderList())	\
		g_matSystem->RegisterShader( factory );

#define DEFINE_SHADER(stringName, className) \
	static IMatSystemShader* C##className##Factory() { \
		IMatSystemShader* pShader = static_cast< IMatSystemShader * >(new className()); \
		return pShader;	\
	} \
	class C_ShaderClassFactoryFoo {	\
	public: \
		C_ShaderClassFactoryFoo() { \
			ShaderFactory& factory = _InternalShaderList().append(); \
			factory.vertexLayoutIds = GetSupportedVertexLayoutIds(); \
			factory.func = &C##className##Factory; \
			factory.shaderName = stringName; \
		} \
	}; \
	static C_ShaderClassFactoryFoo g_CShaderClassFactoryFoo;
