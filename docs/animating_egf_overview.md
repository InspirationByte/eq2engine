# EGF animation overview — `shared/animating`

`animatingLib` animates the skeleton of an EGF model: sequences from motion packages (`.mop`),
activities, blending and transitions, pose controllers, animation events and an experimental IK
solver. The result is an array of bone matrices used to draw the model with skinning (see
[rendering_egf_overview.md](rendering_egf_overview.md)). File formats:
[public_egf_overview.md](public_egf_overview.md).

| File | Contents |
|---|---|
| `Animating.h/.cpp` | `CAnimatingEGF` — the animation component |
| `BoneSetup.h/.cpp` | Runtime structures: `AnimSequence`, `AnimSequenceTimer`, `AnimPoseController`, `AnimIkChain`, `AnimFrame` |
| `anim_activity.h/.cpp` | `Activity` enum, `GetActivityByName`, `GetActivityName` |
| `anim_events.h/.cpp` | `AnimationEvent` enum, `GetEventByName`, `GetEventName` |

## Usage

`CAnimatingEGF` is meant to be a base class (or member) of a game object that has a model:

```cpp
class CPedestrian : public CAnimatingEGF
{
protected:
	Activity	TranslateActivity(Activity act, int slot) const override;
	void		HandleAnimatingEvent(AnimationEvent event, const char* options) override;
};

// the model must be fully loaded (see rendering_egf_overview.md: loading)
EGF_LOADING_CRITICAL_SECTION(geom);	// or init from geom->GetLoadingFuture() callback
ped->InitAnimating(geom);			// skeleton, IK chains, motion packages of the model
ped->InitMotionData("models/peds/shared_anims");	// optional extra package
ped->SetActivity(ACT_IDLE);

// every frame
ped->AdvanceFrame(dt);				// timers, pose controllers, events
ped->RecalcBoneTransforms();		// recomputes bones if anything changed
ped->UpdateIK(dt, worldMatrix);		// optional

// drawing
RenderBoneTransform bones[128];
const int numBones = geom->ConvertBoneMatricesToQuaternions(ped->GetBoneMatrices(), bones);
...
```

- `InitAnimating(geom)` — binds the model: copies the bind pose to the bone matrices, creates IK
  chains and adds all motion packages referenced by the model. The model must be in
  `MODEL_LOAD_OK` state (asserted): skeleton and motion packages are filled by the loading jobs. `DestroyAnimating()` releases it;
  `IsAnimatingInitialized()`.
- `InitMotionData(name)` — adds one more motion package (by file name, through `g_studioCache`).
  Packages are identified by file name without path and extension.
- `GetBoneMatrices()` — model-space bone matrices (`Matrix4x4[numBones]`), valid after
  `RecalcBoneTransforms`.
- `FindBone(name)`, `GetLocalBoneOrigin(bone)`, `GetLocalBoneDirection(bone)`.
- `GetLocalStudioTransformMatrix(idx)` — model-space matrix of an EGF transform (attachment),
  following its bone if it is attached to one.

## Sequences and activities

A **sequence** (from the `.mop`, compiled by `animca`) is what gets played: one or more
animations, framerate, flags (`SEQFLAG_LOOP`, `SEQFLAG_SLOTBLEND`), transition time, an optional
pose controller, layered sequences and events. An **activity** is a game-level name for "what the
object is doing" (`ACT_IDLE`, `ACT_WALK`, `ACT_VEHICLE_STEER`, ...); sequences declare the activity
and slot they implement.

```cpp
SetActivity(ACT_RUN);								// sequence by activity (+ slot)
SetSequenceByName("pedestrian.wave", 1);			// "package.sequence"

const int seqId = FindSequence("pedestrian.wave");	// -1 if not found
SetSequence(seqId, 1);
ResetSequenceTime(1);
PlaySequence(1);
```

- `SetActivity(act, slot)` calls `TranslateActivity` (override to pick a variant, e.g. a different
  run for a wounded character), finds the sequence with `FindSequenceByActivity`, resets the time
  and plays it.
