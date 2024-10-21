//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ShaderAPI_defs.h"
#include "materialsystem1/IMaterialVar.h"

class IMaterial;
class IShaderAPI;
class CBaseShader;
struct MatSysCamera;

#define BEGIN_SHADER_CLASS(name, ...)					\
	namespace C##name##ShaderLocalNamespace {				\
	class C##name##Shader;									\
	using ThisShaderClass = C##name##Shader;				\
	static const char ThisClassNameStr[] = #name;			\
	static ArrayCRef<int> GetSupportedVertexLayoutIds() {	\
		static const int supportedFormats[] = {	0, __VA_ARGS__	};	\
		return ArrayCRef(supportedFormats);					\
	}														\
	class C##name##Shader : public CBaseShader {			\
	public:													\
		ArrayCRef<int> GetSupportedVertexLayoutIds() const override { \
			return C##name##ShaderLocalNamespace::GetSupportedVertexLayoutIds(); \
		} \
		const char* GetName() const override { return ThisClassNameStr; } \
		int	GetNameHash() const	override { return StringIdConst24(#name); } \
		C##name##Shader(IMaterial* material) : CBaseShader(material) {} \
		void Init(IShaderAPI* renderAPI) override { \
			CBaseShader::Init(renderAPI); ShaderInitParams(renderAPI); \
		}

#define END_SHADER_CLASS };	\
	DEFINE_SHADER(ThisClassNameStr, ThisShaderClass) }

#define SHADER_INIT_PARAMS()	void ShaderInitParams(IShaderAPI* renderAPI)
#define SHADER_INIT_TEXTURES()	void InitTextures(IShaderAPI* renderAPI)

#define SHADER_PARAM_TEXTURE(param, variable, ...)			variable = LoadTextureByVar(#param, true, ##__VA_ARGS__ )
#define SHADER_PARAM_TEXTURE_NOERROR(param, variable, ...)	variable = LoadTextureByVar(#param, false, ##__VA_ARGS__ )
#define SHADER_PARAM_TEXTURE_FIND(param, variable, ...)		variable = FindTextureByVar(#param, false, ##__VA_ARGS__ )

namespace bufferutil {
template <typename T>
constexpr int GetAlignment()
{
    if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<T, uint>)
        return 4;
    else if constexpr (std::is_same_v<T, Vector2D> || std::is_same_v<T, IVector2D>)
        return 8;
    else if constexpr (std::is_same_v<T, Vector3D> || std::is_same_v<T, IVector3D> || std::is_same_v<T, IVector4D> || std::is_same_v<T, Vector4D> || std::is_same_v<T, Matrix4x4>)
        return 16;
    else
        return alignof(T);
}

template <typename T>
constexpr int GetSize()
{
    return sizeof(T);
}

template<typename T>
constexpr int AlignOffset(size_t offset)
{
	int alignment = GetAlignment<T>();
    return (offset + alignment - 1) & ~(alignment - 1); // Align the offset to the alignment boundary
}

template <typename... Args>
constexpr int CalculateMaxBufferSize()
{
	int offset = 0;

    // Helper lambda to calculate total size including alignment
    ([&]{
        using T = std::decay_t<Args>;
        offset = AlignOffset<T>(offset);  // Align the offset to the current type
        offset += GetSize<T>();           // Add the size of the current type
    }(), ...);

    return offset;
}
}

template<typename... Args>
IGPUBufferPtr MakeParameterUniformBuffer(Args... data)
{
    // Calculate the maximum buffer size at compile-time using constexpr
    constexpr int MaxBufferSize = bufferutil::CalculateMaxBufferSize<Args...>();

    // Allocate the buffer on the stack based on MaxBufferSize
    ubyte buffer[MaxBufferSize];
    int offset = 0;

    // Helper lambda to pack each argument into the buffer with std430 alignment
    auto packData = [&](auto& arg) {
        using T = std::decay_t<decltype(arg)>;
        offset = bufferutil::AlignOffset<T>(offset);  // Align the offset for this type

        // Copy the data into the buffer
        int dataSize = bufferutil::GetSize<T>();
        memcpy(buffer + offset, &arg, dataSize);
        offset += dataSize;
    };

    // Expand the argument pack and call packData on each argument
    (packData(data), ...);

    // Create the buffer on GPU with the packed data
    return g_matSystem->GetShaderAPI()->CreateBuffer(BufferInfo(buffer, offset), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "materialParams");
}


// base shader class
class CBaseShader : public IMatSystemShader
{
public:
	CBaseShader(IMaterial* material);

	void						Unload();

	virtual void				Init(IShaderAPI* renderAPI);
	void						InitShader(IShaderAPI* renderAPI);

	bool						IsInitialized() const { return m_isInit; }
	int							GetFlags() const { return m_flags; }

	virtual void				UpdateProxy(IGPUCommandRecorder* cmdRecorder) const {}

	// returns base texture from shader
	virtual const ITexturePtr&	GetBaseTexture(int stage) const	{ return ITexturePtr::Null(); };
	virtual const ITexturePtr&	GetBumpTexture(int stage) const	{ return ITexturePtr::Null(); };

	virtual bool				SetupRenderPass(IShaderAPI* renderAPI, const PipelineInputParams& pipelineParams, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext, IMaterial* originalMaterial);

protected:
	struct PipelineInfo
	{
		mutable IGPUBindGroupPtr	bindGroup[MAX_BINDGROUPS];
		IGPURenderPipelinePtr		pipeline;
		IGPUPipelineLayoutPtr		layout;
		int							vertexLayoutId{ 0 };
		mutable uint				instMngToken{ COM_UINT_MAX };
	};

