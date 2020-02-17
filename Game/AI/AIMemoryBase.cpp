//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI memory
//////////////////////////////////////////////////////////////////////////////////

#include "AIMemoryBase.h"

CAIMemoryBase::CAIMemoryBase()
{
	memset(m_pMemoryBytes, 0, sizeof(m_pMemoryBytes));
}

void CAIMemoryBase::Update()
{
	// nothing
}

// the hint that was seen by AI character
void CAIMemoryBase::AddHint(CAIHint* pHint)
{
	m_pHints.addUnique( pHint );
}

CAIHint* CAIMemoryBase::FindHint(HintType_e type, CAIHint* pCurHint)
{
	bool bDoSearch = false;

	if(!pCurHint)
		bDoSearch = true;

	for(int i = 0; i < m_pHints.numElem(); i++)
	{
		if(bDoSearch && m_pHints[i]->GetHintType() == type)
			return m_pHints[i];

		if(m_pHints[i] == pCurHint)
			bDoSearch = true;
	}

	return NULL;
}

// forget hinted object
void CAIMemoryBase::ForgetHint(CAIHint* pHint)
{
	m_pHints.remove( pHint );
}

// forget all hints with same type
void CAIMemoryBase::ForgetHints(HintType_e type)
{
	for(int i = 0; i < m_pHints.numElem(); i++)
	{
		if( m_pHints[i]->GetHintType() == type )
		{
			m_pHints.removeIndex(i);
			i--;
		}
	}
}

void CAIMemoryBase::ForgetAllHints()
{
	m_pHints.clear();
}

// used as conditional bits
// set bit flag
void CAIMemoryBase::SetBit( AIMemBits_e bit, bool bValue )
{
	int cond_byte = floor((float)(bit / 8));
	int cond_bit = bit - cond_byte*8;

	if(bValue)
		m_pMemoryBytes[cond_byte] |= (1 << cond_bit);
	else
		m_pMemoryBytes[cond_byte] &= ~(1 << cond_bit);
}

// get bit flag
bool CAIMemoryBase::GetBit( AIMemBits_e bit )
{
	int cond_byte = floor((float)(bit / 8));
	int cond_bit = bit - cond_byte*8;

	return (m_pMemoryBytes[cond_byte] & (1 << cond_bit));
}