//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: main control for any streaming sound output device
//////////////////////////////////////////////////////////////////////////////////

#include "snd_dma.h"

#include "Audio/source/snd_source.h"

#include "ConVar.h"
#include "DebugInterface.h"
#include "ppmem.h"

#include "IDebugOverlay.h"

#pragma warning(disable:4244)

ConVar snd_disable("snd_disable", "0", "disables sound playback", CV_ARCHIVE);

ConVar snd_volume("snd_volume", "0.5", "sound volume", CV_ARCHIVE);
ConVar snd_frequency("snd_frequency", "44100", "sound playback speed", CV_ARCHIVE);
ConVar snd_mixahead("snd_mixahead", "0.1", "sound mix ahead time, in seconds", CV_ARCHIVE);
ConVar snd_primary("snd_primary", "0", "use primary sound buffer", CV_ARCHIVE);

//----------------------------------------------------------

ISoundEngine* ISoundEngine::Create()
{
	return new CSoundEngine();
}

void ISoundEngine::Destroy(ISoundEngine* soundSys)
{
	delete soundSys;
}

//----------------------------------------------------------

CSoundEngine::CSoundEngine() : m_initialized(false)
{
	m_sampleChain.pNext = m_sampleChain.pPrev = &m_sampleChain; 

	m_paintBuffer.data = NULL;
	m_paintBuffer.size = 0;

	m_channelBuffer.data = NULL;
	m_channelBuffer.size = 0;

	memset( m_samples, 0, sizeof(m_samples) );
	memset( &m_listener, 0, sizeof(m_listener) );

	for(int i = 0; i < MAX_CHANNELS; i++)
		m_channels[i].m_owner = this;
}

CSoundEngine::~CSoundEngine()
{
	Shutdown();
}

//----------------------------------------------------------

void CSoundEngine::Initialize(void* wndhandle)
{
	if ( snd_disable.GetBool( ) )
		return;

	if(m_initialized)
		return;

	m_device = ISoundDevice::Create( wndhandle );

	if(m_device)
		m_initialized = true;
}

void CSoundEngine::Shutdown()
{
	if(!m_initialized)
		return;

	m_initialized = false;

	// stop all sources and release cache
	ReleaseCache();

	if ( m_paintBuffer.data )
	{
		PPFree( m_paintBuffer.data );
		PPFree( m_channelBuffer.data );

		m_paintBuffer.data = NULL;
		m_paintBuffer.size = 0;

		m_channelBuffer.data = NULL;
		m_channelBuffer.size = 0;
	}

	ISoundDevice::Destroy( m_device );
	m_device = NULL;
}

void CSoundEngine::Update()
{
	if (!m_device)
		return;

	buffer_info_t devBufInfo;
	m_device->GetBufferInfo( devBufInfo );

	int mixSamples = snd_mixahead.GetFloat() * devBufInfo.frequency;
	paintbuffer_t* paintBuf = GetPaintBuffer( mixSamples * devBufInfo.channels * PAINTBUFFER_BYTES );

	paintBuf->frequency = devBufInfo.frequency;
	paintBuf->channels = devBufInfo.channels;
	paintBuf->volume = snd_volume.GetFloat() * 255;

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
	m_device->WriteToBuffer( paintBuf->data, sizeInBytes );
}

// returns paint buffer for writing purposes
paintbuffer_t* CSoundEngine::GetPaintBuffer(int nBytes)
{
	if ( !m_paintBuffer.data )
	{
		m_paintBuffer.data = (ubyte *)PPAlloc( nBytes );
		m_paintBuffer.size = nBytes;

		// make channel buffer same as the output paint buffer
		m_channelBuffer.data = (ubyte *)PPAlloc( nBytes );
		m_channelBuffer.size = nBytes;

		DevMsg(DEVMSG_SOUND, "Created sound buffers (2), size: %d bytes", nBytes);
	}
	else if ( nBytes != m_paintBuffer.size )
	{
		PPFree( m_paintBuffer.data );
		PPFree( m_channelBuffer.data );

		m_paintBuffer.data = (ubyte *)PPAlloc( nBytes );
		m_paintBuffer.size = nBytes;

		// make channel buffer same as the output paint buffer
		m_channelBuffer.data = (ubyte *)PPAlloc( nBytes );
		m_channelBuffer.size = nBytes;

		DevMsg(DEVMSG_SOUND, "Reset sound buffers (2), new size: %d bytes", nBytes);
	}

	memset( m_paintBuffer.data, 0, m_paintBuffer.size );

	return &m_paintBuffer;
}

//----------------------------------------------------------

void CSoundEngine::SetListener(const Vector3D& vOrigin, const Matrix3x3& orient)
{
	m_listener.origin = vOrigin;

	m_listener.orient = orient;
}

const ListenerInfo_t& CSoundEngine::GetListener() const
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

void CSoundEngine::ReleaseCache()
{
	for(int i = 0; i < MAX_CHANNELS; i++)
		FreeChannel( &m_channels[i] );

	// clear sound chain
	while ( m_sampleChain.pNext != &m_sampleChain )
		DeleteSample( m_sampleChain.pNext );
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

	int channelsActive = 0;

	for(int i = 0; i < MAX_CHANNELS; i++ )
	{
		if(!m_channels[i].IsPlaying())
			continue;

		m_channels[i].MixChannel(chanBuffer, output, numSamples );
		channelsActive++;
	}

	debugoverlay->Text(color4_white, "Sound channels active: %d / %d", channelsActive, MAX_CHANNELS);
}