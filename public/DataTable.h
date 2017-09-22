//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity data table for use in savegames and restoring from
//				savegames / parsing entity KVs
//////////////////////////////////////////////////////////////////////////////////

#ifndef DATATABLES_H
#define DATATABLES_H

#include "utils/eqstring.h"
#include "math/Vector.h"
#include "math/Matrix.h"

#include "math/FVector.h"

enum FieldFlag_e
{
	FIELD_KEY			= (1 << 0),	// accessible parameter

	FIELD_FUNCTIONPTR	= (1 << 1),	// This is a function that can be looked up

	FIELD_ARRAY			= (1 << 2),	// simple array
	FIELD_LIST			= (1 << 3), // DkList dynamic array
	FIELD_LINKEDLIST	= (1 << 4), // DkLinkedList by array
};

enum DataVarType_e
{
	// basic types
	DTVAR_TYPE_VOID = 0,

	DTVAR_TYPE_FLOAT,
	DTVAR_TYPE_INTEGER,
	DTVAR_TYPE_BOOLEAN,
	DTVAR_TYPE_SHORT,
	DTVAR_TYPE_BYTE,
	DTVAR_TYPE_STRING,

	// vector types
	DTVAR_TYPE_VECTOR2D,
	DTVAR_TYPE_VECTOR3D,
	DTVAR_TYPE_VECTOR4D,

	DTVAR_TYPE_FVECTOR3D,

	// optimized angles
	DTVAR_TYPE_ANGLES,

	// matrix types
	DTVAR_TYPE_MATRIX2X2,
	DTVAR_TYPE_MATRIX3X3,
	DTVAR_TYPE_MATRIX4X4,

	// a function name (when save/restore is in place, it searches for FIELDFLAG_FUNCTION)
	DTVAR_TYPE_FUNCTION,

	// embedded type with own datamap
	DTVAR_TYPE_NESTED,

	// type count
	DTVAR_TYPE_COUNT,
};

static int s_dataVarTypeSize[DTVAR_TYPE_COUNT] = 
{
	0,
	sizeof(float),
	sizeof(int),
	sizeof(bool),
	sizeof(short),
	sizeof(char),
	sizeof(EqString),

	sizeof(Vector2D),
	sizeof(Vector3D),
	sizeof(Vector4D),

	sizeof(FVector3D),

	sizeof(Vector3D),

	sizeof(Matrix2x2),
	sizeof(Matrix3x3),
	sizeof(Matrix4x4),

	sizeof(intptr_t),

	0,
};

static char* s_dataVarTypeNames[DTVAR_TYPE_COUNT] = 
{
	"void",
	"float",
	"int",
	"bool",
	"short",
	"byte",
	"string",

	"Vector2D",
	"Vector3D",
	"Vector4D",

	"FVector3D",

	"Angles",

	"Matrix2x2",
	"Matrix3x3",
	"Matrix4x4",

	"function",

	"nested",
};

//
// variable_t is used for input/output system
//
class CDataVariable
{
public:
	CDataVariable()	
		: varType(DTVAR_TYPE_VOID) {}

	CDataVariable(DataVarType_e type)	
		: varType(type) {}

	DataVarType_e		GetVarType() const				{return varType;}

	inline bool			GetBool() const					{ return( varType == DTVAR_TYPE_BOOLEAN ) ? bVal : false; }
	inline const char*	GetString() const				{ return( varType == DTVAR_TYPE_STRING ) ? pszVal : "(null)"; }
	inline int			GetInt() const					{ return( varType == DTVAR_TYPE_INTEGER ) ? iVal[0] : 0; }
	inline float		GetFloat() const				{ return( varType == DTVAR_TYPE_FLOAT ) ? flVal[0] : 0; }

