# Sound emitter system overview — `shared/audio`

The audio code lives in `soundSystemLib` (`shared/audio`, `public/audio`; uses OpenAL Soft,
minivorbis and ImGui). It has two layers:

| Layer | Global | Files | Responsibility |
|---|---|---|---|
| Audio system | `g_audioSystem` (`IEqAudioSystem`) | `public/audio/IEqAudioSystem.h`, `eqAudioSystemAL.*`, `eqAudioSourceAL.*`, `source/*` | OpenAL device and context, voices (`IEqAudioSource`), sample loading, mixer channels, EFX effects, listener |
| Sound emitter system | `g_sounds` (`CSoundEmitterSystem`) | `eqSoundEmitterSystem.*`, `eqSoundEmitterObject.*`, `eqSoundScript.*`, `eqSoundEmitterPrivateTypes.*`, `eqSoundEmitterCommon.h` | Named sounds from script banks, emitters owned by game objects, distance-based virtualization, sound parameter graphs |

Game code normally uses only the emitter system: it plays sounds by script name and never touches
OpenAL voices directly.

```cpp
#include "audio/eqSoundEmitterSystem.h"
#include "audio/eqSoundEmitterObject.h"	// CSoundingObject, CEmitterObjectSound
```

## Setup

```cpp
enum ESoundChannelType
{
	CHAN_STATIC = 0,
	CHAN_STREAM,
	CHAN_MENU,
	CHAN_COUNT
};

static ChannelDef s_soundChannels[] = {
	DEFINE_SOUND_CHANNEL(CHAN_STATIC, 16),	// { "CHAN_STATIC", CHAN_STATIC, 16 }
	DEFINE_SOUND_CHANNEL(CHAN_STREAM, 2),
	DEFINE_SOUND_CHANNEL(CHAN_MENU, 16),
};

g_audioSystem->Init();							// OpenAL device, EFX effects
g_sounds->Init(1000.0f, s_soundChannels);		// default maxDistance, channel types
g_sounds->LoadScriptBank("scripts/EmitterSounds.txt");
g_sounds->PrecacheSound("effect.rock");

// every frame
g_audioSystem->SetListener(camPos, camVelocity, camForward, camUp);
g_sounds->Update();

// shutdown
g_sounds->Shutdown();
g_audioSystem->Shutdown();
```

- **Channel types** are defined by the game (up to `CHAN_MAX` = 16). A channel type is a mixer
  channel (`g_audioSystem->SetChannelVolume/SetChannelPitch`, `PauseAllSounds(chan)`) and a
  per-object limit: a `CSoundingObject` plays at most `limit` sounds of that channel at once; the
  oldest one is stopped when the limit is reached. Scripts refer to channels by their enum name.
- `Update()` starts a job on `g_parallelJobs` (the system is an `IParallelJob`) unless the previous
  update is still running. The job updates all sounding objects, starts pending sounds and wraps
  the voice updates in `BeginUpdate` / `EndUpdate`.
- Example: `samples/Demo_RockPaperScissors` (`states.cpp`, `state_sample_game_demo.cpp`).

## Sound script banks

A bank is a KeyValues file loaded with `LoadScriptBank(fileName)` (see [keyvalues.md](keyvalues.md)).
Each section is one sound script; its name is the sound name used in `EmitParams`. Sound names
must be unique across all loaded banks. `FreeScriptBank` / `FreeAllBanks` unload them.

```
// defaults for all sounds below in this bank
default maxDistance 80;
default channel CHAN_STATIC;

effect.rock
{
	wave "effects/rock_hit.wav";		// relative to sounds/
	volume 0.8;
	pitch 1.0;
	distance 5;							// reference distance
	maxDistance 60;
}

effect.hits
{
	rndwave
	{
		wave "effects/hit1.wav";
		wave "effects/hit2.wav";
		wave "effects/hit3.wav";
	}
	channel CHAN_STATIC;
}

music.menu
{
	wave "$music/menu.ogg";			// '$' — path from the file system root, not sounds/
	is2d 1;
	loop 1;
	channel CHAN_STREAM;
}
```

| Key | Meaning |
|---|---|
| `wave "file"` | One sample, path relative to `sounds/` (`$` prefix: full path) |
| `wave { wave ...; }` | Several samples played together as layers of one voice (up to 8, `MAX_SOUND_SAMPLES_SCRIPT`) — mixed with `svolume` / `spitch` |
| `rndwave { wave ...; }` | Several samples; one is picked per emit (random without repeats until all were played, or `EmitParams::sampleId`) |
| `channel CHAN_NAME` | Channel type (default: channel `0`) |
| `maxDistance` | Beyond this distance from the listener the sound is virtual (no voice); default from `Init` |
| `is2d` | Listener-relative (no 3D positioning) |
| `loop` | Looping; samples with WAV loop regions (cue / sampler chunks) loop as well |
| `startLoopTime`, `stopLoopTime` | Default fade times for `StartLoop` / `StopLoop` |
| `volume`, `pitch` | Output parameters (default 1) |
| `distance` | Reference distance (OpenAL), multiplied by `EmitParams::radiusMultiplier` |
| `rollOff` (default 2), `airAbsorption` (0) | Distance attenuation parameters |
| `lpf`, `hpf` | Low / high frequency gain (band pass) |
| `svolume <idx> <value>`, `spitch <idx> <value>` | Per-sample volume / pitch of the layers |
| `default <key> <value>` | (bank level) default for the scripts that follow |

