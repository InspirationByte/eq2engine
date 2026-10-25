#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "audio/IEqAudioSystem.h"
#include "audio/source/snd_source.h"
#include "materialsystem1/IMaterialSystem.h"

#ifndef MOVIELIB_DISABLE
#include "MovieDemuxWebM.h"
#include "MovieVideoDecoder.h"
#include "MovieAudioDecoderVorbis.h"
#include "MovieYUVConverter.h"
#endif // MOVIELIB_DISABLE

#include "MoviePlayer.h"

using namespace Threading;

static constexpr const int MOVIE_VIDEO_PACKET_CAPACITY = 10;
static constexpr const int MOVIE_AUDIO_PACKET_CAPACITY = 25;
static constexpr const int MOVIE_AUDIO_BUFFER_CAPACITY = 25;

// gap between the last and the first frame when movie is looped
static constexpr const int64 MOVIE_DEFAULT_FRAME_DURATION_NS = 33333333;

enum EPlayerCmd : int
{
	PLAYER_CMD_NONE = 0,
	PLAYER_CMD_PLAYING,
	PLAYER_CMD_REWIND,
	PLAYER_CMD_STOP,
};

#ifndef MOVIELIB_DISABLE
struct MovieAudioBuffer
{
	Array<short>	samples{ PP_SL };
	int64			ptsNs{ 0 };
	int				numFrames{ 0 };
	int				cursor{ 0 };
};

using VPacketQueue = FixedList<MoviePacket*, MOVIE_VIDEO_PACKET_CAPACITY>;
using APacketQueue = FixedList<MoviePacket*, MOVIE_AUDIO_PACKET_CAPACITY>;
using AudioBufferQueue = FixedList<MovieAudioBuffer*, MOVIE_AUDIO_BUFFER_CAPACITY>;
#endif // MOVIELIB_DISABLE

struct MoviePlayerData
{
#ifndef MOVIELIB_DISABLE
	IFileStreamPtr				file;
	CMovieDemuxWebM				demux;
	bool						demuxEof{ false };

	// Video
	IMovieVideoDecoder*			videoDecoder{ nullptr };
	CMovieYUVConverter			yuvConverter;
	VPacketQueue				videoPacketQueue;
	MoviePacket*				videoPacket{ nullptr };		// packet which decoder did not accept yet
	MovieVideoFramePtr			readyFrame;					// decoded frame waiting for its presentation time
	bool						videoError{ false };

	CEqMutex					presentMutex;
	MovieVideoFramePtr			presentFrame;
	int64						presentedPtsNs{ 0 };

	// Audio
	CMovieAudioDecoderVorbis	audioDecoder;
	APacketQueue				audioPacketQueue;
	int64						audioNextPtsNs{ 0 };
	bool						hasAudio{ false };

	CEqMutex					audioMutex;
	AudioBufferQueue			audioBuffers;

	// timing
	CEqTimer					clock;
	double						clockStartSec{ 0.0 };
	float						clockSpeed{ 1.0f };
	int64						ptsOffsetNs{ 0 };
	int64						lastPtsNs{ 0 };
#endif // MOVIELIB_DISABLE
};

#ifndef MOVIELIB_DISABLE
static double PlayerClockSeconds(MoviePlayerData* player)
{
	return player->clockStartSec + player->clock.GetTime();
}

static double PlayerPtsSeconds(MoviePlayerData* player, int64 ptsNs)
{
	return static_cast<double>(ptsNs) * 1e-9 / player->clockSpeed;
}

static void ClearPlayerQueues(MoviePlayerData* player)
{
	for (MoviePacket* packet : player->videoPacketQueue)
		delete packet;
	player->videoPacketQueue.clear();

	for (MoviePacket* packet : player->audioPacketQueue)
		delete packet;
	player->audioPacketQueue.clear();

	SAFE_DELETE(player->videoPacket);
	player->readyFrame = nullptr;

	{
		CScopedMutex m(player->presentMutex);
		player->presentFrame = nullptr;
	}

	{
		CScopedMutex m(player->audioMutex);
		for (MovieAudioBuffer* buffer : player->audioBuffers)
			delete buffer;
		player->audioBuffers.clear();
	}
}
#endif // MOVIELIB_DISABLE

