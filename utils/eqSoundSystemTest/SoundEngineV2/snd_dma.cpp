//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: main control for any streaming sound output device
//////////////////////////////////////////////////////////////////////////////////

#include "snd_dma.h"
#include "snd_source.h"

#include "ConVar.h"
#include "DebugInterface.h"
#include "ppmem.h"

ISoundEngine* gSound = NULL;

#pragma warning(disable:4244)

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

CSoundEngine::CSoundEngine() : m_initialized(false)
{
	m_sampleChain.pNext = m_sampleChain.pPrev = &m_sampleChain; 
	Init();
}

CSoundEngine::~CSoundEngine()
{
	Shutdown();
}

int CSoundEngine::Init()
{
	m_initialized = false;

	m_paintBuffer.pData = NULL;
	m_paintBuffer.nSize = 0;

	m_channelBuffer.pData = NULL;
	m_channelBuffer.nSize = 0;

	memset( m_samples, 0, sizeof(m_samples) );
	memset( &m_listener, 0, sizeof(m_listener) );

	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		m_channels[i].SetReserved( false );
		m_channels[i].StopSound();
	}

	return 0;
}

int CSoundEngine::Shutdown()
{
	StopAllSounds();
	m_initialized = false;

	return 0;
}

//----------------------------------------------------------

void CSoundEngine::InitDevice (void* wndhandle)
{
	if ( snd_disable.GetBool( ) )
		return;

	m_device = ISoundDevice::Create( wndhandle );

	m_initialized = true;
}

void CSoundEngine::DestroyDevice()
{
	// clear sound chain
	while ( m_sampleChain.pNext != &m_sampleChain )
		DeleteSample( m_sampleChain.pNext );

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
		FreeChannel( &m_channels[i] );

	ISoundDevice::Destroy( m_device );
	m_device = NULL;
}

void CSoundEngine::Update ()
{
	if (!m_device)
		return;

	buffer_info_t devBufInfo;
	m_device->GetBufferInfo( devBufInfo );

	int mixSamples = snd_mixahead.GetFloat() * devBufInfo.frequency;
	paintbuffer_t* paintBuf = GetPaintBuffer( mixSamples * devBufInfo.channels * PAINTBUFFER_BYTES );

	paintBuf->nFrequency = devBufInfo.frequency;
	paintBuf->nChannels = devBufInfo.channels;
	paintBuf->nVolume = snd_volume.GetFloat() * 255;

	int written = devBufInfo.write - devBufInfo.read;

	if ( written < 0 )
		written += devBufInfo.size;

	if ( mixSamples > devBufInfo.size )
		mixSamples = devBufInfo.size;

	mixSamples -= written / (devBufInfo.channels * devBufInfo.bitwidth / 8);

	if ( mixSamples < 0 )
		return;

	// mix all the channels which currently playing
	MixChannels( paintBuf, mixSamples );

	// write out to the device
	int sizeInBytes = mixSamples * devBufInfo.channels * devBufInfo.bitwidth / 8;
	m_device->WriteToBuffer( paintBuf->pData, sizeInBytes );
}

