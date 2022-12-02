#pragma once

// flags
enum EEmitSoundFlags
{
	EMITSOUND_FLAG_FORCE_CACHED		= (1 << 2),		// forces emitted sound to be loaded if not cached by PrecacheScriptSound (not recommended, debug only)
	EMITSOUND_FLAG_FORCE_2D			= (1 << 3),		// force 2D sound (music, etc.)
	EMITSOUND_FLAG_STARTSILENT		= (1 << 4),		// starts silent
	EMITSOUND_FLAG_START_ON_UPDATE	= (1 << 5),		// start playing sound on emitter system update
	EMITSOUND_FLAG_RANDOM_PITCH		= (1 << 6),		// apply slightly random pitch (best for static hit sounds)

	EMITSOUND_FLAG_PENDING			= (1 << 7),		// was in pending list
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

	EqString				name;
	Vector3D				origin{ vec3_zero };
	float					volume{ 1.0f };
	float					pitch{ 1.0f };
	float					radiusMultiplier{ 1.0f };

	int						flags{ 0 };
	int						sampleId{ -1 };
	int						channelType{ CHAN_INVALID };
};