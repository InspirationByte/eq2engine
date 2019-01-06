//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Editor definition file
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITYDEF_H
#define ENTITYDEF_H

#include "DebugInterface.h"
#include "IEqModel.h"
#include "utils/EqString.h"
#include "math/Vector.h"
#include "math/Matrix.h"

// All these structs contains only default values, and you shouldn't modify it.

enum eDef_ParamType_e
{
	PARAM_TYPE_INVALID = -1,

	PARAM_TYPE_VOID = 0,

	PARAM_TYPE_BOOL,
	PARAM_TYPE_INT,
	PARAM_TYPE_CHOICE,			// string value.
	PARAM_TYPE_FLAG,			// flag. integer.

	PARAM_TYPE_STRING,			// string type, with maximum of EDEF_MAX_STRING_LENGTH
	PARAM_TYPE_MODEL,			// same as string
	PARAM_TYPE_TEXTURE,
	PARAM_TYPE_MATERIAL,
	PARAM_TYPE_SOUND,			

	PARAM_TYPE_FLOAT,
	PARAM_TYPE_VEC2,
	PARAM_TYPE_VEC3,
	PARAM_TYPE_VEC4,

	PARAM_TYPE_COLOR3,			// color, rgb
	PARAM_TYPE_COLOR4,			// color, rgba

	PARAM_TYPE_ENTITY,			// entity. usually for target param. as PARAM_TYPE_STRING
	PARAM_TYPE_TARGETPOINT,		// light target. as PARAM_TYPE_VEC3
	PARAM_TYPE_RADIUS,			// radius. as PARAM_TYPE_FLOAT
	PARAM_TYPE_SPOTRADIUS,			// radius. as PARAM_TYPE_FLOAT

	PARAM_TYPE_COUNT,
};

// parser-use only
static char* param_type_text[PARAM_TYPE_COUNT] =
{
	"void",
	"bool",
	"int",
	"choice",
	"flag",
	"string",
	"model",
	"texture",
	"material",
	"sound",
	"float",
	"vec2",
	"vec3",
	"vec4",
	"color3",
	"color4",
	"entity",
	"targetpoint",
	"radius",
	"spotradius",
};

#define EDEF_MAX_STRING_LENGTH 256

// input
struct edef_input_t
{
	EqString name;
	eDef_ParamType_e valuetype;
};

// output
struct edef_output_t
{
	EqString name;
};

// choice value
struct edef_choiceval_t
{
	EqString value;
	EqWString desc;
};

// choice type
struct edef_choice_t
{
	edef_choiceval_t*	choice_list;
	int					choice_num;
};

// choice type
struct edef_string_t
{
	EqString data;
};

// parameter
struct edef_param_t
{
	eDef_ParamType_e	type;			// type
	EqString			typeString;
	EqString			key;			// key text (for KV)
	EqString			description;	// description text

	// default values
	edef_string_t	string;
	edef_choice_t	choice;

	union
	{
		int				ival;
		float			fval;
		bool			bval;
	};
};

// entity
struct edef_entity_t
{
	EqString				classname;		// entity classname
	EqWString				description;	// description text

	edef_entity_t*			basedef;		// base definition
	bool					showinlist;		// show it in list

	bool					showanglearrow;		// shows entity angle arrow

	bool					showradius;

	ColorRGBA				drawcolor;		// draw color
	bool					colorfromkey;
	EqString				modelname;		// model name
	IEqModel*				model;			// model reference.
	IMaterial*				sprite;
	float					spritesize;

	Vector3D				bboxmins;		// bounding box. For
	Vector3D				bboxmaxs;

	DkList<edef_param_t>	parameters;		// entity parameters
	DkList<edef_output_t>	outputs;
	DkList<edef_input_t>	inputs;
};

Vector2D			UTIL_StringToVector2(const char *str);
ColorRGB			UTIL_StringToColor3(const char *str);
ColorRGBA			UTIL_StringToColor4(const char *str);

struct evariable_t
{
public:
	evariable_t() : varType(PARAM_TYPE_VOID), iVal(0) {}

	eDef_ParamType_e	GetVarType()			{return varType;}

