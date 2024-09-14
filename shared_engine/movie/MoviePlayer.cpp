#ifndef MOVIELIB_DISABLE

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
}
#endif // MOVIELIB_DISABLE

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "audio/IEqAudioSystem.h"
#include "audio/source/snd_source.h"
#include "materialsystem1/IMaterialSystem.h"

#include "MoviePlayer.h"

struct AVPacket;
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVBufferRef;
struct SwrContext;
struct SwsContext;

using namespace Threading;

static CEqMutex	s_audioSourceMutex;

static constexpr const int AV_PACKET_AUDIO_CAPACITY = 25;
static constexpr const int AV_PACKET_VIDEO_CAPACITY = 10;

using APacketQueue = FixedList<AVPacket*, AV_PACKET_AUDIO_CAPACITY>;
using VPacketQueue = FixedList<AVPacket*, AV_PACKET_VIDEO_CAPACITY>;
using FrameQueue = FixedList<AVFrame*, AV_PACKET_AUDIO_CAPACITY>;

enum DecodeState : int
{
	DEC_ERROR = -1,
	DEC_DEQUEUE_PACKET = 0,
	DEC_SEND_PACKET,
	DEC_RECV_FRAME,
	DEC_READY_FRAME,
};

enum EPlayerCmd : int
{
	PLAYER_CMD_NONE = 0,
	PLAYER_CMD_PLAYING,
	PLAYER_CMD_REWIND,
	PLAYER_CMD_STOP,
};


struct MoviePlayerData
{
#ifndef MOVIELIB_DISABLE
	AVPacket			packet;
	IFilePtr			file;
	AVFormatContext*	formatCtx{ nullptr };

	// Video
	struct VideoState
	{
		CEqTimer	timer;
		int64_t		videoOffset{ 0 };
		int64_t		lastVideoPts{ 0 };
		double		presentationDelay{ 0.0f };

		AVFrame*	presentFrame{ nullptr };

		AVFrame*	frame{ nullptr };
		AVPacket*	deqPacket{ nullptr };
		DecodeState state{ DEC_ERROR };

	} videoState;

	AVStream*		videoStream{ nullptr };
	AVCodecContext* videoCodec{ nullptr };
	SwsContext*		videoSws{ nullptr };
	VPacketQueue	videoPacketQueue;

	// Audio
	struct AudioState
	{
		int64_t		audioOffset{ 0 };
		int64_t		lastAudioPts{ 0 };

		AVFrame*	frame{ nullptr };
		AVPacket*	deqPacket{ nullptr };

		DecodeState state{ DEC_ERROR };
	} audioState;

	AVStream*		audioStream{ nullptr };
	AVCodecContext* audioCodec{ nullptr };
	SwrContext*		audioSwr{ nullptr };
	APacketQueue	audioPacketQueue;

	float			clockSpeed{ 1.0f };
	int64_t			clockStartTime{ AV_NOPTS_VALUE };
#endif // MOVIELIB_DISABLE
};

static int CreateCodec(AVCodecContext** _cc, AVStream* stream, AVBufferRef* hwDeviceCtx)
{
#ifndef MOVIELIB_DISABLE
	bool failed = false;

	const AVCodec* c = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!c)
		return -1;

	AVCodecContext* cc = avcodec_alloc_context3(c);
	if (cc == nullptr)
		return -1;

	defer{
		if (failed && cc)
			avcodec_free_context(&cc);
	};

	int r = avcodec_parameters_to_context(cc, stream->codecpar);
	if (r != 0)
	{
		failed = true;
		return -1;
	}

	if (hwDeviceCtx)
		cc->hw_device_ctx = av_buffer_ref(hwDeviceCtx);

	r = avcodec_open2(cc, c, nullptr);
	if (r < 0)
	{
		failed = true;
		return -1;
	}

	*_cc = cc;
#endif // MOVIELIB_DISABLE
	return 0;
}

static int movAVIORead(void* opaque, uint8_t* buf, int buf_size)
{
	IVirtualStream* stream = reinterpret_cast<IVirtualStream*>(opaque);
	return stream->Read(buf, buf_size);
}