WAV and OGG files are supported (PCM, 8/16 bit, mono or stereo). Files larger than 1 MB are
streamed, smaller ones are cached in memory. Samples are loaded by `PrecacheSound`; a sound that
was not precached does not play (`EMITSOUND_FLAG_FORCE_CACHED` loads it on emit, debug only).

### Parameter graph: inputs and mixers

Output parameters can be driven by values the game sets at run time:

```
engine.idle
{
	wave
	{
		wave "engine/idle_low.wav";
		wave "engine/idle_high.wav";
	}
	loop 1;

	input rpm 0 8000;								// input NAME [min max]
	mixer rpmNorm div rpm 8000;
	mixer pitch spline rpmNorm 0 0.8 1 1.6;			// x0 y0 x1 y1 ...
	mixer layerFade fade 2 rpmNorm 0 1 1 0;			// 2 outputs: layerFade[0], layerFade[1]

	pitch "pitch";
	svolume 0 "layerFade[0]";
	svolume 1 "layerFade[1]";
}
```

- `input NAME [min max]` declares a value set from code (`SetInputValue`, `EmitParams::inputs`).
  The range is used by the editor.
- `mixer NAME FUNC args...` declares a computed value. Functions: `copy`, `add`, `sub`, `mul`,
  `div`, `min`, `max` (2 args), `abs`, `average` (N args), `spline value x0 y0 x1 y1 ...`,
  `fade N value x0 y0 ...` (N outputs, `name[i]`).
- Arguments are numbers or names of inputs / mixers; outputs (`volume`, `pitch`, `svolume`, ...)
  take a quoted name instead of a number.
- `loopRemainTimeFactor` is a built-in input (1 → 0 while a loop fades out with `StopLoop`, 0 → 1
  on `StartLoop`); use it in mixers to shape fades.
- Up to 32 nodes per script (`SoundNodeDesc::MAX_SOUND_NODES`), 8 array elements per node, 10
  spline points.

The parameter graph has a visual editor in the Sound Script Editor (`audio/SoundScriptEditorUI.h`,
ImGui). Add it to the ImGui debug menu in the application:

```cpp
#include "audio/SoundScriptEditorUI.h"

#ifdef IMGUI_ENABLED
g_imGuiHost->AddDebugMenu("ENGINE/SOUND/SCRIPT EDITOR UI", SoundScriptEditorUIDraw);
#endif
```

It shows inputs, mixers and outputs as a node graph (imnodes) where nodes can be added, linked and
removed, edits splines in a curve editor, and plays the sound with live input values. Changes are
applied to running emitters; "Copy to clipboard" gives the script text to paste into the bank file.

The graph is evaluated per emitter when an input changed. The comment block in `eqSoundScript.h`
has more examples, but its `fade` lines use an outdated argument order (`fade value N`); the parser
expects `fade N value ...`, and `copy` is the current name of `use`.

## Emitting sounds

### One-shot sounds — `g_sounds->EmitSound`

```cpp
EmitParams ep("effect.rock", hitPosition);
ep.volume = 0.5f;
ep.flags |= EMITSOUND_FLAG_RANDOM_PITCH;
g_sounds->EmitSound(&ep);
```

The sound has no owner: it is released when it stops, and a sound farther than `maxDistance` is
not started at all. Looping sounds without an owner stop once the listener moves out of range.
Returns the channel type, or `CHAN_INVALID` if the sound was not started.

`EmitParams`:

| Field | Meaning |
|---|---|
| `name` | Sound script name |
| `origin` | World position |
| `volume`, `pitch`, `radiusMultiplier` | Multipliers for the script values |
| `channelType` | Overrides the script channel |
| `sampleId` | Sample index for `rndwave` (`-1` — random) |
| `effectSlot` | EFX effect slot (`-1` — none) |
| `inputs` | Initial input values (`{ "rpm", 1200.0f }`, up to 8) |
| `flags` | `EEmitSoundFlags`, below |

| Flag | Meaning |
|---|---|
| `EMITSOUND_FLAG_FORCE_2D` | Play listener-relative (music, UI) |
| `EMITSOUND_FLAG_STARTSILENT` | Create the emitter stopped; start it later with `PlayEmitter` |
| `EMITSOUND_FLAG_START_ON_UPDATE` | Queue and start in the next update job (e.g. sounds requested while the game is paused) |
| `EMITSOUND_FLAG_RANDOM_PITCH` | Add a random pitch offset of ±0.05 |
| `EMITSOUND_FLAG_RELEASE_ON_STOP` | Free the emitter when it stops or goes out of range (set automatically for one-shots) |
| `EMITSOUND_FLAG_FORCE_CACHED` | Precache on emit (debug only) |

### Owned sounds — `CSoundingObject`