	struct BindGroupSetupParams
	{
		ArrayCRef<RenderBufferInfo>			uniformBuffers;
		const PipelineInfo&					pipelineInfo;
		const RenderPassContext&			passContext;
		IMaterial* 							originalMaterial{ nullptr };
		const IShaderMeshInstanceProvider*	meshInstProvider{ nullptr };
	};

	virtual ArrayCRef<int>		GetSupportedVertexLayoutIds() const = 0;

	virtual IGPUBindGroupPtr	GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const { return nullptr; }
	virtual void				FillBindGroupLayout_Constant(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const {}
	virtual void				FillBindGroupLayout_RenderPass(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const {}
	virtual void				FillBindGroupLayout_Transient(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const {}

	void						FillBindGroupLayout_Constant_Samplers(BindGroupLayoutDesc& bindGroupLayout) const;
	void						FillBindGroup_Constant_Samplers(BindGroupDesc& bindGroupDesc) const;

	uint						GetRenderPipelineId(const PipelineInputParams& inputParams) const;
	virtual void				FillRenderPipelineDesc(const PipelineInputParams& inputParams, RenderPipelineDesc& renderPipelineDesc) const;
	virtual void				BuildPipelineShaderQuery(Array<EqString>& shaderQuery) const {}

	const PipelineInfo&			EnsureRenderPipeline(IShaderAPI* renderAPI, const PipelineInputParams& inputParams, bool onInit);

	IGPUBindGroupPtr			CreatePersistentBindGroup(BindGroupDesc& bindGroupDesc, EBindGroupId bindGroupId, IShaderAPI* renderAPI, const PipelineInfo& pipelineInfo) const;
	IGPUBindGroupPtr			CreateBindGroup(BindGroupDesc& bindGroupDesc, EBindGroupId bindGroupId, IShaderAPI* renderAPI, const PipelineInfo& pipelineInfo) const;
	IGPUBindGroupPtr			GetEmptyBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const PipelineInfo& pipelineInfo) const;


	template<typename T>
	T GetMaterialValue(const char* param, T defaultValue) const
	{
		using ProxyType = typename MatProxyTypeResolve<T>::ProxyType;
		ProxyType mvParam = FindMaterialVar(param, false);
		return mvParam.IsValid() ? mvParam.Get() : defaultValue;
	}
	MatVarProxyUnk				GetMaterialVar(const char* paramName, const char* defaultValue = nullptr) const;
	MatVarProxyUnk				FindMaterialVar(const char* paramName, bool allowGlobals = true) const;
	MatTextureProxy				FindTextureByVar(const char* paramName, bool errorTextureIfNoVar = true, int texFlags = 0);
	MatTextureProxy				LoadTextureByVar(const char* paramName, bool errorTextureIfNoVar = true, int texFlags = 0);
	void						AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex);

	// makes a texture atlas rectangle collection buffer
	IGPUBufferPtr				CreateAtlasBuffer(IShaderAPI* renderAPI) const;

	// makes a texture transform (scale + offset)
	Vector4D					GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const;

	IMaterial*					m_material{ nullptr };

	MatVec2Proxy				m_baseTextureTransformVar;
	MatVec2Proxy				m_baseTextureScaleVar;
	MatIntProxy					m_baseTextureFrame;

	Array<MatTextureProxy>		m_usedTextures{ PP_SL };

	mutable Map<uint, PipelineInfo>	m_renderPipelines{ PP_SL };
	ETexAddressMode				m_texAddressMode{ TEXADDRESS_WRAP };
	ETexFilterMode				m_texFilter{ TEXFILTER_TRILINEAR_ANISO };
	EShaderBlendMode			m_blendMode{ SHADER_BLEND_NONE };

	Array<EqString>				m_shaderQuery{ PP_SL };
	int							m_shaderQueryId{ 0 };
	int							m_flags{ 0 };
	bool						m_isInit{ false };
};

// DEPRECATED

#define SetParameterFunctor( type, a)
#define SetupDefaultParameter( type )

#define SHADER_INIT_RENDERPASS_PIPELINE()	bool InitRenderPassPipeline(IShaderAPI* renderAPI)
#define SHADER_DECLARE_PASS(shader)
#define SHADER_DECLARE_FOGPASS(shader)
#define SHADER_SETUP_STAGE()				void SetupShader(IShaderAPI* renderAPI)
#define SHADER_SETUP_CONSTANTS()			void SetupConstants(IShaderAPI* renderAPI, uint paramMask)

#define SHADER_PASS(shader) true
#define SHADER_FOGPASS(shader) true
#define SHADER_BIND_PASS_FOGSELECT(shader) 
#define SHADER_BIND_PASS_SIMPLE(shader)	
#define SHADER_PASS_UNLOAD(shader)
#define SHADER_FOGPASS_UNLOAD(shader)

#define SHADERDEFINES_DEFAULTS 
#define SHADERDEFINES_BEGIN EqString defines; EqString findQuery;

#define SHADER_BEGIN_DEFINITION(b, def)	\
	if(b) {
#define SHADER_DECLARE_SIMPLE_DEFINITION(b, def)
#define SHADER_ADD_FLOAT_DEFINITION(def, num)
#define SHADER_ADD_INT_DEFINITION(def, num)	
#define SHADER_END_DEFINITION \
	}

#define SHADER_FIND_OR_COMPILE(shader, sname)
#define SHADER_FIND_OR_COMPILE_FOG(shader, sname)