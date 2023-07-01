//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source stream
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "snd_ogg_cache.h"

class IVirtualStream;

class CSoundSource_OggStream : public CSoundSource_OggCache
{
public:
	virtual int     GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const;
	void*			GetDataPtr(int& dataSize) const { dataSize = 0; return nullptr; }

	virtual bool	Load();
	virtual void	Unload();

	bool			IsStreaming() const { return true; }

protected:
	void			ParseData(OggVorbis_File* file);

	int				ReadData(void* out, int offset, int count) const;

	IVirtualStreamPtr	m_oggFile;
	OggVorbis_File		m_oggStream;

	int					m_dataSize;     // in bytes
};
