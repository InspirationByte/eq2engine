//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Cached WAVe data
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "snd_wav_source.h"

class CSoundSource_WaveCache : public CSoundSource_Wave
{
public:
    virtual int     GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping);

    virtual bool	Load(const char *szFilename);
    virtual void	Unload();

protected:
	virtual void    ParseData(CRIFF_Parser &chunk);

	ubyte*			m_dataCache;   // data chunk
	int				m_cacheSize;    // in bytes
};
