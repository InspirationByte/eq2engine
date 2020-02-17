//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity data table for use in savegames and restoring from
//				savegames / parsing entity KVs
//////////////////////////////////////////////////////////////////////////////////

#ifndef DATAMAP_H
#define DATAMAP_H

#include "SaveRestoreManager.h"

#include "SaveGame_DkList.h"
#include "SaveGame_Events.h"

#include "utils/eqstring.h"
#include "math/Vector.h"
#include "math/Matrix.h"

enum FieldFlag_e
{
	FIELDFLAG_SAVE		= (1 << 0),
	FIELDFLAG_KEY		= (1 << 1),
	FIELDFLAG_INPUT		= (1 << 2),
	FIELDFLAG_OUTPUT	= (1 << 3),
	FIELDFLAG_FUNCTION	= (1 << 4),
};

enum EVariableType
{
	// basic types
	VTYPE_VOID = 0,

	VTYPE_FLOAT,
	VTYPE_INTEGER,
	VTYPE_BOOLEAN,
	VTYPE_SHORT,
	VTYPE_BYTE,
	VTYPE_STRING,

	// vector types
	VTYPE_VECTOR2D,
	VTYPE_VECTOR3D,
	VTYPE_VECTOR4D,

	// optimized origin
	VTYPE_ORIGIN,

	// optimized angles
	VTYPE_ANGLES,

	// angle - fixed float
	VTYPE_ANGLE, // TODO: remove

	// matrix types
	VTYPE_MATRIX2X2,
	VTYPE_MATRIX3X3,
	VTYPE_MATRIX4X4,

	// special float that fix ups automatically
	VTYPE_TIME,

	// function that registered as string
	VTYPE_FUNCTION,

	// custom type TODO: save/restore props
	VTYPE_CUSTOM,

	// embedded type with own datamap
	VTYPE_EMBEDDED,

	// physics object pointer
	VTYPE_PHYSICSOBJECT,

	// model
	VTYPE_MODELNAME,

	// sound
	VTYPE_SOUNDNAME,

	// entity
	VTYPE_ENTITYPTR,

	// type count
	VTYPE_COUNT,
};

static int typeSize[VTYPE_COUNT] = 
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

	sizeof(Vector3D),
	sizeof(Vector3D),

	sizeof(float),

	sizeof(Matrix2x2),
	sizeof(Matrix3x3),
	sizeof(Matrix4x4),

	sizeof(float),

	sizeof(int*),

	0,

	0,

	0,

	sizeof(EqString),

	sizeof(EqString),

	sizeof(int*), 
};

//
// variable_t is used for input/output system
//
struct variable_t
{
public:
	variable_t()	: varType(VTYPE_VOID), iVal(0) {}

	EVariableType		GetVarType() const				{return varType;}

	inline bool			GetBool() const					{ return( varType == VTYPE_BOOLEAN ) ? bVal : false; }
	inline const char*	GetString() const				{ return( varType == VTYPE_STRING ) ? pszVal : "(null)"; }
	inline int			GetInt() const					{ return( varType == VTYPE_INTEGER ) ? iVal : 0; }
	inline float		GetFloat() const				{ return( varType == VTYPE_FLOAT ) ? flVal : 0; }

	inline Matrix2x2	GetMatrix2x2() const			{ return( varType == VTYPE_MATRIX2X2 ) ? Matrix2x2(	matVal2[0],matVal2[1],matVal2[2],matVal2[3]) : identity2();}
	inline Matrix3x3	GetMatrix3x3() const			{ return( varType == VTYPE_MATRIX3X3 ) ? Matrix3x3(	matVal3[0],matVal3[1],matVal3[2],matVal3[3],
																											matVal3[4],matVal3[5],matVal3[6],matVal3[7],
																											matVal3[8]) : identity3();}

	inline Matrix4x4	GetMatrix4x4() const			{ return( varType == VTYPE_MATRIX4X4 ) ? Matrix4x4(	matVal4[0],matVal4[1],matVal4[2],matVal4[3],
																											matVal4[4],matVal4[5],matVal4[6],matVal4[7],
																											matVal4[8],matVal4[9],matVal4[10],matVal4[11],
																											matVal4[12],matVal4[13],matVal4[14],matVal4[15]) : identity4();}

	inline Vector2D		GetVector2D() const				{ return( varType == VTYPE_VECTOR2D ) ? Vector2D(vecVal2[0],vecVal2[1]) : Vector2D(0);}
	inline Vector3D		GetVector3D() const				{ return( varType == VTYPE_VECTOR3D ) ? Vector3D(vecVal3[0],vecVal3[1],vecVal3[2]) : Vector3D(0);}
	inline Vector4D		GetVector4D() const				{ return( varType == VTYPE_VECTOR4D ) ? Vector4D(vecVal4[0],vecVal4[1],vecVal4[2],vecVal4[3]) : Vector4D(0);}

	inline void			SetString(const char* pszStr)	{ strcpy(pszVal, pszStr); varType = VTYPE_STRING;}

