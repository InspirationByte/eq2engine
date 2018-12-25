//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound entities
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEngineHeader.h"
#include "IGameRules.h"

class CAmbientGeneric : public BaseEntity
{
public:

	// always do this type conversion
	typedef BaseEntity BaseClass;

	CAmbientGeneric()
	{
		m_bRepeat = false;
		m_bEnabled = true;
		m_fPitch = 1.0f;
		m_fVolume = 1.0f;
		m_fRandomMod = 0.0f; // -+ 1 second pepeat
		m_fRepeatInterval = 1.0f; // 1 second + random by default
		m_fLastTime = 0.0f;
		m_fDistanceScale = 1.0f;
	}

	void PlaySound()
	{
		EmitSound_t ep;
		ep.fPitch = m_fPitch;
		ep.fRadiusMultiplier = m_fDistanceScale;
		ep.fVolume = m_fVolume;
		ep.origin = Vector3D(0);
		ep.name = (char*)m_soundName.GetData();
		ep.pObject = this;
		ep.nFlags |= (EMITSOUND_FLAG_ROOM_OCCLUSION | EMITSOUND_FLAG_START_ON_UPDATE);

		ses->EmitSound( &ep );
	}

	void Precache()
	{
		ses->PrecacheSound( m_soundName.GetData() );
	}

	void Spawn()
	{
		if(m_bEnabled && !m_bRepeat)
			PlaySound();
	}

	void Update(float decaytime)
	{
		if(m_bRepeat)
		{
			if(m_fLastTime == 0.0f)
				m_fLastTime = gpGlobals->curtime + m_fRepeatInterval + RandomFloat(-1*m_fRandomMod, m_fRandomMod);

			if(gpGlobals->curtime > m_fLastTime)
			{
				if(m_bEnabled)
					PlaySound();

				m_fLastTime = 0.0f;
			}
		}

		BaseClass::Update(decaytime);
	}

	void InputPlaySound(inputdata_t &data)
	{
		PlaySound();
	}

private:

	EqString m_soundName;

	bool m_bEnabled;

	bool m_bRepeat;

	float m_fRepeatInterval;
	float m_fRandomMod;
	float m_fPitch;
	float m_fVolume;
	float m_fDistanceScale;

	float m_fLastTime;

	DECLARE_DATAMAP();
};

// declare data info
BEGIN_DATAMAP(CAmbientGeneric)
	DEFINE_KEYFIELD( m_soundName,		"soundname", VTYPE_STRING),
	DEFINE_KEYFIELD( m_fPitch,			"startpitch", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fVolume,			"startvolume", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fDistanceScale,	"distance", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fRandomMod,		"randomMod", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fRepeatInterval,	"repeatTime", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_bRepeat,			"repeat", VTYPE_BOOLEAN),
	DEFINE_KEYFIELD( m_bEnabled,		"starton", VTYPE_BOOLEAN),
	DEFINE_INPUTFUNC( "Play", InputPlaySound)
END_DATAMAP()

DECLARE_ENTITYFACTORY(ambient_generic, CAmbientGeneric)

extern sndEffect_t* g_pSoundEffect;

//// this is debug-only structure
//struct soundEffect_t
//{
//	char		name[64];
//};

// Room effect
class CEnvRoomEffect : public BaseEntity
{
public:

	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvRoomEffect()
	{
		m_pEffect = NULL;
		m_effectName = "none";
		m_bEnabled = true;
	}

	void Spawn()
	{

	}

	void Activate()
	{
		m_pEffect = soundsystem->FindEffect( m_effectName.GetData() );

		m_nRooms = eqlevel->GetRoomsForPoint( GetAbsOrigin(), m_pRooms );

		if(!m_nRooms)
			Msg("env_room_effect is outside the world!\n");

		BaseClass::Activate();
	}

	void OnPreRender()
	{
		if( g_pViewEntity && m_bEnabled )
		{
			int view_rooms[2] = {-1, -1};
			int num_rooms = eqlevel->GetRoomsForPoint(g_pViewEntity->GetEyeOrigin(), view_rooms);

			if(num_rooms > 0 && (view_rooms[0] == m_pRooms[0]))
			{
				//soundEffect_t* pEffect = (soundEffect_t*)m_pEffect;

				//if(pEffect)
				//	debugoverlay->Text(ColorRGBA(1,1,1,1), "Current room effect: %s\n", pEffect->name);

				g_pSoundEffect = m_pEffect;
			}
		}
	}

private:

