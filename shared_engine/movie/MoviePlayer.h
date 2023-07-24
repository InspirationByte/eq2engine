#pragma once

#include "audio/source/snd_source.h"
#include "ds/event.h"

class CMovieAudioSource;
class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

struct MoviePlayerData;
using MovieCompletedEvent = Event<void()>;

class CMoviePlayer : Threading::CEqThread, public RefCountedObject<CMoviePlayer>
{
public:
	CMoviePlayer();
	~CMoviePlayer();

	bool				Init(const char* pathToVideo);
	void				Destroy();

	void				Start();
	void				Stop();
	void				Rewind();

	bool				IsPlaying() const;
	
	// if frame is decoded this will update texture image
	void				Present();
	ITexturePtr			GetImage() const;

	void				SetTimeScale(float value);

	// Used to signal user when movie is completed. Not thread-safe
	MovieCompletedEvent	OnCompleted;

protected:
	int					Run() override;

	CRefPtr<CMovieAudioSource>	m_audioSrc;
	ITexturePtr					m_texture;
	MoviePlayerData*			m_player{ nullptr };
	int							m_playerCmd{ 0 };
};