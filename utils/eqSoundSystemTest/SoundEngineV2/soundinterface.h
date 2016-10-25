//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CM_SOUND_H
#define CM_SOUND_H

#include "math/Vector.h"
#include "snd_defs_public.h"

#define ATTN_STATIC     0.0f

#define MAX_SOUNDS      256
#define MAX_CHANNELS    64

class ISoundSource;

class ISoundChannel
{
public:
	virtual ~ISoundChannel() {}

	virtual bool			IsPlaying () = 0;
	virtual bool			IsLooping () = 0;

	virtual int				PlaySound(int nSound) = 0;
	virtual int				PlayLoop(int nSound) = 0;

	virtual void			StopSound() = 0;   // stops loop also

	virtual void			SetOrigin(const Vector3D& vOrigin) = 0;
	virtual void			SetVolume(float flVolume) = 0;
	virtual void			SetPitch(float flPitch) = 0;
	virtual void			SetAttenuation(float flAttn) = 0;

	virtual const Vector3D&	GetOrigin() const = 0;
	virtual float			GetVolume() const = 0;
	virtual float			GetPitch() const = 0;
	virtual float			GetAttenuation() const = 0;
};

//-------------------------------------------------------------

class ISoundEngine
{
public:
	static ISoundEngine*		Create();
	static void					Destroy(ISoundEngine* sound);

	virtual void				Initialize(void* winhandle) = 0;
	virtual void				Shutdown() = 0;

	virtual void				Update() = 0;

	virtual void				SetListener(const Vector3D& origin, const Vector3D& forward, const Vector3D& right, const Vector3D& up) = 0;
	virtual const ListenerInfo&	GetListener() const = 0;

	virtual void				PlaySound(int soundId, const Vector3D& origin, float volume, float attenuation) = 0;

	virtual ISoundChannel*		AllocChannel(bool reserve = false) = 0;
	virtual void				FreeChannel(ISoundChannel* chan) = 0;

	virtual void				StopAllSounds() = 0;
	virtual void				ReleaseCache() = 0;

	virtual int					PrecacheSound(const char* fileName) = 0;
	virtual ISoundSource*		GetSound(int soundId) = 0;
	virtual ISoundSource*		FindSound(const char* fileName) = 0;
};

#endif //__CM_SOUND_H__
