//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base shader public code
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem/IMaterialSystem.h"

#define DECLARE_SHADER(classname)	\
	class C##classname##Shader : public CBaseShader

#define REGISTER_SHADER(classname) \
	DEFINE_SHADER(classname, C##classname##Shader)

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

#define SHADER_PARAM_STRING(param, variable) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetString();

#define SHADER_PARAM_BOOL(param, variable) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = (mv_##param->GetInt() > 0);

#define SHADER_PARAM_INT(param, variable) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetInt();

#define SHADER_PARAM_FLOAT(param, variable) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetFloat();

#define SHADER_PARAM_VECTOR3(param, variable) \
	IMatVar *mv_##param = GetAssignedMaterial()->FindMaterialVar(#param); \
	if(mv_##param) variable = mv_##param->GetVector();

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
		mv_##param->AssignTexture(variable);\
		variable->Ref_Grab();	\
		AddTextureToAutoremover(&variable);\
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
		variable = g_pShaderAPI->LoadTexture(mv_##param->GetString(),m_nTextureFilter,m_nAddressMode);	\
		if(variable){ \
			mv_##param->AssignTexture(variable);\
			variable->Ref_Grab();	\
			AddTextureToAutoremover(&variable);\
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
		findQuery.Append("_"def);					\
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
		if(!status) return false;														\
	}																					\
	else																				\
		m_pShader##shader->Ref_Grab();													\
	AddShaderToAutoremover(&m_pShader##shader);											\
	}

class CBaseShader;

// this is a special callback for shader parameter binding
typedef void (CBaseShader::*SHADERPARAMFUNC)(void);

// base shader class
class CBaseShader : public IMaterialSystemShader
{
public:
	friend class				CMaterial;

								CBaseShader();

	// Sets parameters
	virtual void				InitParams();

	// Unload shaders, textures
	virtual void				Unload(bool bUnloadShaders = true, bool bUnloadTextures = true);

	// Get real shader name
	const char*					GetName() = 0;

	bool						IsError();	// Is error shader?

	bool						IsInitialized();	// Is initialized?

	IMaterial*					GetAssignedMaterial();	// Get material assigned to this shader

	int							GetFlags();	// get flags

	Vector4D					GetTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar);	// get texture transformation from vars

	void						SetupVertexShaderTextureTransform(IMatVar* pTransformVar, IMatVar* pScaleVar, char* pszConstName);	// sends texture transformation to shader


	int							GetBaseTextureStageCount()	{return 1;}
	int							GetBumpStageCount()			{return 0;}

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

	//virtual void				SetConstant();

// changed from private to do less recalls
protected:

	void						AddShaderToAutoremover(IShaderProgram** pShader);
	void						AddTextureToAutoremover(ITexture** pShader);

	void						SetupDefaultParameter(ShaderDefaultParams_e paramtype);

	void						EmptyFunctor() {}

	IMaterial*					m_pAssignedMaterial;

	int							m_nFlags; // shader flags

	bool						m_bNoCull; // Two-sided textures support

	bool						m_bIsError;
	bool						m_bInitialized;

	AddressMode_e				m_nAddressMode;
	Filter_e					m_nTextureFilter;

	SHADERPARAMFUNC				m_param_functors[SHADERPARAM_COUNT];

	bool						m_depthwrite;
	bool						m_depthtest;
	bool						m_fogenabled;

	IMatVar*					m_pBaseTextureTransformVar;
	IMatVar*					m_pBaseTextureScaleVar;
	IMatVar*					m_pBaseTextureFrame;

	DkList<ITexture**>			m_UsedTextures;
	DkList<IShaderProgram**>	m_UsedPrograms;
	DkList<IRenderState**>		m_UsedRenderStates;
};

#define SetParameterFunctor( type, a) m_param_functors[type] = (static_cast <SHADERPARAMFUNC>(a))
