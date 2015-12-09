//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Special zero sound emittter//////////////////////////////////////////////////////////////////////////////////

#ifndef SOUNDZERO_H
#define SOUNDZERO_H

#include "ISoundSystem.h"

class DkSoundSampleZero : public ISoundSample
{
public:
	int			GetFlags();
};

class DkSoundEmitterZero : public ISoundEmitter
{
public:
	void		SetPosition(Vector3D &position);
	void		SetVelocity(Vector3D &vec);

	bool		IsStopped();
	bool		IsVirtual();

	void		Play(ISoundSample* sample, soundParams_t *param = NULL); //Param may be NULL
	void		Stop();
	void		Pause();
	void		GetParams(soundParams_t *param);
	void		SetParams(soundParams_t *param);
};

extern DkSoundSampleZero zeroSample;
//extern DkSoundEmitterZero zeroEmitter;

#endif