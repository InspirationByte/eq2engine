//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine sound system
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEQSOUNDSYSTEM_H
#define IEQSOUNDSYSTEM_H

#include "IEqSoundSample.h"
#include "IEqSoundEmitter.h"

#define SOUND_LOAD_PATH					"sounds/"

#define SOUNDSYSTEM_INTERFACE_VERSION	"IEqSoundSystem_001"

class IEqSoundEffect
{
public:
	virtual					~IEqSoundEffect();
	virtual const char*		GetName();
};

// sound engine
class IEqSoundSystem
{
public:
	virtual						~IEqSoundSystem() {}

	virtual void				Initialize() = 0;
	virtual	void				Shutdown() = 0;

	virtual void				ReleaseEmitters() = 0;
	virtual void				ReleaseSamples() = 0;

	virtual bool				IsInitialized() const = 0;

	//---------------------

	// samples
	virtual IEqSoundSample*		LoadSample(const char* name, int nFlags = 0) = 0;
	virtual void				FreeSample(IEqSoundSample* pSample) = 0;

	// emitters
	virtual IEqSoundEmitter*	AllocSoundEmitter() = 0;
	virtual void				FreeSoundEmitter(IEqSoundEmitter* pEmitter) = 0;

	// searches for effect
	virtual IEqSoundEffect*		FindEffect( const char* pszName ) const = 0;
	
	//---------------------

	// updates sound system (send fDt = 0.0 to pause all sounds)
	virtual void				Update(float fDt, float fGlobalPitch = 1.0f) = 0;

	// listener's
	virtual void				SetListener(const Vector3D& origin, 
											const Vector3D& zForward, 
											const Vector3D& yUp, 
											const Vector3D& velocity, 
											IEqSoundEffect* pEffect = NULL) = 0;

	virtual Vector3D			GetListenerPosition()  = 0;

	// sets volume (not affecting on ambient sources)
	virtual void				SetVolumeScale( float vol_scale ) = 0;
};

extern IEqSoundSystem*			g_pSound;

#endif