- `SetSequenceByName(name, slot)` / `FindSequence(name)` take `"<package>.<sequence>"`.
- `SetSequence(seqId, slot)` only assigns the sequence (no reset, no play).
- Playback: `PlaySequence`, `StopSequence` (pause), `IsSequencePlaying`, `ResetSequenceTime`,
  `SetSequenceTime(frame)`, `SetPlaybackSpeedScale`.
- Queries: `GetCurrentActivity`, `GetSequenceById`, `GetSequenceTimer`,
  `GetCurrentAnimationFrameCount`, `GetCurrentAnimationDuration` / `Time` /
  `RemainingDuration` (seconds).
- Non-looping sequences stop on the last frame; looping ones wrap and restart their events.
- `ACT_SCRIPTED_SEQUENCE` sequences are not registered by activity — play them by name.
- To add an activity, extend `Activity` in `anim_activity.h` and the name table in
  `anim_activity.cpp` (sequences refer to activities by name).

### Slots, blending and transitions

There are `MAX_SEQUENCE_TIMERS` (5) slots; each plays its own sequence:

- Slot 0 is the base pose (weight always 1). Other slots are mixed in with
  `SetSequenceBlending(weight, slot)`: sequences with `SEQFLAG_SLOTBLEND` are added on top
  (scaled by the weight; e.g. upper-body gestures), others are interpolated towards by the weight.
- When a slot switches to another sequence, the previous one keeps playing in a transition timer
  and fades out over the new sequence's `transitiontime` (default `SEQ_DEFAULT_TRANSITION_TIME` =
  0.15 s). `IsTransitionCompleted(slot)`.
- `SwapSequenceTimers(a, b)` swaps two slots.

### Pose controllers

A pose controller is a named value that blends between the animations of a sequence (e.g. aim
direction, steering angle): the value is mapped over the controller's blend range onto the
sequence's animations, and the two nearest animations are interpolated.

```cpp
const int steerCtrl = FindPoseController("steer");
SetPoseControllerValue(steerCtrl, steerAngle);
```

- `GetPoseControllerValue`, `GetPoseControllerInterpValue` (the value used for blending; it
  follows the set value smoothly in `AdvanceFrame`), `GetPoseControllerRange`.
- Controllers are shared by name between all motion packages of the object; their initial value is
  the middle of the range.

## Animation events

Sequences carry events at frames (`command` + `parameter` strings in the `.asc` script). When the
timer passes the event frame, `HandleAnimatingEvent(event, parameter)` is called with the
`AnimationEvent` resolved by name (`EV_SOUND`, `EV_MUZZLEFLASH`, `EV_IK_*`) or, if the name is
unknown, by number (`atoi(command)`). Override it in the game object, e.g. to play a sound:

```cpp
void CPedestrian::HandleAnimatingEvent(AnimationEvent event, const char* options)
{
	if (event == EV_SOUND)
	{
		EmitParams ep(options, GetOrigin());
		EmitSound(CSoundingObject::ID_RANDOM, &ep);	// object is also a CSoundingObject
	}
}
```

To add an event type, extend `AnimationEvent` and `anim_event_register` in `anim_events.cpp`.

## Inverse kinematics

IK chains come from the EGF (`studioIkChain_t`, defined in the `egfca` script):

```cpp
const int chain = FindIKChain("left_leg");
SetIKChainEnabled(chain, true);
SetIKWorldTarget(chain, footTargetWorld, worldMatrix);	// or SetIKLocalTarget(chain, localPos)
...
UpdateIK(dt, worldMatrix);	// after RecalcBoneTransforms
```

Disabled chains follow the animated pose. The solver iterates `r_ikIterations` times per update.
The IK code is marked as broken in the source (`UpdateIkChain` FIXME) — treat it as experimental.

## Debugging

- `DebugRender(worldMatrix)` with cheat ConVars `r_debugSkeleton` (bones), `r_debugShowBone <idx>`
  and `r_debugIK` (IK targets).
- Lua: `AnimatingEGF` is bound in `shared/sys/scripting/sys_esl_animating.cpp` (see
  [scripting_esl_overview.md](scripting_esl_overview.md)).
- `tools/egfMan2` (`CAnimatedModel`, derived from `CAnimatingEGF`) plays sequences, edits pose
  controllers and shows bones; use it to check motion packages.
