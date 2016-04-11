//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound classes and maint system
//
// TODO: Needs serious refactoring
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef ISOUNDSYSTEM_H
#define ISOUNDSYSTEM_H

#include "math/Vector.h"
#include "math/Matrix.h"
#include "ppmem.h"

#define SOUND_DEFAULT_PATH					"sounds/"
#define SOUNDSYSTEM_INTERFACE_VERSION		"IEqSoundSystem_001"

struct soundParams_t
{
	float				referenceDistance;
	float				maxDistance;
	float				rolloff;
	float				airAbsorption;
	float				volume;					// [0.0, 1.0]
	float				pitch;					// [0.5, 2.0]
	bool				is2D;
};

struct sndEffect_t;

//------------------------------------------------------------------------------------

class ISoundSample
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual			~ISoundSample() {}

	virtual int		GetFlags() const = 0;

	// TODO: float	GetDurationInSeconds() const = 0;
};

enum ESampleFlags
{
	SAMPLE_FLAG_REMOVEWHENSTOPPED	= (1 << 0),
	SAMPLE_FLAG_STREAMED			= (1 << 1),
	SAMPLE_FLAG_LOOPING				= (1 << 2),
};

//------------------------------------------------------------------------------------

enum ESoundState
{
	SOUND_STATE_STOPPED = 0,
	SOUND_STATE_PLAYING,
	SOUND_STATE_PAUSED,
};

class ISoundPlayable
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual					~ISoundPlayable() {}

	virtual ESoundState		GetState() const = 0;

	virtual void			Play() = 0;
	virtual void			Stop() = 0;
	virtual void			Pause() = 0;

	virtual void			SetVolume(float val) = 0;
	virtual void			SetPitch(float val) = 0;

	virtual void			SetSample(ISoundSample* sample) = 0;
	virtual ISoundSample*	GetSample() const = 0;
};

//------------------------------------------------------------------------------------

class ISoundEmitter : public ISoundPlayable
{
public:
	virtual					~ISoundEmitter() {}

	virtual bool			IsVirtual() const = 0;

	virtual void			SetPosition(Vector3D &position) = 0;
	virtual void			SetVelocity(Vector3D &velocity) = 0;

	virtual void			GetParams(soundParams_t *param) const = 0;
	virtual void			SetParams(soundParams_t *param) = 0;
};

//------------------------------------------------------------------------------------

// sound engine
class ISoundSystem
{
public:
	virtual						~ISoundSystem() {}

	virtual void				Init() = 0;
	virtual void				Update() = 0;
	virtual	void				Shutdown() = 0;

	virtual bool				IsInitialized() = 0;

	//-----------------------------------------------------------------------------------------

	// sets volume (not affecting on ambient sources)
	virtual void				SetVolumeScale(float vol_scale) = 0;

	// sets the pause state
	virtual void				SetPauseState(bool pause) = 0;
	virtual bool				GetPauseState() = 0;

	virtual void				SetListener(const Vector3D& position,
											const Vector3D& forwardVec,
											const Vector3D& upVec,
											const Vector3D& velocity,
											sndEffect_t* pEffect = NULL) = 0;

	virtual Vector3D			GetListenerPosition()  = 0;

	//-----------------------------------------------------------------------------------------

	// Effect host

	// searches for effect
	virtual sndEffect_t*		FindEffect(const char* pszName) = 0;

	//-----------------------------------------------------------------------------------------

	// emitters

	virtual ISoundPlayable*		GetStaticStreamChannel( int channel ) = 0;
	virtual int					GetNumStaticStreamChannels() = 0;

	virtual ISoundEmitter*		AllocEmitter() = 0;
	virtual void				FreeEmitter(ISoundEmitter* pEmitter) = 0;
	virtual bool				IsValidEmitter(ISoundEmitter* pEmitter) const = 0;

	virtual ISoundSample*		LoadSample(const char *name, int nFlags = 0) = 0;
	virtual ISoundSample*		FindSampleByName( const char *name ) = 0;
	virtual void				ReleaseSample(ISoundSample *pSample) = 0;

	virtual void				ReleaseEmitters() = 0;
	virtual void				ReleaseSamples() = 0;
};

extern ISoundSystem*	soundsystem;

#endif // ISOUNDSYSTEM_H