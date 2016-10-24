//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine
//////////////////////////////////////////////////////////////////////////////////

#include "snd_dma.h"
#include "snd_source.h"

#include "ConVar.h"
#include "DebugInterface.h"


CSoundEngine* gSound = NULL;

#pragma warning(disable:4244)
#pragma warning(disable:4310)

ConVar snd_disable("snd_disable", "0", "disables sound playback", CV_ARCHIVE);

ConVar snd_volume("snd_volume", "0.5", "sound volume", CV_ARCHIVE);
ConVar snd_frequency("snd_frequency", "44100", "sound playback speed", CV_ARCHIVE);
ConVar snd_mixahead("snd_mixahead", "0.05", "sound mix ahead time, in seconds", CV_ARCHIVE);
ConVar snd_primary("snd_primary", "0", "use primary sound buffer", CV_ARCHIVE);

//----------------------------------------------------------

void ISoundEngine::Create()
{
	gSound = new CSoundEngine();
}

void ISoundEngine::Destroy()
{
	delete gSound;
}

//----------------------------------------------------------

int CSoundEngine::Init()
{
    m_bInitialized = false;

    m_paintBuffer.pData = NULL;
    m_paintBuffer.nSize = 0;

    m_channelBuffer.pData = NULL;
    m_channelBuffer.nSize = 0;

    memset( m_Sounds, 0, sizeof(m_Sounds) );
	memset( &m_listener, 0, sizeof(m_listener) );

    for(int i = 0; i < MAX_CHANNELS; i++)
    {
        m_Channels[i].SetReserved( false );
        m_Channels[i].StopSound( );
    }

    if( !snd_disable.GetBool() )
    {
		/*
        SYSTEM_INFO sysinfo;

        GetSystemInfo( &sysinfo );

        if ( (m_hHeap = HeapCreate( 0, sysinfo.dwPageSize, 0 )) == NULL )
            pMain->Message( "sound heap allocation failed\n" );
		*/
    }
    else
        m_hHeap = false;

    return 0;
}

int CSoundEngine::Shutdown()
{
    if ( m_hHeap )
    {
        //HeapDestroy( m_hHeap );
        m_hHeap = NULL;
    }

    return 0;
}

//----------------------------------------------------------

void CSoundEngine::InitDevice (void* wndhandle)
{
    if ( snd_disable.GetBool( ) )
        return;

    pAudioDevice = ISoundDevice::Create( wndhandle );

	m_bInitialized = true;
}

void CSoundEngine::DestroyDevice()
{
    // clear sound chain
    while ( m_Chain.pNext != &m_Chain )
        Delete( m_Chain.pNext );

    if ( m_paintBuffer.pData )
    {
        free( m_paintBuffer.pData );
        free( m_channelBuffer.pData );

        m_paintBuffer.pData = NULL;
        m_paintBuffer.nSize = 0;

        m_channelBuffer.pData = NULL;
        m_channelBuffer.nSize = 0;
    }

    for(int i = 0; i < MAX_CHANNELS; i++)
        FreeChannel( m_Channels + i );

    ISoundDevice::Destroy( pAudioDevice );
    pAudioDevice = NULL;
}

void CSoundEngine::MixStereo16(samplepair_t *pInput, stereo16_t *pOutput, int nSamples, int nVolume)
{
    int i, val;

    for ( i=0 ; i<nSamples ; i++ )
    {
        val = (pInput[i].left * nVolume) >> 8;

        if ( val > 0x7fff )
            pOutput[i].left = 0x7fff;
        else if ( val < (short)0x8000 )
            pOutput[i].left = (short)0x8000;
        else
            pOutput[i].left = (short)val;

        val = (pInput[i].right * nVolume) >> 8;

        if ( val > 0x7fff )
            pOutput[i].right = 0x7fff;
        else if ( val < (short)0x8000 )
            pOutput[i].right = (short)0x8000;
        else
            pOutput[i].right = (short)val;
    }
}

void CSoundEngine::Update ()
{
    paintbuffer_t   *pBuffer;
    int             nBytes;
    buffer_info_t   info;
    int             nSamples, nWritten;

    if ( !pAudioDevice )
        return;

    info = pAudioDevice->GetBufferInfo( );

    nSamples = snd_mixahead.GetFloat() * info.frequency;
    pBuffer = GetPaintBuffer( nSamples * info.channels * PAINTBUFFER_BYTES );

    pBuffer->nFrequency = info.frequency;
    pBuffer->nChannels = info.channels;
    pBuffer->nVolume = snd_volume.GetFloat() * 255;

    nWritten = info.write - info.read;
    if ( nWritten < 0 )
        nWritten += info.size;

    if ( nSamples > info.size )
        nSamples = info.size;

    nSamples -= nWritten / (info.channels * info.bitwidth / 8);

    if ( nSamples < 0 )
        return;

    m_mixChannels( pBuffer, nSamples );

    nBytes = nSamples * info.channels * info.bitwidth / 8;

    pAudioDevice->WriteToBuffer( pBuffer->pData, nBytes );
}

