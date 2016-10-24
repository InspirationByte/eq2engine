//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Sound Engine DirectSound device impl.
//////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <dsound.h>

#include "snd_dev_dsound.h"
#include "snd_dma.h"

#include "ConVar.h"
#include "DebugInterface.h"

HRESULT (WINAPI *pDirectSoundCreate)(GUID *, LPDIRECTSOUND8 *, IUnknown *);

extern ConVar snd_primary;
extern ConVar snd_frequency;

//--------------------------------------------------

CDirectSoundDevice::CDirectSoundDevice(HWND hWnd)
{
    HRESULT         hResult;

    pDirectSound = NULL;
    pSoundBuffer = NULL;
    hDirectSound = NULL;

    m_hWnd = hWnd;

    m_State = SNDDEV_FAIL;

	MsgInfo( "Loading DirectSound module...\n" );

    if ( (hDirectSound = LoadLibraryA( "dsound.dll" )) == NULL )
    {
        MsgError( "failed" );
        return;
    }

    if ( (pDirectSoundCreate = (HRESULT (__stdcall *)(GUID *, LPDIRECTSOUND8 *, IUnknown *))GetProcAddress( hDirectSound, "DirectSoundCreate8" )) == NULL )
    {
		MsgError( "DirectSound loading error - GetProcAddress failed\n" );
        return;
    }

    if ( ( hResult = pDirectSoundCreate( NULL, &pDirectSound, NULL )) != DS_OK )
    {
        if ( hResult == DSERR_ALLOCATED )
        {
            MsgError("DirectSound creation failed, device in use\n" );
            m_State = SNDDEV_ABORT;
        }
        else
            MsgError("DirectSound creation failed\n" );

        return;
    }

    m_DeviceCaps.dwSize = sizeof(DSCAPS);
    if ( pDirectSound->GetCaps( &m_DeviceCaps ) != DS_OK )
    {
        MsgError("DirectSound GetCaps failed\n" );
        return;
    }

    if ( m_DeviceCaps.dwFlags & DSCAPS_EMULDRIVER )
    {
        MsgError("DirectSound sound drivers not present\n" );
        return;
    }

    if ( CreateBuffers( ) != 0 )
        return;

	Msg("DirectSound successfully initialized\n\n" );

    m_State = SNDDEV_READY;
}

void CDirectSoundDevice::Destroy ()
{
	Msg( "Shutting down DirectSound\n" );

    if ( pSoundBuffer )
        DestroyBuffers( );

    if ( pDirectSound )
    {
        pDirectSound->Release( );
        pDirectSound = NULL;
    }

    if ( hDirectSound )
    {
        FreeLibrary( hDirectSound );
        hDirectSound = NULL;
    }
}

int CDirectSoundDevice::CreateBuffers ()
{
    DSBUFFERDESC        dsbd;
    WAVEFORMATEX        wfx;
    bool                primary_set = false;

	MsgInfo( "Initilizing DirectSound buffers...\n" );

    if ( pDirectSound->SetCooperativeLevel( m_hWnd, DSSCL_EXCLUSIVE | DSSCL_PRIORITY ) != DS_OK )
        return -1;

    memset( &wfx, 0, sizeof(wfx) );
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.wBitsPerSample = 16;

    wfx.nSamplesPerSec = clamp( snd_frequency.GetInt( ), 11025, 44100 );

    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    memset( &dsbd, 0, sizeof(DSBUFFERDESC) );
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat = 0;

    memset( &m_BufferCaps, 0, sizeof(DSBCAPS) );
    m_BufferCaps.dwSize = sizeof(DSBCAPS);

    if ( pDirectSound->CreateSoundBuffer( &dsbd, &pPrimaryBuffer, 0 ) == DS_OK )
    {
        m_BufferFormat = wfx;
        if ( pPrimaryBuffer->SetFormat( &m_BufferFormat ) != DS_OK )
			MsgError( "Failed to set primary buffer format!\n" );
        else
        {
            primary_set = true;
        }
    }
    else
        MsgError( "Failed to create primary buffer\n" );

    if ( !primary_set || !snd_primary.GetBool( ) )
    {
		m_BufferFormat = wfx;

		memset( &dsbd, 0, sizeof(DSBUFFERDESC) );
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = DSBCAPS_CTRLFREQUENCY|DSBCAPS_LOCSOFTWARE;
		dsbd.dwBufferBytes = DEFAULT_BUFFER_SIZE;
		dsbd.lpwfxFormat = &m_BufferFormat;

		if ( pDirectSound->CreateSoundBuffer( &dsbd, &pSoundBuffer, 0 ) != DS_OK )
		{
			MsgError( "Failed to create primary buffer\n" );
			return -1;
		}

		if ( pSoundBuffer->GetCaps( &m_BufferCaps ) != DS_OK )
		{
			MsgError( "SoundBuffer GetCaps failed\n" );
			return -1;
		}

		MsgInfo( "Using secondary buffer\n" );
    }
    else
    {
		if ( pDirectSound->SetCooperativeLevel( m_hWnd, DSSCL_WRITEPRIMARY ) != DS_OK )
		{
			MsgError( "Failed to SetCooperativeLevel with DSSCL_WRITEPRIMARY\n" );
			return -1;
		}

		if ( pPrimaryBuffer->GetCaps( &m_BufferCaps ) != DS_OK )
		{
			MsgError( "SoundBuffer GetCaps failed\n" );
			return -1;
		}

		pSoundBuffer = pPrimaryBuffer;
		MsgInfo( "Using primary buffer\n" );
    }

    MsgInfo( "output buffer format:\n" );
    MsgInfo( "...channels:  %d\n", m_BufferFormat.nChannels );
    MsgInfo( "...bit width: %d\n", m_BufferFormat.wBitsPerSample );
    MsgInfo( "...frequency: %d\n\n", m_BufferFormat.nSamplesPerSec );

    pSoundBuffer->Play( 0, 0, DSBPLAY_LOOPING );
    m_nOffset = 0;

    return 0;
}