	inline void			SetBool(bool val)				{ bVal = val; varType = VTYPE_BOOLEAN;}
	inline void			SetInt(int val)					{ iVal = val; varType = VTYPE_INTEGER;}
	inline void			SetFloat(float val)				{ flVal = val; varType = VTYPE_FLOAT;}

	inline void			SetVector2D(Vector2D& val)		{ *((Vector2D*)vecVal2) = val; varType = VTYPE_VECTOR2D;}
	inline void			SetVector3D(Vector3D& val)		{ *((Vector3D*)vecVal3) = val; varType = VTYPE_VECTOR3D;}
	inline void			SetVector4D(Vector4D& val)		{ *((Vector4D*)vecVal4) = val; varType = VTYPE_VECTOR4D;}

	inline void			SetMatrix2x2(Matrix2x2& val)	{ *((Matrix2x2*)matVal2) = val; varType = VTYPE_MATRIX2X2;}
	inline void			SetMatrix3x3(Matrix3x3& val)	{ *((Matrix3x3*)matVal3) = val; varType = VTYPE_MATRIX3X3;}
	inline void			SetMatrix4x4(Matrix4x4& val)	{ *((Matrix4x4*)matVal4) = val; varType = VTYPE_MATRIX4X4;}

	// use this if you want to get different values
	bool				Convert(EVariableType newType);

private:
	union
	{
		bool		bVal;
		char		pszVal[256];
		int			iVal;
		float		flVal;

		float		vecVal2[2];
		float		vecVal3[3];
		float		vecVal4[4];

		float		matVal2[4];
		float		matVal3[9];
		float		matVal4[16];
	};

	EVariableType varType;
};

class BaseEntity;

struct inputdata_t
{
	BaseEntity*		pActivator;	// initial caller
	BaseEntity*		pCaller;	// the actual caller

	variable_t		value;		// value
	uint			id;			// unique id of this message
};

// entity think function
typedef void (BaseEntity::*thinkfn_t)( void );

// entity i/o data
typedef void (BaseEntity::*inputfn_t)(inputdata_t &data);

struct datamap_t;

// this is used by key fields and save data
struct datavariant_t
{
	EVariableType			type;

	// name (key)
	const char*				name;

	// object offset in bytes
	int						offset;

	// field array size
	int						fieldSize;

	// embedded stride
	int						embeddedStride;

	// field flags
	int						nFlags;

	// input function or think function
	inputfn_t				functionPtr;

	// for embedding
	datamap_t*				customDatamap;

	// other save/restore operators
	ISaveRestoreOperators*	ops;
};

// the datamap
struct datamap_t
{
	const char*		dataClassName;
	datamap_t*		baseMap;

	int16			m_dataNumFields;

	datavariant_t*	m_fields;
};

//
// this is a compile-time data description map creator
//

#define BEGIN_DATAMAP_GUTS( className ) \
	template <typename T> datamap_t *DataMapInit(T *); \
	template <> datamap_t *DataMapInit<className>( className * ); \
	namespace className##_DataDescInit /* namespaces are ideal for this */ \
	{ \
		datamap_t *g_DataMapHolder = DataMapInit( (className *)NULL );  \
	} \
	\
	template <> datamap_t *DataMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static datavariant_t dataDesc[] = \
		{ \
			{ VTYPE_VOID, NULL, 0,0,0},	/* An empty element because we can't have empty array */

#define END_DATAMAP() \
		}; \
		\
		if ( sizeof( dataDesc ) > sizeof( dataDesc[0] ) ) \
		{ \
			classNameTypedef::m_DataMap.m_dataNumFields = ARRAYSIZE( dataDesc ) - 1; \
			classNameTypedef::m_DataMap.m_fields = &dataDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_DataMap.m_dataNumFields = 1; \
			classNameTypedef::m_DataMap.m_fields = dataDesc; \
		} \
		return &classNameTypedef::m_DataMap; \
	}

