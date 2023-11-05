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

using IShaderProgramPtr = CRefPtr<IShaderProgram>;

// valid if MATERIAL_FLAG_TRANSPARENT
enum EShaderBlendMode : int
{
	SHADER_BLEND_NONE = 0,
	SHADER_BLEND_TRANSLUCENT,		// is transparent
	SHADER_BLEND_ADDITIVE,			// additive transparency
	SHADER_BLEND_MODULATE,			// modulate
};

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


#define SHADER_INIT_PARAMS()				void ShaderInitParams(IShaderAPI* renderAPI)
#define SHADER_INIT_TEXTURES()				void InitTextures(IShaderAPI* renderAPI)
#define SHADER_INIT_RENDERPASS_PIPELINE()	bool InitRenderPassPipeline(IShaderAPI* renderAPI)

#define SHADER_SETUP_STAGE()				void SetupShader(IShaderAPI* renderAPI)
#define SHADER_SETUP_CONSTANTS()			void SetupConstants(IShaderAPI* renderAPI, uint paramMask)

#define END_SHADER_CLASS }; DEFINE_SHADER(ThisClassNameStr, ThisShaderClass) }

#define SHADER_DECLARE_PASS(shader) \
	IShaderProgramPtr	m_pShader##shader

#define SHADER_DECLARE_FOGPASS(shader) \
	IShaderProgramPtr	m_pShader##shader##_fog

#define SHADER_PASS(shader) m_pShader##shader
#define SHADER_FOGPASS(shader) m_pShader##shader##_fog

