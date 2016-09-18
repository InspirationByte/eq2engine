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

// tune this (depends on size of used memory)
#define KV_MAX_NAME_LENGTH		128

enum KVSearchFlags_e
{
	KV_FLAG_SECTION = (1 << 0),
	KV_FLAG_NOVALUE = (1 << 1),
	KV_FLAG_ARRAY	= (1 << 2)
};

// key values base
struct kvkeybase_t
{
	kvkeybase_t();
	~kvkeybase_t();

	void					Cleanup();
	void					ClearValues();

	// sets keybase name
	void					SetName(const char* pszName);

	//----------------------------------------------
	// The section functions
	//----------------------------------------------

	// searches for keybase
	kvkeybase_t*			FindKeyBase(const char* pszName, int nFlags = 0) const;

	// adds new keybase
	kvkeybase_t*			AddKeyBase(const char* pszName, const char* pszValue = NULL);

	// sets the key
	void					SetKey(const char* pszName, const char* pszValue = NULL);
	void					SetKey(const char* pszName, int value);
	void					SetKey(const char* pszName, float value);
	void					SetKey(const char* pszName, const Vector3D& value);
	void					SetKey(const char* pszName, const Vector4D& value);

	// removes key base
	void					RemoveKeyBase(const char* pszName);

	//----------------------------------------------
	// The self-key functions
	//----------------------------------------------

	// sets string value array
	void					SetValue(const char* pszString);

	// sets string value to array index
	void					SetValueByIndex(const char* pszValue, int nIdx = 0);

	// sets array of values
	void					SetValues(const char** pszStrings, int count);

	// adds value to array
	int						AppendValue(const char* pszValue);
	int						AppendValue(int value);
	int						AppendValue(float value);
	int						AppendValue(const Vector3D& value);
	int						AppendValue(const Vector4D& value);

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
	DkList<char*>			values;

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
Vector3D		KV_GetVector3D( kvkeybase_t* pBase, int nIndex = 0, const Vector3D& vDefault = vec3_zero);
Vector4D		KV_GetVector4D( kvkeybase_t* pBase, int nIndex = 0, const Vector4D& vDefault = vec4_zero);

#endif //KEYVALUES_H
