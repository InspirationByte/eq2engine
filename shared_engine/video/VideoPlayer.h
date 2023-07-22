#pragma once

#include "audio/source/snd_source.h"

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

struct VideoPlayerData;
struct AudioPlayerData;
struct AVPacket;
struct AVFrame;

static constexpr const int AV_PACKET_AUDIO_CAPACITY = 20;
static constexpr const int AV_PACKET_VIDEO_CAPACITY = 20;

using APacketQueue = FixedList<AVPacket*, AV_PACKET_AUDIO_CAPACITY>;
using VPacketQueue = FixedList<AVPacket*, AV_PACKET_VIDEO_CAPACITY>;
using FrameQueue = FixedList<AVFrame*, AV_PACKET_AUDIO_CAPACITY>;

class CVideoAudioSource : public ISoundSource
{
	friend class CVideoPlayer;
public:
	CVideoAudioSource();

	int					GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const;

	const Format&		GetFormat() const { return m_format; }
	int					GetSampleCount() const;

	void*				GetDataPtr(int& dataSize) const { return nullptr; }
	int					GetLoopRegions(int* samplePos) const { return 0; }
	bool				IsStreaming() const { return true; }

private:
	bool				Load() { return true; }
	void				Unload() { }

	FrameQueue			m_frameQueue;
	Format				m_format;
	int					m_cursor{ 0 };
};

class CVideoPlayer : Threading::CEqThread
{
public:
	CVideoPlayer();
	~CVideoPlayer();

	bool				Init(const char* pathToVideo);
	void				Shutdown();

	void				Start();
	void				Stop();

	// if frame is decoded this will update texture image
	void				Present();
	ITexturePtr			GetImage() const;

	void				SetTimeScale(float value);

protected:
	int					Run() override;

	CVideoAudioSource	m_audioSrc;
	ITexturePtr			m_texture;
	VideoPlayerData*	m_player{ nullptr };
	bool				m_pendingQuit{ false };
};