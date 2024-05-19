//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define SOUNDSYSTEM_INTERFACE_VERSION		"IEqAudioSystem_002"

class ISoundSource;
using ISoundSourcePtr = CRefPtr<ISoundSource>;
using AudioEffectId = uint;

#define EFFECT_ID_NONE				(0)

#define EQSND_EFFECT_SLOTS			(6)
#define EQSND_MIXER_CHANNELS		(16)
#define EQSND_STREAM_BUFFER_COUNT	(4)
#define EQSND_SAMPLE_COUNT			(8)

//-----------------------------------------------------------------
// Audio source interface

class IEqAudioSource : public RefCountedObject<IEqAudioSource>
{
public:
	enum ESoundSourceUpdate
	{
		UPDATE_POSITION			= (1 << 0),
		UPDATE_VELOCITY			= (1 << 1),
		UPDATE_DIRECTION		= (1 << 2),
		UPDATE_VOLUME			= (1 << 3),
		UPDATE_CONE_ANGLES		= (1 << 2),
		UPDATE_BANDPASS			= (1 << 5),
		UPDATE_PITCH			= (1 << 4),
		UPDATE_REF_DIST			= (1 << 7),
		UPDATE_ROLLOFF			= (1 << 8),
		UPDATE_AIRABSORPTION	= (1 << 9),
		UPDATE_RELATIVE			= (1 << 10),
		UPDATE_STATE			= (1 << 11),
		UPDATE_LOOPING			= (1 << 12),
		UPDATE_EFFECTSLOT		= (1 << 13),
		UPDATE_RELEASE_ON_STOP	= (1 << 14),
		UPDATE_CHANNEL			= (1 << 15),

		// command
		UPDATE_DO_REWIND		= (1 << 16),
	};

	enum State : int
	{
		STOPPED = 0,
		PLAYING,
		PAUSED
	};

	struct Params
	{
		Vector3D			position{ 0.0f };
		Vector3D			velocity{ 0.0f };
		Vector3D			direction{ 0.0f };
		Vector3D			volume{ 1.0f };					// [0.0, 1.0], x - inner cone volume, y - outer cone volume, z - outer cone high frequency volume
		Vector2D			coneAngles{ 360.0f, 360.0f };	// [0.0f, 360.0f], inner and outer angles
		Vector2D			bandPass{ 1.0f };				// low (x) and high (y) frequency gain
		float				pitch{ 1.0f };					// [0.0, 100.0]
		float				referenceDistance{ 1.0f };
		float				rolloff{ 1.0f };
		float				airAbsorption{ 0.0f };
		State				state{ STOPPED };
		int					effectSlot{ -1 };
		bool				relative{ true };
		bool				looping{ false };
		bool				releaseOnStop{ true };
		int					channel{ -1 };

		int					updateFlags{ 0 };

#define PROP_SETTER(var, flag)	template<typename T> inline void set_##var(T value) {var = value; updateFlags |= flag;}

		PROP_SETTER(position, UPDATE_POSITION)
		PROP_SETTER(velocity, UPDATE_VELOCITY)
		PROP_SETTER(direction, UPDATE_DIRECTION)
		PROP_SETTER(volume, UPDATE_VOLUME)
		PROP_SETTER(coneAngles, UPDATE_CONE_ANGLES)
		PROP_SETTER(bandPass, UPDATE_BANDPASS)
		PROP_SETTER(pitch, UPDATE_PITCH)
		PROP_SETTER(referenceDistance, UPDATE_REF_DIST)
		PROP_SETTER(rolloff, UPDATE_ROLLOFF)
		PROP_SETTER(airAbsorption, UPDATE_AIRABSORPTION)
		PROP_SETTER(relative, UPDATE_RELATIVE)
		PROP_SETTER(state, UPDATE_STATE)
		PROP_SETTER(looping, UPDATE_LOOPING)
		PROP_SETTER(effectSlot, UPDATE_EFFECTSLOT)
		PROP_SETTER(releaseOnStop, UPDATE_RELEASE_ON_STOP)
		PROP_SETTER(channel, UPDATE_CHANNEL)

		inline Params& operator |=(const Params& other) {
			merge(other);
			return *this;
		}

#define PROP_MERGE(flags, var, flag) if(flags & flag) {var = other.var;}

