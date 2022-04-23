//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASESHADER_H
#define BASESHADER_H

#include "materialsystem1/IMaterialSystem.h"
#include "utils/strtools.h"
#include "scene_def.h"

#define BEGIN_SHADER_CLASS(name)								\
	namespace C##name##ShaderLocalNamespace						\
	{															\
		class C##name##Shader;									\
		typedef C##name##Shader ThisShaderClass;				\
		static const char* ThisClassNameStr = #name;			\
		class C##name##Shader : public CBaseShader {			\
		public:													\
			const char* GetName() const		{ return ThisClassNameStr; }					\
			void InitParams()				{ _ShaderInitParams();CBaseShader::InitParams(); }


#define SHADER_INIT_PARAMS()		void _ShaderInitParams()
#define SHADER_INIT_RHI()			bool _ShaderInitRHI()
#define SHADER_INIT_TEXTURES()		void InitTextures()
#define SHADER_SETUP_STAGE()		void SetupShader()
#define SHADER_SETUP_CONSTANTS()	void SetupConstants(uint paramMask)

#define END_SHADER_CLASS }; DEFINE_SHADER(ThisClassNameStr, ThisShaderClass) }

#define SHADER_DECLARE_PASS(shader) \
	IShaderProgram*		m_pShader##shader{nullptr}

#define SHADER_DECLARE_FOGPASS(shader) \
	IShaderProgram*		m_pShader##shader##_fog{nullptr}

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
	m_pShader##shader## = nullptr;

#define SHADER_FOGPASS_UNLOAD(shader) \
	g_pShaderAPI->DestroyShaderProgram(m_pShader##shader##_fog);\
	m_pShader##shader##_fog = nullptr;

#define _SHADER_PARAM_OP_EMPTY
#define _SHADER_PARAM_OP_NOT !

#define _SHADER_PARAM_INIT(param, variable, def, getFunc, op) { \
	IMatVar* mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	variable = mv_##param ? op mv_##param->getFunc() : op def; }

#define SHADER_PARAM_FLAG(param, variable, flag, def) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable |= mv_##param->GetInt() ? flag : 0; else variable |= def ? flag : 0;

#define SHADER_PARAM_STRING(param, variable, def)  _SHADER_PARAM_INIT(param, variable, def, GetString, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_BOOL(param, variable, def) _SHADER_PARAM_INIT(param, variable, def, GetInt, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_BOOL_NEG(param, variable, def)  _SHADER_PARAM_INIT(param, variable, def, GetInt, _SHADER_PARAM_OP_NOT)
#define SHADER_PARAM_INT(param, variable, def)  _SHADER_PARAM_INIT(param, variable, def, GetInt, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_FLOAT(param, variable, def) _SHADER_PARAM_INIT(param, variable, def, GetFloat, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_VECTOR2(param, variable, def) _SHADER_PARAM_INIT(param, variable, def, GetVector2, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_VECTOR3(param, variable, def) _SHADER_PARAM_INIT(param, variable, def, GetVector3, _SHADER_PARAM_OP_EMPTY)
#define SHADER_PARAM_VECTOR4(param, variable, def) _SHADER_PARAM_INIT(param, variable, def, GetVector4, _SHADER_PARAM_OP_EMPTY)

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

#define SHADER_FIND_OR_COMPILE(shader, sname)												\
	{																									\
	m_pShader##shader = g_pShaderAPI->FindShaderProgram(sname, (findQuery).GetData());					\
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

#define SHADER_FIND_OR_COMPILE_FOG(shader, sname)	SHADER_FIND_OR_COMPILE(shader##_fog, sname)

class CBaseShader;

// this is a special callback for shader parameter binding
typedef void (CBaseShader::*SHADERPARAMFUNC)(void);

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

	bool						IsError() const;	// Is error shader?

	bool						IsInitialized() const;	// Is initialized?

	IMaterial*					GetAssignedMaterial() const;	// Get material assigned to this shader

	int							GetFlags() const;	// get flags

	Vector4D					GetTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar) const;	// get texture transformation from vars

	void						SetupVertexShaderTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar, const char* pszConstName);	// sends texture transformation to shader


	int							GetBaseTextureStageCount()  const	{return 1;}
	int							GetBumpStageCount() const			{return 0;}

protected:
	struct mvUseTexture_t
	{
		ITexture** texture;
		IMatVar* var;
	};

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

	void						SetupParameter(uint mask, ShaderDefaultParams_e param);

	void						EmptyFunctor() {}

	Array<mvUseTexture_t>		m_UsedTextures;
	Array<IShaderProgram**>	m_UsedPrograms;
	SHADERPARAMFUNC				m_param_functors[SHADERPARAM_COUNT];

	IMatVar*					m_pBaseTextureTransformVar;
	IMatVar*					m_pBaseTextureScaleVar;
	IMatVar*					m_pBaseTextureFrame;

	IMaterial*					m_pAssignedMaterial;

	ER_TextureAddressMode		m_nAddressMode;
	ER_TextureFilterMode		m_nTextureFilter;

	int							m_nFlags; // shader flags

	bool						m_depthwrite : 1;
	bool						m_depthtest : 1;
	bool						m_fogenabled : 1;
	bool						m_msaaEnabled : 1;
	bool						m_polyOffset : 1;

	bool						m_bIsError : 1;
	bool						m_bInitialized : 1;


};

#define SetParameterFunctor( type, a) m_param_functors[type] = (static_cast <SHADERPARAMFUNC>(a))
#define SetupDefaultParameter( type ) SetupParameter(paramMask, type)

#endif // BASESHADER_H