//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Random number generator
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "Random.h"

static CUniformRandomStream s_UniformRandomStream;

void RandomSeed(int nSeed)
{
	s_UniformRandomStream.SetSeed(nSeed);
}

float RandomFloat(float fMinVal, float fMaxVal)
{
	return s_UniformRandomStream.RandomFloat(fMinVal, fMaxVal);
}

int RandomInt(int nMinVal, int nMaxVal)
{
	return s_UniformRandomStream.RandomInt(nMinVal, nMaxVal);
}

static constexpr int IA = 16807;
static constexpr int IM = 2147483647;
static constexpr int IQ = 127773;
static constexpr int IR = 2836;

static constexpr float AM = (1.0f / IM);
static constexpr float EPS = 1.2e-7f;
static constexpr float RNMX = (1.0f - EPS);

CPseudoRandomGenerator::CPseudoRandomGenerator(bool autoRegenerate)
	: m_autoRegenerate(autoRegenerate)
{
}

void CPseudoRandomGenerator::SetSeed(int m)
{
	m_seed = m;
}

int CPseudoRandomGenerator::GetSeed() const
{
	return m_seed;
}

void CPseudoRandomGenerator::Regenerate()
{
	++m_seed;
}

uint CPseudoRandomGenerator::GenerateRandomNumber()
{
	const uint n = (m_seed * m_seed * 0x19660d + 0x3c6ef35fU) >> 8 & 0xffff;
	if (m_autoRegenerate) Regenerate();
	return n;
}

float CPseudoRandomGenerator::RandomFloat(float fLow, float fHigh)
{
	// float in [0,1)
	const float fl = min(AM * GenerateRandomNumber(), RNMX);
	return fl * (fHigh - fLow) + fLow; // float in [low,high)
}

int CPseudoRandomGenerator::RandomInt(int nLow, int nHigh)
{
	const int n = GenerateRandomNumber();

	const int x = nHigh - nLow + 1;
	return nLow + (n % x);
}

int CPseudoRandomGenerator::RandomUInt(uint nLow, uint nHigh)
{
	const uint n = GenerateRandomNumber();

	const uint x = nHigh - nLow + 1;
	return nLow + (n % x);
}

CUniformRandomStream::CUniformRandomStream()
{
	SetSeed(0);
}

void CUniformRandomStream::SetSeed(int nSeed)
{
	m_ndum = nSeed < 0 ? nSeed : -nSeed;
	m_ny = 0;
	memset(m_nv, 0, sizeof(m_nv));
}

int CUniformRandomStream::GenerateRandomNumber()
{
	static constexpr int NDIV = (1 + (IM - 1) / RANDOM_NTAB);

	if (m_ndum <= 0 || !m_ny)
	{
		if (-m_ndum < 1)
			m_ndum = 1;
		else
			m_ndum = -m_ndum;

		for (int j = RANDOM_NTAB + 7; j >= 0; j--)
		{
			const int k = m_ndum / IQ;
			m_ndum = IA * (m_ndum - k * IQ) - IR * k;

			if (m_ndum < 0)
				m_ndum += IM;

			if (j < RANDOM_NTAB)
				m_nv[j] = m_ndum;
		}
		m_ny = m_nv[0];
	}

	const int k = (m_ndum) / IQ;
	m_ndum = IA * (m_ndum - k * IQ) - IR * k;

	if (m_ndum < 0)
		m_ndum += IM;

	const int j = m_ny / NDIV;
	m_ny = m_nv[j];
	m_nv[j] = m_ndum;

	return m_ny;
}

float CUniformRandomStream::RandomFloat(float fLow, float fHigh)
{
	// float in [0,1)
	const float fl = min(AM * GenerateRandomNumber(), RNMX);
	return fl * (fHigh - fLow) + fLow; // float in [low,high)
}

int CUniformRandomStream::RandomInt(int nLow, int nHigh)
{
	static constexpr uint MAX_RANDOM_RANGE = 0x7FFFFFFFUL;

	ASSERT(nLow <= nHigh);

	const uint x = nHigh - nLow + 1;
	if (x <= 1 || MAX_RANDOM_RANGE < x - 1)
		return nLow;

	// The following maps a uniform distribution on the interval [0,MAX_RANDOM_RANGE]
	// to a smaller, client-specified range of [0,x-1] in a way that doesn't bias
	// the uniform distribution unfavorably. Even for a worst case x, the loop is
	// guaranteed to be taken no more than half the time, so for that worst case x,
	// the average number of times through the loop is 2. For cases where x is
	// much smaller than MAX_RANDOM_RANGE, the average number of times through the
	// loop is very close to 1.
	//
	const uint maxAcceptable = MAX_RANDOM_RANGE - ((MAX_RANDOM_RANGE + 1) % x);
	uint n;
	do
	{
		n = GenerateRandomNumber();
	} while (n > maxAcceptable);

	return nLow + (n % x);
}