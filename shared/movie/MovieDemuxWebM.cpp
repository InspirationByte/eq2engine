//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: WebM (Matroska) demuxer for movie player
//////////////////////////////////////////////////////////////////////////////////

#include <nestegg/nestegg.h>
#include "core/core_common.h"
#include "MovieDemuxWebM.h"

static int64_t MovNesteggRead(void* buffer, size_t length, void* userData)
{
	IFileStream* stream = reinterpret_cast<IFileStream*>(userData);
	const VSSize readBytes = stream->Read(buffer, static_cast<VSSize>(length), 1);
	return readBytes > 0 ? readBytes : 0;
}

static int MovNesteggSeek(int64_t offset, int whence, void* userData)
{
	IFileStream* stream = reinterpret_cast<IFileStream*>(userData);

	EFileStreamSeek seekType = FS_SEEK_SET;
	switch (whence)
	{
	case NESTEGG_SEEK_SET:
		seekType = FS_SEEK_SET;
		break;
	case NESTEGG_SEEK_CUR:
		seekType = FS_SEEK_CUR;
		break;
	case NESTEGG_SEEK_END:
		seekType = FS_SEEK_END;
		break;
	default:
		return -1;
	}

	stream->Seek(offset, seekType);
	return 0;
}

static int64_t MovNesteggTell(void* userData)
{
	IFileStream* stream = reinterpret_cast<IFileStream*>(userData);
	return stream->Tell();
}

static void MovNesteggLog(nestegg* ctx, unsigned int severity, const char* format, ...)
{
	if (severity < NESTEGG_LOG_ERROR)
		return;

	va_list argptr;
	va_start(argptr, format);
	const EqString message = EqString::FormatV(format, argptr);
	va_end(argptr);

	DevMsg(DEVMSG_CORE, "nestegg: %s\n", message.ToCString());
}

//-------------------------------------------------------------------------------------------------

CMovieDemuxWebM::~CMovieDemuxWebM()
{
	Close();
}

bool CMovieDemuxWebM::Open(IFileStream* stream)
{
	Close();

	if (!stream)
		return false;

	m_stream = IFileStreamPtr(stream);
	if (!InitContext())
	{
		Close();
		return false;
	}

	return true;
}

void CMovieDemuxWebM::Close()
{
	FreeContext();
	m_stream = nullptr;

	m_videoSize = IVector2D(0);
	m_durationNs = 0;
	m_videoTrack = -1;
	m_audioTrack = -1;
}

bool CMovieDemuxWebM::Rewind()
{
	if (!m_stream)
		return false;

	// re-creating context does not depend on presence of Cues in the file
	FreeContext();
	m_stream->Seek(0, FS_SEEK_SET);

	const int videoTrack = m_videoTrack;
	const int audioTrack = m_audioTrack;
	if (!InitContext())
		return false;

	if (videoTrack != m_videoTrack || audioTrack != m_audioTrack)
	{
		MsgError("CMovieDemuxWebM::Rewind - track layout changed\n");
		return false;
	}

	return true;
}

bool CMovieDemuxWebM::InitContext()
{
	nestegg_io io;
	io.read = MovNesteggRead;
	io.seek = MovNesteggSeek;
	io.tell = MovNesteggTell;
	io.userdata = m_stream.Ptr();

	if (nestegg_init(&m_ctx, io, MovNesteggLog, -1) != 0)
	{
		m_ctx = nullptr;
		MsgError("Unable to open WebM stream\n");
		return false;
	}

	uint64_t duration = 0;
	if (nestegg_duration(m_ctx, &duration) == 0)
		m_durationNs = static_cast<int64>(duration);

	unsigned int numTracks = 0;
	if (nestegg_track_count(m_ctx, &numTracks) != 0)
		return false;

	m_videoTrack = -1;
	m_audioTrack = -1;

	for (uint i = 0; i < numTracks; ++i)
	{
		const int trackType = nestegg_track_type(m_ctx, i);
		const int codecId = nestegg_track_codec_id(m_ctx, i);

		if (trackType == NESTEGG_TRACK_VIDEO && m_videoTrack < 0)
		{
			if (codecId != NESTEGG_CODEC_AV1)
			{
				MsgError("Unsupported video codec %d, only AV1 is supported\n", codecId);
				continue;
			}

			nestegg_video_params params;
			if (nestegg_track_video_params(m_ctx, i, &params) != 0)
				continue;

			m_videoSize = IVector2D(params.width, params.height);
			m_videoTrack = i;
		}
		else if (trackType == NESTEGG_TRACK_AUDIO && m_audioTrack < 0)
		{
			if (codecId != NESTEGG_CODEC_VORBIS)
			{
				MsgError("Unsupported audio codec %d, only Vorbis is supported\n", codecId);
				continue;
			}

			// audio format is read from Vorbis headers by decoder
			m_audioTrack = i;
		}
	}

	if (m_videoTrack < 0)
	{
		MsgError("No supported video stream found\n");
		return false;
	}

	return true;
}

