//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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

#include "core/platform/Platform.h"
#include "core/ppmem.h"

#include "ds/Array.h"
#include "math/DkMath.h"

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

// tune this (depends on size of used memory)
#define KV_MAX_NAME_LENGTH		128

enum KVSearchFlags_e
{
	KV_FLAG_SECTION = (1 << 0),
	KV_FLAG_NOVALUE = (1 << 1),
	KV_FLAG_ARRAY	= (1 << 2)
};

//
// KeyValues typed value holder
//
struct KVPairValue
{
	KVPairValue()
	{
		value = NULL;
		section = NULL;
		type = KVPAIR_STRING;
	}

	~KVPairValue();

	EKVPairType	type;

	union
	{
		int		nValue;
		bool	bValue;
		float	fValue;
	};

	struct KVSection*	section;

	char*				value;

	// sets string value
	void				SetStringValue(const char* pszValue, int len = -1);
	void				SetFromString(const char* pszValue);
	void				SetFrom(KVPairValue* from);

	// get/set
	const char*			GetString() const;
	int					GetInt() const;
	float				GetFloat() const;
	bool				GetBool() const;

	void				SetString(const char* value);
	void				SetInt(int nValue);
	void				SetFloat(float fValue);
	void				SetBool(bool bValue);	
};

//
// key values base
//
struct KVSection
{
	KVSection();
	~KVSection();

	void				Cleanup();
	void				ClearValues();

	// sets section name
	void				SetName(const char* pszName);
	const char*			GetName() const;

	//----------------------------------------------
	// The section functions
	//----------------------------------------------

	// searches for section
	KVSection*			FindSection(const char* pszName, int nFlags = 0) const;

	// adds new section
	KVSection*			CreateSection(const char* pszName, const char* pszValue = NULL, EKVPairType pairType = KVPAIR_STRING);

	// adds existing section. You should set it's name manually. It should not be allocated by other section
	void				AddSection(KVSection* keyBase);

	// removes key base by name
	void				RemoveSectionByName( const char* name, bool removeAll = false );

	// removes key base
	void				RemoveSection(KVSection* base);

	//-----------------------------------------------------


	KVSection&			SetKey(const char* name, const char* value);
	KVSection&			SetKey(const char* name, int nValue);
	KVSection&			SetKey(const char* name, float fValue);
	KVSection&			SetKey(const char* name, bool bValue);
	KVSection&			SetKey(const char* name, const Vector2D& value);
	KVSection&			SetKey(const char* name, const Vector3D& value);
	KVSection&			SetKey(const char* name, const Vector4D& value);
	KVSection&			SetKey(const char* name, KVSection* value);

	KVSection&			AddKey(const char* name, const char* value);
	KVSection&			AddKey(const char* name, int nValue);
	KVSection&			AddKey(const char* name, float fValue);
	KVSection&			AddKey(const char* name, bool bValue);
	KVSection&			AddKey(const char* name, const Vector2D& value);
	KVSection&			AddKey(const char* name, const Vector3D& value);
	KVSection&			AddKey(const char* name, const Vector4D& value);
	KVSection&			AddKey(const char* name, KVSection* value);

	//----------------------------------------------
	// The self-key functions
	//----------------------------------------------

	void					SetValueFrom( KVSection* pOther );

	KVPairValue*			CreateValue();
	KVSection*				CreateSectionValue();

	KVSection*				Clone() const;
	void					CopyTo(KVSection* dest) const;
	void					CopyValuesTo(KVSection* dest) const;

	// adds value to key
	void					AddValue(const char* value);
	void					AddValue(int nValue);
	void					AddValue(float fValue);
	void					AddValue(bool bValue);
	void					AddValue(const Vector2D& vecValue);
	void					AddValue(const Vector3D& vecValue);
	void					AddValue(const Vector4D& vecValue);
	void					AddValue(KVSection* keybase);
	void					AddValue(KVPairValue* value);

