//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Vorbis audio decoder for WebM movie streams
//////////////////////////////////////////////////////////////////////////////////

#include <minivorbis.h>
#include "core/core_common.h"
#include "MovieDemuxWebM.h"
#include "MovieAudioDecoderVorbis.h"

static constexpr const int VORBIS_HEADER_COUNT = 3;

struct MovieVorbisState
{
	vorbis_info			info;
	vorbis_comment		comment;
	vorbis_dsp_state	dsp;
	vorbis_block		block;
	bool				dspReady{ false };
};

CMovieAudioDecoderVorbis::~CMovieAudioDecoderVorbis()
{
	Shutdown();
}

bool CMovieAudioDecoderVorbis::Init(const CMovieDemuxWebM& demux)
{
	Shutdown();

	if (demux.GetAudioCodecDataCount() != VORBIS_HEADER_COUNT)
	{
		MsgError("Invalid Vorbis codec headers\n");
		return false;
	}

	m_state = PPNew MovieVorbisState;
	vorbis_info_init(&m_state->info);
	vorbis_comment_init(&m_state->comment);

	for (int i = 0; i < VORBIS_HEADER_COUNT; ++i)
	{
		const ubyte* data = nullptr;
		int size = 0;
		if (!demux.GetAudioCodecData(i, data, size))
		{
			Shutdown();
			return false;
		}

		ogg_packet header{};
		header.packet = const_cast<unsigned char*>(data);	// libvorbis does not modify the packet
		header.bytes = size;
		header.b_o_s = (i == 0);
		header.packetno = i;
		header.granulepos = -1;

		if (vorbis_synthesis_headerin(&m_state->info, &m_state->comment, &header) != 0)
		{
			MsgError("Failed to read Vorbis header %d\n", i);
			Shutdown();
			return false;
		}
	}

	if (m_state->info.channels < 1 || m_state->info.channels > 2)
	{
		MsgError("Unsupported Vorbis channel count %d\n", m_state->info.channels);
		Shutdown();
		return false;
	}

	if (vorbis_synthesis_init(&m_state->dsp, &m_state->info) != 0)
	{
		Shutdown();
		return false;
	}

	vorbis_block_init(&m_state->dsp, &m_state->block);
	m_state->dspReady = true;

	m_channels = m_state->info.channels;
	m_sampleRate = m_state->info.rate;
	m_packetNo = VORBIS_HEADER_COUNT;

	return true;
}

void CMovieAudioDecoderVorbis::Shutdown()
{
	if (!m_state)
		return;

	if (m_state->dspReady)
	{
		vorbis_block_clear(&m_state->block);
		vorbis_dsp_clear(&m_state->dsp);
	}

	vorbis_comment_clear(&m_state->comment);
	vorbis_info_clear(&m_state->info);

	SAFE_DELETE(m_state);
	m_channels = 0;
	m_sampleRate = 0;
}

void CMovieAudioDecoderVorbis::Reset()
{
	if (m_state && m_state->dspReady)
		vorbis_synthesis_restart(&m_state->dsp);
}

int CMovieAudioDecoderVorbis::Decode(const MoviePacket& packet, Array<short>& output)
{
	if (!m_state || !m_state->dspReady)
		return 0;

	ogg_packet oggPacket{};
	oggPacket.packet = const_cast<unsigned char*>(packet.data.ptr());
	oggPacket.bytes = packet.data.numElem();
	oggPacket.packetno = m_packetNo++;
	oggPacket.granulepos = -1;

	if (vorbis_synthesis(&m_state->block, &oggPacket) != 0)
		return 0;

	vorbis_synthesis_blockin(&m_state->dsp, &m_state->block);

	int totalFrames = 0;
	float** pcm = nullptr;
	int numFrames = 0;
	while ((numFrames = vorbis_synthesis_pcmout(&m_state->dsp, &pcm)) > 0)
	{
		const int channels = m_channels;
		const int startIdx = output.numElem();
		output.setNum(startIdx + numFrames * channels, false);

		short* dest = output.ptr() + startIdx;
		for (int i = 0; i < numFrames; ++i)
		{
			for (int ch = 0; ch < channels; ++ch)
			{
				const float sample = clamp(pcm[ch][i], -1.0f, 1.0f);
				*dest++ = static_cast<short>(sample * 32767.0f);
			}
		}

		vorbis_synthesis_read(&m_state->dsp, numFrames);
		totalFrames += numFrames;
	}

	return totalFrames;
}
