//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ShaderAPI_defs.h"

class IMaterial;
class IShaderProgram;
class IShaderAPI;
struct MatSysCamera;

using IShaderProgramPtr = CRefPtr<IShaderProgram>;

#define BEGIN_SHADER_CLASS(name)								\
	namespace C##name##ShaderLocalNamespace						\
	{															\
		class C##name##Shader;									\
		typedef C##name##Shader ThisShaderClass;				\
		static const char* ThisClassNameStr = #name;			\
		class C##name##Shader : public CBaseShader {			\
		public:													\
			const char* GetName() const	{ return ThisClassNameStr; } \
			void Init(IShaderAPI* renderAPI, IMaterial* material) override \
			{ CBaseShader::Init(renderAPI, material); ShaderInitParams(renderAPI); }

#define END_SHADER_CLASS }; DEFINE_SHADER(ThisClassNameStr, ThisShaderClass) }

#define SHADER_INIT_PARAMS()				void ShaderInitParams(IShaderAPI* renderAPI)
#define SHADER_INIT_TEXTURES()				void InitTextures(IShaderAPI* renderAPI)


#define _SHADER_PARAM_OP_EMPTY
#define _SHADER_PARAM_OP_NOT !

#define _SHADER_PARAM_INIT(param, variable, def, type, op) { \
		Mat##type##Proxy mv_##param = FindMaterialVar(#param, false); \
		variable = mv_##param.IsValid() ? op mv_##param.Get() : op def; \
	}

#define SHADER_PARAM_FLAG(param, variable, flag, enableByDefault) { \
		MatIntProxy mv_##param = FindMaterialVar(#param, false); \
		const bool value = mv_##param.IsValid() ? mv_##param.Get() : enableByDefault; \
		if( value ) variable |= flag; \
	}

#define SHADER_PARAM_FLAG_NEG(param, variable, flag, enableByDefault) { \
		MatIntProxy mv_##param = FindMaterialVar(#param, false); \
		const bool value = mv_##param.IsValid() ? !mv_##param.Get() : enableByDefault; \
		if( value ) variable |= flag; \
	}

#define SHADER_PARAM_ENUM(param, variable, type, enumValue) { \
		MatIntProxy mv_##param = FindMaterialVar(#param, false); \
		if(mv_##param.IsValid()) variable = (type)(mv_##param.Get() ? enumValue : 0); \
	}

#define SHADER_PARAM_BOOL(param, variable, def)			_SHADER_PARAM_INIT(param, variable, def, Int, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_BOOL_NEG(param, variable, def)		_SHADER_PARAM_INIT(param, variable, def, Int, _SHADER_PARAM_OP_NOT)
#define SHADER_PARAM_INT(param, variable, def)			_SHADER_PARAM_INIT(param, variable, def, Int, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_FLOAT(param, variable, def)		_SHADER_PARAM_INIT(param, variable, def, Float, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_VECTOR2(param, variable, def)		_SHADER_PARAM_INIT(param, variable, def, Vec2, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_VECTOR3(param, variable, def)		_SHADER_PARAM_INIT(param, variable, def, Vec3, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_VECTOR4(param, variable, def)		_SHADER_PARAM_INIT(param, variable, def, Vec4, _SHADER_PARAM_OP_EMPTY)

#define SHADER_PARAM_TEXTURE(param, variable, ...)			{ variable = LoadTextureByVar(renderAPI, #param, true, __VA_ARGS__); }
#define SHADER_PARAM_TEXTURE_NOERROR(param, variable, ...)	{ variable = LoadTextureByVar(renderAPI, #param, false, __VA_ARGS__); }
#define SHADER_PARAM_TEXTURE_FIND(param, variable, ...)		{ variable = FindTextureByVar(renderAPI, #param, false, __VA_ARGS__); }

class CBaseShader;

// base shader class
class CBaseShader : public IMatSystemShader
{
public:
	CBaseShader();

	void						Unload();

	virtual void				Init(IShaderAPI* renderAPI, IMaterial* material);
	void						InitShader(IShaderAPI* renderAPI);

	bool						IsInitialized() const { return m_isInit; }
	int							GetFlags() const { return m_flags; }

	virtual void				UpdateProxy(IGPUCommandRecorder* cmdRecorder) const {}

	// returns base texture from shader
	virtual const ITexturePtr&	GetBaseTexture(int stage) const	{ return ITexturePtr::Null(); };
	virtual const ITexturePtr&	GetBumpTexture(int stage) const	{ return ITexturePtr::Null(); };

	virtual bool				SetupRenderPass(IShaderAPI* renderAPI, const MeshInstanceFormatRef& meshInstFormat, EPrimTopology primTopology, ArrayCRef<RenderBufferInfo> uniformBuffers, IGPURenderPassRecorder* rendPassRecorder, const void* userData);

protected:
	struct PipelineInfo
	{
		mutable IGPUBindGroupPtr	emptyBindGroup[4];
		IGPURenderPipelinePtr		pipeline;
		IGPUPipelineLayoutPtr		layout;
		int							vertexLayoutNameHash;
	};

	virtual IGPUBindGroupPtr	GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const PipelineInfo& pipelineInfo, const IGPURenderPassRecorder* rendPassRecorder, ArrayCRef<RenderBufferInfo> uniformBuffers, const void* userData) const { return nullptr; }
	virtual void				FillBindGroupLayout_Constant(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const {}
	virtual void				FillBindGroupLayout_RenderPass(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const {}
	virtual void				FillBindGroupLayout_Transient(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const {}

	void						FillBindGroupLayout_Constant_Samplers(BindGroupLayoutDesc& bindGroupLayout) const;
	void						FillBindGroup_Constant_Samplers(BindGroupDesc& bindGroupDesc) const;

	virtual void				FillRenderPipelineDesc(const IGPURenderPassRecorder* renderPass, const MeshInstanceFormatRef& meshInstFormat, EPrimTopology primitiveTopology, RenderPipelineDesc& renderPipelineDesc) const;
	virtual void				BuildPipelineShaderQuery(const MeshInstanceFormatRef& meshInstFormat, Array<EqString>& shaderQuery) const {}
	
	IGPUBindGroupPtr			CreateBindGroup(BindGroupDesc& bindGroupDesc, EBindGroupId bindGroupId, IShaderAPI* renderAPI, const PipelineInfo& pipelineInfo) const;

	IGPUBindGroupPtr			GetEmptyBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const PipelineInfo& pipelineInfo) const;
	uint						GetRenderPipelineId(const IGPURenderPassRecorder* renderPass, int vertexLayoutNameHash, uint usedVertexLayoutBits, EPrimTopology primitiveTopology) const;

	MatVarProxyUnk				FindMaterialVar(const char* paramName, bool allowGlobals = true) const;
	MatTextureProxy				FindTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar, int texFlags = 0);
	MatTextureProxy				LoadTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar, int texFlags = 0);

	Vector4D					GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const;

	void						AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex);

	IMaterial*					m_material{ nullptr };

	MatVec2Proxy				m_baseTextureTransformVar;
	MatVec2Proxy				m_baseTextureScaleVar;
	MatIntProxy					m_baseTextureFrame;

	Array<MatTextureProxy>		m_usedTextures{ PP_SL };

	mutable Map<uint, PipelineInfo>	m_renderPipelines{ PP_SL };
	ETexAddressMode				m_texAddressMode{ TEXADDRESS_WRAP };
	ETexFilterMode				m_texFilter{ TEXFILTER_TRILINEAR_ANISO };
	EShaderBlendMode			m_blendMode{ SHADER_BLEND_NONE };

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