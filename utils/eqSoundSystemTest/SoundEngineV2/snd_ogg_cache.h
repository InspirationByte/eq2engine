//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source cache
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_OGG_CACHE_H
#define SND_OGG_CACHE_H

#include "snd_ogg_source.h"

class CSoundSource_OggCache : public CSoundSource_Ogg
{
public:
	virtual int     GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping);

	virtual bool	Load(const char* filename);
	virtual void	Unload();

protected:
	virtual void	ParseData(OggVorbis_File* file);

	ubyte*			m_dataCache;   // data chunk
	int				m_cacheSize;    // in bytes
};


#endif // SND_OGG_CACHE_H