// returns paint buffer for writing purposes
paintbuffer_t* CSoundEngine::GetPaintBuffer(int nBytes)
{
	if ( !m_paintBuffer.pData )
	{
		m_paintBuffer.pData = (ubyte *)PPAlloc( nBytes );
		m_paintBuffer.nSize = nBytes;

		// make channel buffer same as the output paint buffer
		m_channelBuffer.pData = (ubyte *)PPAlloc( nBytes );
		m_channelBuffer.nSize = nBytes;
	}
	else if ( nBytes != m_paintBuffer.nSize )
	{
		PPFree( m_paintBuffer.pData );
		PPFree( m_channelBuffer.pData );

		m_paintBuffer.pData = (ubyte *)PPAlloc( nBytes );
		m_paintBuffer.nSize = nBytes;

		// make channel buffer same as the output paint buffer
		m_channelBuffer.pData = (ubyte *)PPAlloc( nBytes );
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

const ListenerInfo&	CSoundEngine::GetListener() const
{
	return m_listener;
}

//----------------------------------------------------------

snd_link_t* CSoundEngine::CreateSample(const char *szFilename)
{
	int i;

	snd_link_t      *pLink, *next;
	ISoundSource    *pSource;

	pSource = ISoundSource::CreateSound( szFilename );
	if ( !pSource )
		return NULL;

	pLink = new snd_link_t;

	// alphabetize
	for ( next = m_sampleChain.pNext; (next != &m_sampleChain) && (stricmp(next->szFilename,szFilename) < 0); next = next->pNext );

	pLink->pNext = next;
	pLink->pPrev = next->pPrev;

	pLink->pNext->pPrev = pLink;
	pLink->pPrev->pNext = pLink;

	for( i = 0; i < MAX_SOUNDS; i++ )
	{
		if ( m_samples[i] == NULL )
		{
			m_samples[i] = pLink;

			pLink->nNumber = i;
			break;
		}
	}

	if ( i == MAX_SOUNDS )
	{
		pLink->nNumber = MAX_SOUNDS;
		DeleteSample( pLink );

		MsgError( "could not load %s: exceeded MAX_SOUNDS (%d)\n", szFilename, MAX_SOUNDS );
		return NULL;
	}

	strcpy( pLink->szFilename, szFilename );
	pLink->nSequence = 0;

	pLink->pSource = pSource;

	return pLink;
}

ISoundSource* CSoundEngine::GetSound(int nSound)
{
	if(m_samples[nSound])
		return m_samples[nSound]->pSource;

	return NULL;
}

snd_link_t* CSoundEngine::FindSample(const char *szFilename)
{
	int cmp;

	for ( snd_link_t* pLink = m_sampleChain.pNext; pLink != &m_sampleChain; pLink = pLink->pNext )
	{
		cmp = strcmp( szFilename, pLink->szFilename );
		if ( cmp == 0 )
			return pLink;
		else if ( cmp < 0 ) // passed it, does not exit
			return NULL;
	}

	return NULL;
}


void CSoundEngine::DeleteSample(snd_link_t *pLink)
{
    pLink->pNext->pPrev = pLink->pPrev;
    pLink->pPrev->pNext = pLink->pNext;

    if ( pLink->nNumber < MAX_SOUNDS )
        m_samples[pLink->nNumber] = NULL;

    ISoundSource::DestroySound( pLink->pSource );

    delete pLink;
}

int CSoundEngine::PrecacheSound(const char* fileName)
{
    snd_link_t* link = FindSample( fileName );

    if( link )
        link->nSequence = 0;
    else
        link = CreateSample( fileName );

    if( link )
        return link->nNumber;

    return -1;
}

ISoundSource* CSoundEngine::FindSound(const char* fileName)
{
	snd_link_t* link = FindSample( fileName );

	if(link)
		return link->pSource;

	return NULL;
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

ISoundChannel* CSoundEngine::AllocChannel(bool reserve)
{
	if ( !m_device )
		return NULL;

	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		if ( m_channels[i].IsPlaying() || m_channels[i].IsReserved() )
			continue;

		if ( reserve )
			m_channels[i].SetReserved( true );

		return &m_channels[i];
	}

	return NULL;
}

void CSoundEngine::FreeChannel( ISoundChannel* chan )
{
	if(!chan)
		return;

	CSoundChannel* channel = (CSoundChannel*)chan;

	channel->StopSound();
	channel->SetReserved(false);
}

void CSoundEngine::StopAllSounds()
{
	for (int i = 0; i < MAX_CHANNELS; i++ )
	{
		if(m_channels[i].IsPlaying())
			m_channels[i].StopSound();
	}
}

void CSoundEngine::MixChannels( paintbuffer_t* output, int numSamples )
{
	paintbuffer_t* chanBuffer = GetChannelBuffer();

	for (int i = 0; i < MAX_CHANNELS; i++ )
	{
		if(m_channels[i].IsPlaying())
			m_channels[i].MixChannel(chanBuffer, output, numSamples );
	}
}