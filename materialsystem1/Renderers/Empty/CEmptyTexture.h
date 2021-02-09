//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Empty texture class
//////////////////////////////////////////////////////////////////////////////////

#ifndef CEMPTYTEXTURE_H
#define CEMPTYTEXTURE_H

#include "../Shared/CTexture.h"
#include "utils/DkList.h"

class CEmptyTexture : public CTexture
{
public:
	friend class ShaderAPIEmpty;

	CEmptyTexture() : m_lockData(NULL) {}

	// dummy class
	// locks texture for modifications, etc
	void	Lock(texlockdata_t* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0) 
	{
		m_lockData = malloc(1024*1024);
		pLockData->pData = (ubyte*)m_lockData;
	}
	
	// unlocks texture for modifications, etc
	void	Unlock() {free(m_lockData); m_lockData = NULL;}

	void*	m_lockData;
};

#endif // CEMPTYTEXTURE_H