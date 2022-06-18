//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas loader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "core/IFileSystem.h"
#include "TextureAtlas.h"

// parses .atlas file
CTextureAtlas* TexAtlas_LoadAtlas(const char* pszFileName, bool quiet)
{
	KeyValues kvs;
	if(kvs.LoadFromFile(pszFileName, SP_MOD))
	{
		KVSection* pAtlasSec = kvs.GetRootSection()->FindSection("atlasgroup");

		if(!pAtlasSec)
		{
			MsgError("Invalid atlas file '%s'\n", pszFileName);
			return nullptr;
		}

		// create
		return PPNew CTextureAtlas(pAtlasSec);
	}
	else if(!quiet)
		MsgError("Couldn't load atlas '%s'\n", pszFileName);

	return nullptr;
}

//------------------------------------------------

CTextureAtlas::CTextureAtlas()
{
	m_entries = nullptr;
	m_num = 0;
}

CTextureAtlas::CTextureAtlas( KVSection* kvs )
{
	InitAtlas(kvs);
}

CTextureAtlas::~CTextureAtlas()
{
	Cleanup();
}

void CTextureAtlas::Cleanup()
{
	delete m_entries;
	m_entries = nullptr;
	m_num = 0;
}

bool CTextureAtlas::Load( const char* pszFileName )
{
	KeyValues kvs;
	if(kvs.LoadFromFile(pszFileName, SP_MOD))
	{
		KVSection* pAtlasSec = kvs.GetRootSection()->FindSection("atlasgroup");

		if(!pAtlasSec)
		{
			MsgError("Invalid atlas file '%s'\n", pszFileName);
			return false;
		}

		InitAtlas(pAtlasSec);
		return true;
	}

	return false;
}

void CTextureAtlas::InitAtlas( KVSection* kvs )
{
	m_material = KV_GetValueString(kvs, 0, "");

	m_num = kvs->keys.numElem();

	m_entries = PPNew TexAtlasEntry_t[m_num];

	for(int i = 0; i < kvs->keys.numElem(); i++)
	{
		KVSection* entrySec = kvs->keys[i];

		strcpy(m_entries[i].name, kvs->keys[i]->name);
		m_entries[i].nameHash = StringToHash(m_entries[i].name, true);

		Rectangle_t& rect = m_entries[i].rect;
		rect.vleftTop.x		= KV_GetValueFloat(entrySec, 0);
		rect.vleftTop.y		= KV_GetValueFloat(entrySec, 1);
		rect.vrightBottom.x = KV_GetValueFloat(entrySec, 2);
		rect.vrightBottom.y = KV_GetValueFloat(entrySec, 3);
	}
}

TexAtlasEntry_t* CTextureAtlas::GetEntry(int idx)
{
	if(idx < 0 || idx >= m_num)
		return nullptr;

	return &m_entries[idx];
}

int CTextureAtlas::GetEntryIndex(TexAtlasEntry_t* entry) const
{
	for(int i = 0; i < m_num; i++)
	{
		if(&m_entries[i] == entry)
			return i;
	}

	return -1;
}

TexAtlasEntry_t* CTextureAtlas::FindEntry(const char* pszName) const
{
	if(!m_entries)
		return nullptr;

	int nameHash = StringToHash(pszName, true);

	for(int i = 0; i < m_num; i++)
	{
		if(m_entries[i].nameHash == nameHash)
			return &m_entries[i];
	}

	return nullptr;
}

int CTextureAtlas::FindEntryIndex(const char* pszName) const
{
	if(!m_entries)
		return -1;

	int nameHash = StringToHash(pszName, true);

	for(int i = 0; i < m_num; i++)
	{
		if(m_entries[i].nameHash == nameHash)
			return i;
	}

	return -1;
}