		inline void merge(const Params& other, int overrideUpdateFlags = -1) {
			const int flags = (overrideUpdateFlags == -1) ? other.updateFlags : overrideUpdateFlags;
			updateFlags |= flags;
			PROP_MERGE(flags, position, UPDATE_POSITION)
			PROP_MERGE(flags, velocity, UPDATE_VELOCITY)
			PROP_MERGE(flags, direction, UPDATE_DIRECTION)
			PROP_MERGE(flags, volume, UPDATE_VOLUME)
			PROP_MERGE(flags, coneAngles, UPDATE_CONE_ANGLES)
			PROP_MERGE(flags, bandPass, UPDATE_BANDPASS)
			PROP_MERGE(flags, pitch, UPDATE_PITCH)
			PROP_MERGE(flags, referenceDistance, UPDATE_REF_DIST)
			PROP_MERGE(flags, rolloff, UPDATE_ROLLOFF)
			PROP_MERGE(flags, airAbsorption, UPDATE_AIRABSORPTION)
			PROP_MERGE(flags, relative, UPDATE_RELATIVE)
			PROP_MERGE(flags, state, UPDATE_STATE)
			PROP_MERGE(flags, looping, UPDATE_LOOPING)
			PROP_MERGE(flags, effectSlot, UPDATE_EFFECTSLOT)
			PROP_MERGE(flags, releaseOnStop, UPDATE_RELEASE_ON_STOP)
			PROP_MERGE(flags, channel, UPDATE_CHANNEL)
		}

#undef PROP_SETTER
#undef PROP_MERGE
	};

	using UpdateCallback = EqFunction<int(IEqAudioSource* source, Params& params)>;		// returns EVoiceUpdateFlags

	virtual ~IEqAudioSource() { }

	virtual void			Setup(int chanId, const ISoundSource* sample, UpdateCallback fnCallback) = 0;
	virtual void			Setup(int chanId, ArrayCRef<const ISoundSource*> samples, UpdateCallback fnCallback) = 0;
	virtual void			Release() = 0;

	// full scale
	virtual void			GetParams(Params& params) const = 0;
	virtual void			UpdateParams(const Params& params, int overrideUpdateFlags = -1) = 0;

	virtual void			SetSamplePlaybackPosition(int sourceIdx, float seconds) = 0;
	virtual float			GetSamplePlaybackPosition(int sourceIdx) const = 0;

	virtual void			SetSampleVolume(int sourceIdx, float volume) = 0;
	virtual float			GetSampleVolume(int sourceIdx) const = 0;
	virtual void			SetSamplePitch(int sourceIdx, float pitch) = 0;
	virtual float			GetSamplePitch(int sourceIdx) const = 0;

	virtual int				GetSampleCount() const = 0;

	// atomic
	virtual State			GetState() const = 0;
	virtual bool			IsLooping() const = 0;
};


//-----------------------------------------------------------------
// Audio system interface

// Audio system, controls voices
class IEqAudioSystem
{
public:
	virtual ~IEqAudioSystem() {}

	virtual void				Init() = 0;
	virtual void				Shutdown() = 0;

	virtual CRefPtr<IEqAudioSource>		CreateSource() = 0;
	virtual void						DestroySource(IEqAudioSource* source) = 0;

	virtual void				BeginUpdate() = 0;
	virtual void				EndUpdate() = 0;

	virtual void				StopAllSounds(int chanType = -1) = 0;
	virtual void				PauseAllSounds(int chanType = -1) = 0;
	virtual void				ResumeAllSounds(int chanType = -1) = 0;

	virtual void				ResetMixer(int chanId) = 0;
	virtual void				SetChannelVolume(int chanType, float value) = 0;
	virtual void				SetChannelPitch(int chanType, float value) = 0;

	virtual void				SetMasterVolume(float value) = 0;

	// sets listener properties
	virtual void				SetListener(const Vector3D& position,
											const Vector3D& velocity,
											const Vector3D& forwardVec,
											const Vector3D& upVec) = 0;

	// gets listener properties
	virtual const Vector3D&		GetListenerPosition() const = 0;

	// loads sample source data
	virtual ISoundSourcePtr		GetSample(const char* filename) = 0;

	virtual void				AddSample(ISoundSource* sample) = 0;
	virtual void				OnSampleDeleted(ISoundSource* sample) = 0;

	// finds the effect. May return EFFECT_ID_NONE
	virtual AudioEffectId		FindEffect(const char* name) const = 0;

	// sets the new effect
	virtual void				SetEffect(int slot, AudioEffectId effect) = 0;
	virtual int					GetEffectSlotCount() const = 0;
};

extern IEqAudioSystem* g_audioSystem;