paintbuffer_t* CSoundEngine::GetPaintBuffer(int nBytes)
{
    if ( !m_paintBuffer.pData )
    {
        m_paintBuffer.pData = (ubyte *)alloc( nBytes );
        m_paintBuffer.nSize = nBytes;

        m_channelBuffer.pData = (ubyte *)alloc( nBytes );
        m_channelBuffer.nSize = nBytes;
    }
    else if ( nBytes != m_paintBuffer.nSize )
    {
        free( m_paintBuffer.pData );
        free( m_channelBuffer.pData );

        m_paintBuffer.pData = (ubyte *)alloc( nBytes );
        m_paintBuffer.nSize = nBytes;

        m_channelBuffer.pData = (ubyte *)alloc( nBytes );
        m_channelBuffer.nSize = nBytes;
    }

    memset( m_paintBuffer.pData, 0, m_paintBuffer.nSize );
    return &m_paintBuffer;
}

//----------------------------------------------------------

void CSoundEngine::SetListener(const Vector3D& vOrigin, const Vector3D& vForward, const Vector3D& vRight, const Vector3D& vUp)
{
    m_listener.origin = vOrigin;
    m_listener.forward = vForward;
    m_listener.right = vRight;
    m_listener.up = vUp;
}

//----------------------------------------------------------

void* CSoundEngine::alloc(unsigned int size)
{
	return malloc( size );
}

void CSoundEngine::free(void *ptr)
{
	::free( ptr );
}

//----------------------------------------------------------

snd_link_t* CSoundEngine::Create(const char *szFilename)
{
    int i;

    snd_link_t      *pLink, *next;
    ISoundSource    *pSource;

    pSource = ISoundSource::CreateSound( szFilename );
    if ( !pSource )
        return NULL;

    pLink = (snd_link_t *)alloc( sizeof(snd_link_t) );

    // alphabetize
    for ( next = m_Chain.pNext ; (next != &m_Chain) && (stricmp(next->szFilename,szFilename)<0) ; next = next->pNext );

    pLink->pNext = next;
    pLink->pPrev = next->pPrev;

    pLink->pNext->pPrev = pLink;
    pLink->pPrev->pNext = pLink;

    for ( i = 0 ; i<MAX_SOUNDS ; i++ )
    {
        if ( m_Sounds[i] == NULL )
        {
            m_Sounds[i] = pLink;
            pLink->nNumber = i;
            break;
        }
    }
    if ( i == MAX_SOUNDS )
    {
        pLink->nNumber = MAX_SOUNDS;
        Delete( pLink );

		MsgError( "could not load %s: out of room\n", szFilename );
        return NULL;
    }

    strcpy( pLink->szFilename, szFilename );
    pLink->nSequence = 0;

    pLink->pSource = pSource;

    return pLink;
}


snd_link_t* CSoundEngine::Find(const char *szFilename)
{
    snd_link_t  *pLink;
    int         cmp;

    for ( pLink = m_Chain.pNext ; pLink != &m_Chain ; pLink = pLink->pNext )
    {
        cmp = strcmp( szFilename, pLink->szFilename );
        if ( cmp == 0 )
            return pLink;
        else if ( cmp < 0 ) // passed it, does not exit
            return NULL;
    }

    return NULL;
}


void CSoundEngine::Delete(snd_link_t *pLink)
{
    pLink->pNext->pPrev = pLink->pPrev;
    pLink->pPrev->pNext = pLink->pNext;

    if ( pLink->nNumber < MAX_SOUNDS )
        m_Sounds[pLink->nNumber] = NULL;

    ISoundSource::DestroySound( pLink->pSource );

    free( pLink );
}

int CSoundEngine::PrecacheSound(const char *szFilename)
{
    snd_link_t* pLink = Find( szFilename );

    if( pLink )
        pLink->nSequence = 0;
    else
        pLink = Create( szFilename );

    if ( pLink )
        return pLink->nNumber;

    return -1;
}

void CSoundEngine::PlaySound(int nIndex, const Vector3D& vOrigin, float flVolume, float flAttenuation)
{
    ISoundChannel* pChannel = AllocChannel( false );

    if ( pChannel )
    {
        pChannel->SetVolume( flVolume );
        pChannel->SetOrigin( vOrigin );
        pChannel->SetPitch( 1.0f );
        pChannel->SetAttenuation( flAttenuation );
        
        pChannel->PlaySound( nIndex );
    }
}

//----------------------------------------------------------

ISoundChannel* CSoundEngine::AllocChannel(bool bReserve)
{
    if ( !pAudioDevice )
        return NULL;

    for(int i = 0; i < MAX_CHANNELS; i++)
    {
        if ( m_Channels[i].IsPlaying() )
            continue;

        if ( m_Channels[i].IsReserved() )
            continue;

        if ( bReserve )
            m_Channels[i].SetReserved( true );

        return (ISoundChannel*)&m_Channels[i];
    }

    return NULL;
}

void CSoundEngine::FreeChannel( ISoundChannel* pChan )
{
    CSoundChannel* pChannel = (CSoundChannel*)pChan;

    pChannel->StopSound();
    pChannel->SetReserved(false);
}

void CSoundEngine::m_mixChannels( paintbuffer_t *pBuffer, int nSamples )
{
    for (int i = 0; i < MAX_CHANNELS; i++ )
	{
		if ( m_Channels[i].IsPlaying( ) )
			m_Channels[i].MixChannel( pBuffer, nSamples );
	}
}