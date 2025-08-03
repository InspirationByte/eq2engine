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
	KVSection kvs;
	if (!KV_LoadFromFile(pszFileName, SP_MOD, kvs))
		return false;

	const KVSection* pAtlasSec = kvs.FindSection("atlasgroup");
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
	m_entries.reserve(kvs->KeyCount());

	int maxNamesLen = 1;
	for (const KVSection& entrySec : kvs->Keys())
		maxNamesLen += strlen(entrySec.GetName()) + 1;

	m_names = PPNew char[maxNamesLen];
	m_names[0] = 0;
	char* namesPtr = m_names;
	for(const KVSection& entrySec : kvs->Keys())
	{
		const int nameHash = StringId24(entrySec.GetName(), true);
		m_entryMap.insert(nameHash, m_entries.numElem());

		float x1, y1, x2, y2;
		entrySec.GetValues(x1, y1, x2, y2);

		m_entries.append({ AARectangle(x1,y1,x2,y2), namesPtr});

		// copy the name
		strcpy(namesPtr, entrySec.GetName());
		const int nameLen = strlen(namesPtr);
		namesPtr[nameLen] = 0;
		namesPtr += nameLen + 1;
	}
}

const AtlasEntry* CTextureAtlas::GetEntry(int idx) const
{
	return &m_entries[idx];
}

const AtlasEntry* CTextureAtlas::FindEntry(const char* pszName) const
{
	const int index = FindEntryIndex(pszName);
	if (index == -1)
		return nullptr;
	return &m_entries[index];
}

int CTextureAtlas::FindEntryIndex(const char* pszName) const
{
	const int nameHash = StringId24(pszName, true);
	auto it = m_entryMap.find(nameHash);
	if (it.atEnd())
		return -1;

	return *it;
}