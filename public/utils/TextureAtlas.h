//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas loader. Uses material system
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// atlas element
struct TexAtlasEntry_t
{
	char		name[64];
	int			nameHash;
	Rectangle_t rect;
};

struct KVSection;

// atlas structure
class CTextureAtlas
{
public:
	CTextureAtlas();
	CTextureAtlas( KVSection* kvs );
	virtual ~CTextureAtlas();

	void					InitAtlas( KVSection* kvs );
	void					Cleanup();

	bool					Load( const char* pszFileName );

	TexAtlasEntry_t*		GetEntry(int idx) const;

	TexAtlasEntry_t*		FindEntry(const char* pszName) const;
	int						FindEntryIndex(const char* pszName) const;

	int						GetEntryCount() const		{ return m_num; }
	const char*				GetMaterialName() const		{ return m_material.ToCString(); }
protected:

	//char					m_name[64];

	EqString				m_material;
	int						m_num;
	TexAtlasEntry_t*		m_entries;
};