static void FreePlayerData(MoviePlayerData* player)
{
	if (!player)
		return;
#ifndef MOVIELIB_DISABLE
	ClearPlayerQueues(player);

	// frames must be released before the decoder
	if (player->videoDecoder)
		player->videoDecoder->Shutdown();
	SAFE_DELETE(player->videoDecoder);

	player->audioDecoder.Shutdown();
	player->yuvConverter.Shutdown();
	player->demux.Close();
	player->file = nullptr;
#endif // MOVIELIB_DISABLE
}

static MoviePlayerData* CreatePlayerData(const char* filename)
{
	MoviePlayerData* player = PPNew MoviePlayerData;
#ifndef MOVIELIB_DISABLE
	bool failed = false;
	defer{
		if (!failed || !player)
			return;

		FreePlayerData(player);
		SAFE_DELETE(player);
	};

	player->file = g_fileSystem->Open(filename, FS_OPEN_READ);
	if (!player->file)
	{
		MsgError("Cannot open video file %s\n", filename);
		failed = true;
		return nullptr;
	}

	if (!player->demux.Open(player->file))
	{
		MsgError("Failed to open video file %s\n", filename);
		failed = true;
		return nullptr;
	}

	player->videoDecoder = CreateMovieVideoDecoderDav1d();
	if (!player->videoDecoder->Init(player->demux.GetVideoSize()))
	{
		failed = true;
		return nullptr;
	}

	if (player->demux.HasAudio())
	{
		player->hasAudio = player->audioDecoder.Init(player->demux);
		if (!player->hasAudio)
			MsgWarning("Video %s audio stream can't be decoded, playing without sound\n", filename);
	}
#endif // MOVIELIB_DISABLE
	return player;
}

#ifndef MOVIELIB_DISABLE
static void PlayerRewind(MoviePlayerData* player)
{
	if (!player->demux.Rewind())
	{
		DevMsg(DEVMSG_CORE, "Failed to rewind video\n");
		player->demuxEof = true;
		return;
	}

	// timestamps keep growing so playback clock is not reset on loop
	player->ptsOffsetNs = player->lastPtsNs + MOVIE_DEFAULT_FRAME_DURATION_NS;

	player->audioDecoder.Reset();
	player->demuxEof = false;
}

static void PlayerDemuxStep(MoviePlayerData* player, MovieCompletedEvent& completedEvent)
{
	if (player->demuxEof)
		return;

	if (player->videoPacketQueue.getCount() >= MOVIE_VIDEO_PACKET_CAPACITY)
		return;

	if (player->audioPacketQueue.getCount() >= MOVIE_AUDIO_PACKET_CAPACITY)
		return;

	MoviePacket* packet = PPNew MoviePacket;
	const EMovieDemuxResult result = player->demux.ReadPacket(*packet);
	if (result != MOVIE_DEMUX_OK)
	{
		delete packet;

		if (result == MOVIE_DEMUX_ERROR)
			DevMsg(DEVMSG_CORE, "Failed to read video packet\n");

		player->demuxEof = true;
		completedEvent();
		return;
	}

	packet->ptsNs += player->ptsOffsetNs;
	player->lastPtsNs = max(player->lastPtsNs, packet->ptsNs);

	if (packet->track == MOVIE_TRACK_VIDEO)
		player->videoPacketQueue.append(packet);
	else if (packet->track == MOVIE_TRACK_AUDIO && player->hasAudio)
		player->audioPacketQueue.append(packet);
	else
		delete packet;
}

