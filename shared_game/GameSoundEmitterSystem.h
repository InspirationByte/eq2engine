//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve'Source)
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMESNDEMITSYSTEM_H
#define GAMESNDEMITSYSTEM_H

#include "math/Vector.h"
#include "ISoundSystem.h"
#include "utils/eqstring.h"
#include "utils/DkList.h"

class BaseEntity;

// flags
enum EmitSoundFlags_e
{
	EMITSOUND_FLAG_OCCLUSION		= (1 << 0),		// occludes source by the world geometry
	EMITSOUND_FLAG_ROOM_OCCLUSION	= (1 << 1),		// uses more expensive occlusion system, use it for ambient sounds
	EMITSOUND_FLAG_FORCE_CACHED		= (1 << 2),		// forces emitted sound to be loaded if not cached by PrecacheScriptSound (not recommended, debug only)
	EMITSOUND_FLAG_FORCE_2D			= (1 << 3),		// force 2D sound (music, etc.)
	EMITSOUND_FLAG_STARTSILENT		= (1 << 4),		// starts silent
	EMITSOUND_FLAG_START_ON_UPDATE	= (1 << 5),		// start playing sound on emitter system update
};

// channel type for entity call
enum Channel_t
{
	CHAN_INVALID = -1,

	CHAN_STATIC,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	CHAN_WEAPON,
	CHAN_STREAM,	// streaming channel

	CHAN_COUNT
};

static int channel_max_sounds[CHAN_COUNT] =
{
	16, // 16 static channels for entity
	1,	// 1 voice/speech of human
	3,	// 3 for item (clicks, etc)
	16,	// 3 for body
	1,	// 1 for weapon shoot sounds
	1	// one streaming sound
};

static const char* channel_names[CHAN_COUNT] =
{
	"CHAN_STATIC",
	"CHAN_VOICE",
	"CHAN_ITEM",
	"CHAN_BODY",
	"CHAN_WEAPON",
	"CHAN_STREAM",
};

static Channel_t ChannelFromString(char* str)
{
	for(int i = 0; i < CHAN_COUNT; i++)
	{
		if(!stricmp(str, channel_names[i]))
			return (Channel_t)i;
	}

	return CHAN_INVALID;
}

struct channelemitter_t
{
	ISoundEmitter **staticEmitters;
	Channel_t		channel;
};

//---------------------------------------------------------------------------------

struct EmitSound_t;

class CSoundChannelObject
{
public:
	//PPMEM_MANAGED_OBJECT()

	CSoundChannelObject();
	virtual ~CSoundChannelObject() {}

	// emit sound
	void		EmitSound(const char* name);

	// emit sound with parameters
	void		EmitSoundWithParams(EmitSound_t *ep);

	int			GetChannelSoundCount(Channel_t chan);
	void		DecrementChannelSoundCount(Channel_t chan);

	void		SetSoundVolumeScale( float fScale ) {m_volumeScale = fScale;}
	float		GetSoundVolumeScale() const { return m_volumeScale;}

protected:
	// sounds at channel counter
	int			m_numChannelSounds[CHAN_COUNT];

	float		m_volumeScale;
};

//---------------------------------------------------------------------------------

struct EmitSound_t
{
	// the first and biggest
	void Init( const char* pszName, const Vector3D& origin, float volume, float pitch, float radius, CSoundChannelObject* obj, int flags );