Game objects that make sounds derive from (or contain) `CSoundingObject`. Each of its emitters is
identified by a unique id chosen by the game, usually a string hash:

```cpp
class CCar : public CSoundingObject { ... };

static constexpr int SND_ENGINE = StringIdConst24("engine");

EmitParams ep("engine.idle", carPos);
car->EmitSound(SND_ENGINE, &ep);			// replaces a previous SND_ENGINE emitter

// each frame
car->SetPosition(SND_ENGINE, carPos);
car->SetVelocity(SND_ENGINE, carVelocity);
car->SetInputValue(SND_ENGINE, "rpm", rpm);

car->StopLoop(SND_ENGINE, 0.5f);			// fade out the loop, then stop
car->StopEmitter(SND_ENGINE, true);		// stop and destroy
```

- `EmitSound(CSoundingObject::ID_RANDOM, &ep)` (`-1`) — fire-and-forget sound at the object:
  a random id is generated and the emitter is released when it stops.
- `ID_ALL` applies a call to all emitters of the object (`SetEmitterState`, `StopEmitter`,
  `SetPitch`, `SetVolume`, ...).
- Control: `PlayEmitter(id, rewind)`, `PauseEmitter`, `StopEmitter(id, destroy)`,
  `SetEmitterState`, `GetEmitterState`, `HasEmitter`, `StopLoop(id, fadeOut)`.
- Parameters: `SetPosition`, `SetVelocity`, `SetConeProperties`, `SetPitch` / `SetVolume`
  (replace the `EmitParams` multipliers), `SetSampleVolume`, `SetSamplePlaybackPosition`,
  `SetParams(id, IEqAudioSource::Params)`, `SetInputValue`, `SetEmitterSampleId` (restarts with
  another `rndwave` sample).
- `SetSoundVolumeScale` scales all sounds of the object.
- Owned emitters stay alive while out of range: they become **virtual** (no OpenAL voice, state
  and parameters kept) and get a voice again when the listener comes closer. Emitters flagged
  `RELEASE_ON_STOP` are freed instead.
- The destructor stops and frees all emitters of the object.

`CEmitterObjectSound(obj, id)` is a small handle to one existing emitter with the same methods
plus `StartLoop`; it holds a weak reference, so create it after `EmitSound`.

## Update flow

```
game thread                         job thread (Execute)
-----------                         --------------------
EmitSound / SetXxx  ──►  emitter data (virtual params, inputs)
Update() ─────────────────────────► BeginUpdate
                                    for each CSoundingObject:
                                      virtual emitter in range  → create voice
                                      voice stopped / released  → free emitter
                                    start pending (START_ON_UPDATE) sounds
                                    EndUpdate → voice update callbacks:
                                      evaluate parameter graph, loop fades,
                                      apply params, go virtual when out of range
```

## Audio system — `IEqAudioSystem`

Used directly for global control and by the emitter system internally:

- Listener: `SetListener(position, velocity, forward, up)`, `GetListenerPosition()`.
- Mixer: `SetMasterVolume`, `SetChannelVolume(chan, v)`, `SetChannelPitch`, `StopAllSounds(chan)`,
  `PauseAllSounds(chan)`, `ResumeAllSounds(chan)` (`-1` — all channels).
- EFX: effects are loaded at `Init` from the file named by `Sound { EFXScript "..." }` in
  `E2.CONFIG`; each section is an effect with `type reverb` or `type echo` and its parameters.
  `FindEffect(name)` + `SetEffect(slot, effect)` assign an effect to one of `EQSND_EFFECT_SLOTS` (6)
  slots; sounds use a slot through `EmitParams::effectSlot`.
- Samples: `GetSample(fileName)` (cached by name).
- Voices: `CreateSource` / `DestroySource`; `IEqAudioSource::Setup(channel, samples, callback)` and
  `UpdateParams(Params)`. The emitter system drives voices through the update callback.
- ConVars: `snd_device` (device index), `snd_hrtf`, `snd_debug`.

## Tools and debugging

- `snd_test_scriptsound <name>` — play a script sound (2D, precached on demand).
- `snd_scriptsound_info` — channel types and banks with cached sample counts.
- `snd_scriptsound_showWarnings 1` — warnings about unknown or not loaded sounds.
- `snd_scriptsound_debug 1` (cheat) — debug overlay spheres and text at emitter starts and updates.
- Sound Script Editor (`SoundScriptEditorUIDraw`, ImGui; add it with
  `g_imGuiHost->AddDebugMenu("ENGINE/SOUND/SCRIPT EDITOR UI", SoundScriptEditorUIDraw)` — the
  samples and the `sysLib` Lua bindings do this): browse scripts, edit waves and the parameter graph
  (see [above](#parameter-graph-inputs-and-mixers)), play / pause / stop with live input values,
  isolate one sound, apply changes to running emitters and copy the script text to the clipboard.
- Lua: `sounds` (`CSoundEmitterSystem`), `EmitParams`, `CSoundingObject` and the `EMITSOUND_FLAG_*`
  constants are bound in `shared/sys/scripting/sys_esl_engine.cpp` (see
  [scripting_esl_overview.md](scripting_esl_overview.md)).
