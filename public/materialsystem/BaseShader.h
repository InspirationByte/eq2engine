//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem/IMaterialSystem.h"
#include "utils/strtools.h"

#define BEGIN_SHADER_CLASS(name)								\
	namespace C##name##ShaderLocalNamespace						\
	{															\
		class C##name##Shader;									\
		typedef C##name##Shader ThisShaderClass;				\
		static const char* ThisClassNameStr = #name;					\
		class C##name##Shader : public CBaseShader				\
		{														\
		public:																	\
			const char* GetName() { return ThisClassNameStr; }					\
			void InitParams() { _ShaderInitParams();CBaseShader::InitParams(); }


#define SHADER_INIT_PARAMS()	void _ShaderInitParams()
#define SHADER_INIT_RHI()		bool _ShaderInitRHI()
#define SHADER_INIT_TEXTURES()	void InitTextures()

#define END_SHADER_CLASS }; DEFINE_SHADER(ThisClassNameStr, ThisShaderClass) }

#define SHADER_DEFINE_VAR(type, name)	\
	type name;

#define SHADER_DECLARE_PASS(shader) \
	IShaderProgram*		m_pShader##shader

#define SHADER_DECLARE_FOGPASS(shader) \
	IShaderProgram*		m_pShader##shader##_fog

#define SHADER_PASS(shader) m_pShader##shader
#define SHADER_FOGPASS(shader) m_pShader##shader##_fog

#define SHADER_BIND_PASS_FOGSELECT(shader)					\
	{														\
	FogInfo_t fog;											\
	materials->GetFogInfo(fog);								\
	if(fog.enableFog && m_fogenabled)										\
		g_pShaderAPI->SetShader(m_pShader##shader##_fog);	\
	else													\
		g_pShaderAPI->SetShader(m_pShader##shader);			\
	}

#define SHADER_BIND_PASS_SIMPLE(shader)						\
	g_pShaderAPI->SetShader(m_pShader##shader);

#define SHADER_PASS_UNLOAD(shader) \
	g_pShaderAPI->DestroyShaderProgram(m_pShader##shader);\
	m_pShader##shader## = NULL;

#define SHADER_FOGPASS_UNLOAD(shader) \
	g_pShaderAPI->DestroyShaderProgram(m_pShader##shader##_fog);\
	m_pShader##shader##_fog = NULL;

#define SHADER_PARAM_FLAG(param, variable, flag, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable |= (mv_##param->GetInt() > 0) ? flag : 0; else variable |= def ? flag : 0;

#define SHADER_PARAM_STRING(param, variable, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetString(); else variable = def;

#define SHADER_PARAM_BOOL(param, variable, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = (mv_##param->GetInt() > 0); else variable = def;

#define SHADER_PARAM_BOOL_NEG(param, variable, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = !(mv_##param->GetInt() > 0); else variable = !def;

#define SHADER_PARAM_INT(param, variable, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetInt(); else variable = def;

#define SHADER_PARAM_FLOAT(param, variable, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetFloat(); else variable = def;

#define SHADER_PARAM_VECTOR3(param, variable, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetVector(); else variable = def;

#define SHADER_PARAM_TEXTURE(param, variable) \
	{\
	IMatVar *mv_##param;\
			\
	if(materials->GetConfiguration().editormode)		\
	{		\
		mv_##param = GetAssignedMaterial()->FindMaterialVar(#param "_editor"); \
		if(!mv_##param)\
			mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	}		\
	else\
	{	\
		mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	}	\
	\
	if(mv_##param) {\
		variable = g_pShaderAPI->LoadTexture(mv_##param->GetString(),m_nTextureFilter,m_nAddressMode);	\
		if(variable){\
			AddManagedTexture(mv_##param, &variable);\
		}else{\
			variable = g_pShaderAPI->GetErrorTexture();\
		}\
	}\
	else\
		variable = g_pShaderAPI->GetErrorTexture();\
	}

#define SHADER_PARAM_RENDERTARGET_FIND(param, variable) \
	{	\
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) {\
		variable = g_pShaderAPI->FindTexture(mv_##param->GetString());	\
		if(variable){\
			mv_##param->AssignTexture(variable);\
		}\
	}\
	else\
		variable = g_pShaderAPI->GetErrorTexture();\
	}

#define SHADER_PARAM_TEXTURE_NOERROR(param, variable) \
	{	\
	IMatVar *mv_##param = NULL;variable = NULL;\
			\
	if(materials->GetConfiguration().editormode)		\
	{		\
		mv_##param = GetAssignedMaterial()->FindMaterialVar(#param "_editor"); \
		if(!mv_##param)\
			mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	}		\
	else\
	{	\
		mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	}	\
	\
	if(mv_##param) {\
		variable = g_pShaderAPI->LoadTexture(mv_##param->GetString(),m_nTextureFilter,m_nAddressMode, TEXFLAG_NULL_ON_ERROR);	\
		if(variable){ \
			AddManagedTexture(mv_##param, &variable);\
	}}\
	}

#define SHADERDEFINES_DEFAULTS \
	SHADER_DECLARE_SIMPLE_DEFINITION(materials->GetConfiguration().lowShaderQuality, "LOWQUALITY");\
	SHADER_DECLARE_SIMPLE_DEFINITION(materials->GetConfiguration().editormode, "EDITOR");

#define SHADERDEFINES_BEGIN \
	EqString defines;				\
	EqString findQuery;				\
	SHADERDEFINES_DEFAULTS

#define SHADER_BEGIN_DEFINITION(b, def)			\
	if(b)										\
	{											\
		defines.Append("#define " def "\n");		\
		findQuery.Append("_" def);

#define SHADER_DECLARE_SIMPLE_DEFINITION(b, def)			\
	if(b)										\
	{											\
		defines.Append("#define " def "\n");		\
		findQuery.Append("_" def);					\
	}

#define SHADER_ADD_FLOAT_DEFINITION(def, num)		\
	defines.Append(varargs("#define " def " %g\n", num));\
	findQuery.Append(varargs("_" def "%g", num));

#define SHADER_ADD_INT_DEFINITION(def, num)		\
	defines.Append(varargs("#define " def " %d\n", num));\
	findQuery.Append(varargs("_" def "%d", num));

#define SHADER_END_DEFINITION				\
	}

#define SHADER_FIND_OR_COMPILE(shader, sname)															\
	{																									\
	m_pShader##shader = g_pShaderAPI->FindShaderProgram(sname, findQuery.GetData());					\
	if(!m_pShader##shader)																				\
	{																									\
		m_pShader##shader = g_pShaderAPI->CreateNewShaderProgram(sname, findQuery.GetData());			\
		bool status	= g_pShaderAPI->LoadShadersFromFile(m_pShader##shader, sname, defines.GetData());	\
		if(!status){																					\
			g_pShaderAPI->DestroyShaderProgram(m_pShader##shader);										\
			return false;																				\
		}																								\
	}																									\
	AddManagedShader(&m_pShader##shader);															\
	}

class CBaseShader;

// this is a special callback for shader parameter binding
typedef void (CBaseShader::*SHADERPARAMFUNC)(void);

struct mvUseTexture_t
{
	ITexture**	texture;
	IMatVar*	var;
};

// base shader class
class CBaseShader : public IMaterialSystemShader
{
public:
								CBaseShader();

	// Sets parameters
	void						Init(IMaterial* assignee) {m_pAssignedMaterial = assignee; InitParams();}

	virtual void				InitParams();
	void						InitShader();

	// Unload shaders, textures
	virtual void				Unload();

	// Get real shader name
	const char*					GetName() = 0;

	bool						IsError();	// Is error shader?

	bool						IsInitialized();	// Is initialized?

	IMaterial*					GetAssignedMaterial();	// Get material assigned to this shader

	int							GetFlags();	// get flags

	Vector4D					GetTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar);	// get texture transformation from vars

	void						SetupVertexShaderTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar, const char* pszConstName);	// sends texture transformation to shader


	int							GetBaseTextureStageCount()	{return 1;}
	int							GetBumpStageCount()			{return 0;}

protected:

	void						ParamSetup_CurrentAsBaseTexture();

	virtual void				ParamSetup_AlphaModel_Solid();
	virtual void				ParamSetup_AlphaModel_Alphatest();
	virtual void				ParamSetup_AlphaModel_Translucent();
	virtual void				ParamSetup_AlphaModel_Additive();
	virtual void				ParamSetup_AlphaModel_Additive_Fog();
	virtual void				ParamSetup_AlphaModel_Modulate();
	virtual void				ParamSetup_DepthSetup();

	virtual void				ParamSetup_RasterState();
	virtual void				ParamSetup_Transform();
	virtual void				ParamSetup_TextureFrames();
	virtual void				ParamSetup_Fog();
	virtual void				ParamSetup_Cubemap() {}

	virtual bool				_ShaderInitRHI() = 0;

	void						AddManagedShader(IShaderProgram** pShader);
	void						AddManagedTexture(IMatVar* var, ITexture** tex);

	void						SetupDefaultParameter(ShaderDefaultParams_e paramtype);

	void						EmptyFunctor() {}

	SHADERPARAMFUNC				m_param_functors[SHADERPARAM_COUNT];

	AddressMode_e				m_nAddressMode;
	Filter_e					m_nTextureFilter;

	IMaterial*					m_pAssignedMaterial;

	int							m_nFlags; // shader flags

	bool						m_depthwrite : 1;
	bool						m_depthtest : 1;
	bool						m_fogenabled : 1;
	bool						m_msaaEnabled : 1;
	bool						m_polyOffset : 1;

	bool						m_bIsError : 1;
	bool						m_bInitialized : 1;

	IMatVar*					m_pBaseTextureTransformVar;
	IMatVar*					m_pBaseTextureScaleVar;
	IMatVar*					m_pBaseTextureFrame;

	DkList<mvUseTexture_t>		m_UsedTextures;

	DkList<IShaderProgram**>	m_UsedPrograms;
	DkList<IRenderState**>		m_UsedRenderStates;
};

#define SetParameterFunctor( type, a) m_param_functors[type] = (static_cast <SHADERPARAMFUNC>(a))
