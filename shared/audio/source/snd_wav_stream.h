//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Streamed WAVe source
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "snd_wav_source.h"

class CSoundSource_WaveStream : public CSoundSource_Wave
{
public:
	int				GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const;
	void*			GetDataPtr(int& dataSize) const { dataSize = 0; return nullptr; }

	bool			Load();
	void			Unload();

	bool			IsStreaming() const { return true; }

private:
	virtual void    ParseData(CRIFF_Parser &chunk);

	int             ReadData(void* out, int offset, int count) const;

	int				m_dataOffset;   // data chunk
	int				m_dataSize;     // in bytes

	CRIFF_Parser*	m_reader;
};