void CDirectSoundDevice::DestroyBuffers ( )
{
    Msg( "Destroying DirectSound buffers...\n" );

    if ( pDirectSound )
    {
        pDirectSound->SetCooperativeLevel( m_hWnd, DSSCL_NORMAL );
    }

    if ( pSoundBuffer && pSoundBuffer != pPrimaryBuffer )
    {
        pSoundBuffer->Stop( );
        pSoundBuffer->Release( );
        
        if ( pPrimaryBuffer )
            pPrimaryBuffer->Release( );

        pPrimaryBuffer = NULL;
        pSoundBuffer = NULL;
    }
    else if ( pSoundBuffer )
    {
        pSoundBuffer->Stop( );
        pSoundBuffer->Release( );
        pPrimaryBuffer = NULL;
        pSoundBuffer = NULL;
    }
}

buffer_info_t CDirectSoundDevice::GetBufferInfo ()
{
    buffer_info_t info;

    info.channels = m_BufferFormat.nChannels;
    info.bitwidth = m_BufferFormat.wBitsPerSample;
    info.frequency = m_BufferFormat.nSamplesPerSec;
    info.size = m_BufferCaps.dwBufferBytes;
    
    pSoundBuffer->GetCurrentPosition( (LPDWORD )&info.read, (LPDWORD )&info.write );

    info.write = m_nOffset;

    return info;
}

void CDirectSoundDevice::WriteToBuffer(ubyte *pAudioData, int nBytes)
{
    void *pBuffer1, *pBuffer2;
    int nBytes1, nBytes2;
    int  nSamples1, nSamples2;

    DWORD dwStatus;

    pSoundBuffer->GetStatus( &dwStatus );

    if ( dwStatus & DSBSTATUS_BUFFERLOST )
        pSoundBuffer->Restore( );
    if ( !(dwStatus & DSBSTATUS_PLAYING) )
        pSoundBuffer->Play( 0, 0, DSBPLAY_LOOPING );
    
    pSoundBuffer->Lock( m_nOffset, nBytes, &pBuffer1, (LPDWORD )&nBytes1, &pBuffer2, (LPDWORD )&nBytes2, 0 );

    nSamples1 = nBytes1 / (m_BufferFormat.nChannels * m_BufferFormat.wBitsPerSample / 8);
    nSamples2 = nBytes2 / (m_BufferFormat.nChannels * m_BufferFormat.wBitsPerSample / 8);

    gSound->MixStereo16( (samplepair_t *)pAudioData, (stereo16_t *)pBuffer1, nSamples1, 255 );
    gSound->MixStereo16( (samplepair_t *)(pAudioData+nBytes1*2), (stereo16_t *)pBuffer2, nSamples2, 255 );

    pSoundBuffer->Unlock( pBuffer1, nBytes1, pBuffer2, nBytes2 );

    m_nOffset = (m_nOffset + nBytes1 + nBytes2)%m_BufferCaps.dwBufferBytes;
}
