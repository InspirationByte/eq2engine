//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas loader. Uses material system
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// atlas element
struct AtlasEntry
{
	const AARectangle	rect;
	const char*			name{ nullptr };
};

struct KVSection;

// atlas structure
class CTextureAtlas
{
public:
	CTextureAtlas() = default;
	CTextureAtlas(const KVSection* kvs);
	virtual ~CTextureAtlas();

	bool				Load(const char* pszFileName);

	void				InitAtlas(const KVSection* kvs);
	void				Cleanup();

	AtlasEntry*			GetEntry(int idx) const;
	AtlasEntry*			FindEntry(const char* pszName) const;
	int					FindEntryIndex(const char* pszName) const;

	int					GetEntryCount() const		{ return m_entries.numElem(); }
	const char*			GetMaterialName() const		{ return m_material.ToCString(); }
protected:
	EqString			m_material;
	char*				m_names{ nullptr };
	Map<int, int>		m_entryMap{ PP_SL };
	Array<AtlasEntry>	m_entries{ PP_SL };
};
