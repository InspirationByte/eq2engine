//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Recursive Key-Values system for configuration and other files
//				Implements JSON-like, but lightweight notation
//////////////////////////////////////////////////////////////////////////////////

/*

Example of key-values file contents:

key;
key2 value;
key3 value1 value2;			// value array
key4 "string with spaces";	// string including special characters like \n \r \"

key_section
{
	nested value value2
	{
		key value;
		// ...
	};
};

*/

#ifndef KEYVALUES_H
#define KEYVALUES_H

class IVirtualStream;

#include "platform/Platform.h"
#include "utils/DkList.h"
#include "math/DkMath.h"
#include "ppmem.h"

//
enum EKVPairType
{
	KVPAIR_STRING = 0,	// default
	KVPAIR_INT,
	KVPAIR_FLOAT,
	KVPAIR_BOOL,

	KVPAIR_SECTION,		// sections

	KVPAIR_TYPES,
};

const char* GetKVTypeFormatStr(EKVPairType type);

// tune this (depends on size of used memory)
#define KV_MAX_NAME_LENGTH		128

enum KVSearchFlags_e
{
	KV_FLAG_SECTION = (1 << 0),
	KV_FLAG_NOVALUE = (1 << 1),
	KV_FLAG_ARRAY	= (1 << 2)
};

struct kvpairvalue_t
{
	PPMEM_MANAGED_OBJECT()

	kvpairvalue_t()
	{
		value = NULL;
		section = NULL;
		type = KVPAIR_STRING;
	}

	~kvpairvalue_t();

	EKVPairType	type;

	union
	{
		int		nValue;
		bool	bValue;
		float	fValue;
		struct kvkeybase_t*	section;
	};

	char*				value;

	void				SetValueFrom(kvpairvalue_t* from);

	// sets string value
	void				SetStringValue( const char* pszValue );
	void				SetValueFromString( const char* pszValue );
};

// key values base
struct kvkeybase_t
{
	PPMEM_MANAGED_OBJECT()

	kvkeybase_t();
	~kvkeybase_t();

	void					Cleanup();
	void					ClearValues();

	// sets keybase name
	void					SetName(const char* pszName);
	const char*				GetName() const;

	//----------------------------------------------
	// The section functions
	//----------------------------------------------

	// searches for keybase
	kvkeybase_t*			FindKeyBase(const char* pszName, int nFlags = 0) const;

	// adds new keybase
	kvkeybase_t*			AddKeyBase(const char* pszName, const char* pszValue = NULL, EKVPairType pairType = KVPAIR_STRING);

	// removes key base by name
	void					RemoveKeyBaseByName( const char* name, bool removeAll = false );

	//-----------------------------------------------------


	kvkeybase_t&			SetKey(const char* name, const char* value);
	kvkeybase_t&			SetKey(const char* name, int nValue);
	kvkeybase_t&			SetKey(const char* name, float fValue);
	kvkeybase_t&			SetKey(const char* name, bool bValue);
	kvkeybase_t&			SetKey(const char* name, kvkeybase_t* value);

	kvkeybase_t&			AddKey(const char* name, const char* value);
	kvkeybase_t&			AddKey(const char* name, int nValue);
	kvkeybase_t&			AddKey(const char* name, float fValue);
	kvkeybase_t&			AddKey(const char* name, bool bValue);
	kvkeybase_t&			AddKey(const char* name, kvkeybase_t* value);

	//----------------------------------------------
	// The self-key functions
	//----------------------------------------------

	void					SetValueFrom( kvkeybase_t* pOther );

	kvpairvalue_t*			CreateValue();
	kvkeybase_t*			CreateSectionValue();

	kvkeybase_t*			Clone() const;
	void					CopyTo(kvkeybase_t* dest) const;

	// adds value to key
	void					AddValue(const char* value);
	void					AddValue(int nValue);
	void					AddValue(float fValue);
	void					AddValue(bool bValue);
	void					AddValue(kvkeybase_t* keybase);
	void					AddValue(kvpairvalue_t* value);

	// adds unique value to key
	void					AddUniqueValue(const char* value);
	void					AddUniqueValue(int nValue);
	void					AddUniqueValue(float fValue);
	void					AddUniqueValue(bool bValue);

	// sets value
	void					SetValueAt(const char* value, int idxAt);
	void					SetValueAt(int nValue, int idxAt);
	void					SetValueAt(float fValue, int idxAt);
	void					SetValueAt(bool bValue, int idxAt);
	void					SetValueAt(kvpairvalue_t* value, int idxAt);

	//----------------------------------------------

	// copy all values recursively
	void					MergeFrom(const kvkeybase_t* base, bool recursive);

	// checkers
	bool					IsSection() const;
	bool					IsArray() const;
	bool					IsDefinition() const;

	// the line that the key is on
	int						line;

	char					name[KV_MAX_NAME_LENGTH];
	int						nameHash;

	DkList<kvpairvalue_t*>	values;
	EKVPairType				type;		// default type of values

	// the nested keys
	DkList<kvkeybase_t*>	keys;
	bool					unicode;
};

// special wrapper class
// for better compatiblity of new class
class KeyValues
{
public:
	KeyValues()
	{
		m_pKeyBase = new kvkeybase_t;
	}

	~KeyValues()
	{
		delete m_pKeyBase;
	}

	void Reset()
	{
		delete m_pKeyBase;
		m_pKeyBase = NULL;
	}

	// searches for keybase
	kvkeybase_t*		FindKeyBase(const char* pszName, int nFlags = 0);

	// loads from file
	bool				LoadFromFile(const char* pszFileName, int nSearchFlags = -1);
	bool				LoadFromStream( ubyte* pData);

	void				SaveToFile(const char* pszFileName, int nSearchFlags = -1);

	kvkeybase_t*		GetRootSection() {return m_pKeyBase;}

private:
	kvkeybase_t*		m_pKeyBase;
};

// KVAPI functions

kvkeybase_t*	KV_LoadFromFile( const char* pszFileName, int nSearchFlags = -1, kvkeybase_t* pParseTo = NULL );
kvkeybase_t*	KV_ParseSection( const char* pszBuffer, const char* pszFileName = NULL, kvkeybase_t* pParseTo = NULL, int nLine = 0 );

void			KV_PrintSection(kvkeybase_t* base);

bool			UTIL_StringNeedsQuotes( const char* pszString );
void			KV_WriteToStream_r(kvkeybase_t* pKeyBase, IVirtualStream* pStream, int nTabs = 0, bool bOldFormat = false, bool pretty = true);

// safe and fast value returner
const char*		KV_GetValueString( kvkeybase_t* pBase, int nIndex = 0, const char* pszDefault = "no key" );
int				KV_GetValueInt( kvkeybase_t* pBase, int nIndex = 0, int nDefault = 0 );
float			KV_GetValueFloat( kvkeybase_t* pBase, int nIndex = 0, float fDefault = 0.0f );
bool			KV_GetValueBool( kvkeybase_t* pBase, int nIndex = 0, bool bDefault = false );

Vector2D		KV_GetVector2D( kvkeybase_t* pBase, int nIndex = 0, const Vector2D& vDefault = vec2_zero);
IVector2D		KV_GetIVector2D( kvkeybase_t* pBase, int nIndex = 0, const IVector2D& vDefault = 0);

Vector3D		KV_GetVector3D( kvkeybase_t* pBase, int nIndex = 0, const Vector3D& vDefault = vec3_zero);
Vector4D		KV_GetVector4D( kvkeybase_t* pBase, int nIndex = 0, const Vector4D& vDefault = vec4_zero);

#endif //KEYVALUES_H
