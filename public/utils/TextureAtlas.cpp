//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas loader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "core/IFileSystem.h"
#include "TextureAtlas.h"

CTextureAtlas::CTextureAtlas(const KVSection* kvs)
{
	InitAtlas(kvs);
}

CTextureAtlas::~CTextureAtlas()
{
	Cleanup();
}

void CTextureAtlas::Cleanup()
{
	m_material.Empty();
	m_entryMap.clear(true);
	m_entries.clear(true);
	SAFE_DELETE_ARRAY(m_names);
}

bool CTextureAtlas::Load( const char* pszFileName )
{
	KeyValues kvs;
	if (!kvs.LoadFromFile(pszFileName, SP_MOD))
		return false;

	KVSection* pAtlasSec = kvs.GetRootSection()->FindSection("atlasgroup");

	if(!pAtlasSec)
	{
		MsgError("Invalid atlas file '%s'\n", pszFileName);
		return false;
	}

	InitAtlas(pAtlasSec);
	return true;
}

void CTextureAtlas::InitAtlas( const KVSection* kvs )
{
	m_material = KV_GetValueString(kvs, 0, "");
	m_entries.reserve(kvs->keys.numElem());

	int maxNamesLen = 1;
	for (const KVSection* entrySec : kvs->keys)
		maxNamesLen += strlen(entrySec->GetName()) + 1;

	m_names = PPNew char[maxNamesLen];
	m_names[0] = 0;
	char* namesPtr = m_names;
	for(const KVSection* entrySec : kvs->keys)
	{
		const int nameHash = StringId24(entrySec->GetName(), true);
		m_entryMap.insert(nameHash, m_entries.numElem());

		AARectangle rect {
			KV_GetValueFloat(entrySec, 0),
			KV_GetValueFloat(entrySec, 1),
			KV_GetValueFloat(entrySec, 2),
			KV_GetValueFloat(entrySec, 3) 
		};
		m_entries.append({ rect, namesPtr });

		// copy the name
		strcpy(namesPtr, entrySec->GetName());
		const int nameLen = strlen(namesPtr);
		namesPtr[nameLen] = 0;
		namesPtr += nameLen + 1;
	}
}

AtlasEntry* CTextureAtlas::GetEntry(int idx) const
{
	return (AtlasEntry*)&m_entries[idx];
}

AtlasEntry* CTextureAtlas::FindEntry(const char* pszName) const
{
	const int index = FindEntryIndex(pszName);
	if (index == -1)
		return nullptr;
	return (AtlasEntry*)&m_entries[index];
}

int CTextureAtlas::FindEntryIndex(const char* pszName) const
{
	const int nameHash = StringId24(pszName, true);
	auto it = m_entryMap.find(nameHash);
	if (it.atEnd())
		return -1;

	return *it;
}