	inline bool			GetBool() const			{ return( varType == PARAM_TYPE_BOOL ) ? bVal : false; }
	inline const char*	GetString() const		{ return( varType == PARAM_TYPE_STRING || varType == PARAM_TYPE_ENTITY || varType == PARAM_TYPE_SOUND ) ? pszVal : "(null)"; }
	inline int			GetInt() const			{ return( varType == PARAM_TYPE_INT ) ? iVal : 0; }
	inline float		GetFloat() const		{ return( varType == PARAM_TYPE_FLOAT || varType == PARAM_TYPE_RADIUS ) ? flVal : 0; }

	inline Vector2D		GetVector2D() const		{ return( varType == PARAM_TYPE_VEC2 ) ? Vector2D(vecVal2[0],vecVal2[1]) : Vector2D(0);}
	inline Vector3D		GetVector3D() const		{ return( varType == PARAM_TYPE_VEC3 || varType == PARAM_TYPE_TARGETPOINT ) ? Vector3D(vecVal3[0],vecVal3[1],vecVal3[2]) : Vector3D(0);}
	inline Vector4D		GetVector4D() const		{ return( varType == PARAM_TYPE_VEC4 ) ? Vector4D(vecVal4[0],vecVal4[1],vecVal4[2],vecVal4[3]) : Vector4D(0);}

	inline void			SetValue(char* pszString)
	{
		if(varType == PARAM_TYPE_STRING)
			strcpy(pszVal, pszString);
		else if(varType == PARAM_TYPE_BOOL)
			bVal = atoi(pszString) > 0;
		else if(varType == PARAM_TYPE_INT)
			iVal = atoi(pszString);
		else if(varType == PARAM_TYPE_FLOAT || varType == PARAM_TYPE_RADIUS)
			flVal = atof(pszString);
		else if(varType == PARAM_TYPE_VEC2)
		{
			Vector2D val = UTIL_StringToVector2(pszString);
			vecVal2[0] = val[0];
			vecVal2[1] = val[1];
		}
		else if(varType == PARAM_TYPE_VEC3 || varType == PARAM_TYPE_TARGETPOINT)
		{
			Vector3D val = UTIL_StringToColor3(pszString);
			vecVal3[0] = val[0];
			vecVal3[1] = val[1];
			vecVal3[2] = val[2];
		}
		else if(varType == PARAM_TYPE_VEC4)
		{
			Vector4D val = UTIL_StringToColor4(pszString);
			vecVal4[0] = val[0];
			vecVal4[1] = val[1];
			vecVal4[2] = val[2];
			vecVal4[3] = val[3];
		}
		else if(varType == PARAM_TYPE_ENTITY || varType == PARAM_TYPE_SOUND)
		{
			// set it as string
			strcpy(pszVal, pszString);
		}
	}

	union
	{
		bool		bVal;
		char		pszVal[128];
		int			iVal;
		float		flVal;

		float		vecVal2[2];
		float		vecVal3[3];
		float		vecVal4[4];
		//void*		pEntPtr;
	};

	eDef_ParamType_e varType;
};

struct OutputData_t // entity output
{
	EqString	szOutputName;
	EqString	szOutputTarget;
	EqString	szTargetInput;
	EqString	szOutputValue;

	float	fDelay;
	int		nFireTimes;
};

bool				EDef_Load(const char* filename, bool clean = true);
void				EDef_Flush();
void				EDef_Unload();
edef_entity_t*		EDef_Find(const char* pszClassName);
edef_param_t*		EDef_FindParameter(const char* pszName, edef_entity_t* pDef);
void				EDef_GetFullParamList(DkList<edef_param_t*> &list, edef_entity_t* pDef);
void				EDef_GetFullOutputList(DkList<edef_output_t*> &list, edef_entity_t* pDef);
void				EDef_GetFullInputList(DkList<edef_input_t*> &list, edef_entity_t* pDef);
eDef_ParamType_e	EDef_ResolveParamTypeFromString(const char* pStr);

extern DkList<edef_entity_t*> g_defs;
extern EqString g_entdefname;

#endif // ENTITYDEF_H