#define SHADER_BIND_PASS_FOGSELECT(shader) { \
	FogInfo fog;										\
	g_matSystem->GetFogInfo(fog);						\
	if(fog.enableFog)					\
		renderAPI->SetShader(m_pShader##shader##_fog);	\
	else												\
		renderAPI->SetShader(m_pShader##shader);		\
	}

#define SHADER_BIND_PASS_SIMPLE(shader)	\
	renderAPI->SetShader(m_pShader##shader);

#define SHADER_PASS_UNLOAD(shader) \
	m_pShader##shader## = nullptr;

#define SHADER_FOGPASS_UNLOAD(shader) \
	m_pShader##shader##_fog = nullptr;

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

#define SHADERDEFINES_DEFAULTS \
	SHADER_DECLARE_SIMPLE_DEFINITION(g_matSystem->GetConfiguration().lowShaderQuality, "LOWQUALITY");\
	SHADER_DECLARE_SIMPLE_DEFINITION(g_matSystem->GetConfiguration().editormode, "EDITOR");

#define SHADERDEFINES_BEGIN \
	EqString defines, findQuery; \
	SHADERDEFINES_DEFAULTS

#define SHADER_BEGIN_DEFINITION(b, def)				\
	if(b){											\
		defines.Append("#define " def "\n");		\
		findQuery.Append("_" def);

#define SHADER_DECLARE_SIMPLE_DEFINITION(b, def)			\
	if(b){											\
		defines.Append("#define " def "\n");		\
		findQuery.Append("_" def);					\
	}

#define SHADER_ADD_FLOAT_DEFINITION(def, num)		\
	defines.Append(EqString::Format("#define " def " %g\n", num));\
	findQuery.Append(EqString::Format("_" def "%g", num));

#define SHADER_ADD_INT_DEFINITION(def, num)		\
	defines.Append(EqString::Format("#define " def " %d\n", num));\
	findQuery.Append(EqString::Format("_" def "%d", num));

#define SHADER_END_DEFINITION \
	}

#define SHADER_FIND_OR_COMPILE(shader, sname) \
	{																						\
	m_pShader##shader = renderAPI->FindShaderProgram(sname, (findQuery).GetData());			\
	if(!m_pShader##shader) {																\
		m_pShader##shader = renderAPI->CreateNewShaderProgram(sname, findQuery.GetData());	\
		if(!renderAPI->LoadShadersFromFile(m_pShader##shader, sname, defines.GetData())) {	\
			renderAPI->FreeShaderProgram(m_pShader##shader);								\
			return false;																	\
		}																					\
	}																						\
	AddManagedShader(&m_pShader##shader);													\
	}

#define SHADER_FIND_OR_COMPILE_FOG(shader, sname)	SHADER_FIND_OR_COMPILE(shader##_fog, sname)

class CBaseShader;

// this is a special callback for shader parameter binding
typedef void (CBaseShader::*SHADERPARAMFUNC)(IShaderAPI* renderAPI);

// base shader class
class CBaseShader : public IMatSystemShader
{
public:
	CBaseShader();

	void						Unload();

	virtual void				Init(IShaderAPI* renderAPI, IMaterial* material);
	void						InitShader(IShaderAPI* renderAPI);

	bool						IsError() const { return m_error; }
	bool						IsInitialized() const { return m_isInit; }
	int							GetFlags() const { return m_flags; }

	virtual void				FillShaderBindGroupLayout(BindGroupLayoutDesc& bindGroupLayout) const {}
	virtual void				FillPipelineLayoutDesc(PipelineLayoutDesc& renderPipelineLayoutDesc) const;
	virtual void				FillRenderPipelineDesc(RenderPipelineDesc& renderPipelineDesc) const;

	// returns base texture from shader
	virtual const ITexturePtr&	GetBaseTexture(int stage) const	{ return ITexturePtr::Null(); };
	virtual const ITexturePtr&	GetBumpTexture(int stage) const	{ return ITexturePtr::Null(); };

protected:
	virtual bool				InitRenderPassPipeline(IShaderAPI* renderAPI) = 0;

	MatVarProxyUnk				FindMaterialVar(const char* paramName, bool allowGlobals = true) const;
	MatTextureProxy				FindTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar);
	MatTextureProxy				LoadTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar);

	void						AddManagedShader(IShaderProgramPtr* pShader);
	void						AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex);

	// TODO: Ideally shader is just a pipeline and bind group associated with it
	// IRenderPipeline*			m_renderPipeline{ nullptr };
	// IBindGroup*				m_materialBindGroup{ nullptr };

	IMaterial*					m_material{ nullptr };

	MatVec2Proxy				m_baseTextureTransformVar;
	MatVec2Proxy				m_baseTextureScaleVar;
	MatIntProxy					m_baseTextureFrame;

	Array<MatTextureProxy>		m_usedTextures{ PP_SL };
	Array<IShaderProgramPtr*>	m_usedPrograms{ PP_SL };

	int							m_texAddressMode{ TEXADDRESS_WRAP };
	int							m_texFilter{ TEXFILTER_TRILINEAR_ANISO };
	EShaderBlendMode			m_blendMode{ SHADER_BLEND_NONE };

	int							m_flags{ 0 };
	bool						m_error{ false };
	bool						m_isInit{ false };

	// DEPRECATED all things down below 
	void						SetupParameter(IShaderAPI* renderAPI, uint mask, EShaderParamSetup param);
	Vector4D					GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const;	// get texture transformation from vars
	
	void						ParamSetup_Empty(IShaderAPI* renderAPI) {}
	void						ParamSetup_AlphaModel_Solid(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_Translucent(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_Additive(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_Modulate(IShaderAPI* renderAPI);
	void						ParamSetup_DepthSetup(IShaderAPI* renderAPI);
	void						ParamSetup_RasterState(IShaderAPI* renderAPI);
	void						ParamSetup_RasterState_NoCull(IShaderAPI* renderAPI);
	void						ParamSetup_Transform(IShaderAPI* renderAPI);
	void						ParamSetup_Fog(IShaderAPI* renderAPI);
	void						ParamSetup_BoneTransforms(IShaderAPI* renderAPI);

	SHADERPARAMFUNC				m_paramFunc[SHADERPARAM_COUNT]{ nullptr };
};

// DEPRECATED
#define SetParameterFunctor( type, a) m_paramFunc[type] = (static_cast <SHADERPARAMFUNC>(a))
#define SetupDefaultParameter( type ) SetupParameter(renderAPI, paramMask, type)