	inline Matrix2x2	GetMatrix2x2() const			{ return( varType == DTVAR_TYPE_MATRIX2X2 ) ? Matrix2x2(	flVal[0],flVal[1],flVal[2],flVal[3]) : identity2();}
	inline Matrix3x3	GetMatrix3x3() const			{ return( varType == DTVAR_TYPE_MATRIX3X3 ) ? Matrix3x3(	flVal[0],flVal[1],flVal[2],flVal[3],
																													flVal[4],flVal[5],flVal[6],flVal[7],
																													flVal[8]) : identity3();}

	inline Matrix4x4	GetMatrix4x4() const			{ return( varType == DTVAR_TYPE_MATRIX4X4 ) ? Matrix4x4(	flVal[0],flVal[1],flVal[2],flVal[3],
																													flVal[4],flVal[5],flVal[6],flVal[7],
																													flVal[8],flVal[9],flVal[10],flVal[11],
																													flVal[12],flVal[13],flVal[14],flVal[15]) : identity4();}

	inline Vector2D		GetVector2D() const				{ return( varType == DTVAR_TYPE_VECTOR2D ) ? Vector2D(flVal[0],flVal[1]) : Vector2D(0);}
	inline Vector3D		GetVector3D() const				{ return( varType == DTVAR_TYPE_VECTOR3D ) ? Vector3D(flVal[0],flVal[1],flVal[2]) : GetVector3DFromFixed();}
	inline Vector4D		GetVector4D() const				{ return( varType == DTVAR_TYPE_VECTOR4D ) ? Vector4D(flVal[0],flVal[1],flVal[2],flVal[3]) : Vector4D(0);}

	inline FVector3D	GetFVector3D() const			{ return( varType == DTVAR_TYPE_FVECTOR3D ) ? FVector3D(FReal(iVal[0], 0),FReal(iVal[1], 0), FReal(iVal[2], 0)) : GetFVector3DFromFloat();}

	inline void			SetString(const char* pszStr)		{ strcpy(pszVal, pszStr); varType = DTVAR_TYPE_STRING;}

	inline void			SetBool(bool val)					{ bVal = val; varType = DTVAR_TYPE_BOOLEAN;}
	inline void			SetInt(int val)						{ iVal[0] = val; varType = DTVAR_TYPE_INTEGER;}
	inline void			SetFloat(float val)					{ flVal[0] = val; varType = DTVAR_TYPE_FLOAT;}

	inline void			SetVector2D(const Vector2D& val)	{ *((Vector2D*)flVal) = val; varType = DTVAR_TYPE_VECTOR2D;}
	inline void			SetVector3D(const Vector3D& val)	{ *((Vector3D*)flVal) = val; varType = DTVAR_TYPE_VECTOR3D;}
	inline void			SetVector4D(const Vector4D& val)	{ *((Vector4D*)flVal) = val; varType = DTVAR_TYPE_VECTOR4D;}

	inline void			SetMatrix2x2(const Matrix2x2& val)	{ *((Matrix2x2*)flVal) = val; varType = DTVAR_TYPE_MATRIX2X2;}
	inline void			SetMatrix3x3(const Matrix3x3& val)	{ *((Matrix3x3*)flVal) = val; varType = DTVAR_TYPE_MATRIX3X3;}
	inline void			SetMatrix4x4(const Matrix4x4& val)	{ *((Matrix4x4*)flVal) = val; varType = DTVAR_TYPE_MATRIX4X4;}

	// use this if you want to get different values
	bool				Convert(DataVarType_e newType);

private:

	inline Vector3D		GetVector3DFromFixed() const	{ return( varType == DTVAR_TYPE_FVECTOR3D ) ? Vector3D(FReal(iVal[0], 0),FReal(iVal[1], 0),FReal(iVal[2], 0)) : Vector3D(0);}
	inline FVector3D	GetFVector3DFromFloat() const	{ return( varType == DTVAR_TYPE_VECTOR3D ) ? FVector3D(flVal[0],flVal[1],flVal[2]) : Vector3D(0);}

	union
	{
		bool		bVal;
		char		pszVal[256];

		float		flVal[16];
		int			iVal[3];
	};

