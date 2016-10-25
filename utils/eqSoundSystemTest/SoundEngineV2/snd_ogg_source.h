//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source base class
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_OGG_SOURCE_H
#define SND_OGG_SOURCE_H

#include "snd_source.h"
#include "IFileSystem.h"

#include "utils/eqstring.h"
#include <vorbis/vorbisfile.h>

#define STREAM_THRESHOLD    0x10000     // 65k

namespace eqVorbisFile
{
	size_t	fread(void *ptr, size_t size, size_t nmemb, void *datasource);
	int		fseek(void *datasource, ogg_int64_t offset, int whence);
	long	ftell(void *datasource);
	int		fclose(void *datasource);
};

//---------------------------------------------------------------------

class CSoundSource_Ogg : public ISoundSource
{
public:
	virtual soundFormat_t*	GetFormat()							{ return &m_format; }
	virtual const char*		GetFilename() const					{ return m_filename.c_str(); }
	virtual float			GetLoopPosition(float flPosition);

protected:
	void					ParseFormat(vorbis_info& info);
	virtual void			ParseData(OggVorbis_File* file) = 0;

	soundFormat_t			m_format;
	EqString				m_filename;

	int						m_numSamples;
};

#endif // SND_OGG_SOURCE_H
