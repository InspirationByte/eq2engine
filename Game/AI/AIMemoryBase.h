//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Basic AI memory that can remember hints, and some bits
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMEMORY_H
#define AIMEMORY_H

#include "AINode.h"
#include "AIMemoryBits.h"

#define AIMEMORY_BYTES 64

class CAIMemoryBase
{
public:
	CAIMemoryBase();

	void				Update();

	void				AddHint(CAIHint* pHint);							// the hint that was seen by AI character
	CAIHint*			FindHint(HintType_e type, CAIHint* pCurHint = NULL);
	void				ForgetHint(CAIHint* pHint);							// forget hinted object
	void				ForgetHints(HintType_e type);						// forget all hints with same type
	void				ForgetAllHints();

	// used as conditional bits
	void				SetBit(AIMemBits_e bit, bool bValue);			// set bit flag
	bool				GetBit(AIMemBits_e bit);				// get bit flag

protected:
	ubyte				m_pMemoryBytes[AIMEMORY_BYTES];
	
	DkList<CAIHint*>	m_pHints;
};

#endif // AIMEMORY_H