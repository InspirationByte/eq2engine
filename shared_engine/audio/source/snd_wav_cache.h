//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Cached WAVe data
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "snd_wav_source.h"

class CSoundSource_WaveCache : public CSoundSource_Wave
{
	friend class CSoundSource_OpenALCache;
public:
	virtual int     GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const;
	void*			GetDataPtr(int& dataSize) const { dataSize = m_cacheSize; return m_dataCache; }

	virtual bool	Load(const char *szFilename);
	virtual void	Unload();

	bool			IsStreaming() const { return false; }

protected:
	virtual void    ParseData(CRIFF_Parser &chunk);

	ubyte*			m_dataCache;   // data chunk
	int				m_cacheSize;    // in bytes
};
