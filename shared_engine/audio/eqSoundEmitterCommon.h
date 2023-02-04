#pragma once

class CSoundingObject;

// flags
enum EEmitSoundFlags
{
	EMITSOUND_FLAG_RELEASE_ON_STOP	= (1 << 0),		// can be used on temporary distant sounds
	EMITSOUND_FLAG_FORCE_CACHED		= (1 << 1),		// forces emitted sound to be loaded if not cached by PrecacheScriptSound (not recommended, debug only)
	EMITSOUND_FLAG_FORCE_2D			= (1 << 2),		// force 2D sound (music, etc.)
	EMITSOUND_FLAG_STARTSILENT		= (1 << 3),		// starts silent
	EMITSOUND_FLAG_START_ON_UPDATE	= (1 << 4),		// start playing sound on emitter system update
	EMITSOUND_FLAG_RANDOM_PITCH		= (1 << 5),		// apply slightly random pitch (best for static hit sounds)

	EMITSOUND_FLAG_PENDING			= (1 << 6),		// was in pending list
};

static constexpr const int CHAN_INVALID = -1;
static constexpr const int CHAN_MAX = 16;

// mandatory: rename to SoundChannelDef
struct ChannelDef
{
	const char* name;
	int id;
	int limit;
};

#define DEFINE_SOUND_CHANNEL(name, limit) { #name, name, limit }

// mandatory: rename to EmitSoundParams
struct EmitParams
{
	EmitParams() = default;

	EmitParams( const char* pszName ) :
		name(pszName)
	{
	}

	EmitParams( const char* pszName, int flags ) :
		name(pszName),
		flags(flags)
	{
	}

	EmitParams( const char* pszName, float volume, float pitch ) :
		name(pszName),
		volume(volume), pitch(pitch)
	{
	}

	EmitParams( const char* pszName, const Vector3D& pos ) :
		name(pszName),
		origin(pos)
	{
	}

	EmitParams( const char* pszName, const Vector3D& pos, float volume, float pitch ) :
		name(pszName),
		origin(pos),
		volume(volume), pitch(pitch)
	{
	}

	struct InputValue
	{
		InputValue() = default;

		InputValue(const char* name, float value)
			: nameHash(StringToHash(name)), value(value) {}

		InputValue(int nameHash, float value)
			: nameHash(nameHash), value(value) {}

		int nameHash{ 0 };
		float value{ 0.0f };
	};

	EqString			name;
	Vector3D			origin{ vec3_zero };
	float				volume{ 1.0f };
	float				pitch{ 1.0f };
	float				radiusMultiplier{ 1.0f };

	int					effectSlot{ -1 };
	int					flags{ 0 };
	int					sampleId{ -1 };
	int					channelType{ CHAN_INVALID };

	int							objUniqueId{ -1 };
	CWeakPtr<CSoundingObject>	soundingObj;
	FixedArray<InputValue, 8>	inputs;
};