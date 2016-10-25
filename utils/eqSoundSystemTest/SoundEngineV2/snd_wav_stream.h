//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Streamed WAVe source
//////////////////////////////////////////////////////////////////////////////////

#include "snd_wav_source.h"

class CSoundSource_WaveStream : public CSoundSource_Wave
{
public:
	int				GetSamples (ubyte *pOutput, int nSamples, int nOffset, bool bLooping);

	bool			Load(const char *szFilename);
	void			Unload();

private:
	virtual void    ParseData(CRIFF_Parser &chunk);

	int             ReadData (ubyte *pOutput, int nStart, int nBytes);

	int				m_dataOffset;   // data chunk
	int				m_dataSize;     // in bytes

	CRIFF_Parser*	m_reader;
};
