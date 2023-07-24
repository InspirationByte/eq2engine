#pragma once

#include "audio/source/snd_source.h"

class CMovieAudioSource;
class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

struct MoviePlayerData;

class CMoviePlayer : Threading::CEqThread, public RefCountedObject<CMoviePlayer>
{
public:
	CMoviePlayer();
	~CMoviePlayer();

	bool				Init(const char* pathToVideo);
	void				Destroy();

	void				Start();
	void				Stop();

	// if frame is decoded this will update texture image
	void				Present();
	ITexturePtr			GetImage() const;

	void				SetTimeScale(float value);

protected:
	int					Run() override;

	CRefPtr<CMovieAudioSource>	m_audioSrc;
	ITexturePtr					m_texture;
	MoviePlayerData*			m_player{ nullptr };
	bool						m_pendingQuit{ false };
};