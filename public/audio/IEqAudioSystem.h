//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define SOUNDSYSTEM_INTERFACE_VERSION		"IEqAudioSystem_002"

class ISoundSource;
typedef uint effectId_t;

#define EFFECT_ID_NONE				(0)
#define EQSND_EFFECT_SLOTS			(6)

//-----------------------------------------------------------------
// Audio source interface

class IEqAudioSource : public RefCountedObject<IEqAudioSource>
{
public:
	enum ESoundSourceUpdate
	{
		UPDATE_POSITION			= (1 << 0),
		UPDATE_VELOCITY			= (1 << 1),
		UPDATE_VOLUME			= (1 << 2),
		UPDATE_PITCH			= (1 << 3),
		UPDATE_REF_DIST			= (1 << 4),
		UPDATE_ROLLOFF			= (1 << 5),
		UPDATE_AIRABSORPTION	= (1 << 6),
		UPDATE_RELATIVE			= (1 << 7),
		UPDATE_STATE			= (1 << 8),
		UPDATE_LOOPING			= (1 << 9),
		UPDATE_EFFECTSLOT		= (1 << 10),
		UPDATE_RELEASE_ON_STOP	= (1 << 11),
		UPDATE_CHANNEL			= (1 << 12),

		// command
		UPDATE_DO_REWIND		= (1 << 13),
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
		float				volume{ 1.0f };					// [0.0, 1.0]
		float				pitch{ 1.0f };					// [0.0, 100.0]
		float				referenceDistance{ 1.0f };
		float				rolloff{ 0.0f };
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
		PROP_SETTER(volume, UPDATE_VOLUME)					// [0.0, 1.0]
		PROP_SETTER(pitch, UPDATE_PITCH)					// [0.0, 100.0]
		PROP_SETTER(referenceDistance, UPDATE_REF_DIST)
		PROP_SETTER(rolloff, UPDATE_ROLLOFF)
		PROP_SETTER(airAbsorption, UPDATE_AIRABSORPTION)
		PROP_SETTER(state, UPDATE_STATE)
		PROP_SETTER(effectSlot, UPDATE_EFFECTSLOT)
		PROP_SETTER(relative, UPDATE_RELATIVE)
		PROP_SETTER(looping, UPDATE_LOOPING)
		PROP_SETTER(releaseOnStop, UPDATE_RELEASE_ON_STOP)
		PROP_SETTER(channel, UPDATE_CHANNEL)

#define PROP_MERGE(var, flag) if(other.updateFlags & flag) {var = other.##var;}

		inline Params& operator |=(const Params& other) {
			updateFlags |= other.updateFlags;
			PROP_MERGE(position, UPDATE_POSITION)
			PROP_MERGE(velocity, UPDATE_VELOCITY)
			PROP_MERGE(volume, UPDATE_VOLUME)
			PROP_MERGE(pitch, UPDATE_PITCH)
			PROP_MERGE(referenceDistance, UPDATE_REF_DIST)
			PROP_MERGE(rolloff, UPDATE_ROLLOFF)
			PROP_MERGE(airAbsorption, UPDATE_AIRABSORPTION)
			PROP_MERGE(state, UPDATE_STATE)
			PROP_MERGE(effectSlot, UPDATE_EFFECTSLOT)
			PROP_MERGE(relative, UPDATE_RELATIVE)
			PROP_MERGE(looping, UPDATE_LOOPING)
			PROP_MERGE(releaseOnStop, UPDATE_RELEASE_ON_STOP)
			PROP_MERGE(channel, UPDATE_CHANNEL)
			return *this;
		}

#undef PROP_SETTER
#undef PROP_MERGE
	};

	using UpdateCallback = EqFunction<int(void* obj, Params& params)>;		// returns EVoiceUpdateFlags

	virtual ~IEqAudioSource() {}

	virtual void			Setup(int chanId, const ISoundSource* sample, UpdateCallback fnCallback, void* callbackObject = nullptr) = 0;
	virtual void			Setup(int chanId, ArrayCRef<const ISoundSource*> samples, UpdateCallback fnCallback, void* callbackObject = nullptr) = 0;
	virtual void			Release() = 0;

	// full scale
	virtual void			GetParams(Params& params) const = 0;
	virtual void			UpdateParams(const Params& params, int mask = 0) = 0;

	virtual void			SetSampleVolume(int sourceIdx, float volume) = 0;
	virtual float			GetSampleVolume(int sourceIdx) = 0;
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

	virtual IEqAudioSource*		CreateSource() = 0;
	virtual void				DestroySource(IEqAudioSource* source) = 0;

	virtual void				BeginUpdate() = 0;
	virtual void				EndUpdate() = 0;

	virtual void				StopAllSounds(int chanType = -1, void* callbackObject = nullptr) = 0;
	virtual void				PauseAllSounds(int chanType = -1, void* callbackObject = nullptr) = 0;
	virtual void				ResumeAllSounds(int chanType = -1, void* callbackObject = nullptr) = 0;

	virtual void				SetChannelVolume(int chanType, float value) = 0;
	virtual void				SetChannelPitch(int chanType, float value) = 0;

	virtual void				SetMasterVolume(float value) = 0;

	// sets listener properties
	virtual void				SetListener(const Vector3D& position,
											const Vector3D& velocity,
											const Vector3D& forwardVec,
											const Vector3D& upVec) = 0;

	// gets listener properties
	virtual void				GetListener(Vector3D& position, Vector3D& velocity) = 0;

	// loads sample source data
	virtual ISoundSource*		LoadSample(const char* filename) = 0;
	virtual void				FreeSample(ISoundSource* sample) = 0;

	// finds the effect. May return EFFECT_ID_NONE
	virtual effectId_t			FindEffect(const char* name) const = 0;

	// sets the new effect
	virtual void				SetEffect(int slot, effectId_t effect) = 0;
};

extern IEqAudioSystem* g_audioSystem;