#define BEGIN_DATAMAP( className ) \
	datamap_t className::m_DataMap = { #className, &BaseClass::m_DataMap, 0, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	BEGIN_DATAMAP_GUTS( className )

//
#define BEGIN_DATAMAP_NO_BASE( className ) \
	datamap_t className::m_DataMap = { #className, NULL, 0, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	BEGIN_DATAMAP_GUTS( className )

// creates data description map for structures
#define DECLARE_SIMPLE_DATAMAP() \
	static datamap_t m_DataMap; \
	template <typename T> friend datamap_t *DataMapInit(T *);

// creates data description map
#define	DECLARE_DATAMAP() \
	DECLARE_SIMPLE_DATAMAP() \
	virtual datamap_t *GetDataDescMap( void );

// a helper macro for baseclass defintion
#define DEFINE_CLASS_BASE( className ) typedef className BaseClass

#define _NAMEFIELD(var, name, fieldtype, flags)			{ fieldtype, name, offsetOf(classNameTypedef, var), 1, 0, flags, NULL, NULL, NULL }
#define _FIELD(name, fieldtype, count, flags)			{ fieldtype, #name, offsetOf(classNameTypedef, name), count, 0, flags, NULL, NULL, NULL }

#define DEFINE_FIELD(name,fieldtype)					_FIELD(name, fieldtype, 1, FIELDFLAG_SAVE )
#define DEFINE_ARRAYFIELD(name,fieldtype,count)			_FIELD(name, fieldtype, count, FIELDFLAG_SAVE )
#define DEFINE_LISTFIELD(name,fieldtype)				{ fieldtype, #name, offsetOf(classNameTypedef, name), 1, 0, FIELDFLAG_SAVE, NULL, NULL, CDataopsInstantiator<fieldtype>::GetDataOperators(&(((classNameTypedef *)0)->name)) }
#define DEFINE_KEYFIELD(var,name,fieldtype)				_NAMEFIELD(var, name, fieldtype, FIELDFLAG_KEY | FIELDFLAG_SAVE )
#define DEFINE_EMBEDDED(name)							{ VTYPE_EMBEDDED, #name, offsetOf(classNameTypedef, name), 1, 0, FIELDFLAG_SAVE, NULL, &(((classNameTypedef *)0)->name.m_DataMap), NULL}
#define DEFINE_EMBEDDEDARRAY(name, count)				{ VTYPE_EMBEDDED, #name, offsetOf(classNameTypedef, name), count, sizeof(((classNameTypedef *)0)->name[0]), FIELDFLAG_SAVE, NULL, &(((classNameTypedef *)0)->name[0].m_DataMap), NULL}
#define DEFINE_CUSTOMFIELD(name,ops)					{ VTYPE_CUSTOM, #name, offsetOf(classNameTypedef, name), 1, 0, FIELDFLAG_SAVE, NULL, NULL, ops }
#define DEFINE_FUNCTION_RAW(function, func_type)		{ VTYPE_VOID, #function, 0, 1, 0, FIELDFLAG_FUNCTION, (inputfn_t)((func_type)(&classNameTypedef::function)), NULL, NULL}

#define DEFINE_FUNCTION(function)						DEFINE_FUNCTION_RAW(function, inputfn_t)
#define DEFINE_THINKFUNC(function)						DEFINE_FUNCTION_RAW(function, thinkfn_t)
#define DEFINE_INPUTFUNC(name, function)				{ VTYPE_VOID, name, 0, 1, 0, FIELDFLAG_INPUT, static_cast <inputfn_t>(&classNameTypedef::function), NULL, NULL}

extern ISaveRestoreOperators* g_pEventSaveRestore;
#define DEFINE_OUTPUT(name, output)						{ VTYPE_CUSTOM, name, offsetOf(classNameTypedef, output), 1, 0, FIELDFLAG_OUTPUT | FIELDFLAG_SAVE, NULL, NULL, g_pEventSaveRestore}
/*
extern ISaveRestoreOperators* g_pRagdollSaveRestore;
#define DEFINE_RAGDOLL(name, output)					{ VTYPE_CUSTOM, name, offsetOf(classNameTypedef, output), 1, 0, FIELDFLAG_OUTPUT | FIELDFLAG_SAVE, NULL, NULL, g_pRagdollSaveRestore}
*/
#define EVENT_FIRE_ALWAYS -1

// Event action - command table
class CEventAction
{
public:
	CEventAction() {}
	CEventAction(char* pszEditorActionData);

	EqString		m_szTargetName;
	EqString		m_szTargetInput;
	EqString		m_szParameter;
	float			m_fDelay;
	int				m_nTimesToFire;

	DECLARE_DATAMAP();
};

struct EventQueueEvent_t;

// Event queue that running
class CEventQueue : public ISaveRestoreOperators
{
public:
	void						Init();
	void						Clear(); // clear for shutdown

	void						ServiceEvents(); // fire events whose time has come

	// adds new event action
	void						AddEvent(const char* pszTargetName,const char* pszTargetInput, variable_t& value, float fDelay, int nTimesToFire, BaseEntity *pActivator, BaseEntity *pCaller);

	void						Save( saverestore_fieldinfo_t* info, IVirtualStream* pStream );
	void						Read( saverestore_fieldinfo_t* info, IVirtualStream* pStream );
	void						Restore( saverestore_fieldinfo_t* info, DkList<BaseEntity*> &entlist);

private:

	DkList<EventQueueEvent_t*>	m_pEventList;
};

extern CEventQueue g_EventQueue;

// Basic entity output system
class CBaseEntityOutput
{
public:
							~CBaseEntityOutput();

	void					Clear();

	float					GetDelay() { return m_fDelay;}
	EVariableType			ValueType() { return m_Value.GetVarType(); }

	void					FireOutput( variable_t &Value, BaseEntity *pActivator, BaseEntity *pCaller, float fDelay);
	void					AddAction(char* pszEditorActionData);

	DkList<CEventAction*>*	GetActions() {return &m_pActionList;}
protected:

	float					m_fDelay;
	variable_t				m_Value;

	DkList<CEventAction*>	m_pActionList; // action list for that output
};

#endif // DATAMAP_H