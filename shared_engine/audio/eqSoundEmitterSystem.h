//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Sound Emitter System
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IEqParallelJobs.h"
#include "audio/IEqAudioSystem.h"
#include "eqSoundEmitterCommon.h"

struct SoundScriptDesc;
struct SoundEmitterData;
struct KVSection;
struct ChannelDef;
class CSoundingObject;
class CEmitterObjectSound;
class CSoundScriptEditor;
class ConCommandBase;

// the sound emitter system
class CSoundEmitterSystem : public IParallelJob
{
	friend class CSoundingObject;
	friend class CEmitterObjectSound;
	friend class CSoundScriptEditor;
public:
	CSoundEmitterSystem();
	~CSoundEmitterSystem();

	void				Init(float defaultMaxDistance, ArrayCRef<ChannelDef> channelDefs);
	void				Shutdown();

	void				LoadScriptSoundFile(const char* fileName);
	bool				CreateSoundScript(const KVSection* scriptSection, const KVSection* defaultsSec = nullptr);

	bool				PrecacheSound(const char* pszName);
	int					EmitSound(EmitParams* emit);
	void				StopAllSounds();

	void				Update();

	void				GetAllSoundsList(Array<SoundScriptDesc*>& list) const;
	static const char*	GetScriptName(SoundScriptDesc* desc);

private:
	int					EmitSoundInternal(EmitParams* emit, int objUniqueId, CSoundingObject* soundingObj);

	SoundScriptDesc*	FindSoundScript(const char* soundName) const;
	void				OnRemoveSoundingObject(CSoundingObject* obj);

	static int			EmitterUpdateCallback(IEqAudioSource* source, IEqAudioSource::Params& params, CWeakPtr<SoundEmitterData> emitter);
	static int			LoopSourceUpdateCallback(IEqAudioSource* source, IEqAudioSource::Params& params, SoundScriptDesc* script);

	bool				SwitchSourceState(SoundEmitterData* emit, bool isVirtual);
	int					ChannelTypeByName(const char* str) const;

	void				RestartEmittersByScript(SoundScriptDesc* soundScript);	// for SoundScript editor

	void				Execute() override;

	struct PendingSound
	{
		EmitParams					params;
		int							objUniqueId;
		CWeakPtr<CSoundingObject>	soundingObj;
	};

	CEqTimer							m_updateTimer;

	FixedArray<ChannelDef, CHAN_MAX>	m_channelTypes;
	Map<int, SoundScriptDesc*>			m_allSounds{ PP_SL };
	Set<CSoundingObject*>				m_soundingObjects{ PP_SL };
	Array<PendingSound>					m_pendingStartSounds{ PP_SL };
	SoundScriptDesc*					m_isolateSound{ nullptr };
	
	float								m_defaultMaxDistance{ 100.0f };
	float								m_deltaTime{ 0.0f };
	bool								m_isInit{ false };
};

extern CStaticAutoPtr<CSoundEmitterSystem> g_sounds;
