//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IMaterial;
class IShaderProgram;
class IShaderAPI;

using IShaderProgramPtr = CRefPtr<IShaderProgram>;

// valid if MATERIAL_FLAG_TRANSPARENT
enum ShaderBlendMode : int
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
			const char* GetName() const				{ return ThisClassNameStr; }					\
			void InitParams(IShaderAPI* renderAPI)	{ _ShaderInitParams(renderAPI); CBaseShader::InitParams(renderAPI); }


#define SHADER_INIT_PARAMS()		void _ShaderInitParams(IShaderAPI* renderAPI)
#define SHADER_INIT_RHI()			bool _ShaderInitRHI(IShaderAPI* renderAPI)
#define SHADER_INIT_TEXTURES()		void InitTextures(IShaderAPI* renderAPI)
#define SHADER_SETUP_STAGE()		void SetupShader(IShaderAPI* renderAPI)
#define SHADER_SETUP_CONSTANTS()	void SetupConstants(IShaderAPI* renderAPI, uint paramMask)

#define END_SHADER_CLASS }; DEFINE_SHADER(ThisClassNameStr, ThisShaderClass) }

#define SHADER_DECLARE_PASS(shader) \
	IShaderProgramPtr	m_pShader##shader

#define SHADER_DECLARE_FOGPASS(shader) \
	IShaderProgramPtr	m_pShader##shader##_fog

#define SHADER_PASS(shader) m_pShader##shader
#define SHADER_FOGPASS(shader) m_pShader##shader##_fog

#define SHADER_BIND_PASS_FOGSELECT(shader) \
	{													\
	FogInfo fog;										\
	g_matSystem->GetFogInfo(fog);						\
	if(fog.enableFog && m_fogenabled)					\
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
	variable = mv_##param.IsValid() ? op mv_##param.Get() : op def; }

#define SHADER_PARAM_FLAG(param, variable, flag, def) \
	MatIntProxy mv_##param = FindMaterialVar(#param, false); \
	if(mv_##param.IsValid()) variable |= (mv_##param.Get() ? flag : 0); else if(def) variable |= flag;

#define SHADER_PARAM_ENUM(param, variable, enumValue) \
	MatIntProxy mv_##param = FindMaterialVar(#param, false); \
	if(mv_##param.IsValid()) variable = (mv_##param.Get() ? enumValue : 0);

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
class CBaseShader : public IMaterialSystemShader
{
public:
								CBaseShader();

	// Sets parameters
	void						Init(IShaderAPI* renderAPI, IMaterial* assignee);

	virtual void				InitParams(IShaderAPI* renderAPI);
	void						InitShader(IShaderAPI* renderAPI);

	// Unload shaders, textures
	void						Unload();

	bool						IsError() const;	// Is error shader?
	bool						IsInitialized() const;	// Is initialized?

	IMaterial*					GetAssignedMaterial() const;	// Get material assigned to this shade
	int							GetFlags() const;	// get flags

	Vector4D					GetTextureTransform(const MatVec2Proxy& transformVar, const MatVec2Proxy& scaleVar) const;	// get texture transformation from vars

	// returns base texture from shader
	virtual const ITexturePtr&	GetBaseTexture(int stage) const { return ITexturePtr::Null(); };
	virtual const ITexturePtr&	GetBumpTexture(int stage) const { return ITexturePtr::Null(); };

	int							GetBaseTextureStageCount()  const	{ return 1; }
	int							GetBumpStageCount() const			{ return 0; }

protected:
	void						ParamSetup_AlphaModel_Solid(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_Translucent(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_Additive(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_AdditiveLight(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_Additive_Fog(IShaderAPI* renderAPI);
	void						ParamSetup_AlphaModel_Modulate(IShaderAPI* renderAPI);

	void						ParamSetup_DepthSetup(IShaderAPI* renderAPI);
	void						ParamSetup_RasterState(IShaderAPI* renderAPI);
	void						ParamSetup_RasterState_NoCull(IShaderAPI* renderAPI);

	void						ParamSetup_Transform(IShaderAPI* renderAPI);
	void						ParamSetup_Fog(IShaderAPI* renderAPI);
	virtual void				ParamSetup_Cubemap(IShaderAPI* renderAPI);

	virtual bool				_ShaderInitRHI(IShaderAPI* renderAPI) = 0;

	void						ParamSetup_BoneTransforms(IShaderAPI* renderAPI);

	MatVarProxyUnk				FindMaterialVar(const char* paramName, bool allowGlobals = true) const;

	MatTextureProxy				FindTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar);
	MatTextureProxy				LoadTextureByVar(IShaderAPI* renderAPI, const char* paramName, bool errorTextureIfNoVar);

	void						AddManagedShader(IShaderProgramPtr* pShader);
	void						AddManagedTexture(MatTextureProxy var, const ITexturePtr& tex);

	void						SetupParameter(IShaderAPI* renderAPI, uint mask, EShaderParamSetup param);

	void						EmptyFunctor(IShaderAPI* renderAPI) {}

	SHADERPARAMFUNC				m_param_functors[SHADERPARAM_COUNT]{ nullptr };
	Array<MatTextureProxy>		m_UsedTextures{ PP_SL }; // TODO: remove it and capture MatVarProxy instead
	Array<IShaderProgramPtr*>	m_UsedPrograms{ PP_SL };

	MatVec2Proxy				m_baseTextureTransformVar;
	MatVec2Proxy				m_baseTextureScaleVar;
	MatIntProxy					m_baseTextureFrame;
	MatTextureProxy				m_cubemapTexture;

	IMaterial*					m_material{ nullptr };

	int							m_texAddressMode{ TEXADDRESS_WRAP };
	int							m_texFilter{ TEXFILTER_TRILINEAR_ANISO };
	int							m_blendMode{ SHADER_BLEND_NONE };
	int							m_flags{ 0 }; // shader flags

	bool						m_depthwrite : 1;
	bool						m_depthtest : 1;
	bool						m_fogenabled : 1;
	bool						m_msaaEnabled : 1;
	bool						m_polyOffset : 1;

	bool						m_bIsError : 1;
	bool						m_bInitialized : 1;
};

#define SetParameterFunctor( type, a) m_param_functors[type] = (static_cast <SHADERPARAMFUNC>(a))
#define SetupDefaultParameter( type ) SetupParameter(renderAPI, paramMask, type)
