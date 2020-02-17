//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Cached WAVe data
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_WAV_CACHE_H
#define SND_WAV_CACHE_H

#include "snd_wav_source.h"

class CSoundSource_WaveCache : public CSoundSource_Wave
{
public:
	virtual int     GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping);

	virtual bool	Load(const char *szFilename);
	virtual void	Unload();

	bool			IsStreaming() { return false; }

protected:
	virtual void    ParseData(CRIFF_Parser &chunk);

	ubyte*			m_dataCache;   // data chunk
	int				m_cacheSize;    // in bytes
};

#endif // SND_WAV_CACHE_H