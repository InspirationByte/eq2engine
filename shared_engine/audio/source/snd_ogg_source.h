//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source base class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "snd_source.h"

namespace eqVorbisFile
{
	size_t	fread(void *ptr, size_t size, size_t nmemb, void *datasource);
	int		fseek(void *datasource, ogg_int64_t offset, int whence);
	long	ftell(void *datasource);
	int		fclose(void *datasource);

	static ov_callbacks callbacks {
		eqVorbisFile::fread,
		eqVorbisFile::fseek,
		eqVorbisFile::fclose,
		eqVorbisFile::ftell,
	};
};

//---------------------------------------------------------------------

class CSoundSource_Ogg : public ISoundSource
{
public:
	virtual const Format&	GetFormat() const					{ return m_format; }
	int						GetLoopRegions(int* samplePos) const{ return 0; }
	virtual int				GetSampleCount() const				{ return m_numSamples; }

protected:
	void					ParseFormat(vorbis_info& info);
	virtual void			ParseData(OggVorbis_File* file) = 0;

	Format					m_format;
	int						m_numSamples;
};
