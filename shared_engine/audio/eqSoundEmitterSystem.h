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
class ConCommandBase;

// the sound emitter system
class CSoundEmitterSystem
{
	friend class CSoundingObject;
	friend class CEmitterObjectSound;
public:
	static void cmd_vars_sounds_list(const ConCommandBase* base, Array<EqString>& list, const char* query);

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

private:

	int					EmitSound(EmitParams* emit, CSoundingObject* soundingObj, int objUniqueId, bool releaseOnStop = true);

	SoundScriptDesc*	FindSound(const char* soundName) const;
	void				RemoveSoundingObject(CSoundingObject* obj);

	static int			EmitterUpdateCallback(void* obj, IEqAudioSource::Params& params);
	static int			LoopSourceUpdateCallback(void* obj, IEqAudioSource::Params& params);

	bool				SwitchSourceState(SoundEmitterData* emit, bool isVirtual);

	int					ChannelTypeByName(const char* str) const;

	Threading::CEqSignal				m_updateDone;
	FixedArray<ChannelDef, CHAN_MAX>	m_channelTypes;
	Map<int, SoundScriptDesc*>			m_allSounds{ PP_SL };
	Set<CSoundingObject*>				m_soundingObjects{ PP_SL };
	Array<EmitParams>					m_pendingStartSounds{ PP_SL };
	
	float								m_defaultMaxDistance{ 100.0f };

	bool								m_isInit{ false };
};

extern CSoundEmitterSystem* g_sounds;
