//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Vorbis audio decoder for WebM movie streams
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CMovieDemuxWebM;
struct MoviePacket;
struct MovieVorbisState;

class CMovieAudioDecoderVorbis
{
public:
	~CMovieAudioDecoderVorbis();

	// reads codec headers from demuxer
	bool		Init(const CMovieDemuxWebM& demux);
	void		Shutdown();

	// appends interleaved 16 bit samples to output, returns number of decoded sample frames
	int			Decode(const MoviePacket& packet, Array<short>& output);

	// resets decoder state after seeking
	void		Reset();

	int			GetChannels() const { return m_channels; }
	int			GetSampleRate() const { return m_sampleRate; }

private:
	MovieVorbisState*	m_state{ nullptr };
	int					m_channels{ 0 };
	int					m_sampleRate{ 0 };
	int64				m_packetNo{ 0 };
};