	// adds unique value to key
	void					AddUniqueValue(const char* value);
	void					AddUniqueValue(int nValue);
	void					AddUniqueValue(float fValue);
	void					AddUniqueValue(bool bValue);

	// sets value
	void					SetValue(const char* value, int idxAt = 0);
	void					SetValue(int nValue, int idxAt = 0);
	void					SetValue(float fValue, int idxAt = 0);
	void					SetValue(bool bValue, int idxAt = 0);
	void					SetValue(const Vector2D& value, int idxAt = 0);
	void					SetValue(const Vector3D& value, int idxAt = 0);
	void					SetValue(const Vector4D& value, int idxAt = 0);
	void					SetValue(KVPairValue* value, int idxAt = 0);

	const char*				GetValue( int nIndex = 0, const char* pszDefault = "" );
	int						GetValue( int nIndex = 0, int nDefault = 0 );
	float					GetValue( int nIndex = 0, float fDefault = 0.0f );
	bool					GetValue( int nIndex = 0, bool bDefault = false );
	Vector2D				GetValue( int nIndex = 0, const Vector2D& vDefault = vec2_zero);
	IVector2D				GetValue( int nIndex = 0, const IVector2D& vDefault = 0);
	Vector3D				GetValue( int nIndex = 0, const Vector3D& vDefault = vec3_zero);
	Vector4D				GetValue( int nIndex = 0, const Vector4D& vDefault = vec4_zero);

	KVSection*				operator[](const char* pszName);
	KVPairValue*			operator[](int index);


	//----------------------------------------------

	// copy all values recursively
	void					MergeFrom(const KVSection* base, bool recursive);

	// checkers
	bool					IsSection() const;
	bool					IsArray() const;
	bool					IsDefinition() const;

	int						KeyCount() const;
	KVSection*				KeyAt(int idx) const;

	int						ValueCount() const;
	KVPairValue*			ValueAt(int idx) const;

	void					SetType(int newType);
	int						GetType() const;

	// the line that the key is on
	int						line;

	char					name[KV_MAX_NAME_LENGTH];
	int						nameHash;

	Array<KVPairValue*>		values{ PP_SL };
	EKVPairType				type;		// default type of values

	// the nested keys
	Array<KVSection*>		keys{ PP_SL };
	bool					unicode;
};

// special wrapper class
// for better compatiblity of new class
class KeyValues
{
public:
	KeyValues();
	~KeyValues();

	void			Reset();

	// searches for keybase
	KVSection*		FindSection(const char* pszName, int nFlags = 0);

	// loads from file
	bool			LoadFromFile(const char* pszFileName, int nSearchFlags = -1);
	bool			LoadFromStream(ubyte* pData);

	bool			SaveToFile(const char* pszFileName, int nSearchFlags = -1);

	KVSection*		GetRootSection();

	KVSection*		operator[](const char* pszName);

private:
	KVSection	m_root;
};

//---------------------------------------------------------------------------------------------------------
// KEYVALUES API Functions
//---------------------------------------------------------------------------------------------------------

KVSection*		KV_LoadFromFile( const char* pszFileName, int nSearchFlags = -1, KVSection* pParseTo = NULL );

KVSection*		KV_ParseSection(const char* pszBuffer, int bufferSize, const char* pszFileName = NULL, KVSection* pParseTo = NULL, int nStartLine = 0);
KVSection*		KV_ReadBinaryBase(IVirtualStream* stream, KVSection* pParseTo = NULL);
KVSection*		KV_ParseBinary(const char* pszBuffer, int bufferSize, KVSection* pParseTo = NULL);

void			KV_PrintSection(KVSection* base);

void			KV_WriteToStream(IVirtualStream* outStream, KVSection* section, int nTabs = 0, bool pretty = true);
void			KV_WriteToStreamV3(IVirtualStream* outStream, KVSection* section, int nTabs = 0, bool pretty = true);

