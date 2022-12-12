//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve's Source)
//////////////////////////////////////////////////////////////////////////////////

/*
	TODO:
		Sound source direction (for CSoundingObject only)
		Spatial optimizations to query with less distance checks
		Basic cue envelope control support for CSoundingObject
*/

#pragma once

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
class CSoundEmitterSystem
{
	friend class CSoundingObject;
	friend class CEmitterObjectSound;
	friend class CSoundScriptEditor;
public:
	CSoundEmitterSystem();
	~CSoundEmitterSystem();

	void				Init(float defaultMaxDistance, ChannelDef* channelDefs, int numChannels);
	void				Shutdown();

	void				LoadScriptSoundFile(const char* fileName);
	void				CreateSoundScript(const KVSection* scriptSection, const KVSection* defaultsSec = nullptr);

	void				PrecacheSound(const char* pszName);
	int					EmitSound(EmitParams* emit);
	void				StopAllSounds();

	void				Update();

	void				GetAllSoundsList(Array<SoundScriptDesc*>& list) const;
private:

	int					EmitSound(EmitParams* emit, CSoundingObject* soundingObj, int objUniqueId, bool releaseOnStop = true);

	SoundScriptDesc*	FindSound(const char* soundName) const;
	void				RemoveSoundingObject(CSoundingObject* obj);

	static int			EmitterUpdateCallback(IEqAudioSource* source, IEqAudioSource::Params& params, void* obj);
	static int			LoopSourceUpdateCallback(IEqAudioSource* source, IEqAudioSource::Params& params, void* obj);

	bool				SwitchSourceState(SoundEmitterData* emit, bool isVirtual);

	int					ChannelTypeByName(const char* str) const;

	// Editor features
	void				RestartEmittersByScript(SoundScriptDesc* soundScript);

	CEqTimer							m_updateTimer;
	Threading::CEqSignal				m_updateDone;
	FixedArray<ChannelDef, CHAN_MAX>	m_channelTypes;
	Map<int, SoundScriptDesc*>			m_allSounds{ PP_SL };
	Set<CSoundingObject*>				m_soundingObjects{ PP_SL };
	Array<EmitParams>					m_pendingStartSounds{ PP_SL };

	SoundScriptDesc*					m_isolateSound{ nullptr };
	
	float								m_defaultMaxDistance{ 100.0f };
	float								m_deltaTime{ 0.0f };
	bool								m_isInit{ false };
};

extern CSoundEmitterSystem* g_sounds;