static void PlayerVideoDecodeStep(MoviePlayerData* player)
{
	if (player->videoError)
		return;

	if (player->readyFrame)
	{
		if (PlayerPtsSeconds(player, player->readyFrame->ptsNs) > PlayerClockSeconds(player))
			return;

		{
			CScopedMutex m(player->presentMutex);
			player->presentFrame = player->readyFrame;
			player->presentedPtsNs = player->readyFrame->ptsNs;
		}
		player->readyFrame = nullptr;
	}

	MovieVideoFramePtr frame;
	const EMovieDecodeResult recvResult = player->videoDecoder->ReceiveFrame(frame);
	if (recvResult == MOVIE_DECODE_OK)
	{
		player->readyFrame = frame;
		return;
	}

	if (recvResult == MOVIE_DECODE_ERROR)
	{
		MsgError("Video decoding failed\n");
		player->videoError = true;
		return;
	}

	// decoder needs more data
	if (!player->videoPacket)
	{
		if (player->videoPacketQueue.getCount() == 0)
			return;

		player->videoPacket = player->videoPacketQueue.popFront();
	}

	const EMovieDecodeResult sendResult = player->videoDecoder->SendPacket(*player->videoPacket);
	if (sendResult == MOVIE_DECODE_AGAIN)
		return;

	if (sendResult == MOVIE_DECODE_ERROR)
		DevMsg(DEVMSG_CORE, "Video decoder rejected packet\n");

	SAFE_DELETE(player->videoPacket);
}

static void PlayerAudioDecodeStep(MoviePlayerData* player)
{
	if (!player->hasAudio || player->audioPacketQueue.getCount() == 0)
		return;

	MoviePacket* packet = player->audioPacketQueue.popFront();
	defer{
		delete packet;
	};

	// laced frames share the block timestamp
	if (packet->frameIdx == 0)
		player->audioNextPtsNs = packet->ptsNs;

	MovieAudioBuffer* buffer = PPNew MovieAudioBuffer;
	buffer->numFrames = player->audioDecoder.Decode(*packet, buffer->samples);
	buffer->ptsNs = player->audioNextPtsNs;

	player->audioNextPtsNs += static_cast<int64>(buffer->numFrames) * 1000000000LL / player->audioDecoder.GetSampleRate();

	if (buffer->numFrames == 0)
	{
		delete buffer;
		return;
	}

	CScopedMutex m(player->audioMutex);

	// audio is not consumed in time, drop the oldest
	if (player->audioBuffers.getCount() == MOVIE_AUDIO_BUFFER_CAPACITY)
		delete player->audioBuffers.popFront();

	player->audioBuffers.append(buffer);
}

static bool PlayerIsFinished(MoviePlayerData* player)
{
	if (!player->demuxEof)
		return false;

	if (player->videoPacketQueue.getCount() || player->audioPacketQueue.getCount())
		return false;

	if (player->videoError)
		return true;

	return !player->videoPacket && !player->readyFrame;
}
#endif // MOVIELIB_DISABLE

//---------------------------------------------------

class CMovieAudioSource : public ISoundSource
{
	friend class CMoviePlayer;
public:
	CMovieAudioSource(MoviePlayerData* player);

	int					GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const;

	const Format&		GetFormat() const { return m_format; }
	int					GetSampleCount() const;

	void*				GetDataPtr(int& dataSize) const { return nullptr; }
	int					GetLoopRegions(int* samplePos) const { return 0; }
	bool				IsStreaming() const;

private:
	bool				Load() { return true; }
	void				Unload() { }

	MoviePlayerData*	m_player{ nullptr };
	Format				m_format;
};

CMovieAudioSource::CMovieAudioSource(MoviePlayerData* player)
	: m_player(player)
{
#ifndef MOVIELIB_DISABLE
	m_format.channels = player->audioDecoder.GetChannels();
	m_format.frequency = player->audioDecoder.GetSampleRate();
#endif // MOVIELIB_DISABLE
	m_format.bitwidth = 16;
	m_format.dataFormat = 1;
}

bool CMovieAudioSource::IsStreaming() const
{
	// NOTE: this is directly streamed because of alBufferCallbackSOFT
	return false;
}

