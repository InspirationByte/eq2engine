//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium sound emitter
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEQSOUNDEMITTER_H
#define IEQSOUNDEMITTER_H

#include "ppmem.h"
#include "refcounted.h"
#include "Math/Vector.h"

//-------------------------------------------------------------------------------

struct emitterparams_t
{
	float				referenceDistance;
	float				maxDistance;			// maximum distance
	float				rolloff;
	float				airAbsorption;
	float				volume;					// [0.0, 1.0]
	float				pitch;					// [0.5, 2.0]
};

enum EEmitterFlags
{
	EMITTER_FLAG_UNPAUSABLE			= (1 << 0),	// not pausable
	EMITTER_FLAG_NOGLOBALPITCH		= (1 << 1),	// disable global pitching
	EMITTER_FLAG_IS2D				= (1 << 2), // source is 2D
};

enum EEmitterState
{
	EMITSTATE_STOPPED = 0,
	EMITSTATE_PLAYING,
	EMITSTATE_PAUSED,
};

class IEqSoundSample;

class IEqSoundEmitter : public RefCountedObject
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual					~IEqSoundEmitter() {}

	// state
	virtual EEmitterState	GetState() const = 0;
	virtual bool			IsStopped() const = 0;
	virtual bool			IsPlaying() const = 0;

	// channel index
	virtual int				GetChannelIndex() const = 0;

	// sound control
	virtual void			Play( IEqSoundSample* sample, emitterparams_t* params = NULL ) = 0;
	virtual void			Stop() = 0;
	virtual void			Pause() = 0;

	// sound transform
	virtual void			SetPosition(const Vector3D &position) = 0;
	virtual void			SetVelocity(const Vector3D &velocity) = 0;

	virtual Vector3D		GetPosition() const = 0;
	virtual Vector3D		GetVelocity() const = 0;

	// fills struct with params
	virtual void			GetParams(emitterparams_t* param) const = 0;

	// sends sound params to emitter
	virtual void			SetParams(emitterparams_t* param) = 0;
};

#endif // IEQSOUNDEMITTER_H