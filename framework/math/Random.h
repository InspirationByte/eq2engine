//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Random number generator
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CPseudoRandomGenerator
{
public:
	CPseudoRandomGenerator() = default;
	CPseudoRandomGenerator(bool autoRegenerate);

	void	SetSeed(int m);
	int		GetSeed() const;
	void	Regenerate();

	float	RandomFloat(float fLow, float fHigh);
	int		RandomInt(int nLow, int nHigh);
private:
	int		GenerateRandomNumber();

	int		m_seed{ 0 };
	bool	m_autoRegenerate{ true };
};

class CUniformRandomStream
{
public:
	static constexpr int RANDOM_NTAB = 32;

	CUniformRandomStream();

	// Sets the seed of the random number generator
	void	SetSeed( int nSeed );

	// Generates random numbers
	float	RandomFloat( float fMinVal = 0.0f, float fMaxVal = 1.0f );
	int		RandomInt( int nMinVal, int nMaxVal );

private:
	int		GenerateRandomNumber();

	int		m_nv[RANDOM_NTAB]{ 0 };
	int		m_ndum{ 0 };
	int		m_ny{ 0 };
};

void	RandomSeed( int nSeed );
float	RandomFloat( float fMinVal = 0.0f, float fMaxVal = 1.0f );
int		RandomInt( int nMinVal, int nMaxVal );