static int64_t movAVIOSeek(void* opaque, int64_t offset, int whence)
{
	IVirtualStream* stream = reinterpret_cast<IVirtualStream*>(opaque);
	if (whence & AVSEEK_SIZE)
		return stream->GetSize();

	EVirtStreamSeek nw{};
	switch (whence & 0x7)
	{
	case SEEK_CUR:
		nw = VS_SEEK_CUR;
		break;
	case SEEK_SET:
		nw = VS_SEEK_SET;
		break;
	case SEEK_END:
		nw = VS_SEEK_END;
		break;
	}

	return stream->Seek(offset, nw);
}

static void FreePlayerData(MoviePlayerData* player)
{
	if (!player)
		return;
#ifndef MOVIELIB_DISABLE
	if (player->formatCtx && player->formatCtx->pb)
	{
		av_free(player->formatCtx->pb->buffer);
		avio_context_free(&player->formatCtx->pb);
	}
	avformat_close_input(&player->formatCtx);

	if (player->videoStream)
	{
		avcodec_free_context(&player->videoCodec);
		sws_freeContext(player->videoSws);

		if (player->videoState.deqPacket)
			av_packet_free(&player->videoState.deqPacket);

		if (player->videoState.frame)
			av_frame_free(&player->videoState.frame);
	}

	if (player->audioStream)
	{
		avcodec_free_context(&player->audioCodec);
		swr_free(&player->audioSwr);

		if (player->audioState.deqPacket)
			av_packet_free(&player->audioState.deqPacket);

		if (player->audioState.frame)
			av_frame_free(&player->audioState.frame);

		player->audioStream = nullptr;
	}
#endif // MOVIELIB_DISABLE
}

