//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Random number generator
//////////////////////////////////////////////////////////////////////////////////


#ifndef RANDOM_H
#define RANDOM_H

#define NTAB 32

class CUniformRandomStream
{
public:
	CUniformRandomStream();

	// Sets the seed of the random number generator
	virtual void	SetSeed( int nSeed );

	// Generates random numbers
	virtual float	RandomFloat( float fMinVal = 0.0f, float fMaxVal = 1.0f );
	virtual int		RandomInt( int nMinVal, int nMaxVal );

private:
	int				GenerateRandomNumber();

	int m_ndum;
	int m_ny;
	int m_nv[NTAB];
};

void	RandomSeed( int nSeed );
float	RandomFloat( float fMinVal = 0.0f, float fMaxVal = 1.0f );
int		RandomInt( int nMinVal, int nMaxVal );

#endif // RANDOM_H
