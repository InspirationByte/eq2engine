//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers world renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef PREDICTABLERANDOM_H
#define PREDICTABLERANDOM_H

#include "dktypes.h"

// Predictable random generator for replays
class CPredictableRandomGenerator
{
private:
	static const uint32 INITIAL_VALUE = 0x50000;
	static const uint32 INCREMENT = 0xC39EC3;
	static const uint32 MULTIPLIER = 0x43FD43FD;

private:
	uint32 m_nRnd;

public:
	CPredictableRandomGenerator() { m_nRnd = INITIAL_VALUE; };
	CPredictableRandomGenerator(uint32 nSeed) { m_nRnd = nSeed; };
	virtual ~CPredictableRandomGenerator() {};

	int Get(int nFrom, int nTo)
	{
		if (nTo < nFrom) // nFrom should be less than nTo
		{
			int nTmp = nTo;

			nTo = nFrom;
			nFrom = nTmp;
		}
		else if (nTo == nFrom)
		{
			return (nTo);
		}

		//m_nRnd = (m_nRnd * MULTIPLIER + INCREMENT) & 0xFFFFFF;

		float fTmp = (float)m_nRnd / (float) 16777216.0;

		return ((int)((fTmp * (nTo - nFrom + 1)) + nFrom));
	};

	void	Regenerate()
	{
		m_nRnd = (m_nRnd * MULTIPLIER + INCREMENT) & 0xFFFFFF;
	}

	void	SetSeed(uint32 nSeed) { m_nRnd = nSeed; };
	uint32	GetSeed() { return (m_nRnd); };
};

#endif // PREDICTABLERANDOM_H