static MoviePlayerData* CreatePlayerData(AVBufferRef* hw_device_context, const char* filename)
{
	MoviePlayerData* player = PPNew MoviePlayerData;
#ifndef MOVIELIB_DISABLE
	bool failed = false;
	defer{
		if (!failed || !player)
			return;

		FreePlayerData(player);
	};

	player->file = g_fileSystem->Open(filename, "r");
	if (!player->file)
	{
		MsgError("Cannot open video file %s\n", filename);
		failed = true;
		return nullptr;
	}

	const int avioStupidBufferSize = 4096;
	ubyte* avioStupidBuffer = (ubyte*)av_malloc(avioStupidBufferSize);

	AVIOContext* avioCtx = avio_alloc_context(
		avioStupidBuffer,
		avioStupidBufferSize,
		0,           
		player->file.Ptr(),
		movAVIORead,
		NULL,        
		movAVIOSeek
	);
	player->formatCtx = avformat_alloc_context();
	player->formatCtx->pb = avioCtx;

	if (avformat_open_input(&player->formatCtx, nullptr, nullptr, nullptr) != 0)
	{
		MsgError("Failed to open video file %s\n", filename);
		failed = true;
		return nullptr;
	}

	if (avformat_find_stream_info(player->formatCtx, nullptr) < 0)
	{
		MsgError("Failed to find stream info\n");
		failed = true;
		return nullptr;
	}

	for (uint i = 0; i < player->formatCtx->nb_streams; ++i)
	{
		AVStream* stream = player->formatCtx->streams[i];

		if (!player->videoStream && (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
			player->videoStream = stream;

		if (!player->audioStream && (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO))
			player->audioStream = stream;

		if (player->audioStream && player->videoStream)
			break;
	}

	if (player->videoStream == nullptr)
	{
		MsgError("No video/audio supported stream found\n");
		failed = true;
		return nullptr;
	}

	// Setup video
	if (CreateCodec(&player->videoCodec, player->videoStream, hw_device_context) != 0)
	{
		failed = true;
		return nullptr;
	}

	player->videoSws = sws_getContext(
		player->videoCodec->width, player->videoCodec->height,
		player->videoCodec->pix_fmt,
		player->videoCodec->width, player->videoCodec->height,
		AV_PIX_FMT_RGBA,
		SWS_POINT,
		nullptr,
		nullptr,
		nullptr);

	// Setup audio
	if (player->audioStream)
	{
		if (CreateCodec(&player->audioCodec, player->audioStream, nullptr) != 0)
		{
			failed = true;
			return nullptr;
		}

		AVChannelLayout outChLayout;
		av_channel_layout_default(&outChLayout, 2);

		swr_alloc_set_opts2(&player->audioSwr,
			&outChLayout, AV_SAMPLE_FMT_S16, 44100,
			&player->audioStream->codecpar->ch_layout,
			(enum AVSampleFormat)player->audioStream->codecpar->format, 
			player->audioStream->codecpar->sample_rate,
			0, nullptr);

		if (player->audioSwr == nullptr)
		{
			failed = true;
			return nullptr;
		}

		if (swr_init(player->audioSwr) < 0)
		{
			MsgError("Unable to create SWResample\n");
			failed = true;
			return nullptr;
		}
	}
#endif // MOVIELIB_DISABLE
	return player;
}

static bool PlayerDemuxStep(MoviePlayerData* player, MovieCompletedEvent& completedEvent)
{
#ifndef MOVIELIB_DISABLE
	if (player->videoPacketQueue.getCount() >= AV_PACKET_VIDEO_CAPACITY)
		return true;

	if (player->audioPacketQueue.getCount() >= AV_PACKET_AUDIO_CAPACITY)
		return true;

	MoviePlayerData::VideoState& videoState = player->videoState;
	MoviePlayerData::AudioState& audioState = player->audioState;

	AVPacket& packet = player->packet;

	bool isDone = false;

	switch (av_read_frame(player->formatCtx, &packet))
	{
	case 0:
		if (player->videoStream && (packet.stream_index == player->videoStream->index))
		{
			AVPacket* videoPacket = av_packet_clone(&packet);
			videoPacket->pts += videoState.videoOffset;
			videoPacket->dts += videoState.videoOffset;
			videoState.lastVideoPts = videoPacket->pts;
			
			player->videoPacketQueue.append(videoPacket);
		}
		else if (player->audioStream && (packet.stream_index == player->audioStream->index))
		{
			AVPacket* audioPacket = av_packet_clone(&packet);
			audioPacket->pts += audioState.audioOffset;
			audioPacket->dts += audioState.audioOffset;
			audioState.lastAudioPts = audioPacket->pts;
			
			player->audioPacketQueue.append(audioPacket);
		}
		break;
	case AVERROR_EOF:
	{
		completedEvent();
		break;
	}
	default:
		DevMsg(DEVMSG_CORE, "Failed av_read_frame\n");
		isDone = true;
	}

	// don't forget, otherwise memleak
	av_packet_unref(&packet);

	return !isDone;
#else
	return true;
#endif // MOVIELIB_DISABLE
}

static bool PlayerRewind(MoviePlayerData* player)
{
#ifndef MOVIELIB_DISABLE
	MoviePlayerData::VideoState& videoState = player->videoState;
	MoviePlayerData::AudioState& audioState = player->audioState;

	// restart
	if (av_seek_frame(player->formatCtx, player->videoStream->index, player->videoStream->start_time, AVSEEK_FLAG_BACKWARD) < 0)
	{
		DevMsg(DEVMSG_CORE, "Failed av_seek_frame\n");
		return false;
	}

	if (player->videoStream)
		videoState.videoOffset = videoState.lastVideoPts;

	if (player->audioStream)
		audioState.audioOffset = audioState.lastAudioPts;
#endif // MOVIELIB_DISABLE
	return true;
}

#ifndef MOVIELIB_DISABLE
static double clock_seconds(int64_t start_time)
{
	return (av_gettime() - start_time) / 1000000.;
}

static double pts_seconds(AVFrame* frame, AVStream* stream)
{
	return frame->pts * av_q2d(stream->time_base);
}
#endif // MOVIELIB_DISABLE

static void PlayerVideoDecodeStep(MoviePlayerData* player, ITexturePtr texture)
{
#ifndef MOVIELIB_DISABLE
	if (!player->videoStream)
		return;

	MoviePlayerData::VideoState& videoState = player->videoState;
	DecodeState& state = player->videoState.state;

	if (state == DEC_DEQUEUE_PACKET)
	{
		if (player->videoPacketQueue.getCount() == 0)
			return;

		// must delete old packet as it appears to be processed already
		av_packet_free(&videoState.deqPacket);
		videoState.deqPacket = player->videoPacketQueue.popFront();

		state = DEC_SEND_PACKET;
	}
	else if (state == DEC_SEND_PACKET)
	{
		const int r = avcodec_send_packet(player->videoCodec, videoState.deqPacket);
		if (r == 0)
		{
			state = DEC_DEQUEUE_PACKET;
			return;
		}

		if (r != AVERROR(EAGAIN))
		{
			state = DEC_ERROR;
			return;
		}
		state = DEC_RECV_FRAME;
	}
	else if (state == DEC_RECV_FRAME)
	{
		const int r = avcodec_receive_frame(player->videoCodec, videoState.frame);
		if (r == AVERROR(EAGAIN))
		{
			state = DEC_SEND_PACKET;
			return;
		}

		if (r != 0)
		{
			state = DEC_ERROR;
			return;
		}

		videoState.frame->pts = videoState.frame->pts / player->clockSpeed;

		const double clock = clock_seconds(player->clockStartTime);
		const double pts_secs = pts_seconds(videoState.frame, player->videoStream);
		const double delta = (pts_secs - clock);

		if (delta < -0.01f)
		{
			state = DEC_RECV_FRAME;
			return;
		}

		videoState.presentationDelay = delta;
		
		state = DEC_READY_FRAME;
	}
	else if (state == DEC_READY_FRAME)
	{
		// wait extra amount of time
		if (videoState.presentationDelay > videoState.timer.GetTime())
			return;

		if (!videoState.presentFrame)
			videoState.presentFrame = av_frame_clone(videoState.frame);

		videoState.timer.GetTime(true);
		state = DEC_RECV_FRAME;
	}
#endif // MOVIELIB_DISABLE
}

static void PlayerAudioDecodeStep(MoviePlayerData* player, FrameQueue& frameQueue)
{
#ifndef MOVIELIB_DISABLE
	if (!player->audioStream)
		return;

	MoviePlayerData::AudioState& audioState = player->audioState;
	DecodeState& state = player->audioState.state;

	if (state == DEC_DEQUEUE_PACKET)
	{
		if (player->audioPacketQueue.getCount() == 0)
			return;

		av_packet_free(&audioState.deqPacket);
		audioState.deqPacket = player->audioPacketQueue.popFront();

		state = DEC_SEND_PACKET;
	}
	else if (state == DEC_SEND_PACKET)
	{
		int r = avcodec_send_packet(player->audioCodec, audioState.deqPacket);
		if (r == 0)
		{
			state = DEC_DEQUEUE_PACKET;
			return;
		}

		if (r != AVERROR(EAGAIN))
		{
			state = DEC_ERROR;
			return;
		}
		state = DEC_RECV_FRAME;
	}
	else if (state == DEC_RECV_FRAME)
	{
		int r = avcodec_receive_frame(player->audioCodec, audioState.frame);
		if (r == AVERROR(EAGAIN))
		{
			state = DEC_SEND_PACKET;
			return;
		}

		if (r != 0)
		{
			state = DEC_ERROR;
			return;
		}

		state = DEC_RECV_FRAME;
		
		AVFrame* convFrame = av_frame_alloc();
		av_channel_layout_default(&convFrame->ch_layout, 2);

		convFrame->format = AV_SAMPLE_FMT_S16;
		convFrame->sample_rate = 44100;
		convFrame->pts = static_cast<double>(audioState.frame->pts) / player->clockSpeed;
		if (swr_convert_frame(player->audioSwr, convFrame, audioState.frame) == 0)
		{
			CScopedMutex m(s_audioSourceMutex);
			if (frameQueue.getCount() == AV_PACKET_AUDIO_CAPACITY)
			{
				AVFrame* frame = frameQueue.front();
				av_frame_free(&frame);
				frameQueue.remove(frameQueue.first());
			}

			frameQueue.append(convFrame);
		}
		else
			av_frame_free(&convFrame);
	}
#endif // MOVIELIB_DISABLE
}

static bool StartPlayback(MoviePlayerData* player)
{
	if (!player)
		return false;
#ifndef MOVIELIB_DISABLE
	if (player->videoStream)
	{
		av_init_packet(&player->packet);

		{
			MoviePlayerData::VideoState& videoState = player->videoState;
			videoState.frame = av_frame_alloc();
			videoState.state = DEC_DEQUEUE_PACKET;
			videoState.deqPacket = nullptr;
			videoState.timer.GetTime(true);
		}

		{
			MoviePlayerData::AudioState& audioState = player->audioState;
			audioState.frame = av_frame_alloc();
			audioState.state = DEC_DEQUEUE_PACKET;
			audioState.deqPacket = nullptr;
		}
	}
#endif // MOVIELIB_DISABLE
	return true;
}

//---------------------------------------------------

class CMovieAudioSource : public ISoundSource
{
	friend class CMoviePlayer;
public:
	CMovieAudioSource();

	int					GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const;

	const Format&		GetFormat() const { return m_format; }
	int					GetSampleCount() const;

	void*				GetDataPtr(int& dataSize) const { return nullptr; }
	int					GetLoopRegions(int* samplePos) const { return 0; }
	bool				IsStreaming() const;

private:
	bool				Load() { return true; }
	void				Unload() { }

	FrameQueue			m_frameQueue;
	MoviePlayerData*	m_player{ nullptr };
	Format				m_format;
	int					m_cursor{ 0 };
};

CMovieAudioSource::CMovieAudioSource()
{
	m_format.channels = 2;
	m_format.frequency = 44100;
	m_format.bitwidth = 16;
	m_format.dataFormat = 1;
}

bool CMovieAudioSource::IsStreaming() const 
{
	// NOTE: this is directly streamed because of alBufferCallbackSOFT
	// TODO: audio system capabilities?
	return false; 
}

int CMovieAudioSource::GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const
{
#ifndef MOVIELIB_DISABLE
	CScopedMutex m(s_audioSourceMutex);

	const int sampleSize = m_format.channels * (m_format.bitwidth >> 3);

	const int requestedSamples = samplesToRead;
	int numSamplesRead = 0;

	memset(out, 0, requestedSamples * sampleSize);

	auto it = m_frameQueue.first();
	while (samplesToRead > 0 && !it.atEnd())
	{
		AVFrame* frame = *it;

		const double clock = clock_seconds(m_player->clockStartTime);
		const double pts_secs = pts_seconds(frame, m_player->audioStream);
		const double delta = (pts_secs - clock);

		if (delta > F_EPS)
		{
			++it;
			continue;
		}

		const int frameSamples = frame->nb_samples - frame->height;
		if (frameSamples <= 0)
		{
			++it;
			continue;
		}
		const int paintSamples = min(samplesToRead, frameSamples);
		if (paintSamples <= 0)
			break;

		memcpy((ubyte*)out + numSamplesRead * sampleSize,
			frame->data[0] + frame->height * sampleSize,
			paintSamples * sampleSize);

		numSamplesRead += paintSamples;
		samplesToRead -= paintSamples;

		// keep the frame but we keep read cursor
		frame->height += paintSamples;
		++it;
	}

	if (numSamplesRead < requestedSamples)
		DevMsg(DEVMSG_CORE, "CMovieAudioSource::GetSamples underpaint - %d of %d\n", numSamplesRead, requestedSamples);

	// we don't have frames yet, return 1 because we need a warmup from video system
	return requestedSamples;
#else
	return 0;
#endif // MOVIELIB_DISABLE
}

int	CMovieAudioSource::GetSampleCount() const
{
	return INT_MAX;
}

//---------------------------------------------------

CMoviePlayer::~CMoviePlayer()
{
	Destroy();
}

int	CMoviePlayer::Run()
{
	if (!m_player)
		return 0;
#ifndef MOVIELIB_DISABLE
	m_player->clockStartTime = av_gettime();

	while (m_playerCmd == PLAYER_CMD_PLAYING)
	{
		YieldCurrentThread();

		if (!PlayerDemuxStep(m_player, OnCompleted))
			break;

		if (m_playerCmd == PLAYER_CMD_REWIND)
		{
			PlayerRewind(m_player);
			m_playerCmd = PLAYER_CMD_PLAYING;
		}

		if(m_mvTexture.IsValid())
			PlayerVideoDecodeStep(m_player, m_mvTexture.Get());

		if(m_audioSrc)
			PlayerAudioDecodeStep(m_player, static_cast<CMovieAudioSource*>(m_audioSrc.Ptr())->m_frameQueue);

		if (m_player->videoPacketQueue.getCount() == 0 && m_player->audioPacketQueue.getCount() == 0)
		{
			// NOTE: could be unreliable
			if (av_gettime() - m_player->clockStartTime > 10000)
				break;
		}
	}

#endif // MOVIELIB_DISABLE
	m_playerCmd = PLAYER_CMD_NONE;
	return 0;
}

CMoviePlayer::CMoviePlayer(const char* aliasName)
{
	m_aliasName = aliasName;
}

bool CMoviePlayer::Init(const char* pathToVideo)
{
#ifndef MOVIELIB_DISABLE
	const char* nameOfPlayer = m_aliasName.Length() ? m_aliasName.ToCString() : pathToVideo;

	m_player = CreatePlayerData(nullptr, pathToVideo);
	if(m_player)
	{
		const AVCodecContext* codec = m_player->videoCodec;

		if (m_player->videoStream)
		{
			m_mvTexture = g_matSystem->GetGlobalMaterialVarByName(nameOfPlayer);
			ASSERT(g_renderAPI->FindTexture(nameOfPlayer) == nullptr);
			m_mvTexture.Set(g_renderAPI->CreateProceduralTexture(nameOfPlayer, FORMAT_RGBA8, codec->width, codec->height));
		}

		if (m_player->audioStream)
		{
			m_audioSrc = ISoundSourcePtr(CRefPtr_new(CMovieAudioSource));
			((CMovieAudioSource*)m_audioSrc.Ptr())->m_player = m_player;
			m_audioSrc->SetFilename(nameOfPlayer);
			g_audioSystem->AddSample(m_audioSrc);
		}
	}
#endif // MOVIELIB_DISABLE
	return m_player != nullptr;
}

void CMoviePlayer::Destroy()
{
	Stop();
	FreePlayerData(m_player);
	SAFE_DELETE(m_player);
	m_audioSrc = nullptr;
	m_mvTexture.Set(nullptr);
	m_mvTexture = nullptr;
}

void CMoviePlayer::Start()
{
	if (!StartPlayback(m_player))
		return;

	m_playerCmd = PLAYER_CMD_PLAYING;

	StartThread("vidPlayer");
}

void CMoviePlayer::Stop()
{
	if (!m_player)
		return;

	m_playerCmd = PLAYER_CMD_STOP;

	if (GetThreadID() != GetCurrentThreadID())
		WaitForThread();

#ifndef MOVIELIB_DISABLE
	MoviePlayerData* player = m_player;
	for (AVPacket*& packet : player->videoPacketQueue)
		av_packet_free(&packet);

	for (AVPacket*& packet : player->audioPacketQueue)
		av_packet_free(&packet);

	{
		CScopedMutex m(s_audioSourceMutex);
		if(m_audioSrc)
			static_cast<CMovieAudioSource*>(m_audioSrc.Ptr())->m_frameQueue.clear();

		player->videoPacketQueue.clear();
		player->audioPacketQueue.clear();
	}
#endif // MOVIELIB_DISABLE
}

void CMoviePlayer::Rewind()
{
	m_playerCmd = PLAYER_CMD_REWIND;
}

bool CMoviePlayer::IsPlaying() const
{
	return IsRunning();
}

void CMoviePlayer::Present()
{
#ifndef MOVIELIB_DISABLE
	if (!m_player)
		return;

	ITexture* texture = m_mvTexture.Get();

	MoviePlayerData* player = m_player;
	MoviePlayerData::VideoState& videoState = m_player->videoState;

	if (!videoState.presentFrame)
		return;

	const int width = player->videoCodec->width;
	const int height = player->videoCodec->height;

	// time to update renderer texture
	ITexture::LockInOutData writeTo(TEXLOCK_DISCARD, IAARectangle(0, 0, width, height));
	if (texture->Lock(writeTo))
	{
		uint8_t* data[1] = {
			(ubyte*)writeTo.lockData
		};
		const int stride[1] = {
			(int)width * 4
		};
		sws_scale(player->videoSws, videoState.presentFrame->data, videoState.presentFrame->linesize, 0, videoState.presentFrame->height, data, stride);
		texture->Unlock();
	}

	av_frame_free(&videoState.presentFrame);
#endif // MOVIELIB_DISABLE
}

void CMoviePlayer::SetTimeScale(float value)
{
#ifndef MOVIELIB_DISABLE
	if(m_player)
		m_player->clockSpeed = value;
#endif // MOVIELIB_DISABLE
}

ITexturePtr CMoviePlayer::GetImage() const
{
	return m_mvTexture.Get();
}