//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Cached WAVe data
//////////////////////////////////////////////////////////////////////////////////

#include "snd_dma.h"
#include "snd_wav_cache.h"

#include "DebugInterface.h"

bool CSoundSource_WaveCache::Load(const char* szFilename)
{
	m_filename = szFilename;
    CRIFF_Parser* pReader = new CRIFF_Parser( szFilename );

    while ( pReader->GetName( ) )
    {
        ParseChunk( *pReader );
        pReader->ChunkNext( );
    }

    pReader->ChunkClose( );
    delete pReader;

    return m_numSamples > 0;
}

void CSoundSource_WaveCache::Unload ()
{
    gSound->HeapFree( m_dataCache );
    
    m_dataCache = NULL;
    m_cacheSize = 0;
}

void CSoundSource_WaveCache::ParseData(CRIFF_Parser &chunk)
{
    int sample;

    m_dataCache = (ubyte *)gSound->HeapAlloc( chunk.GetSize( ) );
    m_cacheSize = chunk.GetSize( );

    m_numSamples = m_cacheSize / (m_format.channels * m_format.bitwidth / 8);

    //
    //  read
    //
    chunk.ReadChunk( m_dataCache );

    //
    //  convert
    //
    for ( int i = 0; i < m_numSamples; i++ )
    {
        if ( m_format.bitwidth == 16 )
        {
            sample = ((short *)m_dataCache)[i];
            ((short *)m_dataCache)[i] = sample;
        }
        else
        {
            sample = (int)((unsigned char)(m_dataCache[i]) - 128);
            ((signed char *)m_dataCache)[i] = sample;
        }
    }
}

int CSoundSource_WaveCache::GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping)
{
    int     nRemaining, nCompleted = 0;
    int     nBytes, nStart;

    int     nSampleSize = m_format.channels * m_format.bitwidth / 8;

    nBytes = nSamples * nSampleSize;
    nStart = nOffset * nSampleSize;

    nRemaining = nBytes;

    if ( nStart + nBytes > m_cacheSize )
        nBytes = m_cacheSize - nStart;

    memcpy( (void *)pOutput, (void *)(m_dataCache+nStart), nBytes );

    nRemaining -= nBytes;
    nCompleted += nBytes;

    while ( nRemaining && bLooping )
    {
        nBytes = nRemaining;

        if ( m_loopStart > 0 )
        {
            int loopBytes = m_loopStart * nSampleSize;

            if ( loopBytes + nBytes > m_cacheSize )
                nBytes = m_cacheSize - loopBytes;

            memcpy( (void *)(pOutput+nCompleted), (void *)(m_dataCache+loopBytes), nBytes );
            nRemaining -= nBytes;
            nCompleted += nBytes;
        }
        else
        {
            if ( nBytes > m_cacheSize )
                nBytes = m_cacheSize;

            memcpy( (void *)(pOutput+nCompleted), (void *)m_dataCache, nBytes );
            nRemaining -= nBytes;
            nCompleted += nBytes;
        }
    }

    return nCompleted / nSampleSize;
}
