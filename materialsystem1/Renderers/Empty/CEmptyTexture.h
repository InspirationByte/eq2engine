//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Empty texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CTexture.h"

class CEmptyTexture : public CTexture
{
public:
	friend class ShaderAPIEmpty;

	CEmptyTexture() : m_lockData(nullptr) {}

	// dummy class
	// locks texture for modifications, etc
	void	Lock(LockData* pLockData, Rectangle_t* pRect = nullptr, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0)
	{
		m_lockData = PPAlloc(1024*1024);
		pLockData->pData = (ubyte*)m_lockData;
	}
	
	// unlocks texture for modifications, etc
	void	Unlock() {PPFree(m_lockData); m_lockData = nullptr;}

	void*	m_lockData;
};