void CMovieDemuxWebM::FreeContext()
{
	if (m_packet)
		nestegg_free_packet(m_packet);
	m_packet = nullptr;
	m_packetFrameIdx = 0;
	m_packetFrameCount = 0;

	if (m_ctx)
		nestegg_destroy(m_ctx);
	m_ctx = nullptr;
}

EMovieDemuxResult CMovieDemuxWebM::ReadPacket(MoviePacket& packet)
{
	if (!m_ctx)
		return MOVIE_DEMUX_ERROR;

	while (true)
	{
		if (!m_packet)
		{
			const int r = nestegg_read_packet(m_ctx, &m_packet);
			if (r == 0)
			{
				m_packet = nullptr;
				return MOVIE_DEMUX_EOF;
			}

			if (r < 0)
			{
				m_packet = nullptr;
				return MOVIE_DEMUX_ERROR;
			}

			unsigned int track = 0;
			unsigned int frameCount = 0;
			nestegg_packet_track(m_packet, &track);
			nestegg_packet_count(m_packet, &frameCount);

			if (static_cast<int>(track) != m_videoTrack && static_cast<int>(track) != m_audioTrack)
			{
				nestegg_free_packet(m_packet);
				m_packet = nullptr;
				continue;
			}

			m_packetFrameIdx = 0;
			m_packetFrameCount = frameCount;
		}

		if (m_packetFrameIdx >= m_packetFrameCount)
		{
			nestegg_free_packet(m_packet);
			m_packet = nullptr;
			continue;
		}

		unsigned int track = 0;
		uint64_t tstamp = 0;
		nestegg_packet_track(m_packet, &track);
		nestegg_packet_tstamp(m_packet, &tstamp);

		unsigned char* data = nullptr;
		size_t dataSize = 0;
		if (nestegg_packet_data(m_packet, m_packetFrameIdx, &data, &dataSize) != 0)
			return MOVIE_DEMUX_ERROR;

		packet.track = (static_cast<int>(track) == m_videoTrack) ? MOVIE_TRACK_VIDEO : MOVIE_TRACK_AUDIO;
		packet.ptsNs = static_cast<int64>(tstamp);
		packet.frameIdx = m_packetFrameIdx;
		packet.keyframe = nestegg_packet_has_keyframe(m_packet) == NESTEGG_PACKET_HAS_KEYFRAME_TRUE;
		packet.data.setNum(static_cast<int>(dataSize), false);
		memcpy(packet.data.ptr(), data, dataSize);

		++m_packetFrameIdx;
		return MOVIE_DEMUX_OK;
	}
}

int CMovieDemuxWebM::GetAudioCodecDataCount() const
{
	if (!m_ctx || m_audioTrack < 0)
		return 0;

	unsigned int count = 0;
	if (nestegg_track_codec_data_count(m_ctx, m_audioTrack, &count) != 0)
		return 0;

	return count;
}

bool CMovieDemuxWebM::GetAudioCodecData(int item, const ubyte*& data, int& size) const
{
	if (!m_ctx || m_audioTrack < 0)
		return false;

	unsigned char* itemData = nullptr;
	size_t itemSize = 0;
	if (nestegg_track_codec_data(m_ctx, m_audioTrack, item, &itemData, &itemSize) != 0)
		return false;

	data = itemData;
	size = static_cast<int>(itemSize);
	return true;
}