	EqString		m_effectName;

	sndEffect_t*	m_pEffect;
	bool			m_bEnabled;

	DECLARE_DATAMAP();
};

// declare data info
BEGIN_DATAMAP(CEnvRoomEffect)
	DEFINE_KEYFIELD( m_effectName,		"effectname", VTYPE_SOUNDNAME),
	DEFINE_KEYFIELD( m_bEnabled,		"enabled", VTYPE_BOOLEAN),
END_DATAMAP()

DECLARE_ENTITYFACTORY(env_room_effect, CEnvRoomEffect)

//-------------------------------------------------------------------------
// The sound scape
//-------------------------------------------------------------------------

struct soundScape_randomSound_t
{
	EqString	name;
	float		repeatTime;
	float		repeatRandomMod;
	float		distanceRandom;
};

struct soundScape_entry_t
{
	EqString							name;

	ISoundController*					ambientsound;
	sndEffect_t*						effect;

	DkList<soundScape_randomSound_t>	randomSounds;
};

// Room sounds and effects
class CEnvSoundScape : public BaseEntity
{
public:

	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvSoundScape()
	{
		m_soundScapeName = "none";
	}

	void Activate()
	{
		BaseClass::Activate();

		// read sound params
		KeyValues kvs;
		if(!kvs.LoadFromFile(varargs("scripts/soundscape_%s.txt",m_soundScapeName.GetData())))
		{
			MsgError("Can't open 'scripts/soundscapes/%s.txt' for soundscapes!!!\n", m_soundScapeName.GetData());
			SetState( BaseEntity::ENTITY_REMOVE );
		}
		else
		{
			//kvs.GetRootSection()->FindKeyBase

			//m_soundScape.ambientsound = ses->CreateSoundController( &sound );
		}
	}

private:

	EqString					m_soundScapeName;
	DkList<soundScape_entry_t>	m_soundScapeEntries;

	DECLARE_DATAMAP();
};

// declare data info
BEGIN_DATAMAP( CEnvSoundScape )
	DEFINE_KEYFIELD( m_soundScapeName,	"soundscape", VTYPE_SOUNDNAME),
END_DATAMAP()

DECLARE_ENTITYFACTORY(env_soundscape, CEnvSoundScape)

// Room sounds and effects
class CEnvSoundScapeTarget : public BaseEntity
{
public:

	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvSoundScapeTarget()
	{
		m_bEnabled = true;
		m_szSoundScapeEntry = "not_set";
	}

	void Spawn()
	{
		BaseClass::Spawn();

		CheckTargetEntity();

		CEnvSoundScape* pSoundScape = dynamic_cast<CEnvSoundScape*>(GetTargetEntity());

		if( pSoundScape )
		{
			
		}
	}

	void Activate()
	{
		BaseClass::Activate();
	}

	void OnPreRender()
	{
		if( g_pViewEntity && m_bEnabled )
		{
			int view_rooms[2] = {-1, -1};
			int num_rooms = eqlevel->GetRoomsForPoint(g_pViewEntity->GetEyeOrigin(), view_rooms);

			if(num_rooms > 0 && (view_rooms[0] == m_pRooms[0]))
			{
				
			}
		}
	}

private:

	CEnvSoundScape*		m_pSoundScape;
	soundScape_entry_t*	m_pEntry;

	bool				m_bEnabled;
	EqString			m_szSoundScapeEntry;

	DECLARE_DATAMAP();
};

// declare data info
BEGIN_DATAMAP( CEnvSoundScapeTarget )
	DEFINE_KEYFIELD( m_bEnabled,			"enabled", VTYPE_BOOLEAN),
	DEFINE_KEYFIELD( m_szSoundScapeEntry,	"entry", VTYPE_STRING),
END_DATAMAP()

DECLARE_ENTITYFACTORY(env_soundscape_target, CEnvSoundScapeTarget)