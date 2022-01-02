//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas loader. Uses material system
//////////////////////////////////////////////////////////////////////////////////

#ifndef TEXTUREATLAS_H
#define TEXTUREATLAS_H

#include "math/DkMath.h"
#include "math/Rectangle.h"
#include "ds/eqstring.h"

// atlas element
struct TexAtlasEntry_t
{
	char		name[64];
	int			nameHash;
	Rectangle_t rect;
};

struct kvkeybase_t;

// atlas structure
class CTextureAtlas
{
public:
	CTextureAtlas();
	CTextureAtlas( kvkeybase_t* kvs );
	virtual ~CTextureAtlas();

	void					InitAtlas( kvkeybase_t* kvs );
	void					Cleanup();

	bool					Load( const char* pszFileName );

	TexAtlasEntry_t*		GetEntry(int idx);
	int						GetEntryIndex(TexAtlasEntry_t* entry) const;

	TexAtlasEntry_t*		FindEntry(const char* pszName) const;
	int						FindEntryIndex(const char* pszName) const;

	int						GetEntryCount() const		{return m_num;}

	const char*				GetMaterialName() const		{return m_material.ToCString();}
protected:

	//char					m_name[64];

	EqString				m_material;
	int						m_num;
	TexAtlasEntry_t*		m_entries;
};

CTextureAtlas*				TexAtlas_LoadAtlas(const char* pszFileName, bool quiet);

#endif // TEXTUREATLAS_H