void			KV_WriteToStreamBinary(IVirtualStream* outStream, KVSection* base);

//-----------------------------------------------------------------------------------------------------
// KeyValues value helpers
//-----------------------------------------------------------------------------------------------------

const char*			KV_GetValueString( KVSection* pBase, int nIndex = 0, const char* pszDefault = "" );
int					KV_GetValueInt( KVSection* pBase, int nIndex = 0, int nDefault = 0 );
float				KV_GetValueFloat( KVSection* pBase, int nIndex = 0, float fDefault = 0.0f );
bool				KV_GetValueBool( KVSection* pBase, int nIndex = 0, bool bDefault = false );
Vector2D			KV_GetVector2D( KVSection* pBase, int nIndex = 0, const Vector2D& vDefault = vec2_zero);
IVector2D			KV_GetIVector2D( KVSection* pBase, int nIndex = 0, const IVector2D& vDefault = 0);
Vector3D			KV_GetVector3D( KVSection* pBase, int nIndex = 0, const Vector3D& vDefault = vec3_zero);
Vector4D			KV_GetVector4D( KVSection* pBase, int nIndex = 0, const Vector4D& vDefault = vec4_zero);

// new
inline const char*	KV_GetValue( KVSection* pBase, int nIndex = 0, const char* pszDefault = "" )				{return KV_GetValueString(pBase, nIndex, pszDefault);}
inline int			KV_GetValue( KVSection* pBase, int nIndex = 0, int nDefault = 0 )							{return KV_GetValueInt(pBase, nIndex, nDefault);}
inline float		KV_GetValue( KVSection* pBase, int nIndex = 0, float fDefault = 0.0f )					{return KV_GetValueFloat(pBase, nIndex, fDefault);}
inline bool			KV_GetValue( KVSection* pBase, int nIndex = 0, bool bDefault = false )					{return KV_GetValueBool(pBase, nIndex, bDefault);}
inline Vector2D		KV_GetValue( KVSection* pBase, int nIndex = 0, const Vector2D& vDefault = vec2_zero)		{return KV_GetVector2D(pBase, nIndex, vDefault);}
inline IVector2D	KV_GetValue( KVSection* pBase, int nIndex = 0, const IVector2D& vDefault = 0)				{return KV_GetIVector2D(pBase, nIndex, vDefault);}
inline Vector3D		KV_GetValue( KVSection* pBase, int nIndex = 0, const Vector3D& vDefault = vec3_zero)		{return KV_GetVector3D(pBase, nIndex, vDefault);}
inline Vector4D		KV_GetValue( KVSection* pBase, int nIndex = 0, const Vector4D& vDefault = vec4_zero)		{return KV_GetVector4D(pBase, nIndex, vDefault);}

inline const char*	KVSection::GetValue( int nIndex, const char* pszDefault )					{return KV_GetValueString(this, nIndex, pszDefault);}
inline int			KVSection::GetValue( int nIndex, int nDefault )							{return KV_GetValueInt(this, nIndex, nDefault);}
inline float		KVSection::GetValue( int nIndex, float fDefault )							{return KV_GetValueFloat(this, nIndex, fDefault);}
inline bool			KVSection::GetValue( int nIndex, bool bDefault )							{return KV_GetValueBool(this, nIndex, bDefault);}
inline Vector2D		KVSection::GetValue( int nIndex, const Vector2D& vDefault)				{return KV_GetVector2D(this, nIndex, vDefault);}
inline IVector2D	KVSection::GetValue( int nIndex, const IVector2D& vDefault)				{return KV_GetIVector2D(this, nIndex, vDefault);}
inline Vector3D		KVSection::GetValue( int nIndex, const Vector3D& vDefault)				{return KV_GetVector3D(this, nIndex, vDefault);}
inline Vector4D		KVSection::GetValue( int nIndex, const Vector4D& vDefault )				{return KV_GetVector4D(this, nIndex, vDefault);}

#endif //KEYVALUES_H
