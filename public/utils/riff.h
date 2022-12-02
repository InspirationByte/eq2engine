//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: RIFF reader utility class
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define RIFF_ID				MCHAR4('R','I','F','F')
#define WAVE_ID				MCHAR4('W','A','V','E')

// RIFF WAVE FILE HEADERS
typedef struct
{
	int		Id;
	int		Size;
	int		Type;
} RIFFhdr_t;

typedef struct
{
	int		Id;
	int		Size;
} RIFFchunk_t;

//-----------------------------------------------------------

class CRIFF_Parser
{
public:
	CRIFF_Parser(const char *szFilename);
	CRIFF_Parser(ubyte* pChunkData, int nChunkSize);

	void			ChunkClose();
	bool			ChunkNext();

	uint			GetName();
	int             GetSize();

	int             GetPos();
	int             SetPos(int pos);

	int				SkipData(int size);

	int				ReadChunk(void* dest, int maxLen = -1);
	int				ReadData(void* dest, int len);
	int				ReadInt();

private:
	bool			ChunkSet();

	CMemoryStream	m_riffMem;
	IVirtualStream*	m_file{ nullptr };
	IVirtualStream* m_stream{ nullptr };

	RIFFchunk_t		m_curChunk;
	int				m_chunkRemaining{ 0 };
};