int CMovieAudioSource::GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const
{
#ifndef MOVIELIB_DISABLE
	MoviePlayerData* player = m_player;
	CScopedMutex m(player->audioMutex);

	const int sampleSize = m_format.GetStride();
	const int requestedSamples = samplesToRead;
	int numSamplesRead = 0;

	memset(out, 0, requestedSamples * sampleSize);

	const double clock = PlayerClockSeconds(player);

	while (samplesToRead > 0 && player->audioBuffers.getCount())
	{
		MovieAudioBuffer* buffer = *player->audioBuffers.first();

		// buffers are ordered by time
		if (PlayerPtsSeconds(player, buffer->ptsNs) - clock > F_EPS)
			break;

		const int paintSamples = min(samplesToRead, buffer->numFrames - buffer->cursor);
		memcpy(reinterpret_cast<ubyte*>(out) + numSamplesRead * sampleSize,
			buffer->samples.ptr() + buffer->cursor * m_format.channels,
			paintSamples * sampleSize);

		numSamplesRead += paintSamples;
		samplesToRead -= paintSamples;
		buffer->cursor += paintSamples;

		if (buffer->cursor >= buffer->numFrames)
			delete player->audioBuffers.popFront();
	}

	if (numSamplesRead < requestedSamples)
		DevMsg(DEVMSG_CORE, "CMovieAudioSource::GetSamples underpaint - %d of %d\n", numSamplesRead, requestedSamples);

	// silence is returned until video system produces audio
	return requestedSamples;
#else
	return 0;
#endif // MOVIELIB_DISABLE
}

int	CMovieAudioSource::GetSampleCount() const
{
	return COM_INT_MAX;
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
	MoviePlayerData* player = m_player;

	// continue from the last presented frame
	player->clockStartSec = PlayerPtsSeconds(player, player->presentedPtsNs);
	player->clock.GetTime(true);

	bool started = false;

	while (m_playerCmd == PLAYER_CMD_PLAYING || m_playerCmd == PLAYER_CMD_REWIND)
	{
		YieldCurrentThread();

		PlayerDemuxStep(player, OnCompleted);

		if (m_playerCmd == PLAYER_CMD_REWIND)
		{
			PlayerRewind(player);
			m_playerCmd = PLAYER_CMD_PLAYING;
		}

		// wait until fully bufferized
		if (!started)
		{
			if (player->videoPacketQueue.getCount() >= MOVIE_VIDEO_PACKET_CAPACITY / 2 || player->demuxEof)
			{
				started = true;
				player->clock.GetTime(true);
			}
			continue;
		}

		PlayerVideoDecodeStep(player);

		if (m_audioSrc)
			PlayerAudioDecodeStep(player);

		if (PlayerIsFinished(player))
			break;
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

	m_player = CreatePlayerData(pathToVideo);
	if (!m_player)
		return false;

	const IVector2D videoSize = m_player->demux.GetVideoSize();

	ASSERT(g_renderAPI->FindTexture(nameOfPlayer) == nullptr);
	if (!m_player->yuvConverter.Init(nameOfPlayer, videoSize.x, videoSize.y))
	{
		FreePlayerData(m_player);
		SAFE_DELETE(m_player);
		return false;
	}

	m_mvTexture = g_matSystem->GetGlobalMaterialVarByName(nameOfPlayer);
	m_mvTexture.Set(m_player->yuvConverter.GetTarget());

	if (m_player->hasAudio)
	{
		m_audioSrc = ISoundSourcePtr(CRefPtr_new(CMovieAudioSource, m_player));
		m_audioSrc->SetFilename(nameOfPlayer);
		g_audioSystem->AddSample(m_audioSrc);
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
	if (!m_player)
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
	ClearPlayerQueues(m_player);

	if (m_player->videoDecoder)
		m_player->videoDecoder->Flush();
	m_player->videoError = false;
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

	MovieVideoFramePtr frame;
	{
		CScopedMutex m(m_player->presentMutex);
		frame = m_player->presentFrame;
		m_player->presentFrame = nullptr;
	}

	if (frame)
		m_player->yuvConverter.Convert(*frame);
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
