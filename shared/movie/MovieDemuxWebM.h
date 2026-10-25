//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: WebM (Matroska) demuxer for movie player
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct nestegg;
struct nestegg_packet;

enum EMovieTrackType : int
{
	MOVIE_TRACK_NONE = -1,
	MOVIE_TRACK_VIDEO = 0,
	MOVIE_TRACK_AUDIO,
};

enum EMovieDemuxResult : int
{
	MOVIE_DEMUX_ERROR = -1,
	MOVIE_DEMUX_EOF = 0,
	MOVIE_DEMUX_OK,
};

struct MoviePacket
{
	Array<ubyte>		data{ PP_SL };
	int64				ptsNs{ 0 };
	EMovieTrackType		track{ MOVIE_TRACK_NONE };
	int					frameIdx{ 0 };		// index of laced frame inside container block
	bool				keyframe{ false };
};

class CMovieDemuxWebM
{
public:
	~CMovieDemuxWebM();

	bool					Open(IFileStream* stream);
	void					Close();

	// restarts reading from the beginning of the stream
	bool					Rewind();

	EMovieDemuxResult		ReadPacket(MoviePacket& packet);

	bool					HasVideo() const { return m_videoTrack >= 0; }
	bool					HasAudio() const { return m_audioTrack >= 0; }

	const IVector2D&		GetVideoSize() const { return m_videoSize; }

	// stream duration in nanoseconds, 0 if unknown
	int64					GetDurationNs() const { return m_durationNs; }

	// codec initialization data, owned by demuxer context
	int						GetAudioCodecDataCount() const;
	bool					GetAudioCodecData(int item, const ubyte*& data, int& size) const;

private:
	bool					InitContext();
	void					FreeContext();

	IFileStreamPtr			m_stream;
	nestegg*				m_ctx{ nullptr };

	nestegg_packet*			m_packet{ nullptr };
	int						m_packetFrameIdx{ 0 };
	int						m_packetFrameCount{ 0 };

	IVector2D				m_videoSize{ 0 };
	int64					m_durationNs{ 0 };
	int						m_videoTrack{ -1 };
	int						m_audioTrack{ -1 };
};
