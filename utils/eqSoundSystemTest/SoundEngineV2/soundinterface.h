//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CM_SOUND_H
#define CM_SOUND_H

#include "math/Vector.h"

#define ATTN_STATIC     0.0f

#define MAX_SOUNDS      256
#define MAX_CHANNELS    64

class ISoundChannel
{
public:
	virtual ~ISoundChannel() {}

    virtual bool    IsPlaying () = 0;
    virtual bool    IsLooping () = 0;

    virtual int     PlaySound(int nSound) = 0;
    virtual int     PlayLoop(int nSound) = 0;

    virtual void    StopSound() = 0;   // stops loop also

    virtual void    SetOrigin(const Vector3D& vOrigin) = 0;
    virtual void    SetVolume(float flVolume) = 0;
    virtual void    SetPitch(float flPitch) = 0;
    virtual void    SetAttenuation(float flAttn) = 0;
};

//-------------------------------------------------------------

class ISoundEngine
{
public:
    static void				Create();
    static void				Destroy();

    virtual void			InitDevice(void* winhandle) = 0;
    virtual void			DestroyDevice() = 0;

    virtual void			Update() = 0;

    virtual void			SetListener(const Vector3D& vOrigin, const Vector3D& vForward, const Vector3D& vRight, const Vector3D& vUp) = 0;

    virtual void			PlaySound(int nIndex, const Vector3D& vOrigin, float flVolume, float flAttenuation) = 0;

    virtual ISoundChannel*	AllocChannel(bool reserve = false) = 0;
    virtual void			FreeChannel(ISoundChannel *pChan) = 0;

    virtual int				PrecacheSound(const char* szFilename) = 0;
};

extern ISoundChannel* pSound;

#endif //__CM_SOUND_H__
