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

#define SHADER_PARAM_TEXTURE(param, variable)			{ variable = LoadTextureByVar(renderAPI, #param, true); }
#define SHADER_PARAM_TEXTURE_NOERROR(param, variable)	{ variable = LoadTextureByVar(renderAPI, #param, false); }
#define SHADER_PARAM_TEXTURE_FIND(param, variable)		{ variable = FindTextureByVar(renderAPI, #param, false); }

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

	virtual IGPURenderPipelinePtr	GetRenderPipeline(IShaderAPI* renderAPI, const IGPURenderPassRecorder* renderPass, const IVertexFormat* vertexLayout, int vertexLayoutUsedBufferBits, EPrimTopology primTopology, const void* userData) const;
	IGPUPipelineLayoutPtr		GetPipelineLayout() const;

	IGPUBindGroupPtr			GetBindGroup(uint frameIdx, EBindGroupId bindGroupId, IShaderAPI* renderAPI, const void* userData) const;
	void						SetLastFrame(uint frame) { m_lastFrame = frame; }

	// returns base texture from shader
	virtual const ITexturePtr&	GetBaseTexture(int stage) const	{ return ITexturePtr::Null(); };
	virtual const ITexturePtr&	GetBumpTexture(int stage) const	{ return ITexturePtr::Null(); };

protected:
	virtual IGPUBindGroupPtr	GetBindGroup(EBindGroupId bindGroupId, IShaderAPI* renderAPI, const void* userData) const { return nullptr; }
	virtual void				FillBindGroupLayout_Constant(BindGroupLayoutDesc& bindGroupLayout) const {}
	virtual void				FillBindGroupLayout_RenderPass(BindGroupLayoutDesc& bindGroupLayout) const {}
	virtual void				FillBindGroupLayout_Transient(BindGroupLayoutDesc& bindGroupLayout) const {}

	virtual void				FillRenderPipelineDesc(const IGPURenderPassRecorder* renderPass, const IVertexFormat* vertexLayout, int vertexLayoutUsedBufferBits, EPrimTopology primitiveTopology, RenderPipelineDesc& renderPipelineDesc) const;
	virtual void				BuildPipelineShaderQuery(const IVertexFormat* vertexLayout, Array<EqString>& shaderQuery) const {}
	
	IGPUBindGroupPtr			GetEmptyBindGroup(EBindGroupId bindGroupId, IShaderAPI* renderAPI) const;
	IGPUBufferPtr				GetRenderPassCameraParamsBuffer(IShaderAPI* renderAPI, bool worldViewProj = false) const;

	void						GetCameraParams(MatSysCamera& cameraParams, bool worldViewProj = false) const;

	void						FillPipelineLayoutDesc(PipelineLayoutDesc& renderPipelineLayoutDesc) const;
	uint						GetRenderPipelineId(const IGPURenderPassRecorder* renderPass, int vertexLayoutNameHash, int vertexLayoutUsedBufferBits, EPrimTopology primitiveTopology) const;

	MatVarProxyUnk				FindMaterialVar(const char* paramName, bool allowGlobals = true) const;
	MatTextureProxy				FindTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar);
	MatTextureProxy				LoadTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar);

	Vector4D					GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const;

	void						AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex);

	IGPUPipelineLayoutPtr		m_pipelineLayout;
	mutable IGPUBindGroupPtr	m_emptyBindGroup[4];
	IMaterial*					m_material{ nullptr };

	MatVec2Proxy				m_baseTextureTransformVar;
	MatVec2Proxy				m_baseTextureScaleVar;
	MatIntProxy					m_baseTextureFrame;

	Array<MatTextureProxy>		m_usedTextures{ PP_SL };

	mutable Map<uint, IGPURenderPipelinePtr>	m_renderPipelines{ PP_SL };
	mutable IGPUBindGroupPtr	m_rendPassBindGroup;
	ETexAddressMode				m_texAddressMode{ TEXADDRESS_WRAP };
	ETexFilterMode				m_texFilter{ TEXFILTER_TRILINEAR_ANISO };
	EShaderBlendMode			m_blendMode{ SHADER_BLEND_NONE };

	uint						m_lastFrame{ UINT_MAX };
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