	DataVarType_e	varType;
};

struct dataDescMap_t;

#define MAKE_SIMPLE_CONSTRUCT_FUNC(friendlyName, dtvartype)									\
	static dataDescField_t Field_##friendlyName##(const char* name, int offset, int flags){		\
		return dataDescField_t { dtvartype, name, offset, flags, nullptr };						\
	}

// this is used by key fields and save data
struct dataDescField_t
{
	MAKE_SIMPLE_CONSTRUCT_FUNC(Float, DTVAR_TYPE_FLOAT)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Int, DTVAR_TYPE_INTEGER)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Boolean, DTVAR_TYPE_BOOLEAN)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Short, DTVAR_TYPE_SHORT)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Byte, DTVAR_TYPE_BYTE)

	MAKE_SIMPLE_CONSTRUCT_FUNC(String, DTVAR_TYPE_STRING)

	MAKE_SIMPLE_CONSTRUCT_FUNC(Vector2D, DTVAR_TYPE_VECTOR2D)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Vector3D, DTVAR_TYPE_VECTOR3D)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Vector4D, DTVAR_TYPE_VECTOR4D)

	MAKE_SIMPLE_CONSTRUCT_FUNC(FVector3D, DTVAR_TYPE_FVECTOR3D)

	MAKE_SIMPLE_CONSTRUCT_FUNC(Angles, DTVAR_TYPE_ANGLES)

	MAKE_SIMPLE_CONSTRUCT_FUNC(Matrix2x2, DTVAR_TYPE_MATRIX2X2)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Matrix3x3, DTVAR_TYPE_MATRIX3X3)
	MAKE_SIMPLE_CONSTRUCT_FUNC(Matrix4x4, DTVAR_TYPE_MATRIX4X4)

	DataVarType_e			type;

	// name (key)
	const char*				name;

	// object offset in bytes
	int						offset;

	// field flags
	int						nFlags;

	// for embedding
	dataDescMap_t*			dataMap;
};

// the datamap
struct dataDescMap_t
{
	const char*			dataClassName;
	int					dataSize;		// for array enumeration

	dataDescMap_t*		baseMap;

	int16				numFields;

	dataDescField_t*	fields;
};

// debug functions
void DataMap_PrintFlagsStr(int flags);
void DataMap_Print(dataDescMap_t* dataMap, int spaces = 0);

//
// this is a compile-time data description map creator
//

#define BEGIN_DATAMAP_GUTS( className ) \
	template <typename T> dataDescMap_t* DataMapInit(T *); \
	template <> dataDescMap_t* DataMapInit<className>( className * ); \
	namespace className##_DataDescInit /* namespaces are ideal for this */ \
	{ \
		dataDescMap_t* g_DataMapHolder = DataMapInit( (className *)NULL );  \
	} \
	\
	template <> dataDescMap_t* DataMapInit<className>( className * ) \
	{ \
		typedef className ThisClass; \
		static mappedVar_t dataDesc[] = \
		{ \
			{ DTVAR_TYPE_VOID, nullptr, 0, 0, nullptr},	/* An empty element because we can't have empty array */

#define END_DATAMAP() \
		}; \
		\
		if ( sizeof( dataDesc ) > sizeof( dataDesc[0] ) ) \
		{ \
			ThisClass::m_DataMap.numFields = elementsOf( dataDesc ) - 1; \
			ThisClass::m_DataMap.fields = &dataDesc[1]; \
		} \
		else \
		{ \
			ThisClass::m_DataMap.numFields = 1; \
			ThisClass::m_DataMap.fields = dataDesc; \
		} \
		return &ThisClass::m_DataMap; \
	}

