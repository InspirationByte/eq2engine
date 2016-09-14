//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas loader
//////////////////////////////////////////////////////////////////////////////////

#include "TextureAtlas.h"
#include "core_base_header.h"

// parses .atlas file
CTextureAtlas* TexAtlas_LoadAtlas(const char* pszFileName, const char* pszMyName, bool quiet)
{
	KeyValues kvs;
	if(kvs.LoadFromFile(pszFileName, SP_MOD))
	{
		kvkeybase_t* pAtlasSec = kvs.GetRootSection()->FindKeyBase("atlasgroup");

		if(!pAtlasSec)
		{
			MsgError("Invalid atlas file '%s'\n", pszFileName);
			return NULL;
		}

		// create
		return new CTextureAtlas(pAtlasSec, pszMyName);
	}
	else if(!quiet)
		MsgError("Couldn't load atlas '%s'\n", pszFileName);

	return NULL;
}

//------------------------------------------------

CTextureAtlas::CTextureAtlas()
{
	m_entries = NULL;
	m_num = 0;
}

CTextureAtlas::CTextureAtlas( kvkeybase_t* kvs, const char* pszMyName )
{
	InitAtlas(kvs, pszMyName);
}

CTextureAtlas::~CTextureAtlas()
{
	Cleanup();
}

void CTextureAtlas::Cleanup()
{
	delete m_entries;
	m_entries = NULL;
	m_num = 0;
}

bool CTextureAtlas::Load( const char* pszFileName, const char* pszMyName )
{
	KeyValues kvs;
	if(kvs.LoadFromFile(pszFileName, SP_MOD))
	{
		kvkeybase_t* pAtlasSec = kvs.GetRootSection()->FindKeyBase("atlasgroup");

		if(!pAtlasSec)
		{
			MsgError("Invalid atlas file '%s'\n", pszFileName);
			return NULL;
		}

		InitAtlas(pAtlasSec, pszMyName);
	}
	else
		return false;

	return true;
}

void CTextureAtlas::InitAtlas( kvkeybase_t* kvs, const char* pszMyName )
{
	m_material = KV_GetValueString(kvs, 0, "none");

	strcpy(m_name, pszMyName);

	m_num = kvs->keys.numElem();

	m_entries = new TexAtlasEntry_t[m_num];

	for(int i = 0; i < kvs->keys.numElem(); i++)
	{
		kvkeybase_t* entrySec = kvs->keys[i];

		strcpy(m_entries[i].name, kvs->keys[i]->name);
		m_entries[i].nameHash = StringToHash(m_entries[i].name, true);

		Rectangle_t rect;
		rect.vleftTop.x		= KV_GetValueFloat(entrySec, 0);
		rect.vleftTop.y		= KV_GetValueFloat(entrySec, 1);
		rect.vrightBottom.x = KV_GetValueFloat(entrySec, 2);
		rect.vrightBottom.y = KV_GetValueFloat(entrySec, 3);

		m_entries[i].rect = rect;
	}
}

TexAtlasEntry_t* CTextureAtlas::GetEntry(int idx)
{
	if(idx < 0 || idx >= m_num)
		return NULL;

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
		return NULL;

	int nameHash = StringToHash(pszName, true);

	for(int i = 0; i < m_num; i++)
	{
		if(m_entries[i].nameHash == nameHash)
			return &m_entries[i];
	}

	return NULL;
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