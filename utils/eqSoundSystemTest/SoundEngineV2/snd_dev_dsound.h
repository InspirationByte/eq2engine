//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Sound Engine DirectSound device impl.
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_DSOUND_H
#define SND_DSOUND_H

#include <windows.h>
#include <dsound.h>

#include "snd_dev.h"

#include "IFileSystem.h"

//-----------------------------------------------

typedef HRESULT iDirectSoundCreate (GUID *, LPDIRECTSOUND8 *, IUnknown *);

#define DEFAULT_BUFFER_SIZE 0x10000

class CDirectSoundDevice : public ISoundDevice
{
public:
    CDirectSoundDevice() { CDirectSoundDevice(NULL); }
    CDirectSoundDevice(HWND hWnd);

    virtual void Destroy ( );

    virtual ESoundDeviceState	GetState() { return m_State; }
    virtual void				GetBufferInfo(buffer_info_t& outBufInfo);
    virtual void				WriteToBuffer(ubyte *pAudioData, int nBytes);

private:
    LPDIRECTSOUND8      pDirectSound;
    LPDIRECTSOUNDBUFFER pSoundBuffer;
    LPDIRECTSOUNDBUFFER pPrimaryBuffer;
    WAVEFORMATEX        m_BufferFormat;
    DSCAPS              m_DeviceCaps;
    DSBCAPS             m_BufferCaps;
    int                 m_nOffset;

    int                 CreateBuffers ();
    void                DestroyBuffers ();

    ESoundDeviceState	m_State;
    buffer_info_t		m_Info;

    HINSTANCE			hDirectSound;
    HWND				m_hWnd;
};

#endif // SND_DSOUND_H