#define BEGIN_DATAMAP( className ) \
	dataDescMap_t className::m_DataMap = { #className, sizeof(ThisClass), &BaseClass::m_DataMap, 0, NULL }; \
	dataDescMap_t* className::GetDataDescMap( void ) { return &m_DataMap; } \
	BEGIN_DATAMAP_GUTS( className )

//
#define BEGIN_DATAMAP_NO_BASE( className ) \
	dataDescMap_t className::m_DataMap = { #className, sizeof(ThisClass), NULL, 0, NULL }; \
	dataDescMap_t* className::GetDataDescMap( void ) { return &m_DataMap; } \
	BEGIN_DATAMAP_GUTS( className )

// creates data description map for structures
#define DECLARE_SIMPLE_DATAMAP() \
	static dataDescMap_t m_DataMap; \
	template <typename T> friend dataDescMap_t* DataMapInit(T *);

// creates data description map
#define	DECLARE_DATAMAP() \
	DECLARE_SIMPLE_DATAMAP() \
	virtual dataDescMap_t* GetDataDescMap( void );

#define _NAMEFIELD(var, name, fieldtype, flags)			mappedVar_t::Field_##fieldtype##(name, offsetOf(ThisClass, var), flags)
#define _FIELD(name, fieldtype, flags)					mappedVar_t::Field_##fieldtype##(#name, offsetOf(ThisClass, name), flags)

#define DEFINE_FIELD(name,fieldtype)					_FIELD(name, fieldtype, 0 )
#define DEFINE_KEYFIELD(var,name,fieldtype)				_NAMEFIELD(var, name, fieldtype, FIELD_KEY )
#define DEFINE_ARRAYFIELD(name,fieldtype)				_FIELD(name, fieldtype, FIELD_ARRAY )
#define DEFINE_LISTFIELD(name,fieldtype)				_FIELD(name, fieldtype, FIELD_LIST )

// nested object
#define DEFINE_MAPPEDOBJECT(name)						dataDescField_t { DTVAR_TYPE_NESTED, #name, offsetOf(ThisClass, name), 0, &(((ThisClass *)0)->name.m_DataMap) }

#define DEFINE_MAPPEDOBJECT_ARRAY(name)					dataDescField_t { DTVAR_TYPE_NESTED, #name, offsetOf(ThisClass, name), FIELD_ARRAY, &(((ThisClass *)0)->name[0].m_DataMap) }
#define DEFINE_MAPPEDOBJECT_LIST(name, className)		dataDescField_t { DTVAR_TYPE_NESTED, #name, offsetOf(ThisClass, name), FIELD_LIST, &className::m_DataMap) }

/*
#define DEFINE_LISTFIELD(name,fieldtype)				{ fieldtype, #name, offsetOf(classNameTypedef, name), 1, 0, FIELDFLAG_SAVE, NULL, NULL, CDataopsInstantiator<fieldtype>::GetDataOperators(&(((classNameTypedef *)0)->name)) }
#define DEFINE_EMBEDDED(name)							{ VTYPE_EMBEDDED, #name, offsetOf(classNameTypedef, name), 1, 0, FIELDFLAG_SAVE, NULL, &(((classNameTypedef *)0)->name.m_DataMap), NULL}
#define DEFINE_EMBEDDEDARRAY(name, count)				{ VTYPE_EMBEDDED, #name, offsetOf(classNameTypedef, name), count, sizeof(((classNameTypedef *)0)->name[0]), FIELDFLAG_SAVE, NULL, &(((classNameTypedef *)0)->name[0].m_DataMap), NULL}
#define DEFINE_CUSTOMFIELD(name,ops)					{ VTYPE_CUSTOM, #name, offsetOf(classNameTypedef, name), 1, 0, FIELDFLAG_SAVE, NULL, NULL, ops }
#define DEFINE_FUNCTION_RAW(function, func_type)		{ VTYPE_VOID, #function, 0, 1, 0, FIELDFLAG_FUNCTION, (inputfn_t)((func_type)(&classNameTypedef::function)), NULL, NULL}

#define DEFINE_FUNCTION(function)						DEFINE_FUNCTION_RAW(function, inputfn_t)
*/
#endif //DATATABLES_H