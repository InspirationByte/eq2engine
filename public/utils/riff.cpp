//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: RIFF reader utility class
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "riff.h"

CRIFF_Parser::CRIFF_Parser(const char* szFilename)
{
	m_stream = g_fileSystem->Open(szFilename, "rb");

	if (!m_stream)
	{
		m_curChunk.Id = 0;
		m_curChunk.Size = 0;
		return;
	}

	RIFFhdr_t header;
	m_stream->Read(header);

	if (header.Id != RIFF_ID)
	{
		MsgError("LoadRIFF: '%s' not valid RIFF file\n", szFilename);
		header.Id = 0;
		header.Size = 0;

		ChunkClose();
		return;
	}
	
	if (header.Type != WAVE_ID)
	{
		MsgError("LoadRIFF: '%s' not valid WAVE file\n", szFilename);

		header.Id = 0;
		header.Size = 0;
		ChunkClose();
		return;
	}

	ChunkSet();
}

CRIFF_Parser::CRIFF_Parser(ubyte* pChunkData, int nChunkSize)
{
	if (!pChunkData)
	{
		m_curChunk.Id = 0;
		m_curChunk.Size = 0;
		return;
	}

	m_riffMem.Ref_Grab();
	m_stream = IVirtualStreamPtr(&m_riffMem);

	m_riffMem.Open(pChunkData, VS_OPEN_READ, nChunkSize);

	RIFFhdr_t header;
	m_stream->Read(header);

	if (header.Id != RIFF_ID)
	{
		header.Id = 0;
		header.Size = 0;
		return;
	}
	else
	{
		if (header.Type != WAVE_ID)
		{
			header.Id = 0;
			header.Size = 0;
		}
	}

	ChunkSet();
}

void CRIFF_Parser::ChunkClose()
{
	m_stream = nullptr;
	m_riffMem.Close();
}

int CRIFF_Parser::ReadChunk(void* pOutput, int maxLen)
{
	int numToRead = m_curChunk.Size;

	if (maxLen != -1)
		numToRead = maxLen;

	m_chunkRemaining -= numToRead;
	ASSERT_MSG(m_chunkRemaining >= 0, "CRIFF_Parser - read amount exceeded chunk size");

	return m_stream->Read(pOutput, numToRead, 1);
}

// TODO: needs to be removed
int CRIFF_Parser::ReadData(void* dest, int len)
{
	if (!m_stream)
		return 0;

	return m_stream->Read(dest, len, 1);
}

// TODO: needs to be removed
int CRIFF_Parser::GetPos()
{
	return m_stream->Tell();
}

// TODO: needs to be removed
int CRIFF_Parser::SetPos(int pos)
{
	if (!m_stream)
		return 0;

	return m_stream->Seek(pos, VS_SEEK_SET);
}

int CRIFF_Parser::SkipChunkData(int size)
{
	if (!m_stream || size <= 0)
		return 0;

	m_chunkRemaining -= size;

	return m_stream->Seek(size, VS_SEEK_CUR);
}

uint CRIFF_Parser::GetName()
{
	return m_curChunk.Id;
}

int CRIFF_Parser::GetSize()
{
	return m_curChunk.Size;
}

// goes to the next chunk
bool CRIFF_Parser::ChunkNext()
{
	const bool result = ChunkSet();
	if (!result)
	{
		m_curChunk.Id = 0;
		m_curChunk.Size = 0;
	}
	
	return result;
}

bool CRIFF_Parser::ChunkSet()
{
	SkipChunkData(m_chunkRemaining);

	const int n = m_stream->Read(&m_curChunk, sizeof(m_curChunk), 1);
	m_chunkRemaining = m_curChunk.Size;
	return n == sizeof(m_curChunk);
}