	EmitSound_t()
	{
		Init("", vec3_zero, 1.0f, 1.0f, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName )
	{
		Init(pszName, vec3_zero, 1.0f, 1.0f, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName, int flags )
	{
		Init(pszName, vec3_zero, 1.0f, 1.0f, 1.0f, NULL, flags);
	}

	EmitSound_t( const char* pszName, float volume, float pitch )
	{
		Init(pszName, vec3_zero, volume, pitch, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName, const Vector3D& pos )
	{
		Init(pszName, pos, 1.0f, 1.0f, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName, const Vector3D& pos, float volume, float pitch )
	{
		Init(pszName, pos, volume, pitch, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	const char*				name;
	Vector3D				origin;
	float					fVolume;
	float					fPitch;
	float					fRadiusMultiplier;

	CSoundChannelObject*	pObject;

	int						nFlags;
	int						sampleId;
	int						emitterIndex;
};

typedef EmitSound_t EmitParams;

struct scriptsounddata_t
{
	char*		pszName;

	DkList<ISoundSample*>	pSamples;
	DkList<EqString>		soundFileNames;

	float		fVolume;
	float		fAtten;
	float		fPitch;
	float		fAirAbsorption;
	float		fMaxDistance;

	bool		extraStreaming;
	bool		loop;
	bool		is2d;

	Channel_t	channel;
};


// Dynamic sound controller
class ISoundController
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual ~ISoundController() {}

	virtual bool			IsStopped() const = 0;

	virtual void			StartSound( const char* newSoundName = NULL ) = 0; // starts sound
	virtual void			PauseSound() = 0; // pauses sound
	virtual void			StopSound() = 0; // stops sound and detach channel

	virtual EmitSound_t&	GetEmitParams() = 0;

	virtual void			SetPitch(float fPitch) = 0; // pitch in range from 0.0001 ... 5
	virtual void			SetVolume(float fVolume) = 0; // volume in range from 0 .. 1
	virtual void			SetOrigin(const Vector3D& origin) = 0;
	virtual void			SetVelocity(const Vector3D& velocity) = 0;
};

struct EmitterData_t
{
	EmitterData_t()
	{
		pEmitter = NULL;
		pObject = NULL;
		pController = NULL;
		emitSoundData = NULL;
		script = NULL;
	}

	scriptsounddata_t*		script;
	CSoundChannelObject*	pObject;
	ISoundController*		pController;
	ISoundEmitter*			pEmitter;

	Vector3D				origin;
	Vector3D				interpolatedOrigin;
	Vector3D				velocity;

	float					origVolume;

	Channel_t				channel;

	EmitSound_t				emitSoundData;

	bool					isVirtual;
};

// Dynamic sound controller
class CSoundController : public ISoundController
{
	friend class	CSoundEmitterSystem;
public:

	CSoundController() : m_emitData(NULL)
	{
	}

	bool			IsStopped() const;

	void			StartSound(const char* newSoundName = NULL); // starts sound

	void			PauseSound(); // pauses sound

	void			StopSound(); // stops sound

	EmitSound_t&	GetEmitParams() {return m_emitParams;}

	void			SetPitch(float fPitch);					// pitch in range from 0.0001 ... 5
	void			SetVolume(float fVolume);				// volume in range from 0 .. 1
	void			SetOrigin(const Vector3D& origin);
	void			SetVelocity(const Vector3D& velocity);

protected:

	EmitSound_t		m_emitParams;
	EqString		m_soundName;	// hold sound name

	EmitterData_t*	m_emitData;
};

// the sound emitter system
class CSoundEmitterSystem
{
	friend class				CSoundController;

public:
								CSoundEmitterSystem() : m_isInit(false) {}

	void						Init();
	void						Shutdown();

	void						PrecacheSound(const char* pszName);							// precaches sound

	// emits new sound. returns channel type
	int							EmitSound( EmitSound_t* emit );								// emits sound with specified parameters
	void						Emit2DSound( EmitSound_t* emit, int channel = -1 );


	void						StopAllSounds();

	void						Update();													// updates sound emitter system

	bool						UpdateEmitter( EmitterData_t* emit, soundParams_t &params, bool bForceNoInterp = false );

	scriptsounddata_t*			FindSound(const char* soundName);							// searches for loaded script sound

	ISoundController*			CreateSoundController(EmitSound_t* ep);						// creates new sound controller
	void						RemoveSoundController(ISoundController* cont);

protected:
	void						LoadScriptSoundFile(const char* fileName);

	int							GetEmitterIndexByEntityAndChannel(CSoundChannelObject* pEnt, Channel_t chan);

#ifdef EDITOR
public:
#else
private:
#endif

	DkList<scriptsounddata_t*>	m_scriptsoundlist;

#ifdef EDITOR
private:
#endif

	DkList<EmitterData_t*>		m_pCurrentTempEmitters;
	DkList<ISoundController*>	m_pSoundControllerList;
	DkList<EmitSound_t>			m_pUnreleasedSounds;

	bool						m_isInit;

	Vector3D					m_vViewPos;
	bool						m_bViewIsAvailable;
	int							m_rooms[2];
	int							m_nRooms;
};

extern CSoundEmitterSystem* ses;

#endif // GAMESNDEMITSYSTEM_H
